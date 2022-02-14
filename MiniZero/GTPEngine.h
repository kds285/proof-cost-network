#pragma once

#include <map>
#include <string>
#include <iostream>
#include "ZeroMCTS.h"

class GTPEngine {
private:
	typedef void (GTPEngine::*Function)();

	enum GtpResponse {
		GTP_FAIL = '?',
		GTP_SUCC = '='
	};

	bool m_bQuit;
	Network* m_net;
	ZeroMCTS* m_mcts;
	boost::mutex m_mutex;
	vector<string> m_vArgs;
	map<string, Function> m_functionMap;

public:
	GTPEngine();
	~GTPEngine();

	void run();

private:
	void registration();

	// gtp commands
	void cmdClearBoard();
	void cmdShowBoard();
	void cmdPlay();
	void cmdUndo();
	void cmdGenMove();
	void cmdRegGenMove();
	void cmdLoadSgf();
	void cmdLoadNNModel();
	void cmdSolve();
	void cmdQuit();
	void cmdPV();
	void cmdName();
	void cmdVersion();
	void cmdProtocolVersion();
	void cmdBoardSize();
	void cmdListCommands();
	void cmdFinalScore();
	void cmdUnknown();

	void genMCTSMove(bool bWithPlay);

	void executeCmd();
	bool checkCmd(int minArgc, int maxArgc);
	void reply(GTPEngine::GtpResponse type, const string& sReply);
	template<typename T> void registerFunction(const string& sKey, T f) { m_functionMap.insert(make_pair(sKey, f)); }
};