#pragma once

#include "TreeNode.h"

class BaseMCTS {
protected:
	int m_simulation;
	long long m_nodeUsedIndex;
	int m_backupMove;
	Game m_game;
	TreeNode* m_nodes;
	map<double, int> m_valueMap;
	vector<TreeNode*> m_vSelectNodePath;
	
	// output from neural network
	float m_fValue;
	vector<pair<Move, float>> m_vProbability;

public:
	BaseMCTS()
	{
		long long int nodeSize = 1 + static_cast<long long int>(Configure::MCTS_SIMULATION_COUNT) * Game::getMaxNumLegalAction();
		m_nodes = new TreeNode[nodeSize];
		newGame();
	}

	~BaseMCTS() { delete[] m_nodes; }

	void newGame();
	virtual Move run(Color c, bool bWithPlay = true);
	void play(const Move& move);

	// MCTS four phases
	virtual void selection();
	virtual void evaluation() = 0;
	virtual void expansion();
	virtual void update();

	virtual inline bool isSimulationEnd() { return m_simulation >= Configure::MCTS_SIMULATION_COUNT; }
	inline int getSimulation() const { return m_simulation; }
	inline Game& getGame() { return m_game; }
	inline TreeNode* getRootNode() { return &m_nodes[0]; }
	inline bool isEndGame() { return m_game.isTerminal(); }

protected:
	void newTree();

	TreeNode* decideMCTSAction();
	float calculateInitQValue(TreeNode* pNode);
	float calculateMoveScore(TreeNode* pNode, int nParentSim, float fInitQValue);
	virtual TreeNode* selectChild(TreeNode* pNode);
	void addNoiseToChildren(TreeNode* pNode);
	vector<float> calculateDirichletNoise(int size, float fAlpha);
	void updateByWinLose(TreeNode* pNode, float fValue);
	void updateBySpaceComplexity(TreeNode* pNode, float fValue);
	void updateValueMap(double dOldValue, double dValue);
	string getMCTSSelectedMoveInfo(TreeNode* pSelected);
	string getBestSequenceString(TreeNode* pSelected);

	inline const float& getValue() { return m_fValue; }
	inline const vector<pair<Move, float>>& getProbability() { return m_vProbability; }
	inline void backupGame() { m_backupMove = m_game.getMoves().size(); }
	inline void rollbackGame() {
		assert(("Calling roll back without setting back up move", m_backupMove >= 0));
		while (m_game.getMoves().size() > m_backupMove) { m_game.undo(); }
	}

	inline TreeNode* allocateNewNodes(int size) {
		assert(("Use more nodes than declared node size", m_nodeUsedIndex + size <= 1 + static_cast<long long int>(Configure::MCTS_SIMULATION_COUNT) * Game::getMaxNumLegalAction()));
		//if (m_nodeUsedIndex / 1000000 != (m_nodeUsedIndex + size) / 1000000) { cerr << ((m_nodeUsedIndex + size) / 1000000) * 1000000 << endl; }
		TreeNode* pNode = &m_nodes[m_nodeUsedIndex];
		m_nodeUsedIndex += size;
		return pNode;
	}
};
