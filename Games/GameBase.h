#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "Color.h"
#include "Point.h"
#include "TimeSystem.h"
#include "ColorMessage.h"

using namespace std;
typedef unsigned long long HashKey;

class _Move {
public:
	_Move() {}
	_Move(Color c, int position) : m_color(c), m_position(position) {}
	_Move(Color c, string sGTPString, int boardSize) {
		m_color = c;
		transform(sGTPString.begin(), sGTPString.end(), sGTPString.begin(), ::toupper);
		if (sGTPString == "PASS") { setPass(boardSize); return; }

		assert(("Illegal GoGUI coordinate 'I'", toupper(sGTPString[0]) != 'I'));
		int y = atoi(sGTPString.substr(1).c_str()) - 1, x = toupper(sGTPString[0]) - 'A';
		if (toupper(sGTPString[0]) > 'I') { --x; }
		m_position = y * boardSize + x;
	}
	_Move(pair<string, string> pSgfString, int boardSize) {
		m_color = charToColor(pSgfString.first[0]);
		if (pSgfString.second.length() == 0) { setPass(boardSize); return; }

		int x = toupper(pSgfString.second[0]) - 'A';
		int y = (boardSize - 1) - (toupper(pSgfString.second[1]) - 'A');
		m_position = y * boardSize + x;
	}
	~_Move() {}

	inline void setPass(int boardSize) { m_position = boardSize * boardSize; }
	inline bool isPass(int boardSize) const { return (m_position == boardSize * boardSize); }
	inline void reset() { m_color = COLOR_NONE; m_position = -1; }
	inline Color getColor() const { return m_color; }
	inline int getPosition() const { return m_position; }
	inline string toString() const { return "(" + string(1, colorToChar(m_color)) + to_string(m_position) + ")"; }
	inline string toSgfString(int boardSize) const {
		ostringstream oss;
		oss << colorToChar(m_color) << "[";
		if (!isPass(boardSize)) {
			int x = m_position % boardSize;
			int y = m_position / boardSize;
			oss << (char)(x + 'a') << (char)(((boardSize - 1) - y) + 'a');
		}
		oss << "]";
		return oss.str();
	}
	inline string toGtpString(int boardSize) const {
		if (isPass(boardSize)) { return "PASS"; }
		int x = m_position % boardSize;
		int y = m_position / boardSize;
		ostringstream oss;
		oss << (char)(x + 'A' + (x >= 8)) << y + 1;
		return oss.str();
	}

protected:
	Color m_color;
	int m_position;
};

template<class _Move> class _Game {
public:
	_Game() {}
	~_Game() {}

	inline void reset() {
		m_turnColor = COLOR_NONE;
		m_hashKey = 0;
		m_vMoves.clear();
		m_vComments.clear();
	}

	inline void play(const _Move& move) {
		assert(("Play an illegal move", isLegalMove(move)));
		assert(("Play on an ended game", !isTerminal()));
		m_vMoves.push_back(move);
	}

	inline void addComment(int moveNumber, const string sComment) {
		if (moveNumber >= m_vComments.size()) { m_vComments.resize(moveNumber + 1); }
		m_vComments[moveNumber] = sComment;
	}

	inline string getBoardString(const int boardSize, bool bWithColor = true) const {
		ostringstream oss;
		oss << getBoardCoordinateString(boardSize, bWithColor) << endl;
		for (int r = boardSize - 1; r >= 0; --r) { oss << getOneRowBoardString(r, boardSize, bWithColor) << endl; }
		oss << getBoardCoordinateString(boardSize, bWithColor) << endl;
		return oss.str();
	}

	inline string getGameRecord(const map<string, string> mTag, const int boardSize) const {
		ostringstream oss;

		// header and game tags
		oss << "(;FF[4]CA[UTF-8]SZ[" << boardSize << "]";
		for (auto tag : mTag) { oss << tag.first << "[" << tag.second << "]"; }

		// move information
		for (int i = 0; i < m_vMoves.size(); ++i) {
			oss << ";" << m_vMoves[i].toSgfString(boardSize);
			if (i < m_vComments.size() && m_vComments[i] != "") { oss << "C[" << m_vComments[i] << "]"; }
		}

		// end mark
		oss << ")";
		return oss.str();
	}

	inline void setTurnColor(Color c) { m_turnColor = c; }
	inline Color getTurnColor() const { return m_turnColor; }
	inline vector<_Move>& getMoves() { return m_vMoves; }
	inline const vector<_Move>& getMoves() const { return m_vMoves; }
	inline bool hasCurrentThreat() const { return false; }
	inline HashKey getHashKey() const { return m_hashKey; }
	inline HashKey getTTHashKey() const { return m_hashKey; }

	// virtual function (need to be implemented for each game)
	virtual void undo() = 0;
	virtual bool isLegalMove(const _Move& move) const = 0;
	virtual bool isBadMove(const _Move& move) const = 0;
	virtual bool isTerminal() = 0;
	virtual Color eval() const = 0;
	virtual string getFinalScore() const = 0;
	virtual Color getColor(int position) const = 0;
	virtual vector<float> getFeatures(SymmetryType type = SYM_NORMAL) const = 0;

	// need to be overwritten static function
	static int getBoardSize() { return -1; }
	static int getMaxNumLegalAction() { return -1; }
	static int getNumChannels() { return -1; }

private:
	inline string getBoardCoordinateString(const int boardSize, bool bWithColor) const {
		ostringstream oss;
		if(bWithColor) { oss << getColorMessage("  ", ANSITYPE_BOLD, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
		else { oss << "  "; }
		int size = ('A' + boardSize >= 'I')? boardSize + 1: boardSize;
		for (int x = 0; x < size; ++x) {
			if ('A' + x == 'I') { x++; }
			if (bWithColor) { oss << getColorMessage(" " + string(1, char('A' + x)), ANSITYPE_BOLD, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
			else { oss << " " + string(1, char('A' + x)); }
		}
		if(bWithColor) { oss << getColorMessage("  ", ANSITYPE_BOLD, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
		else { oss << "  "; }
		return oss.str();
	}

	inline string getOneRowBoardString(int row, const int boardSize, bool bWithColor) const {
		ostringstream ossTemp, oss;
		ossTemp << std::setw(2) << (row + 1);
		string sRowNumber = bWithColor ? getColorMessage(ossTemp.str(), ANSITYPE_BOLD, ANSICOLOR_BLACK, ANSICOLOR_YELLOW) : ossTemp.str();
		oss << sRowNumber;
		for (int x = 0; x < boardSize; x++) {
			int pos = row * boardSize + x;
			if (getColor(pos) == COLOR_NONE) {
				if(bWithColor) { oss << getColorMessage(" .", ANSITYPE_BOLD, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
				else { oss << " ."; }
			} else {
				if (m_vMoves.size() >= 1 && m_vMoves.back().getPosition() == pos) {
					if (bWithColor) { oss << getColorMessage("(", ANSITYPE_BOLD, ANSICOLOR_RED, ANSICOLOR_YELLOW); }
					else { oss << "("; }
				} else if (m_vMoves.size() >= 2 && m_vMoves[m_vMoves.size() - 2].getPosition() == pos) {
					if (bWithColor) { oss << getColorMessage("(", ANSITYPE_NORMAL, ANSICOLOR_RED, ANSICOLOR_YELLOW); }
					else { oss << "("; }
				} else {
					if (bWithColor) { oss << getColorMessage(" ", ANSITYPE_NORMAL, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
					else { oss << " "; }
				}

				if (getColor(pos) == COLOR_BLACK) {
					if (bWithColor) { oss << getColorMessage("O", ANSITYPE_NORMAL, ANSICOLOR_BLACK, ANSICOLOR_YELLOW); }
					else { oss << "O"; }
				} else if (getColor(pos) == COLOR_WHITE) {
					if (bWithColor) { oss << getColorMessage("O", ANSITYPE_BOLD, ANSICOLOR_WHITE, ANSICOLOR_YELLOW); }
					else { oss << "X"; }
				}
			}
		}
		oss << sRowNumber;
		return oss.str();
	}

protected:
	Color m_turnColor;
	HashKey m_hashKey;
	vector<_Move> m_vMoves;
	vector<string> m_vComments;
};
