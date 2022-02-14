#pragma once
#include "Bitboard.h"
#include "FeatureIndexList.h"
#include "FeatureList.h"
#include "GameBase.h"
#include "GameConfigure.h"
#include "HashTable.h"
#include "Rand64.h"
#include <map>
#include <unordered_set>

const int BOARD_SIZE = 9;
const int NUM_GRIDS = BOARD_SIZE * BOARD_SIZE;
const int MAX_NUM_MOVES = NUM_GRIDS + 1;
const int MAX_GAME_LENGTH = MAX_NUM_MOVES * 2;
const int MAX_UCT_CLOSEDAREA_SIZE = 30;
const int MAX_NUM_CLOSEDAREA = MAX_NUM_MOVES;
typedef Bitboard<MAX_NUM_MOVES, BOARD_SIZE> GoBitBoard;

class GoMove : public _Move {
  public:
	GoMove() : _Move() {}
	GoMove(Color c, int position) : _Move(c, position) {}
	GoMove(Color c, string sPosition, int boardSize) : _Move(c, sPosition, boardSize) {}
	GoMove(pair<string, string> pSgfString, int boardSize) : _Move(pSgfString, boardSize) {}

	~GoMove() {}
};

enum GoLifeAndDeathStatus {
	LAD_NOT_EYE,
	LAD_TRUE_EYE,
	LAD_LIFE,
	LAD_SIZE // total size, add new element before this one
};

inline string getGoLifeAndDeathString(GoLifeAndDeathStatus status) {
	switch (status) {
	case LAD_NOT_EYE:
		return "LAD_NOT_EYE   ";
	case LAD_TRUE_EYE:
		return "LAD_TRUE_EYE  ";
	case LAD_LIFE:
		return "LAD_LIFE      ";
	default:
		//should not happen
		assert(false);
		return "LAD_ERROR STATUS ";
	}
}

class GoClosedArea {
  public:
	enum ClosedAreaType {
		CA_UNKNOWN,
		CA_CORNER,
		CA_EDGE,
		CA_CENTRAL
	};

  private:
	// TODO m_id type
	uint m_id;
	Color m_color;
	uint m_numStone;
	GoBitBoard m_bmStone;
	FeatureIndexList<int, MAX_GAME_LENGTH> m_blockIDs;
	GoLifeAndDeathStatus m_status;
	ClosedAreaType m_type;

  public:
	GoClosedArea() { Clear(); }

	void Clear() {
		m_id = static_cast<uint>(-1);
	}
	void init() {
		m_color = COLOR_NONE;
		m_numStone = 0;
		m_bmStone.reset();
		m_blockIDs.clear();
		m_status = LAD_NOT_EYE;
		m_type = CA_UNKNOWN;
	}
	inline uint GetID() const { return m_id; }
	inline void SetID(uint id) { m_id = id; }

	inline Color getColor() const { return m_color; }
	inline void setColor(Color c) { m_color = c; }

	inline uint getNumStone() const { return m_numStone; }
	inline void setNumStone(uint numStone) { m_numStone = numStone; }
	inline void decNumStone() { --m_numStone; }
	inline void incNumStone() { ++m_numStone; }

	inline void addBlockID(uint blockID) { m_blockIDs.addFeature(blockID); }
	inline void deleteBlockID(uint blockID) { m_blockIDs.removeFeature(blockID); }
	inline bool hasBlockID(uint blockID) const { return m_blockIDs.contains(blockID); }
	inline uint getNumBlock() const { return m_blockIDs.getNumFeature(); }
	inline uint getBlockID(uint idx) const { return m_blockIDs[idx]; }

	inline GoBitBoard &getStoneMap() { return m_bmStone; }
	inline const GoBitBoard &getStoneMap() const { return m_bmStone; }
	inline void setStoneMap(const GoBitBoard &bmStone) { m_bmStone = bmStone; }
	inline void setStoneMapOff(uint pos) { m_bmStone.setBitOff(pos); }
	inline GoBitBoard getSurroundBitBoard() const { return m_bmStone.dilation() & ~m_bmStone; }

	inline void setStatus(GoLifeAndDeathStatus status) { m_status = status; }
	inline GoLifeAndDeathStatus getStatus() const { return m_status; }

	inline void calculateCAType() {
		if (!(GoBitBoard::getCornerBitBoard()).empty()) {
			m_type = CA_CORNER;
		} else if (!(GoBitBoard::getEdgeBitBoard()).empty()) {
			m_type = CA_EDGE;
		} else {
			m_type = CA_CENTRAL;
		}
	}
	inline ClosedAreaType getType() const { return m_type; }

	inline void merge(const GoClosedArea *mergeCA) {
		assert(mergeCA->getColor() == m_color);

		m_numStone += mergeCA->getNumStone();
		m_bmStone |= mergeCA->getStoneMap();
		for (uint iNum = 0; iNum < mergeCA->getNumBlock(); iNum++) {
			const uint iID = mergeCA->getBlockID(iNum);
			if (!hasBlockID(iID)) {
				addBlockID(iID);
			}
		}
	}

	inline void removePoint(uint pos) {
		assert(m_bmStone.BitIsOn(pos));

		m_bmStone.setBitOff(pos);
		--m_numStone;
		calculateCAType();
	}
	inline void addPoint(uint pos) {
		assert(!m_bmStone.BitIsOn(pos));

		m_bmStone.setBitOn(pos);
		++m_numStone;
		calculateCAType();
	}
};

class GoBlock {
  public:
	enum BlockStatus {
		BLK_UNKNOWN,
		BLK_NOT_LIFE,
		BLK_SENTE_LIFE,
		BLK_GOTE_LIFE,
	};

  private:
	int m_index;
	Color m_color;
	HashKey m_hashKey;
	GoBitBoard m_grids;
	GoBitBoard m_liberties;

	//closed area
	FeatureIndexList<int, MAX_NUM_MOVES> m_closedAreas;
	GoLifeAndDeathStatus m_LADStatus;

  public:
	GoBlock(int index) {
		clear();
		m_index = index;
	}

	void clear() {
		m_color = COLOR_NONE;
		m_hashKey = 0;
		m_grids.reset();
		m_liberties.reset();
		m_closedAreas.clear();

		// TODO Benson, now assume all block life
		m_LADStatus = LAD_LIFE;
	}

	void combine(GoBlock *block) {
		m_hashKey ^= block->m_hashKey;
		m_grids |= block->m_grids;
		m_liberties |= block->m_liberties;
	}

	void removeGridOnBoundary(int pos, HashKey k) {
		m_grids.setBitOff(pos);
		m_liberties &= m_grids.dilation();
		m_liberties.setBitOn(pos);
		addHashKey(k);
	}

	void removeGrids(GoBitBoard bmRmGrids, HashKey k) {
		m_grids -= bmRmGrids;
		addHashKey(k);
	}

	inline void setIndex(int i) { m_index = i; }
	inline void setColor(Color c) { m_color = c; }
	inline void addHashKey(HashKey k) { m_hashKey ^= k; }
	inline void addGrid(int pos) { m_grids.setBitOn(pos); }
	inline void addGrids(const GoBitBoard &bmGrids) { m_grids |= bmGrids; }
	inline void addLiberty(int pos) { m_liberties.setBitOn(pos); }
	inline void addLiberties(const GoBitBoard &bmLiberties) { m_liberties |= bmLiberties; }
	inline void removeLiberty(int pos) { m_liberties.setBitOff(pos); }
	inline void removeLiberties(const GoBitBoard &bmLiberties) { m_liberties -= bmLiberties; }
	inline int getIndex() const { return m_index; }
	inline Color getColor() const { return m_color; }
	inline HashKey getHashKey() const { return m_hashKey; }
	inline GoBitBoard &getGrids() { return m_grids; }
	inline const GoBitBoard &getGrids() const { return m_grids; }
	inline GoBitBoard &getLiberties() { return m_liberties; }
	inline const GoBitBoard &getLiberties() const { return m_liberties; }
	inline int getNumLiberty() const { return m_liberties.bitCount(); }
	inline int getLastLiberty() const {
		assert(getNumLiberty() == 1);
		GoBitBoard bmLib = m_liberties;
		int lib = bmLib.bitScanForward();
		return lib;
	}
	inline GoBitBoard getStonenNbrMap() const { return m_grids.dilation(); }
	inline int getNumStone() const { return m_grids.bitCount(); }

	// closed area
	inline void addClosedArea(GoClosedArea *ca) { m_closedAreas.addFeature(ca->GetID()); }
	inline void deleteClosedArea(GoClosedArea *ca) { m_closedAreas.removeFeature(ca->GetID()); }
	inline void deleteClosedArea(int idx) { m_closedAreas.removeFeature(idx); }
	inline void deleteClosedArea() { m_closedAreas.clear(); }
	inline bool hasClosedArea(const GoClosedArea *ca) const { return m_closedAreas.contains(ca->GetID()); }
	inline void setStatus(GoLifeAndDeathStatus status) { m_LADStatus = status; }
	inline GoLifeAndDeathStatus getStatus() const { return m_LADStatus; }
	inline uint getNumClosedArea() const { return m_closedAreas.size(); }
	inline uint getClosedAreaID(uint idx) const { return m_closedAreas[idx]; }
};

class GoGrid {
  private:
	Color m_color;
	int m_position;
	GoBlock *m_block;
	string ss;
	vector<int> m_vNeighbors;
	GoBitBoard m_bmEyeCorners;
	// closed area
	pair<GoClosedArea *, GoClosedArea *> m_closedArea;

  public:
	GoGrid(int position, int boardSize) {
		assert(("Position not on board", _isOnBoard(position % boardSize, position / boardSize, boardSize)));
		clear();
		m_position = position;
		_initializeNeighbors(boardSize);
		ss.resize(10000);
	}

	void clear() {
		m_block = nullptr;
		m_color = COLOR_NONE;
		m_closedArea.first = nullptr;
		m_closedArea.second = nullptr;
	}

	inline void setColor(Color c) { m_color = c; }
	inline void setBlock(GoBlock *b) { m_block = b; }
	inline Color getColor() const { return m_color; }
	inline int getPosition() const { return m_position; }
	inline GoBlock *getBlock() { return m_block; }
	inline const GoBlock *getBlock() const { return m_block; }
	inline const vector<int> &getNeighbors() const { return m_vNeighbors; }
	inline const GoBitBoard &getEyeCorners() const { return m_bmEyeCorners; }

	// closed area
	inline pair<GoClosedArea *, GoClosedArea *> &getClosedArea() { return m_closedArea; }
	inline const pair<GoClosedArea *, GoClosedArea *> &getClosedArea() const { return m_closedArea; }
	inline GoClosedArea *getClosedAreaP(Color c) { return (c == COLOR_BLACK) ? m_closedArea.first : m_closedArea.second; }
	inline const GoClosedArea *getClosedArea(Color c) const { return (c == COLOR_BLACK) ? m_closedArea.first : m_closedArea.second; }
	inline void setClosedArea(GoClosedArea *closedArea, Color c) {
		if (c == COLOR_BLACK)
			m_closedArea.first = closedArea;
		else
			m_closedArea.second = closedArea;
	}
	inline void deleteClosedArea() {
		m_closedArea.first = nullptr;
		m_closedArea.second = nullptr;
	}

  private:
	void _initializeNeighbors(int boardSize) {
		int x = m_position % boardSize;
		int y = m_position / boardSize;
		vector<pair<int, int>> vPairs{{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
		vector<pair<int, int>> vEyeCorners{{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

		for (auto p : vPairs) {
			int nbrX = p.first + x;
			int nbrY = p.second + y;
			if (!_isOnBoard(nbrX, nbrY, boardSize)) {
				continue;
			}
			m_vNeighbors.push_back(nbrY * boardSize + nbrX);
		}
		for (auto p : vEyeCorners) {
			int nbrX = p.first + x;
			int nbrY = p.second + y;
			if (!_isOnBoard(nbrX, nbrY, boardSize)) {
				continue;
			}
			m_bmEyeCorners.setBitOn(nbrY * boardSize + nbrX);
		}
	}

	inline bool _isOnBoard(int x, int y, int boardSize) const {
		return (x >= 0 && x < boardSize && y >= 0 && y < boardSize);
	}
};

class GoGame : public _Game<GoMove> {

  static const int POSITIONAL_SUPER_KO = 0;
  static const int SITUATIONAL_SUPER_KO = 1;

  private:
	vector<GoGrid> m_vGrids;
	vector<GoBlock> m_vBlocks;
	GoBitBoard m_bmBlockUsage;
	pair<GoBitBoard, GoBitBoard> m_stoneBitBoard;
	vector<pair<GoBitBoard, GoBitBoard>> m_vStoneBitBoard;

	HashTable m_hashTable;
	static vector<vector<HashKey>> m_vGridHash;
	static vector<HashKey> m_vKoHash;
	static HashKey m_twoPassHashKey;
	static HashKey m_turnHashkey;
	static bool initialized;
	vector<HashKey> m_vHash;

	// force move
	map<int, string> m_mForceMoves;

	// closed area
	vector<GoBlock> m_vclosedAreas;
	GoBitBoard m_bmclosedAreaUsage;
	FeatureList<GoClosedArea, MAX_NUM_MOVES> m_closedAreas;

	// Benson
	struct Benson {
		short m_blockNumHealthyCA[MAX_GAME_LENGTH];
		FeatureIndexList<uint, MAX_GAME_LENGTH> m_blockIDs;
		FeatureIndexList<uint, MAX_NUM_CLOSEDAREA> m_closedAreaIDs;
	};
	pair<GoBitBoard, GoBitBoard> m_bmBenson;
	pair<int, int> m_numLifeStone;

  public:
	GoGame() : _Game() {
		_initialize();
		reset();
		if (!initialized) initializeStatic();
	}
	~GoGame() {}

	static void initializeStatic() {
		// bit board
		GoBitBoard::initialize();
		// hash keys
		init_genrand64(0);
		m_vGridHash.resize(getMaxNumLegalAction(), vector<HashKey>(COLOR_SIZE - 1));
		for (int pos = 0; pos < getMaxNumLegalAction(); ++pos) {
			for (int c = 0; c < COLOR_SIZE - 1; ++c) {
				m_vGridHash[pos][c] = rand64();
			}
		}
		m_vKoHash.resize(getMaxNumLegalAction());
		for (int pos = 0; pos < getMaxNumLegalAction(); ++pos) {
			m_vKoHash[pos] = rand64();
		}
		m_twoPassHashKey = rand64();
		m_turnHashkey = 0;
#ifdef GO
		if (GameConfigure::GO_SUPER_KO_RULE == SITUATIONAL_SUPER_KO) { m_turnHashkey = rand64(); }
#endif
		initialized = true;
	}

	void resetCA();
	void reset();
	void play(const GoMove &move);
	// Fast undo
	void undo();
	// Trivial undo
	void deepCheck();
	void trivialUndo();
	bool isLegalMove(const GoMove &move) const;
	bool isBadMove(const GoMove &move) const;
	bool isOwnTrueEye(const GoMove &move) const;
	bool isLastMoveCaptureBlock() const;
	bool isTerminal();
	HashKey getHashKey() const { return m_hashKey; }
	Color eval() const;
	string getFinalScore() const;
	Color getColor(int position) const;
	vector<float> getFeatures(SymmetryType type = SYM_NORMAL) const;
	HashKey getTTHashKey() const;

	// closed area
	inline void findFullBoardUCTClosedArea() {
		setFullBoardClosedArea(COLOR_BLACK);
		setFullBoardClosedArea(COLOR_WHITE);
		setFullBoardBensonLife();
	}

	// overwrite static function
	static int getBoardSize() { return BOARD_SIZE; }
	static int getMaxNumLegalAction() { return getBoardSize() * getBoardSize() + 1; }
	static int getNumChannels() { return 18; }

  private:
	void _initialize();
	void _setColor(const GoMove &move);
	GoBlock *_newBlock(Color c);
	void _removeBlock(GoBlock *b);
	void _removeBlockAndGrids(GoBlock *b);
	void _addGridAndLibertiesToBlock(GoMove m, GoBlock *b, const GoBitBoard &bmNbrLiberties);
	void _addGridsAndLibertiesToBlock(GoBitBoard &bmGrids, GoBlock *b, const GoBitBoard &bmNbrLiberties);
	GoBlock *_combineBlocks(GoBlock *b1, GoBlock *b2);

	// Board information
	inline GoBitBoard getStoneBitBoard(Color c) const {	return (c == COLOR_BLACK ? m_stoneBitBoard.first : m_stoneBitBoard.second); }
	inline int getNumLibertyAfterPlay(const GoMove &move) const { return getLibertyBitBoardAfterPlay(move).bitCount(); }
	GoBitBoard getStoneBitBoardAfterPlay(const GoMove &move) const;
	GoBitBoard getLibertyBitBoardAfterPlay(const GoMove &move) const;
	void getMoveInfluence(const GoMove &move, pair<GoBitBoard, GoBitBoard>& influence) const;
	bool isEatKoMove(const GoMove &move) const;
	bool isCaptureMove(const GoMove &move) const;

	// closed area
	bool isCAHealthy(const GoClosedArea *closedArea) const;
	void setFullBoardClosedArea(Color findColor);
	void setClosedAreaAttribute(GoClosedArea *closedArea, Color caColor, GoBitBoard &bmStone, uint numStone);
	void setFullBoardBensonLife();

	// Benson
	void findBensonSet(Benson &benson) const;
	void findBensonLife(Benson &benson) const;
	void setBensonLife(Benson &benson);
	GoBitBoard getBensonBitboard(Color c) const {
		assert(c == COLOR_BLACK || c == COLOR_WHITE);
		return (c == COLOR_BLACK) ? m_bmBenson.first : m_bmBenson.second;
	}
};
