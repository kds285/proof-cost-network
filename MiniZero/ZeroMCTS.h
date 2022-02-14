#pragma once

#include "BaseMCTS.h"
#include "Network.h"
#include <boost/thread.hpp>
#include "Random.h"

class ZeroMCTS : public BaseMCTS {
private:
	bool m_bDisplay;
	Network* m_network;
	boost::mutex& m_mutex;
	
	const int BATCH_ID;

public:
	ZeroMCTS(int batchID, boost::mutex& threadMutex)
		: BATCH_ID(batchID)
		, m_mutex(threadMutex)
		, m_bDisplay(false)
	{
	}

	~ZeroMCTS() {}

	Move run(Color c, bool bWithPlay = true);
	void runMCTSSimulationBeforeForward();
	void runMCTSSimulationAfterForward();
	void evaluation();

	inline void setDisplay(bool bDisplay) { m_bDisplay = bDisplay; }
	inline void setNetwork(Network* network) { m_network = network; }

private:
	void calculateFeatureAndAddToNet();
	void handleSimulationEnd();
	void handleEndGame();
	void display(TreeNode* pSelected);

	string getMoveComment(TreeNode* pSelected);
	string getMoveCommentDetail(TreeNode* pSelected);
};
