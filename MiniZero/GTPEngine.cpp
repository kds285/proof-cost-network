#include "GTPEngine.h"
#include "SgfLoader.h"
#include "SizeSolver.h"
#include "DepthSolver.h"
typedef DepthSolver Solver;

GTPEngine::GTPEngine()
{
	// initialize MCTS
	m_mcts = new ZeroMCTS(0, m_mutex);
	m_net = new Network(Configure::GPU_LIST[0] - '0', Configure::MODEL_FILE);
	m_net->initialize();
	m_mcts->setDisplay(true);
	m_mcts->setNetwork(m_net);
	m_mcts->newGame();

	m_bQuit = false;
	registration();
	cerr << "Program start successfully." << endl;
}

GTPEngine::~GTPEngine()
{
	delete m_mcts;
	delete m_net;
}

void GTPEngine::run()
{
	string sCommand = "";
	while (!m_bQuit) {
		getline(cin, sCommand);

		// empty command
		if (sCommand.empty()) { continue; }
		
		m_vArgs.clear();
		string s;
		istringstream iss(sCommand);
		while (getline(iss, s, ' ')) { m_vArgs.push_back(s); }
		executeCmd();
	}
}

void GTPEngine::registration()
{
	registerFunction("clear_board", &GTPEngine::cmdClearBoard);
	registerFunction("showboard", &GTPEngine::cmdShowBoard);
	registerFunction("play", &GTPEngine::cmdPlay);
	registerFunction("p", &GTPEngine::cmdPlay);
	registerFunction("undo", &GTPEngine::cmdUndo);
	registerFunction("u", &GTPEngine::cmdUndo);
	registerFunction("genmove", &GTPEngine::cmdGenMove);
	registerFunction("g", &GTPEngine::cmdGenMove);
	registerFunction("reg_genmove", &GTPEngine::cmdRegGenMove);
	registerFunction("loadsgf", &GTPEngine::cmdLoadSgf);
	registerFunction("load_nn_model", &GTPEngine::cmdLoadNNModel);
	registerFunction("solve", &GTPEngine::cmdSolve);
	registerFunction("quit", &GTPEngine::cmdQuit);
	registerFunction("pv", &GTPEngine::cmdPV);
	registerFunction("name", &GTPEngine::cmdName);
	registerFunction("version", &GTPEngine::cmdVersion);
	registerFunction("protocol_version", &GTPEngine::cmdProtocolVersion);
	registerFunction("boardsize", &GTPEngine::cmdBoardSize);
	registerFunction("list_commands", &GTPEngine::cmdListCommands);
	registerFunction("final_score", &GTPEngine::cmdFinalScore);
}

void GTPEngine::cmdClearBoard()
{
	if (!checkCmd(0, 0)) { return; }
	m_mcts->newGame();
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdShowBoard()
{
	if (!checkCmd(0, 0)) { return; }
	cerr << m_mcts->getGame().getBoardString(Game::getBoardSize());
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdPlay()
{
	if (!checkCmd(2, 2)) { return; }
	Move m(charToColor(m_vArgs[1][0]), m_vArgs[2], Game::getBoardSize());
	if (!m_mcts->getGame().isLegalMove(m)) { cerr << "Illegal move!" << endl; }
	else { m_mcts->getGame().play(m); }
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdUndo()
{
	if (!checkCmd(0, 0)) { return; }
	m_mcts->getGame().undo();
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdGenMove()
{
	if (!checkCmd(1, 1)) { return; }
	genMCTSMove(true);
}

void GTPEngine::cmdRegGenMove()
{
	if (!checkCmd(1, 1)) { return; }
	genMCTSMove(false);
}

void GTPEngine::cmdLoadSgf()
{
	if (!checkCmd(1, 1)) { return; }
	m_mcts->getGame().reset();
	SgfLoader sgfLoader;
	if (!sgfLoader.parseFromString(m_vArgs[1])) { cerr << "sgf format error!" << endl; }
	for (int i = 1; i < sgfLoader.getSgfNode().size(); ++i) {
		m_mcts->getGame().play(Move(sgfLoader.getSgfNode()[i].m_move, Game::getBoardSize()));
	}
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdLoadNNModel()
{
	if (!checkCmd(1, 1)) { return; }
	if (m_net->loadModel(m_vArgs[1])) { reply(GTP_SUCC, ""); }
	else { reply(GTP_FAIL, "Failed to load NN model"); }
}

void GTPEngine::cmdSolve()
{
/*
	if (!checkCmd(0, 0)) { return; }
	Solver solver(10000);
	solver.solveWinMoves(m_mcts->getGame());
	reply(GTP_SUCC, "");
*/
	if (!checkCmd(2, 2)) { return; }
	Game game_to_solve;
	game_to_solve.reset();
	SgfLoader sgfLoader;
	if (!sgfLoader.parseFromFile(m_vArgs[1])) { cerr << "sgf format error!" << endl; }
	for (int i = 1; i < sgfLoader.getSgfNode().size() - 5; ++i) {
		game_to_solve.play(Move(sgfLoader.getSgfNode()[i].m_move, Game::getBoardSize()));
	}
	Solver solver(6);
	solver.solveGame(game_to_solve, m_vArgs[2]);
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdQuit()
{
	if (!checkCmd(0, 0)) { return; }
	m_bQuit = true;
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdPV()
{
	if (!checkCmd(0, 0)) { return; }

	m_net->set_data(0, m_mcts->getGame());
	m_net->forward();
	
	float fValue = 0.0f;
	vector<pair<Move, float>> vProb = m_net->getProbability(0);
	cerr << m_net->getRotationString(0) << endl;
	
	// policy
	for (auto p : vProb) { cerr << p.first.toGtpString(Game::getBoardSize()) << " " << p.second << "  "; }
	cerr << endl;
	
	// value
	if (Configure::NET_VALUE_WINLOSS) {
		cerr << "win rate: " << m_net->getValue(0) << endl;
	} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		cerr << "Black value: " << m_net->getValue(0, COLOR_BLACK)
			<< ", White value: " << m_net->getValue(0, COLOR_WHITE) << endl;
	}

	reply(GTP_SUCC, "");
}

void GTPEngine::cmdName()
{
	if (!checkCmd(0, 0)) { return; }
	reply(GTP_SUCC, "MiniZero");
}

void GTPEngine::cmdVersion()
{
	if (!checkCmd(0, 0)) { return; }
	reply(GTP_SUCC, "0.0");
}

void GTPEngine::cmdProtocolVersion()
{
	if (!checkCmd(0, 0)) { return; }
	reply(GTP_SUCC, "2");
}

void GTPEngine::cmdBoardSize()
{
	if (!checkCmd(1, 1)) { return; }
	reply(GTP_SUCC, "");
}

void GTPEngine::cmdListCommands()
{
	if (!checkCmd(0, 0)) { return; }
	ostringstream oss;
	for (auto m : m_functionMap) { oss << m.first << endl; }
	reply(GTP_SUCC, oss.str());
}

void GTPEngine::cmdFinalScore()
{
	if (!checkCmd(0, 0)) { return; }
	reply(GTP_SUCC, m_mcts->getGame().getFinalScore());
}

void GTPEngine::cmdUnknown()
{
	reply(GTP_FAIL, "unknown command");
}

void GTPEngine::genMCTSMove(bool bWithPlay)
{
	Color c = charToColor(m_vArgs[1][0]);
	if (m_mcts->getGame().isTerminal()) {
		Color winner = m_mcts->getGame().eval();
		if (winner == AgainstColor(c)) { reply(GTP_SUCC, "RESIGN"); }
		else if (winner == COLOR_NONE) { reply(GTP_SUCC, "PASS"); }
		else { assert(("Should not be here", false)); }
	} else {
		Move m = m_mcts->run(c, bWithPlay);
		reply(GTP_SUCC, m.toGtpString(Game::getBoardSize()));
	}
}

void GTPEngine::executeCmd()
{
	if (m_functionMap.find(m_vArgs[0]) == m_functionMap.end()) {
		cmdUnknown();
	} else {
		Function f = m_functionMap[m_vArgs[0]];
		(this->*f)();
	}
}

bool GTPEngine::checkCmd(int minArgc, int maxArgc)
{
	int size = m_vArgs.size() - 1;
	if (size >= minArgc && size <= maxArgc) { return true; }

	ostringstream oss;
	oss << "command needs ";

	if (minArgc == maxArgc) { oss << "exactly " << minArgc << " argument" << (minArgc <= 1 ? "" : "s"); }
	else { oss << minArgc << " to " << maxArgc << " arguments"; }
	reply(GTP_FAIL, oss.str());
	return false;
}

void GTPEngine::reply(GTPEngine::GtpResponse type, const string& sReply)
{
	assert((type == GTPEngine::GTP_FAIL || type == GTPEngine::GTP_SUCC, "Error type for GTP response"));
	cout << char(type) << ' ' << sReply << "\n\n";
}
