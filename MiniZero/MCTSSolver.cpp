#include "MCTSSolver.h"
#include "SgfLoader.h"

void MCTSSolver::solve()
{
	newTree();
	backupGame();
	m_TT.clear();
	m_timer.reset();
	m_timer.start();
	
	TreeNode* pRoot = getRootNode();
	while (pRoot->getSolutionStatus() == SOLUTION_UNKNOWN && !isSimulationEnd()) {
		selection();
		evaluation();
		expansion();
		update();
		rollbackGame();
		++m_simulation;
		//if (m_simulation % 10000 == 0) { cerr << m_simulation << endl; }
	}
}

void MCTSSolver::selection()
{
	m_bFoundInTT = false;
	TreeNode* pNode = getRootNode();
	m_vSelectNodePath.clear();
	m_vSelectNodePath.push_back(pNode);
	while (pNode->hasChildren()) {
		pNode = selectChild(pNode);
		m_game.play(pNode->getMove());
		m_vSelectNodePath.push_back(pNode);
		if (foundEntryInTT()) {
			m_bFoundInTT = true;
			break;
		}
	}
}

void MCTSSolver::expansion()
{
	if (m_bFoundInTT) { return; }
	BaseMCTS::expansion();
}

void MCTSSolver::evaluation()
{
	m_vSelectNodePath.back()->setHashkey(m_game.getHashKey());
	if (!Configure::USE_NET) {
		int num = 0;
		Color cTurn = m_game.getTurnColor();
		while (!m_game.isTerminal()) {
			vector<int> legalMoves;
			for (int p = 0; p < m_game.getMaxNumLegalAction(); ++p) {
				if (m_game.isLegalMove(Move(m_game.getTurnColor(), p))) {
					legalMoves.push_back(p);
				}
			}
			int randMove = Random::nextInt(legalMoves.size());
			m_game.play(Move(m_game.getTurnColor(), legalMoves[randMove]));
			++num;
		}
		m_fValue = (m_game.eval() == cTurn) ? -1. : 1.;
		for (int i = 0; i < num; ++i) {
			m_game.undo();
		}
		m_vProbability.clear();
		for (int pos = 0; pos < Game::getMaxNumLegalAction(); ++pos) {
			if (m_game.isLegalMove(Move(m_game.getTurnColor(), pos))) {
				m_vProbability.push_back({Move(m_game.getTurnColor(), pos), -1.});
			}
		}
		return;
	}
	
	m_network->set_data(0, m_game);
	m_network->forward();

	// policy
	m_vProbability = m_network->getProbability(0);

	// value
	if (Configure::NET_VALUE_WINLOSS) {
		m_fValue = m_network->getValue(0);
		if (m_game.isTerminal()) {
			Color winner = m_game.eval();
			m_fValue = (winner == COLOR_NONE ? 0.0f : (winner == m_game.getTurnColor() ? 1.0f : -1.0f));
		}
		// we should reverse the value since the last selected node is the inverse win rate of node color
		m_fValue = -m_fValue;
	} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
		Color proofColor = (Configure::AOT_PROOF_COLOR == COLOR_NONE) ? rootTurn : Configure::AOT_PROOF_COLOR;
		m_fValue = m_network->getValue(0, proofColor);
		if (m_game.isTerminal()) { m_fValue = (m_game.eval() == proofColor ? 0.0f : Configure::NET_NUM_OUTPUT_V); }

		// calculate value from root's perspective
		for (int i = static_cast<int>(m_vSelectNodePath.size()) - 1; i > 0; i--) {
			TreeNode* pNode = m_vSelectNodePath[i];
			TreeNode* pParent = m_vSelectNodePath[i - 1];

			// add log of branching factor to value (only for against proof color)
			assert(("Branching Factor is negative or 0", pParent->getBranchingFactor() > 0));
			Color disproofColor = AgainstColor(proofColor);
			if (pNode->getMove().getColor() == disproofColor) { m_fValue += log10(pParent->getBranchingFactor()); }
		}

		// rescale value to [0, Configure::NET_NUM_OUTPUT_V)
		m_fValue = fmax(0, fmin(m_fValue, Configure::NET_NUM_OUTPUT_V - 1));
	} else { assert(("Error configuration for training value target!", false)); }
}

void MCTSSolver::update()
{
	BaseMCTS::update();
	if (!m_game.isTerminal() && !m_bFoundInTT) { return; }

	// update solution status
	if (m_bFoundInTT) {
		TTentry& entry = m_TT.getEntry(m_TT.lookup(m_game.getTTHashKey()));
		TreeNode* pMatchTT = m_vSelectNodePath.back();
		pMatchTT->setSolutionStatus(entry.m_solutionStatus);
	} else if (m_game.isTerminal()) {
		Color winner = m_game.eval();
		TreeNode* pTerminal = m_vSelectNodePath.back();
		pTerminal->setSolutionStatus((pTerminal->getMove().getColor() == winner ? SOLUTION_WIN : SOLUTION_LOSS));
	}

	for (int i = static_cast<int>(m_vSelectNodePath.size()) - 1; i > 0; --i) {
		TreeNode* pNode = m_vSelectNodePath[i];
		TreeNode* pParent = m_vSelectNodePath[i - 1];

		if (pNode->getSolutionStatus() == SOLUTION_WIN) {
			pParent->setSolutionStatus(SOLUTION_LOSS);
			m_game.undo();
			storeTT(pParent);
		} else if (pNode->getSolutionStatus() == SOLUTION_LOSS) {
			m_game.undo();
			if (isAllChildrenSolutionLoss(pParent)) {
				pParent->setSolutionStatus(SOLUTION_WIN);
				storeTT(pParent);
			}
			else { break; }
		}
	}
}

TreeNode* MCTSSolver::selectChild(TreeNode* pNode)
{
	assert(("Pass a null pointer to selectChild", pNode));

	TreeNode* pBest = nullptr;
	TreeNode* pChild = pNode->getFirstChild();
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());

	float fBestScore = -DBL_MAX;
	float fInitQValue = calculateInitQValue(pNode);
	int nParentSimulation = pNode->getSimCount();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		// skip the node if it is proved
		if (pChild->getSolutionStatus() != SOLUTION_UNKNOWN) { continue; }

		float fMoveScore = calculateMoveScore(pChild, nParentSimulation, fInitQValue);
		if (fMoveScore <= fBestScore) { continue; }

		fBestScore = fMoveScore;
		pBest = pChild;
	}

	return pBest;
}

bool MCTSSolver::isAllChildrenSolutionLoss(TreeNode* pNode)
{
	TreeNode* pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() != SOLUTION_LOSS) { return false; }
	}
	return true;
}

bool MCTSSolver::playSgfGame(SgfLoader& sgfLoader)
{
	newGame();
	for (int i = 1; i < sgfLoader.getSgfNode().size(); ++i) {
		Move m(sgfLoader.getSgfNode()[i].m_move, Game::getBoardSize());
		if (!m_game.isLegalMove(m)) { return false; }
		play(m);
	}
	return true;
}

std::string MCTSSolver::getTreeInfo_r(TreeNode* pNode)
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

string MCTSSolver::getUndoSgf(SgfLoader& sgfLoader)
{
	playSgfGame(sgfLoader);
	m_game.undo();
	m_game.undo();
	map <string, string> tag;
	tag["EV"] = "pos=";
	return m_game.getGameRecord(tag, Game::getBoardSize());
}

bool MCTSSolver::foundEntryInTT()
{
	if (!Configure::USE_TRANSPOSITION_TABLE) { return false; }

	return (m_TT.lookup(m_game.getTTHashKey()) != -1);
}

void MCTSSolver::storeTT(TreeNode* pNode)
{
	if (!Configure::USE_TRANSPOSITION_TABLE) { return; }
	if (foundEntryInTT()) { return; }
	while (m_TT.isFull()) { cerr << "TT is full!" << endl; }

	TTentry entry;
	entry.m_solutionStatus = pNode->getSolutionStatus();
	m_TT.store(m_game.getTTHashKey(), entry);

	return;
}