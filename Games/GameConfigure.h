#pragma once

#include "ConfigureLoader.h"

namespace GameConfigure {
	// Game parameters
	extern int BOARD_SIZE;

#if GOMOKU
	// Gomoku parameters
	extern int GOMOKU_KNOWLEDGE_LEVEL;

#elif GO
	// Go parameters
	extern string GO_EXTRA_FORCE_MOVE;
	extern bool GO_FORBIDDEN_OWN_TRUE_EYE;
	extern bool GO_FORBIDDEN_BENSON_REGION;
	extern bool GO_BLACK_FORBIDDEN_EAT_KO;
	extern bool GO_WHITE_FORBIDDEN_EAT_KO;
	extern int GO_SUPER_KO_RULE;
#endif

	void setConfiguration(ConfigureLoader& cl);
}
