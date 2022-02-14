#pragma once

#include "Network.h"
#include "ZeroMCTS.h"
#include "Configure.h"
#include "TimeSystem.h"
#include "BaseMasterSlave.h"

class ZeroSelfPlayMSSharedData {
public:
	int m_mctsIndex;
	bool m_bForwardGPU;
	boost::mutex m_mutex;
	vector<Network> m_vNetwork;
	vector<ZeroMCTS*> m_vZeroMCTS;

	inline int getNextMCTSIndex() {
		boost::lock_guard<boost::mutex> lock(m_mutex);

		int max_batch_size = m_vNetwork.size() * Configure::NET_BATCH_SIZE;
		if (m_mctsIndex >= max_batch_size) { return -1; }
		return m_mctsIndex++;
	}
};

class ZeroSelfPlaySlave : public BaseSlave<ZeroSelfPlayMSSharedData> {
private:
	bool m_bIsInitialize;
	Network* m_network;

public:
	ZeroSelfPlaySlave(int id, ZeroSelfPlayMSSharedData& sharedData)
		: BaseSlave(id, sharedData)
		, m_bIsInitialize(false)
		, m_network(nullptr)
	{
	}

	bool isOver() { return false; }
	void reset() {}
	void doSlaveJob();

	inline bool isInitialize() { return m_bIsInitialize; }

private:
	void initialize();
};

class ZeroSelfPlayMaster : public BaseMaster<ZeroSelfPlayMSSharedData, ZeroSelfPlaySlave> {
public:
	ZeroSelfPlayMaster()
		: BaseMaster(Configure::NUM_THREAD + static_cast<int>(Configure::GPU_LIST.length()))
	{
		assert(("Number of CPU should not be 0", Configure::NUM_THREAD > 0));
		assert(("Number of GPU should not be 0", Configure::GPU_LIST.length() > 0));
	}

	~ZeroSelfPlayMaster();

	void run();
	bool initialize();

	void summarizeSlavesData() {}

private:
	inline string getTimString() { return TimeSystem::getTimeString("[Y/m/d_H:i:s.f] "); }
};
