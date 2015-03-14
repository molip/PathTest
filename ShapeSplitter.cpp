#include "stdafx.h"
#include "ShapeSplitter.h"

#include "Shape.h"

#include "Jig/Line2.h"
#include "Jig/Util.h"

#include <cassert>
#include <map>

using namespace Jig;

ShapeSplitter::ShapeSplitter(Shape& shape) : m_shape(shape)
{
}


ShapeSplitter::~ShapeSplitter()
{
}

int ShapeSplitter::ChooseConnectVert(int i) const
{
	assert(m_shape.GetAngle(i) < 0); // Concave.

	Vec2 fromPrev = m_shape.GetVec(i - 1, i).Normalised();
	Vec2 fromNext = m_shape.GetVec(i + 1, i).Normalised();
	Vec2 normal = (fromPrev + fromNext) / 2.0;
	normal.Normalise();

	double angleToPrev = normal.GetAngle(-fromPrev);
	double angleToNext = normal.GetAngle(-fromNext);

	assert(angleToPrev >= 0 && angleToNext <= 0);

	// Get candidate verts, best angle first. 
	std::map<double, int> map;
	for (int j = i + 2; j < i + (int)m_shape.size() - 1; ++j) // Non-adjacent to i.
	{
		Vec2 iToJ = m_shape.GetVec(i, j).Normalised();
		const double angle = normal.GetAngle(iToJ);
		if (angle < angleToPrev && angle > angleToNext)
			map.insert(std::make_pair(std::fabs(angle), m_shape.ClampVertIndex(j)));
	}

	for (auto& pair : map)
	{
		const int vert = pair.second;
		Line2 line = Line2::MakeFinite(m_shape.GetVertex(i), m_shape.GetVertex(vert));

		bool hit = false;

		for (int j = i + 1; j < i + (int)m_shape.size() - 1; ++j) // Non-adjacent to i.
		{
			if (m_shape.ClampVertIndex(j) != vert && m_shape.ClampVertIndex(j + 1) != vert) // Ignore edges including vert. 
			{
				Line2 edge = Line2::MakeFinite(m_shape.GetVertex(j), m_shape.GetVertex(j + 1));
				if (hit = edge.Intersect(line))
					break;
			}
		}
		if (!hit)
			return vert;
	}
	
	return -1;
}
