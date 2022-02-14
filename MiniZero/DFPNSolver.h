#pragma once

#include "BaseSolver.h"
#include "TreeNode.h"
#include "Rand64.h"
#include "OpenAddressHashTable.h"
#include <set>
#include "Timer.h"

#define ull unsigned long long

class DFPNTTEntry {
public:
	float m_fProofValue;
	float m_fDisproofValue;
	double m_dProofNumber;
	double m_dDisproofNumber;
	SOLUTION_STATUS m_solutionStatus;
	vector<pair<Move, float>> m_vCandidate;

	DFPNTTEntry() {
		clear();
	}

	DFPNTTEntry(float proofVal, float disproofVal, double pn, double dn, SOLUTION_STATUS status, vector<pair<Move, float>> vProb) {
		m_fProofValue = proofVal;
		m_fDisproofValue = disproofVal;
		m_dProofNumber = pn;
		m_dDisproofNumber = dn;
		m_solutionStatus = status;
		m_vCandidate = vProb;
	}

	void clear() {
		m_dProofNumber = m_dDisproofNumber = -1;
		m_fProofValue = m_fDisproofValue = 0.0f;
		m_solutionStatus = SOLUTION_UNKNOWN;
		m_vCandidate.clear();
	}
};

class DFPNSolver : public BaseSolver
{
	static const int VANILLA_PNS = 0;
	static const int POLICY_PNS = 1;
	static const int FOCUSED_PNS = 2;

public:
	DFPNSolver() : m_transpositionTable(28) {
		if (Configure::PNS_ENABLE_DFPN) { m_nodes = new TreeNode[1]; }
		else { m_nodes = new TreeNode[1 + Configure::PNS_NUM_EXPANSION * Game::getMaxNumLegalAction()]; }
	}
	~DFPNSolver() { delete[] m_nodes; }

	void solve();

private:
	void newTree();
	void evaluate(TreeNode* pNode);
	void setTerminalValue(TreeNode* pMPN);
	void updateSolutionStatus(TreeNode* pNode);
	void updatePNDN(TreeNode* pNode, double initPN = 1.0f, double initDN = 1.0f);
	void MID(TreeNode* pNode, double PNthreshold, double DNthreshold);
	void expandVanillaNode(TreeNode* pNode, vector<TreeNode>& vChildren);
	void expandCNNNode(TreeNode* pNode, vector<pair<Move, float>>& vCandidate, vector<TreeNode>& vChildren, float& proofValue, float& disproofValue);
	TreeNode* selectBestChild(TreeNode* pNode, double& dPnOfMinDNChild, double& dMinDN, double& d2ndMinDN);
	int getNumLimitSize(TreeNode* pNode);
	float getAdjustedValue(TreeNode* pNode);

	void storeTTEntry(HashKey hashkey, DFPNTTEntry& entry);
	DFPNTTEntry& getTTEntry(unsigned int index);
	unsigned int getTTEntryIndex(HashKey hashkey);
	void getPNDNfromTT(TreeNode* pNode);
	inline bool findEntryInTT(HashKey hashkey) { return getTTEntryIndex(hashkey) != -1; }

	inline TreeNode* allocateNewNodes(int size);
	inline TreeNode* getRootNode() { return &m_nodes[0]; }
	inline bool isExpansionEnd() {
		if (Configure::SIM_CONTROL == SIM_CONTROL_TT_NODE) {
			return m_transpositionTable.getCount() >= Configure::PNS_NUM_EXPANSION;
		} else if (Configure::SIM_CONTROL == SIM_CONTROL_TIME) { 
			m_timer.stop();
			return m_timer.getElapsedTime().count() >= Configure::TIME_LIMIT;
		}

		return false;
	}

	inline int getSolvedSimulation() { return m_transpositionTable.getCount(); }
	inline ull getReExpansion() { return m_nMID; }
	inline double getSolvedTime() {
		m_timer.stop();  
		return m_timer.getElapsedTime().count();
	}
	inline TreeNode* getSolvedRootNode() { return &m_nodes[0]; }
	bool playSgfGame(SgfLoader& sgfLoader);
	string getNodeInfo(TreeNode* pNode);
	string getTTEntryInfo(DFPNTTEntry& entry);
	string getUndoSgf(SgfLoader& sgfLoader);

private:
	int m_nExpansion;
	ull m_nMID;
	int m_nodeUsedIndex;
	vector<TreeNode*> m_vSelectNodePath;
	set<HashKey> m_set;
	Game m_game;
	TreeNode* m_nodes;
	StopTimer m_timer;
	OpenAddressHashTable<DFPNTTEntry> m_transpositionTable;
};

