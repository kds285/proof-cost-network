#pragma once

#include <string>

enum ANSITYPE {
	ANSITYPE_NORMAL,
	ANSITYPE_BOLD,
	ANSITYPE_UNDERLINE,

	ANSITYPE_SIZE
};

enum ANSICOLOR {
	ANSICOLOR_BLACK,
	ANSICOLOR_RED,
	ANSICOLOR_GREEN,
	ANSICOLOR_YELLOW,
	ANSICOLOR_BLUE,
	ANSICOLOR_PURPLE,
	ANSICOLOR_CYAN,
	ANSICOLOR_WHITE,

	ANSICOLOR_SIZE
};

inline std::string getColorMessage(std::string sMessage, ANSITYPE type, ANSICOLOR textColor, ANSICOLOR backgroundColor)
{
	const int ansitype[ANSITYPE_SIZE] = {0,1,4};
	return "\33[" + std::to_string(ansitype[type]) + ";3" + std::to_string(textColor) 
			+ ";4" + std::to_string(backgroundColor) + "m" + sMessage + "\33[0m";
}
