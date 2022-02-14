#pragma once

#include <iostream>
#include <string>
#include <cassert>

// just for represent X and Y and do some symmetric
enum SymmetryType {
	SYM_NORMAL, SYM_ROTATE_90, SYM_ROTATE_180, SYM_ROTATE_270,
	SYM_HORIZONTAL_REFLECTION, SYM_HORIZONTAL_REFLECTION_ROTATE_90,
	SYM_HORIZONTAL_REFLECTION_ROTATE_180, SYM_HORIZONTAL_REFLECTION_ROTATE_270,
	SYMMETRY_SIZE
};

const std::string SYMMETRY_TYPE_STRING[SYMMETRY_SIZE] = {
	"SYM_NORMAL", "SYM_ROTATE_90", "SYM_ROTATE_180", "SYM_ROTATE_270",
	"SYM_HORIZONTAL_REFLECTION", "SYM_HORIZONTAL_REFLECTION_ROTATE_90",
	"SYM_HORIZONTAL_REFLECTION_ROTATE_180", "SYM_HORIZONTAL_REFLECTION_ROTATE_270"
};
inline std::string getSymmetryTypeString(SymmetryType type) { return SYMMETRY_TYPE_STRING[type]; }
inline SymmetryType getSymmetryType(std::string sType) {
	for (int i = 0; i<SYMMETRY_SIZE; i++) { if (sType == SYMMETRY_TYPE_STRING[i]) { return static_cast<SymmetryType>(i); } }
	return SYMMETRY_SIZE;
}

const SymmetryType ReverseSymmetricType[SYMMETRY_SIZE] = {
	SYM_NORMAL, SYM_ROTATE_270, SYM_ROTATE_180, SYM_ROTATE_90,
	SYM_HORIZONTAL_REFLECTION, SYM_HORIZONTAL_REFLECTION_ROTATE_90,
	SYM_HORIZONTAL_REFLECTION_ROTATE_180, SYM_HORIZONTAL_REFLECTION_ROTATE_270
};

class Point {
private:
	int m_x;
	int m_y;
public:
	Point() {}
	Point(int x, int y) { m_x = x; m_y = y; }
	int getX() const { return m_x; }
	int getY() const { return m_y; }
	inline int getRadiusDistance(const Point& rhs) const {
		const int x = (m_x>rhs.getX()) ? (m_x - rhs.getX()) : (rhs.getX() - m_x);
		const int y = (m_y>rhs.getY()) ? (m_y - rhs.getY()) : (rhs.getY() - m_y);
		return (x + y + (x>y ? x : y));
	}
	inline Point getSymmetryOf(SymmetryType type) const {
		/*
		symmetric radius pattern:			( changeXY   x*(-1)   y*(-1) )
		0 NORMAL							:
		1 ROTATE_90							: changeXY            y*(-1)
		2 ROTATE_180						:            x*(-1)   y*(-1)
		3 ROTATE_270						: changeXY   x*(-1)
		4 HORIZONTAL_REFLECTION				:            x*(-1)
		5 HORIZONTAL_REFLECTION_ROTATE_90	: changeXY
		6 HORIZONTAL_REFLECTION_ROTATE_180	:                     y*(-1)
		7 HORIZONTAL_REFLECTION_ROTATE_270	: changeXY   x*(-1)   y*(-1)
		*/
		Point symmetryPoint(m_x, m_y);
		switch (type) {
		case SYM_NORMAL:		break;
		case SYM_ROTATE_90:		symmetryPoint.changeXY();	symmetryPoint.minusY();	break;
		case SYM_ROTATE_180:	symmetryPoint.minusX();		symmetryPoint.minusY();	break;
		case SYM_ROTATE_270:	symmetryPoint.changeXY();	symmetryPoint.minusX();	break;
		case SYM_HORIZONTAL_REFLECTION:				symmetryPoint.minusX();		break;
		case SYM_HORIZONTAL_REFLECTION_ROTATE_90:	symmetryPoint.changeXY();	break;
		case SYM_HORIZONTAL_REFLECTION_ROTATE_180:	symmetryPoint.minusY();		break;
		case SYM_HORIZONTAL_REFLECTION_ROTATE_270:	symmetryPoint.changeXY();	symmetryPoint.minusX();	symmetryPoint.minusY();	break;
		default:
			// should not be here
			assert(false);
		}
		return symmetryPoint;
	}
	inline bool operator== (const Point& rhs) const { return (m_x == rhs.m_x && m_y == rhs.m_y) ? true : false; }
	inline bool operator!= (const Point& rhs) const { return !(*this == rhs); }
	inline bool operator< (const Point& rhs) const {
		if (m_x<rhs.m_x) { return true; } else if (m_x>rhs.m_x) { return false; }

		return (m_y<rhs.m_y);
	}
private:
	inline void minusX() { m_x = -m_x; }
	inline void minusY() { m_y = -m_y; }
	inline void changeXY()
	{
		int tmp = m_x;
		m_x = m_y;
		m_y = tmp;
	}
};

inline int getRotatePosition(int pos, int boardSize, SymmetryType type) {
	assert(("Invalid position", pos >= 0 && pos <= boardSize * boardSize));
	if(pos==boardSize*boardSize) { return pos; }
	const int center = boardSize / 2;
	int x = pos % boardSize;
	int y = pos / boardSize;
	Point point(x - center, y - center);
	Point rotatePoint = point.getSymmetryOf(type);
	return (rotatePoint.getY() + center) * boardSize + (rotatePoint.getX() + center);
}