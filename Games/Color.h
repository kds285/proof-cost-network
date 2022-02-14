#pragma once

enum Color {
	COLOR_NONE,
	COLOR_BLACK,
	COLOR_WHITE,
	COLOR_SIZE
};

inline Color charToColor(char c)
{
	switch (c) {
	case 'N': return COLOR_NONE; break;
	case 'B': case 'b': return COLOR_BLACK; break;
	case 'W': case 'w': return COLOR_WHITE; break;
	default: return COLOR_SIZE; break;
	}
}

inline char colorToChar(Color c)
{
	switch (c) {
	case COLOR_NONE: return 'N'; break;
	case COLOR_BLACK: return 'B'; break;
	case COLOR_WHITE: return 'W'; break;
	default: return '?'; break;
	}
}

inline Color AgainstColor(Color c) {
	return (Color)(c^COLOR_SIZE);
}