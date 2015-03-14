#pragma once

#include "Jig/Rect.h"
#include "Jig/Vector.h"

#include <vector>

class Shape : public std::vector<Jig::Vec2>
{
	friend class ShapeSplitter;

public:
	Shape();
	Shape(const Shape& rhs);
	~Shape();

	Jig::Rect GetBBox() const;
	int AddPoint(const Jig::Vec2& point, double tolerance);

	const Jig::Vec2& GetVertex(int vert) const;

	void Update();

	bool IsSelfIntersecting() const { return m_isSelfIntersecting; }
	const std::vector<Shape>& GetSubshapes() const { return m_subshapes; }

private:
	void Convexify(std::vector<Shape>& newShapes);
	void Init();
	void MakeCW();
	double GetAngle(int vert) const;
	Jig::Vec2 GetVecTo(int vert) const;
	Jig::Vec2 GetVec(int from, int to) const;
	int ClampVertIndex(int vert) const;

	std::vector<Shape> m_subshapes;
	bool m_isSelfIntersecting;

};



