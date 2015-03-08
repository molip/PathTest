#include "stdafx.h"
#include "ShapeSplitter.h"

#include "Shape.h"

#include "Jig/Util.h"

#include <cassert>
#include <map>

ShapeSplitter::ShapeSplitter(Shape& shape) : m_shape(shape)
{
}


ShapeSplitter::~ShapeSplitter()
{
}

int ShapeSplitter::ChooseConnectVert(int i) const
{
	assert(m_shape.GetAngle(i) < 0); // Concave.

	Jig::Vec2 fromPrev = m_shape.GetVec(i - 1, i).Normalised();
	Jig::Vec2 fromNext = m_shape.GetVec(i + 1, i).Normalised();
	Jig::Vec2 normal = (fromPrev + fromNext) / 2.0f;
	normal.Normalise();

	double angleToPrev = normal.GetAngle(-fromPrev);
	double angleToNext = normal.GetAngle(-fromNext);

	assert(angleToPrev >= 0 && angleToNext <= 0);

	std::map<double, int> map;
	for (int j = i + 2; j < i + (int)m_shape.size() - 1; ++j) // Non-adjacent to i.
	{
		Jig::Vec2 iToJ = m_shape.GetVec(i, j).Normalised();
		const double angle = normal.GetAngle(iToJ);
		if (angle < angleToPrev && angle > angleToNext)
			map.insert(std::make_pair(angle, m_shape.ClampVertIndex(j)));
	}

	double minAngle = 10.0f; // > 2 * pi.
	int minAngleVert = -1;

	auto Try = [&](const std::pair<double, int>& pair) // Returns true to stop.
	{
		if (std::fabs(pair.first) < minAngle)
		{
			minAngle = std::fabs(pair.first);
			minAngleVert = pair.second;
		}
		return m_shape.GetAngle(pair.second) < 0; // Stop if we've hit a concave vertex.
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
	return minAngleVert;
}
