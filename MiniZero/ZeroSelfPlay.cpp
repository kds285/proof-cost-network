#include "ZeroSelfPlay.h"
#include <boost/algorithm/string.hpp>

void ZeroSelfPlaySlave::doSlaveJob()
{
	if (m_network == nullptr) {
		int gameIndex = -1;
		while ((gameIndex = m_sharedData.getNextMCTSIndex()) != -1) {
			ZeroMCTS* zeroMCTS = m_sharedData.m_vZeroMCTS[gameIndex];
			zeroMCTS->runMCTSSimulationAfterForward();
			zeroMCTS->runMCTSSimulationBeforeForward();
		}
	} else {
		m_network->forward();
	}
}

void ZeroSelfPlaySlave::initialize()
{
	BaseSlave::initialize();

	if (m_id >= m_sharedData.m_vNetwork.size()) {
		m_bIsInitialize = true;
		return;
	}

	// initialize GPU
	m_sharedData.m_vNetwork[m_id].initialize();
	m_network = &m_sharedData.m_vNetwork[m_id];
	m_bIsInitialize = true;
}

ZeroSelfPlayMaster::~ZeroSelfPlayMaster()
{
	for (int i = 0; i < m_sharedData.m_vZeroMCTS.size(); ++i) {
		delete m_sharedData.m_vZeroMCTS[i];
	}
}

void ZeroSelfPlayMaster::run()
{
	if (!initialize()) { return; }

	const int NUM_GPU = static_cast<int>(Configure::GPU_LIST.length());

	m_sharedData.m_bForwardGPU = false;
	while (true) {
		m_sharedData.m_mctsIndex = 0;

		if (m_sharedData.m_bForwardGPU) {
			for (int i = 0; i < NUM_GPU; i++) { m_vSlaves[i]->startRun(); }
			for (int i = 0; i < NUM_GPU; i++) { m_vSlaves[i]->finishRun(); }
		} else {
			for (int i = NUM_GPU; i < m_nThread; i++) { m_vSlaves[i]->startRun(); }
			for (int i = NUM_GPU; i < m_nThread; i++) { m_vSlaves[i]->finishRun(); }
		}

		m_sharedData.m_bForwardGPU = !m_sharedData.m_bForwardGPU;
	}
}

bool ZeroSelfPlayMaster::initialize()
{
	for (int i = 0; i < static_cast<int>(Configure::GPU_LIST.length()); ++i) {
		m_sharedData.m_vNetwork.push_back(Network(Configure::GPU_LIST[i] - '0', Configure::MODEL_FILE));
	}
	if (!BaseMaster::initialize()) { return false; }
	
	// waiting for all slave thread initialization
	for (int i = 0; i < m_nThread; i++) {
		while (!m_vSlaves[i]->isInitialize()) { boost::this_thread::sleep(boost::posix_time::milliseconds(100)); }
	}

	// allocate number of batch size games
	for (int i = 0; i < static_cast<int>(Configure::GPU_LIST.length()); ++i) {
		for (int batchID = 0; batchID < Configure::NET_BATCH_SIZE; ++batchID) {
			ZeroMCTS* zeroMCTS = new ZeroMCTS(batchID, m_sharedData.m_mutex);
			zeroMCTS->setDisplay(i == 0 && batchID == 0);
			zeroMCTS->setNetwork(&m_sharedData.m_vNetwork[i]);
			zeroMCTS->newGame();
			m_sharedData.m_vZeroMCTS.push_back(zeroMCTS);
		}
	}

	return true;
}
