#include "stdafx.h"
#include "Shape.h"

#include "Jig/Vector.h"

#include <algorithm>

Shape::Shape() 
{
	InitColour();
}

Shape::Shape(const Shape& rhs) : std::vector<CPoint>(rhs), m_colour(rhs.m_colour)
{
}

Shape::~Shape()
{
}

void Shape::InitColour()
{
	m_colour = RGB(::rand() % 255, ::rand() % 255, ::rand() % 255);
}

CRect Shape::GetBBox() const
{
	CRect r = {};
	for (CPoint p : *this)
		r |= CRect(p, CSize(1, 1));

	return r;
}

void Shape::MakeCW()
{
	float total = 0.f;
	for (size_t i = 0; i < size(); ++i)
		total += GetAngle(i);

	if (total < 0)
		std::reverse(begin(), end());
}

void Shape::Convexify(std::vector<Shape>& newShapes)
{
	MakeCW();

	for (int i = 0; i < (int)size(); ++i)
	{
		if (GetAngle(i) >= 0) // Convex.
			continue;

		Jig::Vec2 v0 = GetVec(i - 1, i).Normalised();
		Jig::Vec2 v1 = GetVec(i + 1, i).Normalised();
		Jig::Vec2 normal = (v0 + v1) / 2.0f;
		normal.Normalise();

		float minAngle = 10.0f; // > 2 * pi.
		int nearestOpposite = -1;
		for (int j = i + 2; j < i + (int)size() - 1; ++j) // Non-adjacent to i.
		{
			Jig::Vec2 iToJ = GetVec(i, j).Normalised();
			float angle = normal.GetAngle(iToJ);
			if (std::fabs(angle) < minAngle)
			{
				minAngle = std::fabs(angle);
				nearestOpposite = ClampVertIndex(j);
			}
		}
	
		assert(nearestOpposite >= 0 && nearestOpposite != i);

		int split0 = std::min(i, nearestOpposite);
		int split1 = std::max(i, nearestOpposite);

		Shape newShape = *this;
		newShape.InitColour();

		erase(begin() + split0 + 1, begin() + split1);
		
		newShape.erase(newShape.begin() + split1 + 1, newShape.end());
		newShape.erase(newShape.begin(), newShape.begin() + split0);

		newShapes.push_back(newShape);

		Convexify(newShapes);
		newShape.Convexify(newShapes);
	}
}

int Shape::ClampVertIndex(int vert) const
{
	return	
		vert < 0 ? vert + (int)size() : 
		vert >= (int)size() ? vert - (int)size() :
		vert;
}

Jig::Vec2 Shape::GetVecTo(int vert) const
{
	return GetVec(vert - 1, vert);
}

Jig::Vec2 Shape::GetVec(int from, int to) const
{
	CPoint p = at(ClampVertIndex(from));
	CPoint q = at(ClampVertIndex(to));

	return Jig::Vec2(q.x - p.x, q.y - p.y);
}

float Shape::GetAngle(int vert) const
{
	Jig::Vec2 v0 = GetVecTo(vert).Normalised();
	Jig::Vec2 v1 = GetVecTo(vert + 1).Normalised();

	return v0.GetAngle(v1);
}