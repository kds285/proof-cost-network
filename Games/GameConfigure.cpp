#include "GameConfigure.h"

// variable to string
#define GET_VAR_NAME(Variable) (#Variable)

namespace GameConfigure {
	int BOARD_SIZE = 15;

#if GOMOKU
	// Gomoku parameters
	int GOMOKU_KNOWLEDGE_LEVEL = 0;

#elif GO
	// Go parameters
	string GO_EXTRA_FORCE_MOVE = "1:C7,2:PASS,3:G7,4:PASS,5:C3,6:PASS,7:G3";
	bool GO_FORBIDDEN_OWN_TRUE_EYE = true;
	bool GO_FORBIDDEN_BENSON_REGION = false;
	bool GO_BLACK_FORBIDDEN_EAT_KO = false;
	bool GO_WHITE_FORBIDDEN_EAT_KO = false;
	int GO_SUPER_KO_RULE = 0;
#endif

	void setConfiguration(ConfigureLoader& cl)
	{
		cl.addParameter(GET_VAR_NAME(BOARD_SIZE), BOARD_SIZE, "This is an example game parameters. It will not be used in the program currently.", "Game");
#if GOMOKU
		cl.addParameter(GET_VAR_NAME(GOMOKU_KNOWLEDGE_LEVEL), GOMOKU_KNOWLEDGE_LEVEL, "0: no knowledge, 1: pruning.", "Game");
#endif
#if GO
		cl.addParameter(GET_VAR_NAME(GO_EXTRA_FORCE_MOVE), GO_EXTRA_FORCE_MOVE, "E.g. 9x9 killall: 1:C7,2:PASS,3:G7,4:PASS,5:C3,6:PASS,7:G3", "Game");
		cl.addParameter(GET_VAR_NAME(GO_FORBIDDEN_OWN_TRUE_EYE), GO_FORBIDDEN_OWN_TRUE_EYE, "Forbid to play inside his own eye", "Game");
		cl.addParameter(GET_VAR_NAME(GO_BLACK_FORBIDDEN_EAT_KO), GO_BLACK_FORBIDDEN_EAT_KO, "True: forbid the black to eat ko", "Game");
		cl.addParameter(GET_VAR_NAME(GO_WHITE_FORBIDDEN_EAT_KO), GO_WHITE_FORBIDDEN_EAT_KO, "True: forbid the white to eat ko", "Game");
		cl.addParameter(GET_VAR_NAME(GO_SUPER_KO_RULE), GO_SUPER_KO_RULE, "0: Positional Super Ko, 1: Situational Super Ko", "Game");
#endif
	}
}
