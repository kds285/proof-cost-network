#include "BaseSolver.h"

void BaseSolver::runSolver()
{
	string sCommand;
	while (getline(cin, sCommand)) {
		string s1 = sCommand.substr(0, sCommand.find(" "));
		string s2 = sCommand.substr(sCommand.find(" ") + 1);

		if (s1 == "load_nn_model") {
			m_network->loadModel(s2);
		} else if (s1 == "solve") {
			if (!loadProblem(s2)) {
				cerr << "Failed to load problem \"" << s2 << "\"" << endl;
				continue;
			}

			cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ");
			ifstream f(getAnswerFileName());
			if (f.good()) { cerr << "Skip \"" << s2 << "\"" << endl; } else {
				cerr << "Solve \"" << s2 << "\" ... ";
				lockProblem();
				solve();
				saveAnswer();
				saveTree();
			}
			f.close();
		} else if(s1 == "solve_back") {
			string problem_prefix = s2.substr(s2.find(" ") + 1); // problem dir
			string postfix = problem_prefix.substr(problem_prefix.find_last_of("_") + 1);
			int d = stoi(postfix.substr(0, postfix.find(".")));
			string extension = postfix.substr(postfix.find("."));
			problem_prefix = problem_prefix.substr(0, problem_prefix.find_last_of("_") + 1);
			string output_dir = s2.substr(0, s2.find(" "));  // output dir
			for (;;d -= 2) {
				string fileName = problem_prefix + to_string(d) + extension;
				if (!loadProblem(fileName)) {
					cerr << "Failed to load problem \"" << fileName << "\"" << endl;
					break;
				}

				cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ");
				ifstream f(getAnswerFileName());
				int solve_sim = 1000000;
				if (f.good()) {
					cerr << "Skip \"" << fileName << "\"" << endl;
					break;
				} else {
					cerr << "Solve \"" << fileName << "\" ... ";
					lockProblem();
					solve();
					saveAnswer();
					saveTree();
					solve_sim = getSolvedSimulation();
				}
				f.close();

				if (solve_sim >= 1000000) break;

				// back two moves
				SgfLoader sgfLoader;
				sgfLoader.parseFromFile(fileName);
				string undoSgf = getUndoSgf(sgfLoader);
				problem_prefix = output_dir + "/" + problem_prefix.substr(problem_prefix.find_last_of("/") + 1);
				m_sOutputFileName = m_sOutputFileName.substr(0, m_sOutputFileName.find_last_of("_") + 1) + to_string(d-2) + extension;
				ifstream f2(getAnswerFileName());
				if (f2.good()) {
					cerr << "Skip save new problems" << endl;
				} else {
					ofstream fout(problem_prefix + to_string(d-2) + extension);
					fout << undoSgf << endl;
					fout.close();
				}
				f2.close();
			}
		} else if (s1 == "output_name") {
			m_sOutputFileName = s2;
		}
	}
}

bool BaseSolver::loadProblem(string sProblem)
{
	SgfLoader sgfLoader;
	if (!sgfLoader.parseFromFile(sProblem)) { return false; }

	m_sProblemFileName = sProblem;
	m_sSgfString = sgfLoader.getSgfString();
	parseAnswerPosition(sgfLoader.getTag("EV"));
	return playSgfGame(sgfLoader);
}

void BaseSolver::initNetwork()
{
	m_network = new Network(Configure::GPU_LIST[0] - '0', Configure::MODEL_FILE);
	if (Configure::USE_NET) {
		m_network->initialize();
	}
}

void BaseSolver::lockProblem()
{
	m_fAnswer.open(getAnswerFileName(), ios::out);
}

void BaseSolver::saveAnswer()
{
	assert(("File stream is not open", m_fAnswer.is_open()));

	cerr << "Save results to " << getAnswerFileName() << endl;
	m_fAnswer << getSolvedProbabilty() << " "
    << getSolvedRootNode()->getValue() << " "
		<< getSolutionStatusString(getSolvedStatus()) << " "
		<< getSolvedSimulation() << " "
		<< getReExpansion() << " "
		<< getSolvedTime() << " "
		<< getSolvedMove().toGtpString(Game::getBoardSize()) << endl;
	m_fAnswer.close();
}

void BaseSolver::saveTree()
{
	string sFileName = m_sOutputFileName + "_" + to_string(getNNModelIteration()) + ".tree";
	fstream fTreeInfo(sFileName, ios::out);

	TreeNode* pRoot = getSolvedRootNode();
	string sPrefix = m_sSgfString.substr(0, m_sSgfString.find(")"));
	fTreeInfo << sPrefix << "C[" << getNodeInfo(pRoot) << "]" << getTreeInfo_r(getSolvedRootNode()) << ")";
	fTreeInfo.close();
}

int BaseSolver::getNNModelIteration()
{
	if (!Configure::USE_NET) return 0;
	string sModelName = m_network->getModelName();
	sModelName = sModelName.substr(sModelName.find("weight_iter_") + string("weight_iter_").length());
	return stoi(sModelName.substr(0, sModelName.find(".pt")));
}

string BaseSolver::getAnswerFileName()
{
	assert(("File name is empty", !m_sOutputFileName.empty()));
	return m_sOutputFileName + "_" + to_string(getNNModelIteration()) + ".ans";
}

void BaseSolver::parseAnswerPosition(string sEvent)
{
	m_answerPos.clear();
	if (sEvent.find("pos=") == -1) return;
	istringstream in(sEvent.substr(sEvent.find("pos=") + 4));

	string tmp;
	while (getline(in, tmp, '|')) {
		m_answerPos.insert(Move(COLOR_NONE, tmp, Game::getBoardSize()).getPosition());
	}
}

Move BaseSolver::getSolvedMove()
{
	TreeNode* pRoot = getSolvedRootNode();
	TreeNode* pChild = pRoot->getFirstChild();
	for (int i = 0; i < pRoot->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() != SOLUTION_WIN) { continue; }
		return pChild->getMove();
	}
	return Move(AgainstColor(pRoot->getMove().getColor()), Game::getBoardSize() * Game::getBoardSize());
}

float BaseSolver::getSolvedProbabilty()
{
	float fProb = 0.0f;
	TreeNode* pRoot = getSolvedRootNode();
	TreeNode* pChild = pRoot->getFirstChild();
	for (int i = 0; i < pRoot->getNumChild(); ++i, ++pChild) {
		if (m_answerPos.count(pChild->getMove().getPosition()) == 0) { continue; }
		fProb += pChild->getProbability();
	}
	return fProb;
}

string BaseSolver::getTreeInfo_r(TreeNode* pNode)
{
	ostringstream oss;

	// count the number of children
	int numChild = 0;
	TreeNode* pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getUctData().getCount() == 0) { continue; }
		++numChild;
	}

	// log the subtree of each child
	pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getUctData().getCount() == 0) { continue; }
		if (numChild > 1) { oss << "("; }
		oss << ";" << pChild->getMove().toSgfString(Game::getBoardSize())
			<< "C[" << getNodeInfo(pChild) << "]" << getTreeInfo_r(pChild);
		if (numChild > 1) { oss << ")"; }
	}
	return oss.str();
}

string BaseSolver::getNodeInfo(TreeNode* pNode)
{
	ostringstream oss;
	oss << "Hashkey: " << pNode->getHashkey() << "\r\n"
		<< "Status: " << getSolutionStatusString(pNode->getSolutionStatus()) << "\r\n"
		<< "Q: " << pNode->getUctData().toString() << "\r\n"
		<< "P: " << pNode->getProbability() << "\r\n"
		<< "V: " << pNode->getValue() << "\r\n";
	return oss.str();
}