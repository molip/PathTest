#pragma once

#include <vector>

class Shape;

class ShapeSplitter
{
public:
	ShapeSplitter(Shape& shape);
	~ShapeSplitter();

	int ChooseConnectVert(int i) const;

private:

	Shape& m_shape;
};

