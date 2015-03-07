#include "stdafx.h"
#include "Shape.h"

#include "Jig/Util.h"
#include "Jig/Vector.h"

#include <algorithm>
#include <map>

Shape::Shape() 
{
	Init();
}

Shape::Shape(const Shape& rhs) : std::vector<CPoint>(rhs), m_colour(rhs.m_colour), m_subshapes(rhs.m_subshapes)
{
}

Shape::~Shape()
{
}

void Shape::Init()
{
	m_colour = RGB(::rand() % 255, ::rand() % 255, ::rand() % 255);
	m_subshapes.clear();
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

void Shape::Convexify()
{
	m_subshapes.clear();
	Shape sub = *this;
	sub.Convexify(m_subshapes);
	m_subshapes.push_back(sub);
}

void Shape::Draw(CDC& dc) const
{
	if (empty())
		return;

	if (!m_subshapes.empty())
	{
		for (auto& shape : m_subshapes)
			shape.Draw(dc);
		return;
	}

	dc.SelectStockObject(NULL_PEN);
	dc.BeginPath();

	auto i = begin();
	dc.MoveTo(*i);
	while (++i != end())
		dc.LineTo(*i);

	dc.EndPath();
	dc.SelectStockObject(BLACK_PEN);

	CBrush brush(m_colour);
	dc.SelectObject(&brush);
	dc.StrokeAndFillPath();

	dc.SelectStockObject(BLACK_BRUSH);

	for (auto& p : *this)
	{
		CRect r(p, p);
		r.InflateRect(2, 2, 3, 3);
		dc.Rectangle(r);
	}
}

void Shape::Convexify(std::vector<Shape>& newShapes)
{
	if (size() < 4)
		return;

	MakeCW();

	for (int i = 0; i < (int)size(); ++i)
	{
		if (GetAngle(i) >= 0) // Convex.
			continue;

		Jig::Vec2 fromPrev = GetVec(i - 1, i).Normalised();
		Jig::Vec2 fromNext = GetVec(i + 1, i).Normalised();
		Jig::Vec2 normal = (fromPrev + fromNext) / 2.0f;
		normal.Normalise();
		
		float angleToPrev = normal.GetAngle(-fromPrev);
		float angleToNext = normal.GetAngle(-fromNext);

		assert(angleToPrev >= 0 && angleToNext <= 0);

		std::map<float, int> map;
		for (int j = i + 2; j < i + (int)size() - 1; ++j) // Non-adjacent to i.
		{
			Jig::Vec2 iToJ = GetVec(i, j).Normalised();
			const float angle = normal.GetAngle(iToJ);
			if (angle < angleToPrev && angle > angleToNext)
				map.insert(std::make_pair(angle, ClampVertIndex(j)));
		}

		float minAngle = 10.0f; // > 2 * pi.
		int minAngleVert = -1;
		
		auto Try = [&](const std::pair<float, int>& pair) // Returns true to stop.
		{
			if (std::fabs(pair.first) < minAngle)
			{
				minAngle = std::fabs(pair.first);
				minAngleVert = pair.second;
			}
			return GetAngle(pair.second) < 0; // Stop if we've hit a concave vertex.
		};
		
		for (auto& pair : map)
		{
			if (Try(pair))
			{
				for (auto& pair : Util::Reverse(map)) // Try the other way, might get closer to normal.
					if (Try(pair))
						break;
				break;
			}
		}

		assert(minAngleVert >= 0);

		int split0 = std::min(i, minAngleVert);
		int split1 = std::max(i, minAngleVert);

		Shape newShape = *this;
		std::srand(m_colour);
		newShape.Init();

		erase(begin() + split0 + 1, begin() + split1);
		
		newShape.erase(newShape.begin() + split1 + 1, newShape.end());
		newShape.erase(newShape.begin(), newShape.begin() + split0);

		newShapes.push_back(newShape);

		Convexify(newShapes);
		newShape.Convexify(newShapes);
		return;
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