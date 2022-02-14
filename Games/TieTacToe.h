#pragma once
#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "GameBase.h"
#include "TimeSystem.h"

using namespace std;

class TieTacToeMove : public _Move {
public:
	TieTacToeMove() : _Move() {}
	TieTacToeMove(Color c, int position) : _Move(c, position) {}
	TieTacToeMove(Color c, string sPosition, int boardSize) :_Move(c, sPosition, boardSize) {}
	TieTacToeMove(pair<string, string> pSgfString, int boardSize) : _Move(pSgfString, boardSize) {}

	~TieTacToeMove() {}
};

class TieTacToeGame: public _Game<TieTacToeMove> {
private:
	vector<Color> m_vBoard;

public:
	TieTacToeGame()
		: _Game()
	{
		reset();
	}
	~TieTacToeGame() {}

	void reset() {
		_Game::reset();
		m_turnColor = COLOR_BLACK;
		m_vBoard.resize(getBoardSize() * getBoardSize());
		fill(m_vBoard.begin(), m_vBoard.end(), COLOR_NONE);
	}

	void play(const TieTacToeMove& move) {
		_Game::play(move);
		m_vBoard[move.getPosition()] = move.getColor();
		m_turnColor = AgainstColor(move.getColor());
	}

	inline bool isLegalMove(const TieTacToeMove& move) const {
		return (m_vBoard[move.getPosition()] == COLOR_NONE);
	}

	inline bool isBadMove(const TieTacToeMove& move) const {
		return false;
	}

	inline bool isTerminal() {
		// terminal: all points are played by "O" or "X" || any player wins
		return (eval() != COLOR_NONE || find(m_vBoard.begin(), m_vBoard.end(), COLOR_NONE) == m_vBoard.end());
	}
	
	inline Color eval() const {
		Color c;
		bool bEndGame;
		for (int i = 0; i < getBoardSize(); ++i) {
			// rows
			c = m_vBoard[i * getBoardSize()];
			if (c != COLOR_NONE) {
				bEndGame = true;
				for (int j = 0; j < getBoardSize(); ++j) {
					if (m_vBoard[i * getBoardSize() + j] == c) { continue; }
					bEndGame = false;
					break;
				}
				if (bEndGame) { return c; }
			}

			// columns
			c = m_vBoard[i];
			if (c != COLOR_NONE) {
				bEndGame = true;
				for (int j = 0; j < getBoardSize(); ++j) {
					if (m_vBoard[j * getBoardSize() + i] == c) { continue; }
					bEndGame = false;
					break;
				}
				if (bEndGame) { return c; }
			}
		}

		// diagonal (left-up to right-down)
		c = m_vBoard[0];
		bEndGame = true;
		for (int i = 0; i < getBoardSize(); ++i) {
			if (m_vBoard[i * getBoardSize() + i] == c) { continue; }
			bEndGame = false;
			break;
		}
		if (bEndGame) { return c; }

		// diagonal (left-down to right-up)
		c = m_vBoard[getBoardSize() - 1];
		bEndGame = true;
		for (int i = 0; i < getBoardSize(); ++i) {
			if (m_vBoard[i * getBoardSize() + (getBoardSize() - 1 - i)] == c) { continue; }
			bEndGame = false;
			break;
		}
		if (bEndGame) { return c; }

		return COLOR_NONE;
	}

	string getFinalScore() const {
		Color winner = eval();
		if (winner == COLOR_BLACK) { return "B+0"; }
		else if (winner == COLOR_WHITE) { return "W+0"; }
		else { return "0"; }
	}

	void undo() {
		assert(("Undo on the initial game", !m_vMoves.empty()));
		m_vBoard[m_vMoves.back().getPosition()] = COLOR_NONE;
		m_turnColor = m_vMoves.back().getColor();
		m_vMoves.pop_back();
		m_vComments.resize(m_vMoves.size());
	}

	Color getColor(int position) const {
		return m_vBoard[position];
	}

	vector<float> getFeatures(SymmetryType type = SYM_NORMAL) const {
		vector<float> vFeatures;
		// 4 feature planes (Black Board/White Board/Turn Color)
		for (int channel = 0; channel < getNumChannels(); ++channel) {
			for (int pos = 0; pos < getMaxNumLegalAction(); ++pos) {
				int rotatePos = getRotatePosition(pos, getBoardSize(), ReverseSymmetricType[type]);
				if (channel == 0) { vFeatures.push_back((m_vBoard[rotatePos] == m_turnColor ? 1.0f : 0.0f)); }
				else if (channel == 1) { vFeatures.push_back((m_vBoard[rotatePos] == AgainstColor(m_turnColor) ? 1.0f : 0.0f)); }
				else if (channel == 2) { vFeatures.push_back((m_turnColor == COLOR_BLACK ? 1.0f : 0.0f)); }
				else if (channel == 3) { vFeatures.push_back((m_turnColor == COLOR_WHITE ? 1.0f : 0.0f)); }
			}
		}
		return vFeatures;
	}

	// overwrite static function
	static int getBoardSize() { return 3; }
	static int getMaxNumLegalAction() { return 9; }
	static int getNumChannels() { return 4; }
};

