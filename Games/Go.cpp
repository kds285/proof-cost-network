#include "Go.h"

vector<vector<HashKey>> GoGame::m_vGridHash;
vector<HashKey> GoGame::m_vKoHash;
HashKey GoGame::m_twoPassHashKey;
HashKey GoGame::m_turnHashkey;
bool GoGame::initialized;

void GoGame::resetCA() {
	m_vclosedAreas.clear();
	m_bmclosedAreaUsage.reset();
	m_closedAreas.reset();
	m_numLifeStone.first = 0;
	m_numLifeStone.second = 0;
	for (int p = 0; p < getBoardSize() * getBoardSize(); ++p) {
		m_vGrids[p].deleteClosedArea();
		m_vBlocks[p].deleteClosedArea();
	}
	m_bmBenson.first.reset();
	m_bmBenson.second.reset();
}

void GoGame::reset() {
	_Game::reset();
	m_turnColor = COLOR_BLACK;
	m_hashKey = 0;
	m_stoneBitBoard.first.reset();
	m_stoneBitBoard.second.reset();
	m_vStoneBitBoard.clear();
	m_bmBlockUsage.reset();
	m_bmBlockUsage = ~m_bmBlockUsage;
	for (int i = 0; i < getBoardSize() * getBoardSize(); ++i) {
		m_vGrids[i].clear();
		m_vBlocks[i].clear();
	}
	m_hashTable.clear();
	m_vHash.clear();
	resetCA();
}

void GoGame::play(const GoMove &move) {
	_Game::play(move);

	m_turnColor = AgainstColor(move.getColor());
	m_hashKey ^= m_turnHashkey;
	if (move.isPass(getBoardSize())) {
		m_vStoneBitBoard.push_back(m_stoneBitBoard);
		m_vHash.push_back(m_hashKey);
		return;
	}

	_setColor(move);
	m_hashKey ^= m_vGridHash[move.getPosition()][move.getColor() - 1];

	// collect neighbors
	GoBitBoard bmNbrLiberties;
	GoBitBoard bmNbrOwnBlocks;
	GoBitBoard bmNbrOppBlocks;
	vector<GoBlock *> vNbrOwnBlocks;
	vector<GoBlock *> vNbrOppBlocks;
	const vector<int> &vNbr = m_vGrids[move.getPosition()].getNeighbors();

	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		GoGrid &g = m_vGrids[vNbr[i]];
		if (g.getColor() == COLOR_NONE) {
			bmNbrLiberties.setBitOn(vNbr[i]);
		} else {
			GoBlock *b = g.getBlock();
			if (g.getColor() == move.getColor()) {
				if ((b->getGrids() - bmNbrOwnBlocks).empty()) {
					continue;
				}
				bmNbrOwnBlocks |= b->getGrids();
				vNbrOwnBlocks.push_back(b);
			} else {
				if ((b->getGrids() - bmNbrOppBlocks).empty()) {
					continue;
				}
				bmNbrOppBlocks |= b->getGrids();
				vNbrOppBlocks.push_back(b);
			}
		}
	}

	// handle neighbor own blocks
	if (bmNbrOwnBlocks.empty()) {
		// create new blocks
		_addGridAndLibertiesToBlock(move, _newBlock(move.getColor()), bmNbrLiberties);
	} else {
		// merge with other blocks
		GoBlock *b = vNbrOwnBlocks[0];
		_addGridAndLibertiesToBlock(move, b, bmNbrLiberties);
		for (int i = 1; i < static_cast<int>(vNbrOwnBlocks.size()); ++i) {
			b = _combineBlocks(b, vNbrOwnBlocks[i]);
		}

		b->getLiberties().setBitOff(move.getPosition());
	}

	// handle neighbor opponent blocks
	for (int i = 0; i < static_cast<int>(vNbrOppBlocks.size()); ++i) {
		GoBlock *b = vNbrOppBlocks[i];
		b->getLiberties().setBitOff(move.getPosition());
		if (b->getLiberties().bitCount() == 0) {
			m_hashKey ^= b->getHashKey();
			_removeBlockAndGrids(b);
		}
	}

	// store historical hashkey & features & moves
	m_hashTable.store(m_hashKey);
	m_vStoneBitBoard.push_back(m_stoneBitBoard);
	assert(("m_vStoneBitBoard.size() != m_vMoves.size()", m_vStoneBitBoard.size() == m_vMoves.size()));
	m_vHash.push_back(m_hashKey);
	// deepCheck();
}

void GoGame::undo() {
	if (m_vMoves.empty()) return;
	if (m_vMoves.size() == 1) {
		m_vHash.pop_back();
		reset();
		return;
	}

	GoMove prevMove = m_vMoves.back();
	GoBitBoard bmEatenStones;
	// get eaten stones
	if (prevMove.getColor() == COLOR_BLACK) {
		bmEatenStones = m_vStoneBitBoard[m_vMoves.size() - 2].second - m_stoneBitBoard.second - GoBitBoard(getBoardSize() * getBoardSize());
	} else if (prevMove.getColor() == COLOR_WHITE) {
		bmEatenStones = m_vStoneBitBoard[m_vMoves.size() - 2].first - m_stoneBitBoard.first - GoBitBoard(getBoardSize() * getBoardSize());
	}

	// restore stone bit board
	m_vStoneBitBoard.pop_back();
	m_stoneBitBoard = m_vStoneBitBoard[m_vMoves.size() - 1];
	m_vMoves.pop_back();
	m_turnColor = prevMove.getColor();

	if (prevMove.isPass(getBoardSize())) {
		m_hashKey ^= m_turnHashkey;
		m_vHash.pop_back();
		return;
	}

	// handle hash
	m_hashTable.erase(m_hashKey);
	m_hashKey ^= m_vGridHash[prevMove.getPosition()][prevMove.getColor() - 1];

	// handle eaten stones
	if (!bmEatenStones.empty()) {
		// rebuild all eaten blocks
		GoBitBoard bmEatenStonesMap = bmEatenStones;
		while (!bmEatenStonesMap.empty()) {
			int pos = bmEatenStonesMap.bitScanForward();
			GoBitBoard bmEatenBlock = bmEatenStones.floodfill(pos);
			GoBlock *b = _newBlock(AgainstColor(prevMove.getColor()));
			_addGridsAndLibertiesToBlock(bmEatenBlock, b, GoBitBoard(prevMove.getPosition()));
			bmEatenStonesMap -= bmEatenBlock;
			m_hashKey ^= b->getHashKey();
		}

		// remove liberties of boundary own blocks
		GoBitBoard bmBoundaryOwnGrids = bmEatenStones.dilation() - bmEatenStones - GoBitBoard(prevMove.getPosition());
		int pos;
		while ((pos = bmBoundaryOwnGrids.bitScanForward()) != -1) {
			GoBlock *blNbrOwnBlock = m_vGrids[pos].getBlock();
			GoBitBoard bmRmLiberties = blNbrOwnBlock->getGrids().dilation() & bmEatenStones;
			bmBoundaryOwnGrids -= blNbrOwnBlock->getGrids();
			blNbrOwnBlock->removeLiberties(bmRmLiberties);
		}
	}

	m_hashKey ^= m_turnHashkey;
	const vector<int> &vNbr = m_vGrids[prevMove.getPosition()].getNeighbors();

	GoBitBoard bmNbrOwnBlocks;
	GoBitBoard bmNbrOppBlocks;

	// handle neighbor blocks
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		GoGrid &g = m_vGrids[vNbr[i]];
		if (g.getColor() != COLOR_NONE) {
			GoBlock *b = g.getBlock();
			if (g.getColor() == prevMove.getColor()) {
				// handle own block
				GoBitBoard bmBlocksMap = b->getGrids();
				bmBlocksMap.setBitOff(prevMove.getPosition());
				// dilate from vNbr[i], check if the block is able to split to two blocks
				GoBitBoard bmBlock = bmBlocksMap.floodfill(vNbr[i]);

				if ((bmBlock - bmNbrOwnBlocks).empty()) {
					continue;
				}
				bmNbrOwnBlocks |= bmBlock;

				if (bmBlock == bmBlocksMap) {
					// need not split
					b->removeGridOnBoundary(prevMove.getPosition(), m_vGridHash[prevMove.getPosition()][prevMove.getColor() - 1]);
				} else {
					// need split
					// create new block
					GoBitBoard bmNewGrids = bmBlock, bmNewLiberties = (bmBlock.dilation() & b->getLiberties()) | GoBitBoard(prevMove.getPosition());
					GoBlock *blNewBlock = _newBlock(prevMove.getColor());
					_addGridsAndLibertiesToBlock(bmNewGrids, blNewBlock, bmNewLiberties);

					// remove from origin block b
					b->removeGrids(bmNewGrids, blNewBlock->getHashKey());
				}

			} else {
				// handle opp block
				b->addLiberty(prevMove.getPosition());
				if ((b->getGrids() - bmNbrOppBlocks).empty()) {
					continue;
				}
				bmNbrOppBlocks |= b->getGrids();
			}
		}
	}

	if (bmNbrOwnBlocks.empty()) {
		// prevMove is a block, remove it
		GoBlock *b = m_vGrids[prevMove.getPosition()].getBlock();
		_removeBlock(b);
	}

	_setColor(GoMove(COLOR_NONE, prevMove.getPosition()));
	// check
	assert(("Hash restore correctly", m_vHash[m_vMoves.size() - 1] == m_hashKey));
	m_vHash.pop_back();
	// deepCheck();
}

void GoGame::deepCheck() {
	for (int p = 0; p < getBoardSize() * getBoardSize(); ++p) {
		if (m_vGrids[p].getColor() == COLOR_BLACK) {
			assert(("invalid black grid", m_stoneBitBoard.first.BitIsOn(p) && !m_stoneBitBoard.second.BitIsOn(p)));
			GoBitBoard bm = m_vGrids[p].getBlock()->getGrids();
			GoBitBoard lib = m_vGrids[p].getBlock()->getLiberties();
			int pos;
			HashKey h = 0;
			while ((pos = bm.bitScanForward()) != -1) {
				assert(("invalid black block", m_stoneBitBoard.first.BitIsOn(pos) && !m_stoneBitBoard.second.BitIsOn(pos) && m_vGrids[pos].getColor() == COLOR_BLACK));
				h ^= m_vGridHash[pos][COLOR_BLACK - 1];
			}
			assert(("invalid black block hash", h == m_vGrids[p].getBlock()->getHashKey()));
			while ((pos = lib.bitScanForward()) != -1) {
				assert(("invalid black block liberties", !m_stoneBitBoard.first.BitIsOn(pos) && !m_stoneBitBoard.second.BitIsOn(pos) && m_vGrids[pos].getColor() == COLOR_NONE));
			}
		}
		if (m_vGrids[p].getColor() == COLOR_WHITE) {
			assert(("invalid white grid", !m_stoneBitBoard.first.BitIsOn(p) && m_stoneBitBoard.second.BitIsOn(p)));
			GoBitBoard bm = m_vGrids[p].getBlock()->getGrids();
			GoBitBoard lib = m_vGrids[p].getBlock()->getLiberties();
			int pos;
			HashKey h = 0;
			while ((pos = bm.bitScanForward()) != -1) {
				assert(("invalid white block", !m_stoneBitBoard.first.BitIsOn(pos) && m_stoneBitBoard.second.BitIsOn(pos) && m_vGrids[pos].getColor() == COLOR_WHITE));
				h ^= m_vGridHash[pos][COLOR_WHITE - 1];
			}
			assert(("invalid white block hash", h == m_vGrids[p].getBlock()->getHashKey()));
			while ((pos = lib.bitScanForward()) != -1) {
				assert(("invalid white block liberties", !m_stoneBitBoard.first.BitIsOn(pos) && !m_stoneBitBoard.second.BitIsOn(pos) && m_vGrids[pos].getColor() == COLOR_NONE));
			}
		}
		if (m_vGrids[p].getColor() == COLOR_NONE) {
			assert(("invalid grid", !m_stoneBitBoard.first.BitIsOn(p) && !m_stoneBitBoard.second.BitIsOn(p) && !m_vGrids[p].getBlock()));
		}
	}
}

void GoGame::trivialUndo() {
	if (m_vMoves.size() == 0) return;
	//  reset without reset m_vMoves and m_vComments
	m_turnColor = COLOR_BLACK;
	m_hashKey = 0;
	m_stoneBitBoard.first.reset();
	m_stoneBitBoard.second.reset();
	m_vStoneBitBoard.clear();
	m_bmBlockUsage.reset();
	m_bmBlockUsage = ~m_bmBlockUsage;
	for (int i = 0; i < getMaxNumLegalAction(); ++i) {
		m_vGrids[i].clear();
		m_vBlocks[i].clear();
	}
	m_hashTable.clear();
	m_vMoves.pop_back();
	auto moves = m_vMoves;
	m_vMoves.clear();
	for (auto const move : moves) {
		play(move);
	}
}

bool GoGame::isOwnTrueEye(const GoMove &move) const {
	int pos = move.getPosition();
	GoBitBoard bmNbr = GoBitBoard(pos).dilation() - GoBitBoard(pos),
			   bmEyeCorners = m_vGrids[pos].getEyeCorners(),
			   bmStones = (move.getColor() == COLOR_BLACK) ? m_stoneBitBoard.first : m_stoneBitBoard.second;
	if (((bmNbr & bmStones) == bmNbr) && ((bmEyeCorners.bitCount() == 4 && (bmEyeCorners & bmStones).bitCount() >= 3) || ((bmEyeCorners & bmStones) == bmEyeCorners))) return true;
	return false;
}

bool GoGame::isLastMoveCaptureBlock() const
{
	if (m_vMoves.size() <= 2) { return false; }

	GoBitBoard bmPrevStone = m_vStoneBitBoard[m_vMoves.size() - 2].first | m_vStoneBitBoard[m_vMoves.size() - 2].second;
	GoBitBoard bmStone = m_vStoneBitBoard[m_vMoves.size() - 1].first | m_vStoneBitBoard[m_vMoves.size() - 1].second;

	return bmStone.bitCount() <= bmPrevStone.bitCount();
}

bool GoGame::isLegalMove(const GoMove &move) const {
	assert(("Invalid move color", move.getColor() == COLOR_BLACK || move.getColor() == COLOR_WHITE));
	assert(("Invalid move position", move.getPosition() >= 0 && move.getPosition() < getMaxNumLegalAction()));

	int move_num = m_vMoves.size() + 1;
	if (m_mForceMoves.find(move_num) != m_mForceMoves.end()) {
		return (move.toGtpString(getBoardSize()) == m_mForceMoves.at(move_num)) ? true : false;
	}

	if (move.isPass(getBoardSize())) {
		return true;
	}

	int pos = move.getPosition();
	if (m_vGrids[pos].getColor() != COLOR_NONE) {
		return false;
	}

	HashKey key = 0;
	bool bLegal = false;
	unordered_set<int> checkedBlockID;
	const vector<int> &vNbr = m_vGrids[pos].getNeighbors();
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoGrid &g = m_vGrids[vNbr[i]];
		if (g.getColor() == COLOR_NONE) {
			bLegal = true;
		}
		else {
			const GoBlock *b = g.getBlock();
			if(checkedBlockID.count(b->getIndex())) { continue; }

			checkedBlockID.insert(b->getIndex());
			if (g.getColor() == move.getColor()) {
				if (b->getLiberties().bitCount() > 1) {
					bLegal = true;
				}
			}
			else {
				if (b->getLiberties().bitCount() == 1) {
					key ^= b->getHashKey();
					bLegal = true;
				}
			}
		}
	}

	//check forbidden move
#ifdef GO
	if (GameConfigure::GO_FORBIDDEN_OWN_TRUE_EYE && isOwnTrueEye(move)) { bLegal = false; }
	if (GameConfigure::GO_FORBIDDEN_BENSON_REGION && (m_bmBenson.first | m_bmBenson.second).BitIsOn(move.getPosition())) { bLegal = false; }
	if (GameConfigure::GO_BLACK_FORBIDDEN_EAT_KO && move.getColor() == COLOR_BLACK && isEatKoMove(move)) { bLegal = false; }
	if (GameConfigure::GO_WHITE_FORBIDDEN_EAT_KO && move.getColor() == COLOR_WHITE && isEatKoMove(move)) { bLegal = false; }
#endif

	return (bLegal && !m_hashTable.lookup(key ^ m_vGridHash[pos][move.getColor() - 1] ^ m_hashKey ^ m_turnHashkey));
}

bool GoGame::isBadMove(const GoMove &move) const {
	return false;
}

bool GoGame::isTerminal() {
	resetCA();
	findFullBoardUCTClosedArea();
	if (m_bmBenson.second.bitCount() > 0) {
		// White Benson
		return true;
	}

	GoBitBoard bmBValid, bmWValid;
	for (int pos = 0; pos < MAX_NUM_MOVES; ++pos) {
		if (isLegalMove(GoMove(COLOR_BLACK, pos))) bmBValid.setBitOn(pos);
		if (isLegalMove(GoMove(COLOR_WHITE, pos))) bmWValid.setBitOn(pos);
	}
	if (bmBValid.empty() && bmWValid.empty()) {
		// no valid move
		return true;
	}

	if (m_vMoves.size() >= 2 && m_vMoves[m_vMoves.size() - 1].isPass(getBoardSize()) && m_vMoves[m_vMoves.size() - 2].isPass(getBoardSize())) {
		// two pass
		return true;
	}
	return false;
}

Color GoGame::eval() const {
	// White win if two pass but white exist stones
	if (m_vMoves.size() >= 2 && m_vMoves[m_vMoves.size() - 1].isPass(getBoardSize()) && m_vMoves[m_vMoves.size() - 2].isPass(getBoardSize())) {
		for (int pos = 0; pos < NUM_GRIDS; ++pos) {
			if (isLegalMove(GoMove(COLOR_BLACK, pos))) {
				return COLOR_WHITE;
			}
		}
		return m_stoneBitBoard.second.empty() ? COLOR_BLACK : COLOR_WHITE;
	}

	// Benson
	return m_bmBenson.second.empty() ? COLOR_BLACK : COLOR_WHITE;
}

string GoGame::getFinalScore() const {
	return (eval() == COLOR_BLACK) ? "B+1" : "W+1";
}

Color GoGame::getColor(int position) const {
	return m_vGrids[position].getColor();
}

vector<float> GoGame::getFeatures(SymmetryType type /* = SYM_NORMAL*/) const {
	vector<float> vFeatures;
	for (int channel = 0; channel < getNumChannels(); ++channel) {
		for (int pos = 0; pos < getMaxNumLegalAction() - 1; ++pos) {
			int rotatePos = getRotatePosition(pos, getBoardSize(), ReverseSymmetricType[type]);
			if (channel < 16) {
				int lastN = m_vMoves.size() - 1 - channel / 2;
				if (lastN < 0) {
					vFeatures.push_back(0.0f);
				} else {
					const pair<GoBitBoard, GoBitBoard> &bitBoard = m_vStoneBitBoard[lastN];
					if (channel % 2 == 0) {
						vFeatures.push_back(bitBoard.first.BitIsOn(rotatePos) == true ? 1.0f : 0.0f);
					} else {
						vFeatures.push_back(bitBoard.second.BitIsOn(rotatePos) == true ? 1.0f : 0.0f);
					}
				}
			} else if (channel == 16) {
				vFeatures.push_back((m_turnColor == COLOR_BLACK ? 1.0f : 0.0f));
			} else if (channel == 17) {
				vFeatures.push_back((m_turnColor == COLOR_WHITE ? 1.0f : 0.0f));
			}
		}
	}
	return vFeatures;
}

HashKey GoGame::getTTHashKey() const {
	HashKey ttHashkey = m_hashKey;
	for (int pos = 0; pos < getBoardSize() * getBoardSize(); ++pos) {
		GoMove move(m_turnColor, pos);
		if (isLegalMove(move)) { continue; }

		ttHashkey ^= m_vKoHash[pos];
	}

	if (m_vMoves.size() >= 2) {
		if (m_vMoves[m_vMoves.size() - 1].isPass(getBoardSize()) && m_vMoves[m_vMoves.size() - 2].isPass(getBoardSize())) {
			ttHashkey ^= m_twoPassHashKey;
		}
	}

	return ttHashkey;
}

void GoGame::_initialize() {
	m_vGrids.clear();
	m_vBlocks.clear();
	for (int pos = 0; pos < getBoardSize() * getBoardSize(); ++pos) {
		m_vGrids.push_back(GoGrid(pos, getBoardSize()));
		m_vBlocks.push_back(GoBlock(pos));
	}
	// load force moves
	string move;
#ifdef GO
	istringstream moveStream(GameConfigure::GO_EXTRA_FORCE_MOVE);
	while (std::getline(moveStream, move, ',')) {
		int move_num = stoi(move.substr(0, move.find(":")));
		m_mForceMoves[move_num] = move.substr(move.find(":") + 1);
	}
#endif
}

void GoGame::_setColor(const GoMove &move) {
	const int pos = move.getPosition();
	const Color c = move.getColor();
	m_vGrids[pos].setColor(c);
	if (c == COLOR_BLACK) {
		m_stoneBitBoard.first.setBitOn(pos);
	} else if (c == COLOR_WHITE) {
		m_stoneBitBoard.second.setBitOn(pos);
	} else {
		m_stoneBitBoard.first.setBitOff(pos);
		m_stoneBitBoard.second.setBitOff(pos);
		m_vGrids[pos].setBlock(nullptr);
	}
}

GoBlock *GoGame::_newBlock(Color c) {
	const int index = m_bmBlockUsage.bitScanForward();
	GoBlock *b = &m_vBlocks[index];
	b->setColor(c);

	return b;
}

void GoGame::_removeBlock(GoBlock *b) {
	m_bmBlockUsage.setBitOn(b->getIndex());
	b->clear();
}

void GoGame::_removeBlockAndGrids(GoBlock *b) {
	int pos;
	GoBitBoard bmGrid = b->getGrids();
	while ((pos = bmGrid.bitScanForward()) != -1) {
		_setColor(GoMove(COLOR_NONE, pos));
		const vector<int> &vNbr = m_vGrids[pos].getNeighbors();
		for (unsigned int i = 0; i < vNbr.size(); ++i) {
			GoGrid &nbrGrid = m_vGrids[vNbr[i]];
			if (nbrGrid.getColor() != AgainstColor(b->getColor())) {
				continue;
			}
			nbrGrid.getBlock()->getLiberties().setBitOn(pos);
		}
	}
	_removeBlock(b);
}

void GoGame::_addGridAndLibertiesToBlock(GoMove m, GoBlock *b, const GoBitBoard &bmNbrLiberties) {
	const int pos = m.getPosition();
	b->addGrid(pos);
	m_vGrids[pos].setBlock(b);
	b->addLiberties(bmNbrLiberties);
	if (m.getColor() != COLOR_NONE) {
		b->addHashKey(m_vGridHash[m.getPosition()][m.getColor() - 1]);
	}
}

void GoGame::_addGridsAndLibertiesToBlock(GoBitBoard &bmGrids, GoBlock *b, const GoBitBoard &bmNbrLiberties) {
	b->addGrids(bmGrids);
	b->addLiberties(bmNbrLiberties);
	GoBitBoard bmGrid = bmGrids;
	int pos;
	while ((pos = bmGrid.bitScanForward()) != -1) {
		m_vGrids[pos].setBlock(b);
		_setColor(GoMove(b->getColor(), pos));
		b->addHashKey(m_vGridHash[pos][b->getColor() - 1]);
	}
}

GoBlock *GoGame::_combineBlocks(GoBlock *b1, GoBlock *b2) {
	if (b1->getGrids().bitCount() < b2->getGrids().bitCount()) {
		return _combineBlocks(b2, b1);
	}

	b1->combine(b2);
	int pos;
	GoBitBoard bmGrid = b2->getGrids();
	while ((pos = bmGrid.bitScanForward()) != -1) {
		m_vGrids[pos].setBlock(b1);
	}
	_removeBlock(b2);

	return b1;
}


GoBitBoard GoGame::getStoneBitBoardAfterPlay(const GoMove &move) const
{
	assert(m_vGrids[move.getPosition()].getColor() == COLOR_NONE);
	assert(!move.isPass(getBoardSize()));

	Color myColor = move.getColor();
	Color oppColor = AgainstColor(myColor);
	const GoGrid& grid = m_vGrids[move.getPosition()];
	GoBitBoard bmStoneAfterPlay;
	const vector<int> &vNbr = m_vGrids[move.getPosition()].getNeighbors();
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoGrid& nbrGrid = m_vGrids[vNbr[i]];
		if (nbrGrid.getColor() == myColor) {
			bmStoneAfterPlay |= nbrGrid.getBlock()->getGrids();
		}
	}
	bmStoneAfterPlay.setBitOn(move.getPosition());

	return bmStoneAfterPlay;
}


GoBitBoard GoGame::getLibertyBitBoardAfterPlay(const GoMove &move) const
{
	assert(m_vGrids[move.getPosition()].getColor() == COLOR_NONE);
	assert(!move.isPass(getBoardSize()));

	Color myColor = move.getColor();
	Color oppColor = AgainstColor(myColor);
	const GoGrid& grid = m_vGrids[move.getPosition()];
	GoBitBoard bmExclude = (m_stoneBitBoard.first | m_stoneBitBoard.second);
	GoBitBoard bmNewLib;
	bmNewLib.setBitOn(move.getPosition());
	const vector<int> &vNbr = m_vGrids[move.getPosition()].getNeighbors();
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		bmNewLib.setBitOn(m_vGrids[vNbr[i]].getPosition());
	}

	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoGrid& nbrGrid = m_vGrids[vNbr[i]];
		if (nbrGrid.getColor() == myColor) {
			bmNewLib |= nbrGrid.getBlock()->getStonenNbrMap();
		}
		else if (nbrGrid.getColor() == oppColor) {
			if (nbrGrid.getBlock()->getLiberties() == 1) {
				bmExclude -= nbrGrid.getBlock()->getGrids();
			}
		}
	}

	bmNewLib -= bmExclude;
	bmNewLib.setBitOff(move.getPosition());

	return bmNewLib;
}


void GoGame::getMoveInfluence(const GoMove &move, pair<GoBitBoard, GoBitBoard>& influence) const
{
	Color ownColor = move.getColor();
	Color oppColor = AgainstColor(ownColor);

	GoBitBoard bmAllInfluence;
	GoBitBoard bmOwnBlock;
	if (isCaptureMove(move)) {
		const GoGrid& grid = m_vGrids[move.getPosition()];
		const vector<int> &vNbr = m_vGrids[move.getPosition()].getNeighbors();
		GoBitBoard bmDeadStone;
		for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
			const GoGrid& nbrGrid = m_vGrids[vNbr[i]];
			if (nbrGrid.getColor() != oppColor) { continue; }

			const GoBlock* nbrBlock = nbrGrid.getBlock();
			if (nbrBlock->getNumLiberty() == 1) { bmDeadStone |= nbrBlock->getGrids();}
		}

		GoBitBoard bmNbrOwnBlocks = bmDeadStone.dilation() & getStoneBitBoard(ownColor);
		int pos = 0;
		while ((pos = bmNbrOwnBlocks.bitScanForward()) != -1) {
			const GoGrid& grid = m_vGrids[pos];
			const GoBlock* ownBlock = grid.getBlock();
			bmNbrOwnBlocks -= ownBlock->getGrids();
			bmAllInfluence |= ownBlock->getGrids();
			bmOwnBlock |= ownBlock->getGrids();
		}
		bmAllInfluence |= bmDeadStone;
	}
	else {
		if (!move.isPass(getBoardSize())) {
			GoBitBoard bmStoneAfterPlay = getStoneBitBoardAfterPlay(move);
			bmAllInfluence |= bmStoneAfterPlay;
			bmOwnBlock |= bmStoneAfterPlay;
		}
	}

	influence.first = bmAllInfluence;
	influence.second = bmOwnBlock;

	return;
}

bool GoGame::isEatKoMove(const GoMove &move) const
{
	if (move.isPass(getBoardSize())) { return false; }

	const GoGrid& grid = m_vGrids[move.getPosition()];
	Color oppColor = AgainstColor(move.getColor());
	//if (!grid.getPattern().getFalseEye(oppColor)) { return false; }
	const vector<int> &vNbr = grid.getNeighbors();
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoBlock* nbrBlock = m_vGrids[vNbr[i]].getBlock();
		if (!nbrBlock) { return false; }
		if (nbrBlock->getColor() != oppColor) { return false; }
	}
	if (isOwnTrueEye(move)) { return false; }

	bool bIsKoPlay = false;
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoBlock* nbrBlock = m_vGrids[vNbr[i]].getBlock();

		assert(nbrBlock->getColor() == oppColor);

		if (nbrBlock->getNumLiberty() > 1) { continue; }
		if (nbrBlock->getNumStone() > 1) { return false; }

		// lib==1 && num stone==1
		if (bIsKoPlay) { return false; }
		else { bIsKoPlay = true; }
	}

	return bIsKoPlay;
}

bool GoGame::isCaptureMove(const GoMove &move) const
{
	if (move.isPass(getBoardSize())) { return false; }
	if (getNumLibertyAfterPlay(move) == 0) { return false; }

	Color ownColor = move.getColor();
	Color oppColor = AgainstColor(move.getColor());

	const GoGrid& grid = m_vGrids[move.getPosition()];
	const vector<int> &vNbr = m_vGrids[move.getPosition()].getNeighbors();
	for (int i = 0; i < static_cast<int>(vNbr.size()); ++i) {
		const GoBlock* nbrBlock = m_vGrids[vNbr[i]].getBlock();
		if (nbrBlock == NULL) continue;
		if (nbrBlock->getColor() == ownColor) continue;
		if (nbrBlock->getNumLiberty() == 1) { return true; }
	}

	return false;
}

void GoGame::setFullBoardClosedArea(Color findColor) {
	uint pos, numStone;
	GoBitBoard bmFindCAStone = (findColor == COLOR_BLACK) ? ~m_stoneBitBoard.first - GoBitBoard::getPassBitBoard()
														  : ~m_stoneBitBoard.second - GoBitBoard::getPassBitBoard();

	//find full board CA
	while ((pos = bmFindCAStone.bitScanForward()) != -1) {
		bmFindCAStone.setBitOn(pos);
		GoBitBoard bmStone = bmFindCAStone.floodfill(pos);
		bmFindCAStone -= bmStone;
		numStone = bmStone.bitCount();

		assert(("stone num == 0", numStone != 0));

		if (numStone > MAX_UCT_CLOSEDAREA_SIZE) {
			continue;
		}

		GoClosedArea *closedArea = m_closedAreas.NewOne();
		setClosedAreaAttribute(closedArea, findColor, bmStone, numStone);
	}
}

void GoGame::setClosedAreaAttribute(GoClosedArea *closedArea, Color caColor, GoBitBoard &bmStone, uint numStone) {
	// set a new closed area attribute
	//	1. set closed area type
	//	2. link all grid to closed area
	//	3. link neighbor block to closed area & add block to closed area
	closedArea->init();
	closedArea->setColor(caColor);
	closedArea->setStoneMap(bmStone);
	closedArea->setNumStone(numStone);

	// set closed area type
	closedArea->calculateCAType();

	// link all grid to closed area
	uint pos;
	while ((pos = bmStone.bitScanForward()) != -1) {
		GoGrid &grid = m_vGrids[pos];

		assert(("", grid.getColor() != caColor));
		assert(("grid have been assigned", grid.getClosedArea(caColor) == NULL));

		grid.setClosedArea(closedArea, caColor);
	}

	// link neighbor block to closed area
	GoBitBoard bmSurroundStone = closedArea->getSurroundBitBoard();
	while ((pos = bmSurroundStone.bitScanForward()) != -1) {
		GoBlock *nbrBlock = m_vGrids[pos].getBlock();

		assert(("", nbrBlock));
		assert(("", nbrBlock->getColor() == caColor));
		assert(("", !nbrBlock->hasClosedArea(closedArea)));

		nbrBlock->addClosedArea(closedArea);
		closedArea->addBlockID(nbrBlock->getIndex());
		bmSurroundStone -= nbrBlock->getGrids();
	}
}

void GoGame::setFullBoardBensonLife() {
	Benson benson;
	findBensonSet(benson);
	findBensonLife(benson);
	setBensonLife(benson);
}

bool GoGame::isCAHealthy(const GoClosedArea *closedArea) const {
	if (closedArea->getNumStone() == 1) {
		return true;
	}
	if (closedArea->getNumBlock() == 1 && closedArea->getNumStone() < 3) {
		return true;
	}

	for (uint iNum = 0; iNum < closedArea->getNumBlock(); iNum++) {
		GoBitBoard bmCaNbrBlock = m_vBlocks[closedArea->getBlockID(iNum)].getStonenNbrMap();
		GoBitBoard bmBoard = m_stoneBitBoard.first | m_stoneBitBoard.second;
		if (!(closedArea->getStoneMap() & ~bmCaNbrBlock & ~bmBoard).empty()) {
			return false;
		}
	}

	return true;
}

void GoGame::findBensonSet(Benson &benson) const {
	// find closed area
	for (uint iIndex = 0; iIndex < m_closedAreas.getCapacity(); ++iIndex) {
		if (!m_closedAreas.isValidIdx(iIndex)) {
			continue;
		}
		const GoClosedArea *closedArea = m_closedAreas.getAt(iIndex);
		if (!isCAHealthy(closedArea)) {
			continue;
		}
		benson.m_closedAreaIDs.addFeature(closedArea->GetID());
	}

	// find block
	GoBitBoard bmBlocks = ~m_bmBlockUsage;
	int id;
	while ((id = bmBlocks.bitScanForward()) != -1) {
		const GoBlock block = m_vBlocks[id];
		uint numHealthyCA = block.getNumClosedArea();
		for (uint iNum = 0; iNum < block.getNumClosedArea(); ++iNum) {
			uint closedAreaID = block.getClosedAreaID(iNum);
			if (benson.m_closedAreaIDs.contains(closedAreaID)) {
				continue;
			}
			--numHealthyCA;
		}
		benson.m_blockIDs.addFeature(block.getIndex());
		benson.m_blockNumHealthyCA[block.getIndex()] = numHealthyCA;
	}
}

void GoGame::findBensonLife(Benson &benson) const {
	// do benson algorithm
	bool bIsOver = false;

	while (!bIsOver) {
		bIsOver = true;

		for (uint iIndex = 0; iIndex < benson.m_blockIDs.size(); ++iIndex) {
			const GoBlock block = m_vBlocks[benson.m_blockIDs[iIndex]];

			if (benson.m_blockNumHealthyCA[block.getIndex()] >= 2) {
				continue;
			}

			bIsOver = false;
			benson.m_blockIDs.removeFeature(block.getIndex());
			for (uint iNumCA = 0; iNumCA < block.getNumClosedArea(); ++iNumCA) {
				uint blockNbrCAID = block.getClosedAreaID(iNumCA);
				const GoClosedArea *blockNbrCA = m_closedAreas.getAt(blockNbrCAID);
				if (!benson.m_closedAreaIDs.contains(blockNbrCAID)) {
					continue;
				}
				benson.m_closedAreaIDs.removeFeature(blockNbrCAID);
				for (uint iNumBlock = 0; iNumBlock < blockNbrCA->getNumBlock(); iNumBlock++) {
					--benson.m_blockNumHealthyCA[blockNbrCA->getBlockID(iNumBlock)];
				}
			}
		}
	}
}

void GoGame::setBensonLife(Benson &benson) {
	// set block life
	for (uint iNum = 0; iNum < benson.m_blockIDs.size(); ++iNum) {
		GoBlock &block = m_vBlocks[benson.m_blockIDs[iNum]];
		if (block.getColor() == COLOR_BLACK) {
			m_numLifeStone.first += block.getNumStone();
			m_bmBenson.first |= block.getGrids();
		} else if (block.getColor() == COLOR_WHITE) {
			m_numLifeStone.second += block.getNumStone();
			m_bmBenson.second |= block.getGrids();
		}
	}

	// set closed area life
	for (uint iNum = 0; iNum < benson.m_closedAreaIDs.size(); ++iNum) {
		GoClosedArea *closedArea = m_closedAreas.getAt(benson.m_closedAreaIDs[iNum]);
		if (closedArea->getColor() == COLOR_BLACK) {
			m_numLifeStone.first += closedArea->getNumStone();
			m_bmBenson.first |= closedArea->getStoneMap();
		} else if (closedArea->getColor() == COLOR_WHITE) {
			m_numLifeStone.second += closedArea->getNumStone();
			m_bmBenson.second |= closedArea->getStoneMap();
		}
	}
}