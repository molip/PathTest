#pragma once

#include "Jig/Vector.h"

#include <vector>

class Shape : public std::vector<CPoint>
{
public:
	Shape();
	Shape(const Shape& rhs);
	~Shape();

	CRect GetBBox() const;
	void Convexify();

	void Draw(CDC& dc) const;

private:
	void Convexify(std::vector<Shape>& newShapes);
	void Init();
	void MakeCW();
	float GetAngle(int vert) const;
	Jig::Vec2 GetVecTo(int vert) const;
	Jig::Vec2 GetVec(int from, int to) const;
	int ClampVertIndex(int vert) const;

	COLORREF m_colour;
	std::vector<Shape> m_subshapes;

};



