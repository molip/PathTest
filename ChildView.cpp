
// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include "PathTest.h"
#include "ChildView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

namespace
{
	int SquareSize = 32;
}

CChildView::CChildView() : m_adding(false), m_current{}, m_dragging(false), m_dragShape(-1), m_dragPoint(-1)
{
}

CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_CLEAR, OnClear)
END_MESSAGE_MAP()

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect r;
	GetClientRect(r);

	for (auto& shape : m_shapes)
		if (!shape.empty())
		{
			if (!m_adding || &shape != &m_shapes.back())
			{
				std::srand(0); // Reset colours.
				shape.Draw(dc);
			}
			else
			{
				dc.SelectStockObject(BLACK_PEN);
				dc.SelectStockObject(BLACK_BRUSH);

				auto i = shape.begin();
				dc.MoveTo(*i);
				while (++i != shape.end())
					dc.LineTo(*i);

				dc.LineTo(LogToDev(m_current));

				for (auto& p : shape)
				{
					CPoint dev = LogToDev(p);
					CRect r(dev, dev);
					r.InflateRect(2, 2, 3, 3);
					dc.Rectangle(r);
				}
			}
		}
}

bool CChildView::HitPoint(CPoint p, CPoint q) const
{
	return ::abs(p.x - q.x) < 10 && ::abs(p.y - q.y) < 10;
}

CPoint CChildView::Snap(CPoint point) const
{
	if (!m_shapes.back().empty())
	{
		auto& start = m_shapes.back().front();
		if (HitPoint(point, start))
			return start;
	}
	return point;
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_adding)
	{
		if (m_current == m_shapes.back().front())
		{
			m_adding = false;
			InvalidateShape(m_shapes.back());
			m_shapes.back().Convexify();
			return;
		}
	}
	else
	{
		for (size_t i = 0; i < m_shapes.size(); ++i)
			for (size_t j = 0; j < m_shapes[i].size(); ++j)
				if (HitPoint(point, m_shapes[i][j]))
				{
					m_dragging = true, m_dragShape = (int)i, m_dragPoint = (int)j;
					return;
				}

		m_shapes.emplace_back();
		m_adding = true;
	}
	m_shapes.back().push_back(point);
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_dragging = false;
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_adding)
	{
		InvalidateCurrent();
		m_current = Snap(point);
		InvalidateCurrent();
	}
	else if (m_dragging)
	{
		auto& shape = m_shapes[m_dragShape];
		InvalidateShape(shape);
		shape[m_dragPoint] = point;
		shape.Convexify();
		InvalidateShape(shape);
	}
}

void CChildView::OnClear()
{
	m_shapes.clear();
	Invalidate();
}

CPoint CChildView::LogToDev(const CPoint& p) const
{
	return p;
	//return CPoint(SquareSize * p.x, SquareSize * p.y);
}

CPoint CChildView::DevToLog(const CPoint& p) const
{
	return p;
	//return CPoint(int(0.5 + p.x / (double)SquareSize), int(0.5 + p.y / (double)SquareSize));
}

bool CChildView::Colinear(const CPoint& p, const CPoint& q, const CPoint& r) const
{
	return p.x == q.x && q.x == r.x || p.y == q.y && q.y == r.y;
}

void CChildView::InvalidateCurrent()
{
	ASSERT(m_adding);
	if (!m_shapes.back().empty())
	{
		CRect r(m_current, m_shapes.back().back());
		r.NormalizeRect();
		r.InflateRect(10, 10);
		InvalidateRect(r);
	}
}

void CChildView::InvalidateShape(const Shape& shape)
{
	CRect r = shape.GetBBox();
	r.InflateRect(10, 10);
	InvalidateRect(r);
}
