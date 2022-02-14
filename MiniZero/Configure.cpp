#include "Configure.h"

// variable to string
#define GET_VAR_NAME(Variable) (#Variable)

namespace Configure {
	// environment parameters
	bool USE_TIME_SEED = false;
	int SEED = 0;
	int NUM_THREAD = 1;
	int SIM_CONTROL = 0;
	float TIME_LIMIT = 5;
	string GPU_LIST = "0";
	string MACHINE_NAME = "";

	// neural network parameters
	bool USE_NET = true;
	int NET_BATCH_SIZE = 1;
	int NET_FILTER_SIZE = 64;
	int NET_NUM_OUTPUT_V = 1;
	string MODEL_FILE = "";
	int NET_ROTATION = 1;
	bool NET_VALUE_WINLOSS = true;
	bool NET_VALUE_SPACE_COMPLEXITY = false;
	bool USE_TRANSPOSITION_TABLE = false;

	// MCTS parameters
	float MCTS_PUCT_BIAS = 1.5f;
	float MCTS_Q_WEIGHT = 1.0f;
	bool MCTS_USE_NOISE_AT_ROOT = false;
	bool MCTS_SELECT_BY_COUNT_PROPORTION = false;
	int MCTS_SIMULATION_COUNT = 400;

	// PNS parameters
	int PNS_NUM_EXPANSION = 400;
	int PNS_MODE = 0;
	int PNS_FOCUSED_CHILDREN_BASE = 1;
	float PNS_FOCUSED_WIDENING_FACTOR = 0.1f;
	bool PNS_ENABLE_DFPN = false;
	bool PNS_ENABLE_1_PLUS_EPSILON = false;
	float PNS_EPSILON_VALUE = 0.25f;
	bool PNS_ENABLE_WEAK_PNS = false;

	// zero training parameters
	int ZERO_SERVER_PORT = 9999;
	string ZERO_TRAIN_DIR = "";
	int ZERO_START_ITERATION = 1;
	int ZERO_END_ITERATION = 300;
	int ZERO_REPLAY_BUFFER = 20;
	int ZERO_NUM_GAME = 5000;
	float ZERO_NOISE_EPSILON = 0.25f;
	float ZERO_NOISE_ALPHA = 0.2f;

	// AOT training parameters
	int AOT_BRANCHING_FACTOR = 0;
	Color AOT_PROOF_COLOR = COLOR_BLACK;
	
	void setConfiguration(ConfigureLoader& cl)
	{
		// environment parameters
		cl.addParameter(GET_VAR_NAME(USE_TIME_SEED), USE_TIME_SEED, "", "Environment");
		cl.addParameter(GET_VAR_NAME(SEED), SEED, "", "Environment");
		cl.addParameter(GET_VAR_NAME(NUM_THREAD), NUM_THREAD, "", "Environment");
		cl.addParameter(GET_VAR_NAME(SIM_CONTROL), SIM_CONTROL, "0: nodes, 1: time", "Environment");
		cl.addParameter(GET_VAR_NAME(TIME_LIMIT), TIME_LIMIT, "Time limit for the solver", "Environment");
		cl.addParameter(GET_VAR_NAME(GPU_LIST), GPU_LIST, "", "Environment");
		cl.addParameter(GET_VAR_NAME(MACHINE_NAME), MACHINE_NAME, "", "Environment");

		// neural network parameters
		cl.addParameter(GET_VAR_NAME(USE_NET), USE_NET, "If false, run MCTS with playout", "Network");
		cl.addParameter(GET_VAR_NAME(NET_BATCH_SIZE), NET_BATCH_SIZE, "", "Network");
		cl.addParameter(GET_VAR_NAME(NET_FILTER_SIZE), NET_FILTER_SIZE, "", "Network");
		cl.addParameter(GET_VAR_NAME(NET_NUM_OUTPUT_V), NET_NUM_OUTPUT_V, "", "Network");
		cl.addParameter(GET_VAR_NAME(MODEL_FILE), MODEL_FILE, "", "Network");
		cl.addParameter(GET_VAR_NAME(NET_ROTATION), NET_ROTATION, "0: no rotation, 1-8: average of random # rotation", "Network");
		cl.addParameter(GET_VAR_NAME(NET_VALUE_WINLOSS), NET_VALUE_WINLOSS, "", "Network");
		cl.addParameter(GET_VAR_NAME(NET_VALUE_SPACE_COMPLEXITY), NET_VALUE_SPACE_COMPLEXITY, "", "Network");
		cl.addParameter(GET_VAR_NAME(USE_TRANSPOSITION_TABLE), USE_TRANSPOSITION_TABLE, "", "Network");

		// MCTS parameters
		cl.addParameter(GET_VAR_NAME(MCTS_PUCT_BIAS), MCTS_PUCT_BIAS, "", "MCTS");
		cl.addParameter(GET_VAR_NAME(MCTS_Q_WEIGHT), MCTS_Q_WEIGHT, "Weight for MCTS Q value (Action = U + Q * \"weight\")", "MCTS");
		cl.addParameter(GET_VAR_NAME(MCTS_USE_NOISE_AT_ROOT), MCTS_USE_NOISE_AT_ROOT, "", "MCTS");
		cl.addParameter(GET_VAR_NAME(MCTS_SELECT_BY_COUNT_PROPORTION), MCTS_SELECT_BY_COUNT_PROPORTION, "", "MCTS");
		cl.addParameter(GET_VAR_NAME(MCTS_SIMULATION_COUNT), MCTS_SIMULATION_COUNT, "", "MCTS");

		// PNS parameters
		cl.addParameter(GET_VAR_NAME(PNS_NUM_EXPANSION), PNS_NUM_EXPANSION, "The number of maximum expanded nodes", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_MODE), PNS_MODE, "0: vanilla, 1: with policy, 2: with policy and value children restriction", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_FOCUSED_CHILDREN_BASE), PNS_FOCUSED_CHILDREN_BASE, "The number of minimum focused children", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_FOCUSED_WIDENING_FACTOR), PNS_FOCUSED_WIDENING_FACTOR, "The ratio of focused live children", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_ENABLE_DFPN), PNS_ENABLE_DFPN, "enable DFPN to relieve the memory usage", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_ENABLE_1_PLUS_EPSILON), PNS_ENABLE_1_PLUS_EPSILON, "Enable 1+epsilon", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_EPSILON_VALUE), PNS_EPSILON_VALUE, "Set epsilon value in 1+epsilon method", "PNS");
		cl.addParameter(GET_VAR_NAME(PNS_ENABLE_WEAK_PNS), PNS_ENABLE_WEAK_PNS, "Enable weak PN/DN computation in PNS", "PNS");

		// zero training parameters
		cl.addParameter(GET_VAR_NAME(ZERO_SERVER_PORT), ZERO_SERVER_PORT, "", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_TRAIN_DIR), ZERO_TRAIN_DIR, "", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_START_ITERATION), ZERO_START_ITERATION, "", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_END_ITERATION), ZERO_END_ITERATION, "", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_REPLAY_BUFFER), ZERO_REPLAY_BUFFER, "Number of iteration in reply buffer", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_NUM_GAME), ZERO_NUM_GAME, "Number of games for each iteration", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_NOISE_EPSILON), ZERO_NOISE_EPSILON, "", "Zero Training");
		cl.addParameter(GET_VAR_NAME(ZERO_NOISE_ALPHA), ZERO_NOISE_ALPHA, "", "Zero Training");

		// AOT training parameters
		cl.addParameter(GET_VAR_NAME(AOT_BRANCHING_FACTOR), AOT_BRANCHING_FACTOR, "0: maximum actions, 1: legal actions, 2: actions with domain knowledge", "AOT Training");
		cl.addParameter(GET_VAR_NAME(AOT_PROOF_COLOR), AOT_PROOF_COLOR, "", "AOT Training");
	}
}