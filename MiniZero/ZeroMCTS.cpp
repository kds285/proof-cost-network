#include "ZeroMCTS.h"

Move ZeroMCTS::run(Color c, bool bWithPlay)
{
	m_game.setTurnColor(c);
	newTree();
	backupGame();

	while (!isSimulationEnd()) {
		selection();
		if (Configure::USE_NET) {
			calculateFeatureAndAddToNet();
			m_network->forward();
		}
		evaluation();
		expansion();
		update();
		rollbackGame();

		++m_simulation;
	}

	TreeNode* pSelected = decideMCTSAction();
	if (bWithPlay) { play(pSelected->getMove()); }
	display(pSelected);
	return pSelected->getMove();
}

void ZeroMCTS::runMCTSSimulationBeforeForward()
{
	if (isSimulationEnd() || m_simulation == 0) {
		newTree();
		backupGame();
	}
	selection();
	calculateFeatureAndAddToNet();
}

void ZeroMCTS::runMCTSSimulationAfterForward()
{
	if (m_vSelectNodePath.empty()) { return; }

	evaluation();
	expansion();
	update();
	rollbackGame();

	++m_simulation;
	if (isSimulationEnd()) {
		handleSimulationEnd();
		if (isEndGame()) { handleEndGame(); }
	}
}

void ZeroMCTS::evaluation()
{
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
	// policy
	m_vProbability = m_network->getProbability(BATCH_ID);

	// value
	if (Configure::NET_VALUE_WINLOSS) {
		m_fValue = m_network->getValue(BATCH_ID);
		if (m_game.isTerminal()) {
			Color winner = m_game.eval();
			m_fValue = (winner == COLOR_NONE ? 0.0f : (winner == m_game.getTurnColor() ? 1.0f : -1.0f));
		}
		// we should reverse the value since the last selected node is the inverse win rate of node color
		m_fValue = -m_fValue;
	} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
		Color proofColor = (Configure::AOT_PROOF_COLOR == COLOR_NONE) ? rootTurn : Configure::AOT_PROOF_COLOR;
		m_fValue = m_network->getValue(BATCH_ID, proofColor);
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

void ZeroMCTS::calculateFeatureAndAddToNet()
{
	m_network->set_data(BATCH_ID, m_game);
}

void ZeroMCTS::handleSimulationEnd()
{
	assert(("Calling handleSimulationEnd without meeting simulation end", isSimulationEnd()));

	TreeNode* pSelected = decideMCTSAction();
	m_game.addComment(m_game.getMoves().size(), getMoveComment(pSelected));
	play(pSelected->getMove());
	display(pSelected);
}

void ZeroMCTS::handleEndGame()
{
	assert(("Calling handleEndGame without meeting end game", isEndGame()));

	boost::lock_guard<boost::mutex> lock(m_mutex);
	int total_moves = m_game.getMoves().size();
	map<string, string> mTag;
	mTag["EV"] = Configure::MACHINE_NAME + ";" + m_network->getModelName();
	mTag["DT"] = TimeSystem::getTimeString("Y/m/d_H:i:s.f");
	mTag["RE"] = string{colorToChar(m_game.eval())};
	cout << "Self-play " << total_moves << " " << m_game.getGameRecord(mTag, Game::getBoardSize()) << endl;
	newGame();
}

void ZeroMCTS::display(TreeNode* pSelected)
{
	if (!m_bDisplay) { return; }

	cerr << m_game.getBoardString(Game::getBoardSize());
	cerr << "Model: " << m_network->getModelName() << endl;

	if (pSelected) { cerr << getMCTSSelectedMoveInfo(pSelected); }
	cerr << endl << endl;
}

string ZeroMCTS::getMoveComment(TreeNode* pSelected)
{
	string sComment = "";
	TreeNode* pRoot = getRootNode();
	TreeNode* pChild = pRoot->getFirstChild();

	for (int i = 0; i < pRoot->getNumChild(); ++i, ++pChild) {
		int count = pChild->getSimCount();
		if (count == 0) { continue; }

		// count for each move
		sComment += (sComment == "") ? "" : ",";
		sComment += to_string(pChild->getMove().getPosition()) + ":" + to_string(count);
	}

	// branching factor & detail information for debug
	sComment += "%" + to_string(pRoot->getBranchingFactor());
	sComment += "*" + getMoveCommentDetail(pSelected);
	return sComment;
}

string ZeroMCTS::getMoveCommentDetail(TreeNode* pSelected)
{
	assert(("Pass a null pointer to getSearchDetailsString", pSelected));

	ostringstream oss;
	TreeNode* pRoot = getRootNode();
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());

	oss << "@";
	if (Configure::NET_VALUE_SPACE_COMPLEXITY) { oss << "Value Bound: (" << m_valueMap.begin()->first << "," << m_valueMap.rbegin()->first << ")@"; }
	oss << "Root: " << pRoot->getPUCTValueQ(m_valueMap, -1, rootTurn) << "/" << pRoot->getSimCount() << "@";
	oss << "Selected: " << pSelected->getPUCTValueQ(m_valueMap, -1, rootTurn) << "/" << pSelected->getSimCount() << "@";
	oss << "Sequence: " << getBestSequenceString(pSelected) << "@";

	float fBestScore = -DBL_MAX;
	float fInitQValue = calculateInitQValue(pRoot);
	TreeNode* pChild = pRoot->getFirstChild();
	int nParentSimulation = pRoot->getSimCount();
	oss << "Init Q Value: " << fInitQValue << "@";
	oss << "move rank action Q      Q_       V        U     P     P-Dir N    soft-N@";
	for (int i = 0; i < pRoot->getNumChild(); ++i, ++pChild) {
		if (pChild->getSimCount() == 0 && i >= 3) { continue; }

		// action value = U(s,a) + Q(s,a)
		// U(s,a) = c_puct * P(s,a) * sqrt(sum of N(s,b)) / (1 + N(s,a))
		float fValuePa = Configure::MCTS_USE_NOISE_AT_ROOT ? pChild->getProbabilityWithNoise() : pChild->getProbability();
		float fPUCTBias = log((1 + nParentSimulation + 19652) / 19652) + Configure::MCTS_PUCT_BIAS;
		fPUCTBias = (Configure::MCTS_PUCT_BIAS == 0 ? 0 : fPUCTBias);
		float fValueU = fPUCTBias * fValuePa * sqrt(nParentSimulation) / (1 + pChild->getSimCount());

		// Q(s,a)
		float fValueQ = pChild->getPUCTValueQ(m_valueMap, fInitQValue, rootTurn);

		float fMoveScore = fValueU + Configure::MCTS_Q_WEIGHT * fValueQ;
		oss << left << setw(4) << pChild->getMove().toGtpString(Game::getBoardSize()) << " "	// move
			<< setw(4) << (i + 1) << " " << fixed << setprecision(3)							// rank
			<< setw(6) << fMoveScore << " "														// action
			<< setw(6) << fValueQ << " "														// Q
			<< setw(8) << pChild->getUctData().getMean() << " "									// Q_
			<< setw(8) << pChild->getValue() << " "												// V
			<< setw(5) << fValueU << " "														// U
			<< setw(5) << pChild->getProbability() << " "										// P
			<< setw(5) << fValuePa << " "														// P-Dir
			<< setw(5) << pChild->getSimCount() << " "											// N
			<< setw(5) << (pChild->getSimCount() * 1.0f / (nParentSimulation - 1))				// soft-N
			<< (pChild == pSelected ? " *" : "") << "@";
	}

	return oss.str();
}
