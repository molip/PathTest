#include "stdafx.h"
#include "Shape.h"

#include "ShapeSplitter.h"

#include "Jig/Util.h"
#include "Jig/Vector.h"

#include <algorithm>
#include <map>

Shape::Shape() 
{
	Init();
}

Shape::Shape(const Shape& rhs) : std::vector<CPoint>(rhs), m_subshapes(rhs.m_subshapes)
{
}

Shape::~Shape()
{
}

void Shape::Init()
{
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
	double total = 0.f;
	for (int i = 0; i < (int)size(); ++i)
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

	CBrush brush(RGB(std::rand() % 255, std::rand() % 255, std::rand() % 255));
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

		const int connectVert = ShapeSplitter(*this).ChooseConnectVert(i);

		assert(connectVert >= 0);

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

	return Jig::Vec2(float(q.x - p.x), float(q.y - p.y));
}

double Shape::GetAngle(int vert) const
{
	Jig::Vec2 v0 = GetVecTo(vert).Normalised();
	Jig::Vec2 v1 = GetVecTo(vert + 1).Normalised();

	return v0.GetAngle(v1);
}