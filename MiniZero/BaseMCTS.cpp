#include "BaseMCTS.h"
#include "Random.h"
#include "TimeSystem.h"
#include <iomanip>

void BaseMCTS::newGame()
{
	newTree();
	m_game.reset();
}

Move BaseMCTS::run(Color c, bool bWithPlay/* = true*/)
{
	m_game.setTurnColor(c);
	newTree();
	backupGame();

	while (!isSimulationEnd()) {
		selection();
		evaluation();
		expansion();
		update();
		rollbackGame();

		++m_simulation;
	}

	TreeNode* pSelected = decideMCTSAction();
	if (bWithPlay) { play(pSelected->getMove()); }
	return pSelected->getMove();
}

void BaseMCTS::play(const Move& move)
{
	m_game.play(move);
}

void BaseMCTS::selection()
{
	TreeNode* pNode = getRootNode();
	m_vSelectNodePath.clear();
	m_vSelectNodePath.push_back(pNode);
	while (pNode->hasChildren()) {
		pNode = selectChild(pNode);
		m_game.play(pNode->getMove());
		m_vSelectNodePath.push_back(pNode);
	}
}

void BaseMCTS::expansion()
{
	if (m_game.isTerminal()) { return; }

	const vector<pair<Move, float>>& vProbability = getProbability();
	TreeNode* pParent = m_vSelectNodePath.back();
	TreeNode* pChild = allocateNewNodes(vProbability.size());

	// assign info to parent node
	pParent->setFirstChild(pChild);
	pParent->setNumChild(vProbability.size());

	// assign policy to child nodes
	for (auto p : vProbability) {
		assert(("Expand illegal move", m_game.isLegalMove(p.first)));

		pChild->reset(p.first);
		pChild->setProbability(p.second);
		pChild->setProbabilityWithNoise(p.second);
		++pChild;
	}

	assert(("No child can be expanded", vProbability.size() > 0));

	switch (Configure::AOT_BRANCHING_FACTOR) {
	case 0: pParent->setBranchingFactor(Game::getMaxNumLegalAction()); break;
	case 1: pParent->setBranchingFactor(vProbability.size()); break;
	case 2:	default: break;
	}

	// add noise to root node
	if (Configure::MCTS_USE_NOISE_AT_ROOT && pParent == getRootNode()) { addNoiseToChildren(pParent); }
}

void BaseMCTS::update()
{
	assert(("Update without selecting any nodes", m_vSelectNodePath.size() > 0));

	float fValue = getValue();
	m_vSelectNodePath.back()->setValue(fValue);

	// update value
	for (int i = static_cast<int>(m_vSelectNodePath.size()) - 1; i >= 0; --i) {
		TreeNode* pNode = m_vSelectNodePath[i];

		if (Configure::NET_VALUE_WINLOSS) {
			updateByWinLose(pNode, fValue);
			fValue = -fValue;
		} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) { updateBySpaceComplexity(pNode, fValue); }
		else { assert(("Error configuration for training value target!", false)); }
	}
}

// private function
void BaseMCTS::newTree()
{
	m_simulation = 0;
	m_nodeUsedIndex = 1;
	m_backupMove = -1;
	getRootNode()->reset(Move(AgainstColor(m_game.getTurnColor()), -1));
	m_valueMap.clear();
	m_vSelectNodePath.clear();
}

TreeNode* BaseMCTS::decideMCTSAction()
{
	assert(("No Children can be chosen in decideMCTSAction", getRootNode()->getNumChild() > 0));

	int childNum = 0;
	float fSum = 0.0f;
	TreeNode* pRoot = getRootNode();
	TreeNode* pSelected = nullptr;
	TreeNode* pChild = pRoot->getFirstChild();

	for (int i = 0; i < pRoot->getNumChild(); ++i, ++pChild) {
		float fCount = pChild->getSimCount();
		if (fCount == 0) { continue; }

		if (Configure::MCTS_SELECT_BY_COUNT_PROPORTION) {
			// select move according to the number of simulation
			fSum += fCount;
			float fRand = Random::nextReal(fSum);
			if (fRand < fCount) { pSelected = pChild; }
		} else {
			if (pSelected == nullptr || fCount > pSelected->getSimCount()) { pSelected = pChild; }
		}
	}

	return pSelected;
}

float BaseMCTS::calculateInitQValue(TreeNode* pNode)
{
	float fSumOfChildWins = 0.0f;
	float fSumOfChildSims = 0.0f;
	TreeNode* pChild = pNode->getFirstChild();
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		if (!pChild->hasChildren()) { continue; }

		// we only consider the nodes which have been simulated
		fSumOfChildWins += pChild->getPUCTValueQ(m_valueMap, -1, rootTurn);
		fSumOfChildSims += 1;
	}

	// add additional one loss for other uninitialized node
	return (fSumOfChildWins - 1) / (fSumOfChildSims + 1);
}

float BaseMCTS::calculateMoveScore(TreeNode* pNode, int nParentSim, float fInitQValue)
{
	// U(s,a) = c_puct * sqrt(ln( N(s,b))) / N(s,a)
	float fValueU = sqrt(2.) * sqrt(log(nParentSim) / (1. + pNode->getSimCount()));

	// U(s,a) = c_puct * P(s,a) * sqrt(sum of N(s,b)) / (1 + N(s,a))
	if (Configure::USE_NET) {
		float fValuePa = Configure::MCTS_USE_NOISE_AT_ROOT ? pNode->getProbabilityWithNoise() : pNode->getProbability();
		float fPUCTBias = log((1 + nParentSim + 19652) / 19652) + Configure::MCTS_PUCT_BIAS;
		fPUCTBias = (Configure::MCTS_PUCT_BIAS == 0 ? 0 : fPUCTBias);
		fValueU = fPUCTBias * fValuePa * sqrt(nParentSim) / (1 + pNode->getSimCount());
	}

	// Q(s,a)
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
	float fValueQ = pNode->getPUCTValueQ(m_valueMap, fInitQValue, rootTurn);

	// action value = U(s,a) + Q(s,a)
	return fValueU + Configure::MCTS_Q_WEIGHT * fValueQ;
}

TreeNode* BaseMCTS::selectChild(TreeNode* pNode)
{
	assert(("Pass a null pointer to selectChild", pNode));

	TreeNode* pBest = nullptr;
	TreeNode* pChild = pNode->getFirstChild();
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());

	float fBestScore = -DBL_MAX;
	float fInitQValue = calculateInitQValue(pNode);
	int nParentSimulation = pNode->getSimCount();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		float fMoveScore = calculateMoveScore(pChild, nParentSimulation, fInitQValue);
		if (fMoveScore <= fBestScore) { continue; }

		fBestScore = fMoveScore;
		pBest = pChild;
	}

	return pBest;
}

void BaseMCTS::addNoiseToChildren(TreeNode* pNode)
{
	const float fEpsilon = Configure::ZERO_NOISE_EPSILON;
	vector<float> vDirichletNoise = calculateDirichletNoise(pNode->getNumChild(), Configure::ZERO_NOISE_ALPHA);
	TreeNode* pChild = pNode->getFirstChild();
	for (int i = 0; i < pNode->getNumChild(); ++i, ++pChild) {
		pChild->setProbabilityWithNoise((1 - fEpsilon) * pChild->getProbabilityWithNoise() + fEpsilon * vDirichletNoise[i]);
	}
}

vector<float> BaseMCTS::calculateDirichletNoise(int size, float fAlpha)
{
	boost::random::gamma_distribution<> pdf(fAlpha);
	boost::variate_generator<Random::IntegerGenerator&, boost::gamma_distribution<> >
		generator(*Random::integer_generator, pdf);

	vector<float> vDirichlet;
	for (int i = 0; i < size; i++) { vDirichlet.push_back(static_cast<float>(generator())); }
	float fSum = accumulate(vDirichlet.begin(), vDirichlet.end(), 0.0f);
	if (fSum < boost::numeric::bounds<float>::smallest()) { return vDirichlet; }
	for (int i = 0; i < vDirichlet.size(); i++) { vDirichlet[i] /= fSum; }
	return vDirichlet;
}

void BaseMCTS::updateByWinLose(TreeNode* pNode, float fValue)
{
	pNode->getUctData().add(fValue);
}

void BaseMCTS::updateBySpaceComplexity(TreeNode* pNode, float fValue)
{
	double dOld = pNode->getUctData().getMean();
	pNode->getUctData().add(fValue);
	updateValueMap(dOld, pNode->getUctData().getMean());
}

void BaseMCTS::updateValueMap(double dOldValue, double dValue)
{
	if (m_valueMap.find(dOldValue) != m_valueMap.end()) {
		--m_valueMap[dOldValue];
		if (m_valueMap[dOldValue] == 0) { m_valueMap.erase(dOldValue); }
	}
	++m_valueMap[dValue];
}

string BaseMCTS::getMCTSSelectedMoveInfo(TreeNode* pSelected)
{
	assert(("Pass a null pointer to getMCTSSearchInfo", pSelected));

	ostringstream oss;
	TreeNode* pRoot = getRootNode();
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());

	oss << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[" << m_game.getMoves().size() << "] ";
	oss << pSelected->getMove().toGtpString(Game::getBoardSize()) << " (" << pSelected->getUctData().toString();
	if (Configure::NET_VALUE_SPACE_COMPLEXITY) { oss << ", " << pSelected->getPUCTValueQ(m_valueMap, -1, rootTurn) << "/" << pSelected->getSimCount(); }
	oss << "), p: " << pSelected->getProbability();
	if (Configure::MCTS_USE_NOISE_AT_ROOT) { oss << ", p_noise: " << pSelected->getProbabilityWithNoise(); }
	oss << ", v: " << pSelected->getValue() << endl;

	if (Configure::NET_VALUE_SPACE_COMPLEXITY) { oss << "Value Bound: (" << m_valueMap.begin()->first << "," << m_valueMap.rbegin()->first << "), "; }
	oss << "Root: " << pRoot->getPUCTValueQ(m_valueMap, -1, rootTurn) << "/" << pRoot->getSimCount() << endl;

	return oss.str();
}

string BaseMCTS::getBestSequenceString(TreeNode* pSelected)
{
	assert(("Pass a null pointer to getBestSequenceString", pSelected));

	ostringstream oss;
	Color rootTurn = AgainstColor(getRootNode()->getMove().getColor());
	oss << colorToChar(pSelected->getMove().getColor()) << ","
		<< pSelected->getMove().toGtpString(Game::getBoardSize()) << " "
		<< "(" << pSelected->getPUCTValueQ(m_valueMap, -1, rootTurn)
		<< "/" << pSelected->getSimCount() << ")";

	while (pSelected->hasChildren()) {
		// select the child with maximum simulation
		TreeNode* pChild = pSelected->getFirstChild();
		TreeNode* pBest = pChild;
		for (int i = 0; i < pSelected->getNumChild(); ++i, ++pChild) {
			if (pChild->getSimCount() > pBest->getSimCount()) { pBest = pChild; }
		}
		pSelected = pBest;

		oss << " => " << colorToChar(pSelected->getMove().getColor()) << ","
			<< pSelected->getMove().toGtpString(Game::getBoardSize()) << " "
			<< "(" << pSelected->getPUCTValueQ(m_valueMap, -1, rootTurn)
			<< "/" << pSelected->getSimCount() << ")";
	}

	return oss.str();
}
