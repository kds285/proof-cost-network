#pragma once

#include <fstream>
#include "Configure.h"
#include "BaseWorkerHandler.h"

class ZeroWorkerSharedData {
public:
	int m_total_games;
	fstream& m_fWorkerLog;

	boost::mutex m_mutex;
	boost::mutex& m_workerMutex;

	bool m_bOptimization;
	int m_modelIteration;
	std::deque<string> m_selfPlayQueue;
	
	ZeroWorkerSharedData(boost::mutex& workerMutex, fstream& fWorkerLog)
		: m_workerMutex(workerMutex), m_fWorkerLog(fWorkerLog)
	{
	}

	string getOneSelfPlay();
	bool isOptimizationDone();
	int getModelIteration();
};

class ZeroWorkerStatus : public BaseWorkerStatus {
private:
	bool m_bIdle;
	string m_sName;
	string m_sGPUList;
	ZeroWorkerSharedData& m_sharedData;

public:
	ZeroWorkerStatus(boost::shared_ptr<tcp::socket> socket, ZeroWorkerSharedData& sharedData)
		: BaseWorkerStatus(socket)
		, m_sharedData(sharedData)
		, m_bIdle(false)
	{
	}

	~ZeroWorkerStatus()
	{
	}

	inline bool isIdle() const { return m_bIdle; }
	inline string getName() const { return m_sName; }
	inline string getGPUList() const { return m_sGPUList; }
	inline void setIdle(bool bIdle) { m_bIdle = bIdle; }

	boost::shared_ptr<ZeroWorkerStatus> shared_from_this()
	{
		return boost::static_pointer_cast<ZeroWorkerStatus>(BaseWorkerStatus::shared_from_this());
	}

	void handle_msg(const std::string msg);
	void do_close();
};

class ZeroLogger {
public:
	fstream m_fWorkerLog;
	fstream m_fSelfPlayGame;
	fstream m_fSelfPlayDebugGame;
	fstream m_fTrainingLog;

	ZeroLogger() {}
	void createLogDirectoryAndFiles() {
		// create log file
		string sWorkerLogFileName = Configure::ZERO_TRAIN_DIR + "/Worker.log";
		m_fWorkerLog.open(sWorkerLogFileName.c_str(), ios::out | ios::app);

		string sTrainingLogFileName = Configure::ZERO_TRAIN_DIR + "/Training.log";
		m_fTrainingLog.open(sTrainingLogFileName.c_str(), ios::out | ios::app);

		// Separation line
		for (int i = 0; i < 100; i++) {
			m_fWorkerLog << "=";
			m_fTrainingLog << "=";
		}
		m_fWorkerLog << endl;
		m_fTrainingLog << endl;
	}
};

class ZeroServer : public BaseWorkerHandler<ZeroWorkerStatus> {
private:
	int m_seed;
	int m_iteration;
	int m_total_games;
	ZeroLogger m_logger;
	ZeroWorkerSharedData m_sharedData;

	boost::asio::deadline_timer m_keepAliveTimer;

	static const int MAX_NUM_CPU = 48;

public:
	ZeroServer()
		: BaseWorkerHandler(Configure::ZERO_SERVER_PORT)
		, m_sharedData(m_workerMutex, m_logger.m_fWorkerLog)
		, m_keepAliveTimer(m_io_service)
	{
		keepAliveTimer();
	}

	void run();
	boost::shared_ptr<ZeroWorkerStatus> handle_accept_new_worker(boost::shared_ptr<tcp::socket> socket);

	void broadcast(const std::string& msg) {
		boost::lock_guard<boost::mutex> lock(m_workerMutex);
		for (auto worker : m_vWorkers) { worker->write(msg); }
	}

private:
	void initialize();

	void SelfPlay();
	void Optimization();

	string getSelfPlayConfigure();
	string getOptimizationConfigure();
	string deleteUnusedSgfTag(string sSgfString);

	void keepAlive();
	void keepAliveTimer();
};
