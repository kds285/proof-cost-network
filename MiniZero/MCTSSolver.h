#pragma once

#include "BaseMCTS.h"
#include "BaseSolver.h"
#include "Network.h"
#include "Random.h"
#include "Timer.h"

class MCTSSolver : public BaseMCTS, public BaseSolver {
public:
	MCTSSolver() {}
	~MCTSSolver() {}

	void solve();
	void selection();
	void expansion();
	void evaluation();
	void update();

private:
	TreeNode* selectChild(TreeNode* pNode);
	bool isAllChildrenSolutionLoss(TreeNode* pNode);
	bool playSgfGame(SgfLoader& sgfLoader);
	string getTreeInfo_r(TreeNode* pNode);
	string getUndoSgf(SgfLoader& sgfLoader);
	inline TreeNode* getSolvedRootNode() { return getRootNode(); }
	inline bool isSimulationEnd() { 
		if (Configure::SIM_CONTROL == SIM_CONTROL_COUNT) {
			return m_simulation >= Configure::MCTS_SIMULATION_COUNT;
		}
		else if (Configure::SIM_CONTROL == SIM_CONTROL_TIME) {
			m_timer.stop();
			return m_timer.getElapsedTime().count() >= Configure::TIME_LIMIT;
		}
		return false;
	}
	inline double getSolvedTime() {
		m_timer.stop();
		return m_timer.getElapsedTime().count();
	}
	bool foundEntryInTT();
	void storeTT(TreeNode* pNode);

private:
	StopTimer m_timer;
	bool m_bFoundInTT;
};

