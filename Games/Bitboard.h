#pragma once

#include <bitset>
#include <cassert>
#include <cstring>
#include <iostream>

template <int _BITS, int _BOARD_WIDTH>
class Bitboard {
  private:
	static Bitboard<_BITS, _BOARD_WIDTH> s_upBoundary;
	static Bitboard<_BITS, _BOARD_WIDTH> s_bottomBoundary;
	static Bitboard<_BITS, _BOARD_WIDTH> s_leftBoundary;
	static Bitboard<_BITS, _BOARD_WIDTH> s_rightBoundary;
	static Bitboard<_BITS, _BOARD_WIDTH> s_corner;
	static Bitboard<_BITS, _BOARD_WIDTH> s_edge;
	static Bitboard<_BITS, _BOARD_WIDTH> s_pass;
	static bool initialized;
	std::bitset<_BITS> m_bitmap;

  public:
	Bitboard() { reset(); }
	Bitboard(int pos) {
		reset();
		setBitOn(pos);
	} // one hot bitboard

	inline Bitboard(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) { operator=(rhs); }
	inline void reset() { m_bitmap.reset(); }
	inline void setBitOn(int position) { m_bitmap.set(position); }
	inline void setBitOff(int position) { m_bitmap.reset(position); }
	inline int bitCount() const { return m_bitmap.count(); }
	inline bool empty() const { return m_bitmap.none(); }
	inline bool BitIsOn(int position) const { return m_bitmap.test(position); }
	inline bool isSubsetOf(const Bitboard<_BITS, _BOARD_WIDTH>& rhs) const { return ((*this) - rhs).empty(); }
	inline bool hasIntersectionWith(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) { return !(*this & rhs).empty(); }
	inline Bitboard<_BITS, _BOARD_WIDTH> &operator=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) {
		m_bitmap = rhs.m_bitmap;
		return *this;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> &operator|=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) {
		m_bitmap |= rhs.m_bitmap;
		return *this;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> &operator&=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) {
		m_bitmap &= rhs.m_bitmap;
		return *this;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> &operator-=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) {
		m_bitmap &= ~rhs.m_bitmap;
		return *this;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> &operator^=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) {
		m_bitmap ^= rhs.m_bitmap;
		return *this;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator~() const {
		Bitboard bm;
		bm.m_bitmap = ~this->m_bitmap;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator-(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const {
		Bitboard bm = *this;
		bm.m_bitmap &= ~rhs.m_bitmap;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator&(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const {
		Bitboard bm = *this;
		bm.m_bitmap &= rhs.m_bitmap;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator|(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const {
		Bitboard bm = *this;
		bm.m_bitmap |= rhs.m_bitmap;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator^(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const {
		Bitboard bm = *this;
		bm.m_bitmap ^= rhs.m_bitmap;
		return bm;
	}
	inline bool operator==(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const { return m_bitmap == rhs.m_bitmap; }
	inline bool operator!=(const Bitboard<_BITS, _BOARD_WIDTH> &rhs) const { return !this->operator==(rhs); }

	// static bitboard
	inline static const Bitboard<_BITS, _BOARD_WIDTH> getEdgeBitBoard() {
		if (!initialized) initialize();
		return s_edge;
	};
	inline static const Bitboard<_BITS, _BOARD_WIDTH> getCornerBitBoard() {
		if (!initialized) initialize();
		return s_corner;
	};
	inline static const Bitboard<_BITS, _BOARD_WIDTH> getPassBitBoard() {
		if (!initialized) initialize();
		return s_pass;
	};

	inline int bitScanForward() {
		if (m_bitmap.none()) return -1;
		int pos = m_bitmap._Find_first();
		m_bitmap.reset(pos);
		return pos;
	}

	static void initialize() {
		for (int p = 0; p < _BOARD_WIDTH; ++p) {
			s_upBoundary.setBitOn(p);
			s_bottomBoundary.setBitOn(_BOARD_WIDTH * (_BOARD_WIDTH - 1) + p);
			s_leftBoundary.setBitOn(p * _BOARD_WIDTH);
			s_rightBoundary.setBitOn((p + 1) * _BOARD_WIDTH - 1);
		}
		s_corner.setBitOn(0);
		s_corner.setBitOn(_BOARD_WIDTH - 1);
		s_corner.setBitOn(_BOARD_WIDTH * (_BOARD_WIDTH - 1));
		s_corner.setBitOn(_BOARD_WIDTH * _BOARD_WIDTH - 1);
		s_edge = s_upBoundary | s_bottomBoundary | s_leftBoundary | s_rightBoundary;
		s_pass.setBitOn(_BOARD_WIDTH * _BOARD_WIDTH);
		initialized = true;
	}

	// For dilation
	inline Bitboard<_BITS, _BOARD_WIDTH> operator>>(const int shift) const {
		Bitboard bm = *this;
		bm.m_bitmap = bm.m_bitmap >> shift;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> operator<<(const int shift) const {
		Bitboard bm = *this;
		bm.m_bitmap = bm.m_bitmap << shift;
		return bm;
	}
	inline Bitboard<_BITS, _BOARD_WIDTH> dilation() const {
		if (!initialized) initialize();
		/*
		         0 0 0
		origin:  0 1 0
		         1 0 0
                  

				  0 0 0   0 1 0   0 0 0   0 0 0   0 0 0   0 1 0
		dilation: 0 1 0 | 1 0 0 | 0 0 0 | 1 0 0 | 0 0 1 = 1 1 1
				  1 0 0   0 0 0   0 1 0   0 0 0   0 1 0   1 1 0

		*/
		return (*this | ((*this - s_upBoundary) >> _BOARD_WIDTH) | ((*this - s_bottomBoundary) << _BOARD_WIDTH) | ((*this - s_leftBoundary) >> 1) | ((*this - s_rightBoundary) << 1)) - s_pass;
	}
	Bitboard<_BITS, _BOARD_WIDTH> floodfill(int pos) {
		Bitboard bmBlock(pos);
		while (bmBlock != (bmBlock.dilation() & *this)) {
			bmBlock = bmBlock.dilation() & *this;
		}
		return bmBlock;
	}
	void print() {
		std::cout << "print" << std::endl;
		for (int p = 0; p < _BITS; ++p) {
			std::cout << ' ' << m_bitmap[p];
			if (p % _BOARD_WIDTH == _BOARD_WIDTH - 1) std::cout << std::endl;
		}
		std::cout << std::endl;
	}
};

template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_upBoundary;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_bottomBoundary;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_leftBoundary;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_rightBoundary;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_corner;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_edge;
template <int _BITS, int _BOARD_WIDTH>
Bitboard<_BITS, _BOARD_WIDTH> Bitboard<_BITS, _BOARD_WIDTH>::s_pass;
template <int _BITS, int _BOARD_WIDTH>
bool Bitboard<_BITS, _BOARD_WIDTH>::initialized = false;
