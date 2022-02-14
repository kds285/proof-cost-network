#pragma once

#include "Definition.h"
#include "ConfigureLoader.h"

namespace Configure {
	// environment parameters
	extern bool USE_TIME_SEED;
	extern int SEED;
	extern int NUM_THREAD;
	extern int SIM_CONTROL;
	extern float TIME_LIMIT;
	extern string GPU_LIST;
	extern string MACHINE_NAME;

	// neural network parameters
	extern bool USE_NET;
	extern int NET_BATCH_SIZE;
	extern int NET_FILTER_SIZE;
	extern int NET_NUM_OUTPUT_V;
	extern string MODEL_FILE;
	extern int NET_ROTATION;
	extern bool NET_VALUE_WINLOSS;
	extern bool NET_VALUE_SPACE_COMPLEXITY;
	extern bool USE_TRANSPOSITION_TABLE;

	// MCTS parameters
	extern float MCTS_PUCT_BIAS;
	extern float MCTS_Q_WEIGHT;
	extern bool MCTS_USE_NOISE_AT_ROOT;
	extern bool MCTS_SELECT_BY_COUNT_PROPORTION;
	extern int MCTS_SIMULATION_COUNT;

	// PNS parameters
	extern int PNS_NUM_EXPANSION;
	extern int PNS_MODE;
	extern int PNS_FOCUSED_CHILDREN_BASE;
	extern float PNS_FOCUSED_WIDENING_FACTOR;
	extern bool PNS_ENABLE_DFPN;
	extern bool PNS_ENABLE_1_PLUS_EPSILON;
	extern float PNS_EPSILON_VALUE;
	extern bool PNS_ENABLE_WEAK_PNS;

	// zero training parameters
	extern int ZERO_SERVER_PORT;
	extern string ZERO_TRAIN_DIR;
	extern int ZERO_START_ITERATION;
	extern int ZERO_END_ITERATION;
	extern int ZERO_REPLAY_BUFFER;
	extern int ZERO_NUM_GAME;
	extern float ZERO_NOISE_EPSILON;
	extern float ZERO_NOISE_ALPHA;

	// AOT training parameters
	extern int AOT_BRANCHING_FACTOR;
	extern Color AOT_PROOF_COLOR;

	void setConfiguration(ConfigureLoader& cl);
}