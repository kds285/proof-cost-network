#pragma once

#include "Configure.h"
#include "StatisticData.h"

enum SOLUTION_STATUS {
	SOLUTION_UNKNOWN,
	SOLUTION_WIN,
	SOLUTION_LOSS
};

inline SOLUTION_STATUS getReverseSolutionStatus(SOLUTION_STATUS status) {
	switch (status) {
	case SOLUTION_UNKNOWN: return SOLUTION_UNKNOWN;
	case SOLUTION_WIN: return SOLUTION_LOSS;
	case SOLUTION_LOSS: return SOLUTION_WIN;
	default: break;
	}

	return SOLUTION_UNKNOWN;
}

inline string getSolutionStatusString(SOLUTION_STATUS status) {
	switch (status) {
	case SOLUTION_UNKNOWN: return "solution_unknown";
	case SOLUTION_WIN: return "solution_win";
	case SOLUTION_LOSS: return "solution_loss";
	default: break;
	}

	return "";
}

class TreeNode {
private:
	Move m_move;
	int m_nChildren;
	int m_nBranchingFactor;
	float m_fValue;
	float m_fProbabilty;
	float m_fProbabiltyWithNoise;
	HashKey m_hashkey;
	double m_dProofNumber;
	double m_dDisproofNumber;
	StatisticData m_uctData;
	TreeNode* m_pFirstChild;
	SOLUTION_STATUS m_solutionStatus;

public:
	TreeNode() {}
	~TreeNode() {}

	void reset(const Move& move)
	{
		m_move = move;
		m_nChildren = 0;
		m_nBranchingFactor = 0;
		m_fValue = 0.0f;
		m_fProbabilty = 0.0f;
		m_fProbabiltyWithNoise = 0.0f;
		m_hashkey = 0;
		m_dProofNumber = 0.0f;
		m_dDisproofNumber = 0.0f;
		m_uctData.reset();
		m_pFirstChild = nullptr;
		m_solutionStatus = SOLUTION_UNKNOWN;
	}

	inline void setNumChild(int nChild) { m_nChildren = nChild; }
	inline void setBranchingFactor(int b) { m_nBranchingFactor = b; }
	inline void setValue(float fValue) { m_fValue = fValue; }
	inline void setProbability(float fProbability) { m_fProbabilty = fProbability; }
	inline void setProbabilityWithNoise(float fProbabilityWithNoise) { m_fProbabiltyWithNoise = fProbabilityWithNoise; }
	inline void setHashkey(HashKey hashkey) { m_hashkey = hashkey; }
	inline void setProofNumber(double pn) { m_dProofNumber = pn; }
	inline void setDisproofNumber(double dn) { m_dDisproofNumber = dn; }
	inline void setFirstChild(TreeNode* pFirstChild) { m_pFirstChild = pFirstChild; }
	inline void setSolutionStatus(SOLUTION_STATUS status) { m_solutionStatus = status; }

	inline Move getMove() const { return m_move; }
	inline bool hasChildren() const { return (m_nChildren != 0); }
	inline int getNumChild() const { return m_nChildren; }
	inline int getBranchingFactor() const { return m_nBranchingFactor; }
	inline float getValue() const { return m_fValue; }
	inline float getProbability() const { return m_fProbabilty; }
	inline float getProbabilityWithNoise() const { return m_fProbabiltyWithNoise; }
	inline HashKey getHashkey() const { return m_hashkey; }
	inline double getProofNumber() const { return m_dProofNumber; }
	inline double getDisproofNumber() const { return m_dDisproofNumber; }
	inline StatisticData& getUctData() { return m_uctData; }
	inline const StatisticData& getUctData() const { return m_uctData; }
	inline TreeNode* getFirstChild() { return m_pFirstChild; }
	inline SOLUTION_STATUS getSolutionStatus() const { return m_solutionStatus; }

	inline int getSimCount() const { return m_uctData.getCount(); }
	
	inline float getPUCTValueQ(const map<double, int>& valueMap, float fInitValueQ = -1.0f, Color rootTurn = COLOR_NONE) const {
		if (Configure::NET_VALUE_WINLOSS) {
			return (getSimCount() == 0 ? fInitValueQ : getUctData().getMean());
		} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
			if (getSimCount() == 0 || valueMap.empty()) { return fInitValueQ; }

			assert(("Root color should be black or white", (rootTurn == COLOR_BLACK || rootTurn == COLOR_WHITE)));
			float fValue = getNormalizedValueQ(valueMap, fInitValueQ);
			if (Configure::AOT_PROOF_COLOR == COLOR_NONE) {
				return (getMove().getColor() == rootTurn) ? -fValue : fValue;
			} else {
				// since we want to minimize the space complexity, we should flip the win rate if the color is the same as proof color
				return (getMove().getColor() == Configure::AOT_PROOF_COLOR) ? -fValue : fValue;
			}
		}

		assert(("Error configuration for training value target!", false));
		return fInitValueQ;
	}

	inline float getNormalizedValueQ(const map<double, int>& valueMap, float fInitValueQ = -1.0f) const {
		assert(("Value map should not be empty", !valueMap.empty()));

		const float fLowerBound = valueMap.begin()->first;
		const float fUpperBound = valueMap.rbegin()->first;

		float fValue = m_uctData.getMean();
		float fDistance = fUpperBound - fLowerBound;
		if (fDistance == 0.0f || m_uctData.getCount() == 0) { fValue = 0.5f; } else {
			/*fValue = pow(10, (m_uctData.getMean() - fUpperBound) / fDistance);
			float fMin = pow(10, (fLowerBound - fUpperBound) / fDistance);
			float fMax = pow(10, (fUpperBound - fUpperBound) / fDistance);
			fValue = (fValue - fMin) / (fMax - fMin);*/
			fValue = (fValue - fLowerBound) / (fUpperBound - fLowerBound);
		}

		// Normalize to [-1, 1]
		fValue = 2 * fValue - 1;
		return fmin(fmax(fValue, -1), 1);
	}
};