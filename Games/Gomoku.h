#pragma once
#include "GameConfigure.h"
#include "GameBase.h"
#include "Rand64.h"

class GomokuMove : public _Move {
public:
	GomokuMove() : _Move() {}
	GomokuMove(Color c, int position) : _Move(c, position) {}
	GomokuMove(Color c, string sPosition, int boardSize) : _Move(c, sPosition, boardSize) {}
	GomokuMove(pair<string, string> pSgfString, int boardSize) : _Move(pSgfString, boardSize) {}

	~GomokuMove() {}
};

class GomokuGame : public _Game<GomokuMove> {
private:
	vector<Color> m_vBoard;
	vector<int> m_threatRecord;
	int m_currentThreat;
	Color m_winner;

	static vector<vector<HashKey>> m_vGridHash;
	static bool initialized;

public:
	GomokuGame(): _Game()
	{
		reset();
		if (!initialized) initializeStatic();
	}
	~GomokuGame() {}

	static void initializeStatic() {
		// hash keys
		init_genrand64(0);
		m_vGridHash.resize(getMaxNumLegalAction(), vector<HashKey>(COLOR_SIZE - 1));
		for (int pos = 0; pos < getMaxNumLegalAction(); ++pos) {
			for (int c = 0; c < COLOR_SIZE - 1; ++c) {
				m_vGridHash[pos][c] = rand64();
			}
		}
		initialized = true;
	}

	void reset() {
		_Game::reset();
		m_turnColor = COLOR_BLACK;
		m_hashKey = 0;
		m_vBoard.resize(getBoardSize() * getBoardSize());
		fill(m_vBoard.begin(), m_vBoard.end(), COLOR_NONE);
		m_threatRecord.clear();
		m_currentThreat = -1;
		m_winner = COLOR_NONE;
	}

	void play(const GomokuMove& move) {
		if (move.isPass(getBoardSize())) {
			m_turnColor = AgainstColor(move.getColor());
			return;
		}

		_Game::play(move);
		m_vBoard[move.getPosition()] = move.getColor();
		m_turnColor = AgainstColor(move.getColor());
		m_hashKey ^= m_vGridHash[move.getPosition()][move.getColor() - 1];

#if GOMOKU
		if (GameConfigure::GOMOKU_KNOWLEDGE_LEVEL == 0) { updateConnection(); }
		else{ updateThreat(); }
#endif
	}

	inline bool isLegalMove(const GomokuMove& move) const {
		assert(("Invalid move color", move.getColor() == COLOR_BLACK || move.getColor() == COLOR_WHITE));
		assert(("Invalid move position", move.isPass(getBoardSize()) || move.getPosition() >= 0 && move.getPosition() < getMaxNumLegalAction()));
		if(m_currentThreat != -1) {	return move.getPosition() == m_currentThreat; }
		return (m_vBoard[move.getPosition()] == COLOR_NONE);
	}

	inline bool isBadMove(const GomokuMove& move) const {
		return false;
	}

	inline bool isTerminal() {
		// terminal: all points are played by "O" or "X" || any player wins
		return (eval() != COLOR_NONE || find(m_vBoard.begin(), m_vBoard.end(), COLOR_NONE) == m_vBoard.end());
	}

	inline bool hasCurrentThreat() const { return m_currentThreat != -1; }

	inline int to1DIndex(int i, int j) const {
		return i * getBoardSize() + j;
	}

	inline Color eval() const {
		return m_winner;
	}
  /*
	inline Color eval() const {
		if (m_vMoves.empty()) {
			return COLOR_NONE;
		}

		const int lastMoveIndex = m_vMoves.back().getPosition();
		const Color c = m_vBoard[lastMoveIndex];
		const int i = lastMoveIndex / getBoardSize();
		const int j = lastMoveIndex % getBoardSize();
		int chessCount;

		// rows
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (j + k >= getBoardSize() || m_vBoard[to1DIndex(i, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (j - k < 0 || m_vBoard[to1DIndex(i, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { return c; }

		// columns
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i + k >= getBoardSize() || m_vBoard[to1DIndex(i + k, j)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i - k < 0 || m_vBoard[to1DIndex(i - k, j)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { return c; }

		// diagonal (left-up to right-down)
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i + k >= getBoardSize() || j + k >= getBoardSize() || m_vBoard[to1DIndex(i + k, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i - k < 0 || j - k < 0 || m_vBoard[to1DIndex(i - k, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { return c; }

		// diagonal (left-down to right-up)
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i - k < 0 || j + k >= getBoardSize() || m_vBoard[to1DIndex(i - k, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i + k >= getBoardSize() || j - k < 0 || m_vBoard[to1DIndex(i + k, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { return c; }

		return COLOR_NONE;
	}
  */

	string getFinalScore() const {
		Color winner = eval();
		if (winner == COLOR_BLACK) { return "B+0"; }
		else if (winner == COLOR_WHITE) { return "W+0"; }
		else { return "0"; }
	}

	void undo() {
		assert(("Undo on the initial game", !m_vMoves.empty()));

		GomokuMove& preMove = m_vMoves.back();
		m_vBoard[preMove.getPosition()] = COLOR_NONE;
		m_turnColor = preMove.getColor();
		m_hashKey ^= m_vGridHash[preMove.getPosition()][preMove.getColor() - 1];
		m_vMoves.pop_back();
		m_vComments.resize(m_vMoves.size());
#if GOMOKU
		if (GameConfigure::GOMOKU_KNOWLEDGE_LEVEL == 0) { m_winner = COLOR_NONE; }
		else { undoThreat(); }
#endif
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
	static int getBoardSize() { return 15; }
	static int getMaxNumLegalAction() { return 225; }
	static int getNumChannels() { return 4; }

private:

  inline void updateConnection() {
		if (m_vMoves.empty()) {
			return;
		}

		const int lastMoveIndex = m_vMoves.back().getPosition();
		const Color c = m_vBoard[lastMoveIndex];
		const int i = lastMoveIndex / getBoardSize();
		const int j = lastMoveIndex % getBoardSize();
		int chessCount;

		// rows
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (j + k >= getBoardSize() || m_vBoard[to1DIndex(i, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (j - k < 0 || m_vBoard[to1DIndex(i, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { 
      m_winner = c;
      return;
    }

		// columns
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i + k >= getBoardSize() || m_vBoard[to1DIndex(i + k, j)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i - k < 0 || m_vBoard[to1DIndex(i - k, j)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { 
      m_winner = c;
      return;
    }

		// diagonal (left-up to right-down)
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i + k >= getBoardSize() || j + k >= getBoardSize() || m_vBoard[to1DIndex(i + k, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i - k < 0 || j - k < 0 || m_vBoard[to1DIndex(i - k, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { 
      m_winner = c;
      return;
    }

		// diagonal (left-down to right-up)
		chessCount = 0;
		for (int k = 0; k < 5; k++) {
			if (i - k < 0 || j + k >= getBoardSize() || m_vBoard[to1DIndex(i - k, j + k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		for (int k = 1; k < 5; k++) {
			if (i + k >= getBoardSize() || j - k < 0 || m_vBoard[to1DIndex(i + k, j - k)] != c) {
				break;
			} else {
				chessCount++;
			}
		}
		if (chessCount >= 5) { 
      m_winner = c;
      return;
    }
  }


  inline void updateThreat() {
    m_threatRecord.push_back(m_currentThreat);
    m_currentThreat = -1;
    // row
    updateLineThreat(0, 1);
    // column
    updateLineThreat(1, 0);
    // diagonal (left-up to right-down)
    updateLineThreat(1, 1);
    // diagonal (left-down to right-up)
    updateLineThreat(-1, 1);
  }

  inline void updateLineThreat(int di, int dj) {
    if(m_winner != COLOR_NONE) { return; }
		const int lastMoveIndex = m_vMoves.back().getPosition();
    const int i = lastMoveIndex / getBoardSize();
    const int j = lastMoveIndex % getBoardSize();
		const Color c = m_vBoard[lastMoveIndex];
		const Color opponent = (c == COLOR_BLACK ? COLOR_WHITE : COLOR_BLACK);
    
    int si = i, sj = j;
    // find start point
    for (int k = 0; k < 4; k++) {
      if (si - di < 0 || si - di >= getBoardSize() ||
          sj - dj < 0 || sj - dj >= getBoardSize() ||

          m_vBoard[to1DIndex(si - di, sj - dj)] == opponent) {
        break;
      }
      si -= di;
      sj -= dj;
    }
    //investigate the first four place of the first segment
    int ti = si, tj = sj;
    int pieceCount = 0, emptyID = -1;
    for (int k = 0; k < 4; k++) {
      if (ti < 0 || ti >= getBoardSize() ||
          tj < 0 || tj >= getBoardSize() ||
          m_vBoard[to1DIndex(ti, tj)] == opponent) {
        //no legal segment
        return;
      }
      if (m_vBoard[to1DIndex(ti, tj)] == c) {
        pieceCount++;
      }
      else {
        emptyID = to1DIndex(ti, tj);
      }
      ti += di;
      tj += dj;
    }
    //go through all segments by adding tail and removing head
    for (int k = 0; k < 5; k++) {
      if (ti < 0 || ti >= getBoardSize() ||
          tj < 0 || tj >= getBoardSize() ||
          m_vBoard[to1DIndex(ti, tj)] == opponent) {
        break;
      }
      if (m_vBoard[to1DIndex(ti, tj)] == c) {
        pieceCount++;
      }
      else {
        emptyID = to1DIndex(ti, tj);
      }
      if (pieceCount == 4) {
        if(m_currentThreat != -1 && m_currentThreat != emptyID) {
          m_winner = c;
          return;
        }
        m_currentThreat = emptyID;
      }
      if (m_vBoard[to1DIndex(si, sj)] == c) {
        pieceCount--;
      }
      si += di;
      sj += dj;
      ti += di;
      tj += dj;
    }
  }

  inline void undoThreat() {
    m_currentThreat = m_threatRecord.back();
    m_threatRecord.pop_back();
    m_winner = COLOR_NONE;
  }

};
