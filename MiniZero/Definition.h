#pragma once

// game type
#if GOMOKU
#include "Gomoku.h"
typedef GomokuMove Move;
typedef GomokuGame Game;
#elif GO
#include "Go.h"
typedef GoMove Move;
typedef GoGame Game;
#elif HEX
#include "Hex.h"
typedef HexMove Move;
typedef HexGame Game;
#else
#include "TieTacToe.h"
typedef TieTacToeMove Move;
typedef TieTacToeGame Game;
#endif
