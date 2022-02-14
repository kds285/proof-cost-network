#pragma once

#include "Network.h"
#include "TreeNode.h"
#include "SgfLoader.h"
#include "TranspositionTable.h"

class BaseSolver {
protected:
	static const int SIM_CONTROL_COUNT = 0;
	static const int SIM_CONTROL_TIME = 1;
	static const int SIM_CONTROL_TT_NODE = 2;
	static const int SIM_CONTROL_MID = 3;

protected:
	string m_sSgfString;
	string m_sOutputFileName;
	string m_sProblemFileName;
	fstream m_fAnswer;
	Network* m_network;
	set<int> m_answerPos;
	TranspositionTable m_TT;

public:
	BaseSolver() {
		initNetwork();
	}
	~BaseSolver() { delete m_network; }

	void runSolver();
	bool loadProblem(string sProblem);
	virtual void solve() = 0;

protected:
	void initNetwork();
	void lockProblem();
	void saveAnswer();
	void saveTree();
	int getNNModelIteration();
	string getAnswerFileName();
	void parseAnswerPosition(string sEvent);
	Move getSolvedMove();
	float getSolvedProbabilty();
	virtual string getTreeInfo_r(TreeNode* pNode);
	virtual string getNodeInfo(TreeNode* pNode);
	virtual int getSolvedSimulation() { return getSolvedRootNode()->getUctData().getCount(); }
	virtual unsigned long long getReExpansion() { return 0; }
	virtual double getSolvedTime() { return 0; }
	inline SOLUTION_STATUS getSolvedStatus() { return getReverseSolutionStatus(getSolvedRootNode()->getSolutionStatus()); }

	virtual TreeNode* getSolvedRootNode() = 0;
	virtual bool playSgfGame(SgfLoader& sgfLoader) = 0;
	virtual string getUndoSgf(SgfLoader& sgfLoader) = 0;
};

