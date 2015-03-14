#include "stdafx.h"
#include "Shape.h"

#include "ShapeSplitter.h"

#include "Jig/Line2.h"
#include "Jig/Util.h"
#include "Jig/Vector.h"

#include <algorithm>
#include <map>

using namespace Jig;

Shape::Shape() 
{
	Init();
}

Shape::Shape(const Shape& rhs) : std::vector<Vec2>(rhs), m_subshapes(rhs.m_subshapes), m_isSelfIntersecting(false)
{
}

Shape::~Shape()
{
}

void Shape::Init()
{
	m_subshapes.clear();
}

Jig::Rect Shape::GetBBox() const
{
	Rect r = {};
	for (Vec2 p : *this)
	{
		r.m_p0.x = std::min(r.m_p0.x, p.x);
		r.m_p1.x = std::max(r.m_p1.x, p.x);
		r.m_p0.y = std::min(r.m_p0.y, p.y);
		r.m_p1.y = std::max(r.m_p1.y, p.y);
	}

	return r;
}

void Shape::MakeCW()
{
	double total = 0.f;
	for (int i = 0; i < (int)size(); ++i)
		total += GetAngle(i);

	if (total < 0)
		std::reverse(begin(), end());
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

		const int connectVert = ShapeSplitter(*this).ChooseConnectVert(i);

		if (connectVert < 0)
			return;

		int split0 = std::min(i, connectVert);
		int split1 = std::max(i, connectVert);

		Shape newShape = *this;
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

int Shape::AddPoint(const Vec2& point, double tolerance)
{
	int minEdge = -1;
	double minDist = 1 << 16;

	for (int i = 0; i < (int)size(); ++i)
	{
		auto edge = Jig::Line2::MakeFinite(GetVertex(i), GetVertex(i + 1));

		bool intersects = false;
		double dist = edge.PerpDistanceTo(Jig::Vec2(point.x, point.y), &intersects);
		if (intersects && tolerance > dist && minDist > dist)
			minDist = dist, minEdge = i;
	}

	if (minEdge >= 0)
		insert(begin() + minEdge + 1, point);
	
	return minEdge >= 0 ? minEdge + 1 : -1;
}

int Shape::ClampVertIndex(int vert) const
{
	return	
		vert < 0 ? vert + (int)size() : 
		vert >= (int)size() ? vert - (int)size() :
		vert;
}

const Jig::Vec2& Shape::GetVertex(int vert) const
{
	return at(ClampVertIndex(vert));
}

Jig::Vec2 Shape::GetVecTo(int vert) const
{
	return GetVec(vert - 1, vert);
}

Jig::Vec2 Shape::GetVec(int from, int to) const
{
	Vec2 p = at(ClampVertIndex(from));
	Vec2 q = at(ClampVertIndex(to));

	return q - p;
}

double Shape::GetAngle(int vert) const
{
	Jig::Vec2 v0 = GetVecTo(vert).Normalised();
	Jig::Vec2 v1 = GetVecTo(vert + 1).Normalised();

	return v0.GetAngle(v1);
}

void Shape::Update()
{
	m_isSelfIntersecting = false;
	m_subshapes.clear();

	for (int i = 0; i < (int)size(); ++i)
	{
		auto edge0 = Jig::Line2::MakeFinite(GetVertex(i), GetVertex(i + 1));
		for (int j = i + 2; j < (int)size() - (i == 0); ++j) // Don't intersect last edge with first.
		{
			auto edge1 = Jig::Line2::MakeFinite(GetVertex(j), GetVertex(j + 1));
			if (m_isSelfIntersecting = edge0.Intersect(edge1))
				break;
		}
		if (m_isSelfIntersecting)
			break;
	}

	if (!m_isSelfIntersecting)
	{
		Shape sub = *this;
		sub.Convexify(m_subshapes);
		if (!m_subshapes.empty())
			m_subshapes.push_back(sub);
	}
}
