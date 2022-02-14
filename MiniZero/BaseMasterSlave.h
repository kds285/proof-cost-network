#pragma once

#include "Random.h"

template<class _SharedData> class BaseSlave {
protected:
	int m_id;
	int m_seed;
	_SharedData& m_sharedData;

	boost::barrier m_barStart;
	boost::barrier m_barFinish;

public:
	BaseSlave(int id, _SharedData& sharedData)
		: m_id(id), m_sharedData(sharedData), m_barStart(2), m_barFinish(2)
	{}

	void run()
	{
		initialize();

		while (!isOver()) {
			m_barStart.wait();
			reset();
			doSlaveJob();
			m_barFinish.wait();
		}
	}
	void startRun() { m_barStart.wait(); }
	void finishRun() { m_barFinish.wait(); }

	inline int getID() const { return m_id; }
	inline void setTimeSeed(int seed) { m_seed = seed; }

protected:
	virtual void initialize()
	{
		Random::reset(m_seed);
	}

	virtual bool isOver() = 0;
	virtual void reset() = 0;
	virtual void doSlaveJob() = 0;
};

template<class _SharedData, class _Slave> class BaseMaster {
protected:
	int m_seed;
	int m_nThread;
	_SharedData m_sharedData;
	std::vector<_Slave*> m_vSlaves;
	boost::thread_group m_threads;

public:
	BaseMaster(int nThread)
		: m_nThread(nThread)
	{}

	~BaseMaster()
	{
		for (int i = 0; i < m_vSlaves.size(); ++i) { delete m_vSlaves[i]; }
	}

	virtual void run()
	{
		if (!initialize()) { return; }
		for (int i = 0; i < m_nThread; i++) { m_vSlaves[i]->startRun(); }
		for (int i = 0; i < m_nThread; i++) { m_vSlaves[i]->finishRun(); }
		summarizeSlavesData();
	}

	virtual bool initialize()
	{
		m_seed = Configure::USE_TIME_SEED ? static_cast<int>(time(NULL)) : Configure::SEED;
		Random::reset(m_seed);

		for (int id = 0; id < m_nThread; id++) {
			_Slave* slave = newSlave(id);
			slave->setTimeSeed(m_seed + id + 1);
			m_threads.create_thread(boost::bind(&_Slave::run, slave));
			m_vSlaves.push_back(slave);
		}

		return true;
	}

	virtual _Slave* newSlave(int id)
	{
		return new _Slave(id, m_sharedData);
	}

	virtual void summarizeSlavesData() = 0;

	inline int getSeed() const { return m_seed; }
};
