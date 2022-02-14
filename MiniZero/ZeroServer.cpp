#include "ZeroServer.h"
#include "SgfLoader.h"
#include "TimeSystem.h"
#include <boost/algorithm/string.hpp>

string ZeroWorkerSharedData::getOneSelfPlay()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	if (m_selfPlayQueue.empty()) { return ""; }
	else {
		string sSelfPlay = m_selfPlayQueue.front();
		m_selfPlayQueue.pop_front();
		return sSelfPlay;
	}
}

bool ZeroWorkerSharedData::isOptimizationDone()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	return m_bOptimization;
}

int ZeroWorkerSharedData::getModelIteration()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	return m_modelIteration;
}

// ZeroWorkerStatus
void ZeroWorkerStatus::handle_msg(const std::string msg)
{
	std::vector<std::string> vArgs;
	boost::split(vArgs, msg, boost::is_any_of(" "), boost::token_compress_on);

	if (vArgs[0] == "Info") {
		// Info name num_gpu type
		m_sName = vArgs[1];
		boost::lock_guard<boost::mutex> lock(m_sharedData.m_workerMutex);
		m_sharedData.m_fWorkerLog << "[Worker-Connection] "
			<< TimeSystem::getTimeString("Y/m/d_H:i:s.f ")
			<< m_sName << endl;
		m_bIdle = true;
	} else if (vArgs[0] == "Self-play") {
		if (msg.find("Self-play", msg.find("Self-play", 0) + 1) != string::npos) { return; }
		string sSelfPlay = msg.substr(msg.find(vArgs[0]) + vArgs[0].length() + 1);
		boost::lock_guard<boost::mutex> lock(m_sharedData.m_mutex);
		m_sharedData.m_selfPlayQueue.push_back(sSelfPlay);
	} else if (vArgs[0] == "Optimization_Done") {
		boost::lock_guard<boost::mutex> lock(m_sharedData.m_mutex);
		m_sharedData.m_bOptimization = true;
		m_sharedData.m_modelIteration = stoi(vArgs[1]);
	} else {
		// unknown client, reject it
		string sMessage = msg;
		std::replace(sMessage.begin(), sMessage.end(), '\r', ' ');
		std::replace(sMessage.begin(), sMessage.end(), '\n', ' ');
		std::cerr << "[error] receive \"" << sMessage << "\" from worker, disconnect." << std::endl;
		do_close();
	}
}

void ZeroWorkerStatus::do_close()
{
	boost::lock_guard<boost::mutex> lock(m_sharedData.m_workerMutex);
	m_sharedData.m_fWorkerLog << "[Worker-Disconnection] " << TimeSystem::getTimeString("Y/m/d_H:i:s.f ") << m_sName << " " << m_sGPUList << endl;
	BaseWorkerStatus::do_close();
}

// ZeroServer (public function)
void ZeroServer::run()
{
	initialize();
	start_accept();
	cout << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Server initialize over." << endl;

	for (m_iteration = Configure::ZERO_START_ITERATION; m_iteration <= Configure::ZERO_END_ITERATION; ++m_iteration) {
		SelfPlay();
		Optimization();
	}
}

boost::shared_ptr<ZeroWorkerStatus> ZeroServer::handle_accept_new_worker(boost::shared_ptr<tcp::socket> socket) {
	boost::shared_ptr<ZeroWorkerStatus> pWorkerStatus = boost::make_shared<ZeroWorkerStatus>(socket, m_sharedData);
	pWorkerStatus->write("Info");
	return pWorkerStatus;
}

// ZeroServer (private function)
void ZeroServer::initialize()
{
	m_seed = Configure::USE_TIME_SEED ? static_cast<int>(time(NULL)) : Configure::SEED;

	// create log files
	m_logger.createLogDirectoryAndFiles();
	
	// calculate start model iteration
	string sModel = Configure::MODEL_FILE;
	sModel = sModel.substr(sModel.find("weight_iter_") + string("weight_iter_").length());
	sModel = sModel.substr(0, sModel.find("."));
	m_sharedData.m_modelIteration = stoi(sModel);
}

void ZeroServer::SelfPlay()
{
	// setup
	string sSelfPlayGameFileName = Configure::ZERO_TRAIN_DIR + "/sgf/" + to_string(m_iteration) + ".sgf";
	m_logger.m_fSelfPlayGame.open(sSelfPlayGameFileName.c_str(), ios::out);
	string sSelfPlayDebugGameFileName = Configure::ZERO_TRAIN_DIR + "/sgf/debug/" + to_string(m_iteration) + ".sgf";
	m_logger.m_fSelfPlayDebugGame.open(sSelfPlayDebugGameFileName.c_str(), ios::out);
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[Iteration] =====" << m_iteration << "=====" << endl;
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[SelfPlay] Start " << m_sharedData.getModelIteration() << endl;

	// collect selfplay games
	int blackWins, whiteWins, draws, moveLens;
	m_total_games = blackWins = whiteWins = draws = moveLens = 0;
	while (m_total_games < Configure::ZERO_NUM_GAME) {
		// send command
		{
			boost::lock_guard<boost::mutex> lock(m_workerMutex);
			for (auto worker : m_vWorkers) {
				if (!worker->isIdle()) { continue; }
				worker->setIdle(false);
				worker->write(getSelfPlayConfigure());
			}
		}

		string sSelfPlay = m_sharedData.getOneSelfPlay();
		if (sSelfPlay == "") {
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
			continue;
		} else if (sSelfPlay.find("weight_iter_" + to_string(m_sharedData.getModelIteration()) + ".pt") == string::npos) {
			// discard previous self-play games
			continue;
		}

		// record sgf
		string sMoveNumber = sSelfPlay.substr(0, sSelfPlay.find("(") - 1);
		string sSgfString = sSelfPlay.substr(sSelfPlay.find("("));
		string sSimpleSgf = deleteUnusedSgfTag(sSgfString);

		m_logger.m_fSelfPlayGame << m_total_games << " " << sMoveNumber << " " << sSimpleSgf << endl;
		m_logger.m_fSelfPlayDebugGame << m_total_games << " " << sMoveNumber << " " << sSgfString << endl;
		++m_total_games;

		// count win/loss/draw & move lengths
		if (sSgfString.find("RE[B") != string::npos) { ++blackWins; }
		else if (sSgfString.find("RE[W") != string::npos) { ++whiteWins; }
		else { ++draws; }
		moveLens += stoi(sMoveNumber);

		// display progress
		if (m_total_games % static_cast<int>(Configure::ZERO_NUM_GAME * 0.25) == 0) {
			m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[SelfPlay Progress] "
				<< m_total_games << " / " << Configure::ZERO_NUM_GAME << endl;
		}
	}

	m_logger.m_fSelfPlayGame.close();
	m_logger.m_fSelfPlayDebugGame.close();

	// notify selfplay worker
	{
		boost::lock_guard<boost::mutex> lock(m_workerMutex);
		for (auto worker : m_vWorkers) {
			worker->setIdle(true);
			worker->write("Job_Done");
		}
	}
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[SelfPlay] Finished." << endl;
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[SelfPlay Win Rate] "
		<< "Black: " << blackWins * 100.0f / m_total_games << "%"
		<< ", White: " << whiteWins * 100.0f / m_total_games << "%"
		<< ", Draw: " << draws * 100.0f / m_total_games << "%" << endl;
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[SelfPlay Game Lengths] " << moveLens * 1.0f / m_total_games << endl;
}

void ZeroServer::Optimization()
{
	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[Optimization] Start." << endl;

	m_sharedData.m_bOptimization = false;
	string sOptimizationConfigure = getOptimizationConfigure();
	while (!m_sharedData.isOptimizationDone()) {
		// send command
		{
			boost::lock_guard<boost::mutex> lock(m_workerMutex);
			for (auto worker : m_vWorkers) {
				if (!worker->isIdle()) { continue; }
				worker->setIdle(false);
				worker->write(sOptimizationConfigure);
			}
		}
	}

	{
		boost::lock_guard<boost::mutex> lock(m_workerMutex);
		for (auto worker : m_vWorkers) {
			worker->setIdle(true);
			worker->write("Job_Done");
		}
	}

	m_logger.m_fTrainingLog << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "[Optimization] Finished." << endl;
}

string ZeroServer::getSelfPlayConfigure()
{
	string sConfigure = "Job_Selfplay ";
	sConfigure += "USE_TIME_SEED=" + to_string(Configure::USE_TIME_SEED);
	sConfigure += ":SEED=" + to_string(m_seed);
	sConfigure += ":MODEL_FILE=" + Configure::ZERO_TRAIN_DIR + "/model/weight_iter_" + to_string(m_sharedData.m_modelIteration) + ".pt";
	sConfigure += ":NET_NUM_OUTPUT_V=" + to_string(Configure::NET_NUM_OUTPUT_V);
	sConfigure += ":NET_ROTATION=" + to_string(Configure::NET_ROTATION);
	sConfigure += ":NET_VALUE_WINLOSS=" + to_string(Configure::NET_VALUE_WINLOSS);
	sConfigure += ":NET_VALUE_SPACE_COMPLEXITY=" + to_string(Configure::NET_VALUE_SPACE_COMPLEXITY);
	sConfigure += ":MCTS_PUCT_BIAS=" + to_string(Configure::MCTS_PUCT_BIAS);
	sConfigure += ":MCTS_Q_WEIGHT=" + to_string(Configure::MCTS_Q_WEIGHT);
	sConfigure += ":MCTS_USE_NOISE_AT_ROOT=" + to_string(Configure::MCTS_USE_NOISE_AT_ROOT);
	sConfigure += ":MCTS_SELECT_BY_COUNT_PROPORTION=" + to_string(Configure::MCTS_SELECT_BY_COUNT_PROPORTION);
	sConfigure += ":MCTS_SIMULATION_COUNT=" + to_string(Configure::MCTS_SIMULATION_COUNT);
	sConfigure += ":ZERO_NOISE_EPSILON=" + to_string(Configure::ZERO_NOISE_EPSILON);
	sConfigure += ":ZERO_NOISE_ALPHA=" + to_string(Configure::ZERO_NOISE_ALPHA);
	sConfigure += ":AOT_BRANCHING_FACTOR=" + to_string(Configure::AOT_BRANCHING_FACTOR);
	sConfigure += ":AOT_PROOF_COLOR=" + string{colorToChar(Configure::AOT_PROOF_COLOR)};
	
	m_seed += MAX_NUM_CPU;

	return sConfigure;
}

string ZeroServer::getOptimizationConfigure()
{
	string sConfigure = "Job_Optimization ";
	sConfigure += Configure::ZERO_TRAIN_DIR;
	sConfigure += " weight_iter_" + to_string(m_sharedData.m_modelIteration) + ".pkl";
	sConfigure += " " + to_string(max(1, m_iteration - Configure::ZERO_REPLAY_BUFFER));
	sConfigure += " " + to_string(m_iteration);

	string sConfFile = Configure::ZERO_TRAIN_DIR;
	sConfFile = sConfFile + "/" + sConfFile.substr(sConfFile.find_last_of("/") + 1) + ".cfg";
	sConfigure += " " + sConfFile;

	return sConfigure;
}

string ZeroServer::deleteUnusedSgfTag(string sSgfString)
{
	int end = 0;
	string sNewSgfString = "";

	while ((end = sSgfString.find("*")) != string::npos) {
		sNewSgfString += sSgfString.substr(0, end);
		sSgfString = sSgfString.substr(end);
		sSgfString = sSgfString.substr(sSgfString.find("]"));
	}
	sNewSgfString += sSgfString;

	return sNewSgfString;
}

void ZeroServer::keepAlive()
{
	broadcast("keep_alive");
	keepAliveTimer();
}

void ZeroServer::keepAliveTimer()
{
	m_keepAliveTimer.expires_from_now(boost::posix_time::minutes(1));
	m_keepAliveTimer.async_wait(boost::bind(&ZeroServer::keepAlive, this));
}
