#include "DFPNSolver.h"
#include "SgfLoader.h"

void DFPNSolver::solve()
{
	std::srand(Configure::SEED);
	m_timer.reset();
	m_timer.start();
	newTree();
	TreeNode* pRoot = getRootNode();
	Move lastMove = m_game.getMoves().back();
	pRoot->setHashkey(m_game.getTTHashKey());
	MID(pRoot, DBL_MAX, DBL_MAX);
	m_game.play(lastMove);

	return;
}

void DFPNSolver::newTree()
{
	m_set.clear();
	m_nExpansion = 0;
	m_nMID = 0;
	m_nodeUsedIndex = 1;
	m_vSelectNodePath.clear();
	getRootNode()->reset(Move(AgainstColor(m_game.getTurnColor()), -1));
	m_transpositionTable.clear();

	return;
}

void DFPNSolver::evaluate(TreeNode* pNode)
{
	if (m_game.isTerminal()) {
		Color winner = m_game.eval();
		if (pNode->getMove().getColor() == winner) { pNode->setSolutionStatus(SOLUTION_WIN); }
		else if (pNode->getMove().getColor() != winner) { pNode->setSolutionStatus(SOLUTION_LOSS); }
	}

	return;
}

void DFPNSolver::setTerminalValue(TreeNode* pMPN)
{
	Color winner = m_game.eval();
	float value = 0.0f;
	if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
		Color proofColor = (Configure::AOT_PROOF_COLOR == COLOR_NONE) ? rootTurn : Configure::AOT_PROOF_COLOR;
		value = (winner == proofColor ? 0.0f : Configure::NET_NUM_OUTPUT_V);
	} else {
		value = (winner == COLOR_NONE ? 0.0f : (winner == pMPN->getMove().getColor() ? 1.0f : -1.0f));
	}
	pMPN->setValue(value);

	return;
}

void DFPNSolver::updateSolutionStatus(TreeNode* pNode)
{
	if (!pNode->hasChildren()) { return; }

	TreeNode* pChild = pNode->getFirstChild();
	bool bAllChildrenLoss = true;
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() == SOLUTION_WIN) { pNode->setSolutionStatus(SOLUTION_LOSS); }
		if (pChild->getSolutionStatus() != SOLUTION_LOSS) { bAllChildrenLoss = false; }
	}
	if (bAllChildrenLoss) { pNode->setSolutionStatus(SOLUTION_WIN); }

	return;
}

void DFPNSolver::updatePNDN(TreeNode* pNode, double initPN, double initDN)
{
	if (pNode->getSolutionStatus() == SOLUTION_WIN) { pNode->setProofNumber(DBL_MAX); pNode->setDisproofNumber(0); return; }
	else if (pNode->getSolutionStatus() == SOLUTION_LOSS) { pNode->setProofNumber(0); pNode->setDisproofNumber(DBL_MAX); return; }

	// solution unknown
	if (pNode->hasChildren()) {
		TreeNode* pChild = pNode->getFirstChild();
		int numLiveChildren = 0;
		int limitSize = getNumLimitSize(pNode);
		double minProofNumber = DBL_MAX;
		double sumDisproofNumber = 0;
		double maxDisproofNumber = 0; // weak PNS
		for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
			if (pChild->getSolutionStatus() != SOLUTION_UNKNOWN) { continue; }
			minProofNumber = fmin(minProofNumber, pChild->getDisproofNumber());
			sumDisproofNumber += pChild->getProofNumber();
			if (pChild->getProofNumber() > maxDisproofNumber) { maxDisproofNumber = pChild->getProofNumber(); }

			++numLiveChildren;
			if (numLiveChildren == limitSize) { break; }
		}
		// If enable weak PNS, replace with the following
		if (Configure::PNS_ENABLE_WEAK_PNS) { sumDisproofNumber = maxDisproofNumber + (numLiveChildren - 1); }

		pNode->setProofNumber(minProofNumber);
		pNode->setDisproofNumber(sumDisproofNumber);
	} else {
		pNode->setProofNumber(initPN); pNode->setDisproofNumber(initDN);
	}

	return;
}

void DFPNSolver::MID(TreeNode* pNode, double PNthreshold, double DNthreshold)
{
	++m_nMID;
	evaluate(pNode);
	if (m_game.isTerminal()) {
		setTerminalValue(pNode);
		updateSolutionStatus(pNode);
		updatePNDN(pNode);
		DFPNTTEntry entry;
		entry.m_dProofNumber = pNode->getProofNumber();
		entry.m_dDisproofNumber = pNode->getDisproofNumber();
		entry.m_solutionStatus = pNode->getSolutionStatus();
		storeTTEntry(pNode->getHashkey(), entry);
		//if (m_transpositionTable.getCount() % 10000 == 0) { cerr << m_transpositionTable.getCount() << endl; }
		m_game.undo();
		return;
	}

	// Expand node
	vector<TreeNode> vChildren;
	vector<pair<Move, float>> vCandidate;
	float proofValue = 0.0f;
	float disproofValue = 0.0f;

	if (Configure::PNS_MODE == VANILLA_PNS) { expandVanillaNode(pNode, vChildren); } 
	else { expandCNNNode(pNode, vCandidate, vChildren, proofValue, disproofValue); }

	// MID MPN
	while (1) {
		updateSolutionStatus(pNode);
		updatePNDN(pNode);
		if (pNode->getProofNumber() >= PNthreshold || pNode->getDisproofNumber() >= DNthreshold || isExpansionEnd()) {
			// 1. First meet, store value and policy
			// 2. Not first, only update PN and DN to TT.
			int index = getTTEntryIndex(pNode->getHashkey());
			if (index == -1) {
				// new and store
				DFPNTTEntry entry;
				entry.m_fProofValue = proofValue;
				entry.m_fDisproofValue = disproofValue;
				entry.m_dProofNumber = pNode->getProofNumber();
				entry.m_dDisproofNumber = pNode->getDisproofNumber();
				entry.m_solutionStatus = pNode->getSolutionStatus();
				entry.m_vCandidate = vCandidate;
				storeTTEntry(pNode->getHashkey(), entry);
				//if (m_transpositionTable.getCount() % 10000 == 0) { cerr << m_transpositionTable.getCount() << endl; }
			} else {
				// update
				DFPNTTEntry& entry = getTTEntry(index);
				entry.m_dProofNumber = pNode->getProofNumber();
				entry.m_dDisproofNumber = pNode->getDisproofNumber();
				entry.m_solutionStatus = pNode->getSolutionStatus();
			}
			m_game.undo();
			pNode->setNumChild(0);
			return;
		}

		double dMinDN = DBL_MAX;
		double d2ndMinDN = DBL_MAX;
		double dPnOfMinDNChild = 0.0f;
		TreeNode* pMPN = selectBestChild(pNode, dPnOfMinDNChild, dMinDN, d2ndMinDN);
		// If found win in TT, not MID.
		if (pMPN->getSolutionStatus() == SOLUTION_WIN) { continue; }

		m_game.play(pMPN->getMove());
		double dThresChangeTo2nd = Configure::PNS_ENABLE_1_PLUS_EPSILON ? ceil(d2ndMinDN*(1.00f + Configure::PNS_EPSILON_VALUE)) : (d2ndMinDN + 1.00f);
		double dNextPNThreshold = (DNthreshold == DBL_MAX) ? DNthreshold : DNthreshold - pNode->getDisproofNumber() + dPnOfMinDNChild;
		double dNextDNThreshold = fmin(PNthreshold, dThresChangeTo2nd);
		MID(pMPN, dNextPNThreshold, dNextDNThreshold);
	}

	return;
}

void DFPNSolver::expandVanillaNode(TreeNode* pNode, vector<TreeNode>& vChildren)
{
	Color turnColor = m_game.getTurnColor();
	vector<Move> vCandidate;
	for (int pos = 0; pos < Game::getMaxNumLegalAction(); ++pos) {
		const Move m(turnColor, pos);
		if (!m_game.isLegalMove(m)) { continue; }

		vCandidate.push_back(m);
	}	
	std::random_shuffle(vCandidate.begin(), vCandidate.end());
	vChildren.resize(vCandidate.size());
	pNode->setNumChild(vCandidate.size());
	pNode->setFirstChild(&vChildren[0]);
	TreeNode* pChild = pNode->getFirstChild();
	for (int iCandidate = 0; iCandidate < vCandidate.size(); ++iCandidate, ++pChild) {
		pChild->reset(vCandidate[iCandidate]);
		assert(m_game.isLegalMove(pChild->getMove()));
		m_game.play(pChild->getMove());
		pChild->setHashkey(m_game.getTTHashKey());
		m_game.undo();
		if (findEntryInTT(pChild->getHashkey())) { getPNDNfromTT(pChild); }
		else { updatePNDN(pChild); }
	}

	return;
}

void DFPNSolver::expandCNNNode(TreeNode* pNode, vector<pair<Move, float>>& vCandidate, vector<TreeNode>& vChildren, float& proofValue, float& disproofValue)
{
	// expand
	int index = getTTEntryIndex(pNode->getHashkey());
	if (index != -1) {
		vCandidate = getTTEntry(index).m_vCandidate;
	} else {
		m_network->set_data(0, m_game);
		m_network->forward();
		vCandidate = m_network->getProbability(0);
	}
	vChildren.resize(vCandidate.size());
	pNode->setNumChild(vCandidate.size());
	pNode->setFirstChild(&vChildren[0]);

	// value network
	if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
		Color proofColor = (Configure::AOT_PROOF_COLOR == COLOR_NONE) ? rootTurn : Configure::AOT_PROOF_COLOR;
		if (index != -1) {
			proofValue = getTTEntry(index).m_fProofValue;
			disproofValue = getTTEntry(index).m_fDisproofValue;
		} else {
			proofValue = m_network->getValue(0, proofColor);
			disproofValue = m_network->getValue(0, AgainstColor(proofColor));
		}
	} else {
		if (index != -1) {
			float value = getTTEntry(index).m_fProofValue;
			pNode->setValue(value);
		} else {
			float value = m_network->getValue(0);
			proofValue = value;
			pNode->setValue(-1.0f*value);
		}
	}

	TreeNode* pChild = pNode->getFirstChild();
	for (int iCandidate = 0; iCandidate < vCandidate.size(); ++iCandidate, ++pChild) {
		pChild->reset(vCandidate[iCandidate].first);
		pChild->setProbability(vCandidate[iCandidate].second);
		assert(m_game.isLegalMove(pChild->getMove()));
		m_game.play(pChild->getMove());
		pChild->setHashkey(m_game.getTTHashKey());
		m_game.undo();
		if (findEntryInTT(pChild->getHashkey())) { getPNDNfromTT(pChild);}
		else {
			if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
				Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
				Color proofColor = (Configure::AOT_PROOF_COLOR == COLOR_NONE) ? rootTurn : Configure::AOT_PROOF_COLOR;
				if (pChild->getMove().getColor() == proofColor) { updatePNDN(pChild, disproofValue / (float)vCandidate.size(), proofValue); }
				else if (pChild->getMove().getColor() == AgainstColor(proofColor)) { updatePNDN(pChild, proofValue / (float)vCandidate.size(), disproofValue); }
			} else { updatePNDN(pChild); }
		}
	}

	return;
}

TreeNode* DFPNSolver::selectBestChild(TreeNode* pNode, double& dPnOfMinDNChild, double& dMinDN, double& d2ndMinDN)
{
	TreeNode* pChild = pNode->getFirstChild();
	// if win node if found, just return it.
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() == SOLUTION_WIN) { return pChild; }
	}

	TreeNode* pMPN = NULL;
	pChild = pNode->getFirstChild();
	int numLiveChildren = 0;
	int limitSize = getNumLimitSize(pNode);
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() != SOLUTION_UNKNOWN) { continue; }

		double dn = pChild->getDisproofNumber();
		if (dn < dMinDN) {
			d2ndMinDN = dMinDN;
			dMinDN = dn;
			dPnOfMinDNChild = pChild->getProofNumber();
			pMPN = pChild;
		}
		else if (dn < d2ndMinDN) { d2ndMinDN = dn; }
		++numLiveChildren;
		if (numLiveChildren == limitSize) { break; }
	}

	return pMPN;
}

int DFPNSolver::getNumLimitSize(TreeNode* pNode)
{
	if (Configure::PNS_MODE != FOCUSED_PNS) { return pNode->getNumChild(); }

	int numLiveChildren = 0;
	TreeNode* pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() == SOLUTION_UNKNOWN) { ++numLiveChildren; }
	}

	float ratio = Configure::NET_VALUE_SPACE_COMPLEXITY ? Configure::PNS_FOCUSED_WIDENING_FACTOR : fmin(Configure::PNS_FOCUSED_WIDENING_FACTOR, 1.00f + getAdjustedValue(pNode));

	return Configure::PNS_FOCUSED_CHILDREN_BASE + ceil(ratio * (float)(numLiveChildren));
}

float DFPNSolver::getAdjustedValue(TreeNode* pNode)
{
	float fAdjustedValue = (-1.0f)*pNode->getValue();
	return fmin(fmax(fAdjustedValue, -1), 1);
}

void DFPNSolver::storeTTEntry(HashKey hashkey, DFPNTTEntry& entry)
{
	while (m_transpositionTable.isFull()) { cerr << "TT is full!" << endl; }
	m_transpositionTable.store(hashkey, entry);
}

DFPNTTEntry& DFPNSolver::getTTEntry(unsigned int index)
{
	return m_transpositionTable.m_entry[index].m_data;
}

unsigned int DFPNSolver::getTTEntryIndex(HashKey hashkey)
{
	return m_transpositionTable.lookup(hashkey);
}

void DFPNSolver::getPNDNfromTT(TreeNode* pNode)
{
	int index = getTTEntryIndex(pNode->getHashkey());
	if (index != -1) {
		DFPNTTEntry& entry = getTTEntry(index);
		pNode->setSolutionStatus(entry.m_solutionStatus);
		pNode->setProofNumber(entry.m_dProofNumber);
		pNode->setDisproofNumber(entry.m_dDisproofNumber);
	}

	return;
}

TreeNode* DFPNSolver::allocateNewNodes(int size)
{
	if (m_nodeUsedIndex + size > 1 + Configure::PNS_NUM_EXPANSION * Game::getMaxNumLegalAction()) {
		cerr << "Run out of nodes" << endl;
		exit(-1);
	}

	assert(("Use more nodes than declared node size", m_nodeUsedIndex + size <= 1 + Configure::PNS_NUM_EXPANSION * Game::getMaxNumLegalAction()));
	TreeNode* pNode = &m_nodes[m_nodeUsedIndex];
	m_nodeUsedIndex += size;

	return pNode;
}

bool DFPNSolver::playSgfGame(SgfLoader& sgfLoader)
{
	m_game.reset();
	for (int i = 1; i < sgfLoader.getSgfNode().size(); ++i) {
		Move m(sgfLoader.getSgfNode()[i].m_move, Game::getBoardSize());
		if (!m_game.isLegalMove(m)) { return false; }
		m_game.play(m);
	}
	return true;
}

string DFPNSolver::getNodeInfo(TreeNode* pNode)
{
	ostringstream oss;
	oss << "Status: " << getSolutionStatusString(pNode->getSolutionStatus()) << "\r\n"
		<< "count: " << pNode->getUctData().getCount() << "\r\n"
		<< "P: " << pNode->getProbability() << "\r\n"
		<< "V: " << pNode->getValue() << "\r\n";
	oss << "PN: "; if (pNode->getProofNumber() == DBL_MAX) oss << "INF"; else oss << pNode->getProofNumber(); oss << "\r\n";
	oss << "DN: "; if (pNode->getDisproofNumber() == DBL_MAX) oss << "INF"; else oss << pNode->getDisproofNumber(); oss << "\r\n";
	oss << "LimitSize: " << getNumLimitSize(pNode) << "\r\n";
	oss << "Adjusted Value: " << getAdjustedValue(pNode) << "\r\n";
	
	int numLiveChildren = 0;
	TreeNode* pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (pChild->getSolutionStatus() == SOLUTION_UNKNOWN) { ++numLiveChildren; }
	}
	oss << "NumLiveChildren: " << numLiveChildren << "\r\n";

	return oss.str();
}

std::string DFPNSolver::getTTEntryInfo(DFPNTTEntry& entry)
{
	ostringstream oss;
	oss << "Status: " << getSolutionStatusString(entry.m_solutionStatus) << "\r\n"
		<< "ProofValue: " << entry.m_fProofValue << "\r\n"
		<< "DisproofValue: " << entry.m_fDisproofValue << "\r\n";
	oss << "PN: "; if (entry.m_dProofNumber == DBL_MAX) oss << "INF"; else oss << entry.m_dProofNumber; oss << "\r\n";
	oss << "DN: "; if (entry.m_dDisproofNumber == DBL_MAX) oss << "INF"; else oss << entry.m_dDisproofNumber; oss << "\r\n";

	return oss.str();
}

string DFPNSolver::getUndoSgf(SgfLoader& sgfLoader)
{
	playSgfGame(sgfLoader);
	m_game.undo();
	m_game.undo();
	map <string, string> tag;
	tag["EV"] = "pos=";
	return m_game.getGameRecord(tag, Game::getBoardSize());
}