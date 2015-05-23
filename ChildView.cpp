#include "stdafx.h"
#include "PathTest.h"
#include "ChildView.h"
#include "MainFrm.h"
#include "MemoryDC.h"

#include "Jig/PathFinder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CChildView::CChildView() : m_adding(false), m_current{}, m_dragging(false), m_dragShape(-1), m_dragPoint(-1), m_optimise(false), m_start{}, m_end{}, m_status{}
{
}

CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_CLEAR, OnClear)
	ON_COMMAND(ID_OPTIMISE, OnOptimise)
	ON_COMMAND(ID_START, OnStart)
	ON_COMMAND(ID_END, OnEnd)
	ON_UPDATE_COMMAND_UI(ID_OPTIMISE, OnUpdateOptimise)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
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
	MemoryDC dc(*this);

	dc.FillSolidRect(dc.GetRect(), 0xffffff);

	for (auto& face : m_mesh.GetFaces())
		DrawShape(face->GetPolygon(), dc, false);

	CRect rStart(m_start, CSize(0, 0));
	CRect rEnd(m_end, CSize(0, 0));

	if (!m_path.empty())
	{
		CPen pen(PS_SOLID, 3, 0xff0000);
		dc.SelectObject(&pen);
		
		dc.MoveTo(Convert(m_path.front()));
		for (auto& p : m_path)
			dc.LineTo(Convert(p));
		
		dc.SelectStockObject(BLACK_PEN);
	}

	rStart.InflateRect(3, 3);
	rEnd.InflateRect(3, 3);
	dc.FillSolidRect(rStart, 0x0000ff00);
	dc.FillSolidRect(rEnd, 0x000000ff);

	if (m_adding)
	{
		dc.SelectStockObject(BLACK_PEN);
		dc.SelectStockObject(BLACK_BRUSH);

		auto i = m_shapes.back().begin();
		dc.MoveTo(Convert(*i));
		while (++i != m_shapes.back().end())
			dc.LineTo(Convert(*i));

		dc.LineTo(LogToDev(m_current));

		for (auto& p : m_shapes.back())
		{
			CPoint dev = LogToDev(Convert(p));
			CRect r(dev, dev);
			r.InflateRect(2, 2, 3, 3);
			dc.Rectangle(r);
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
		if (HitPoint(point, Convert(start)))
			return Convert(start);
	}
	return point;
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	for (auto& shape : m_shapes)
		for (size_t j = 0; j < shape.size(); ++j)
			if (HitPoint(point, Convert(shape[j])))
			{
				shape.erase(shape.begin() + j);
				UpdateShapes();
				return;
			}

	for (size_t i = 0; i < m_shapes.size(); ++i)
	{
		auto& shape = m_shapes[i];
		int vert = shape.AddPoint(Convert(point), 10);
		if (vert >= 0)
		{
			UpdateShapes();
			m_dragging = true, m_dragShape = (int)i, m_dragPoint = vert;
			break;
		}
	}
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (nFlags & MK_CONTROL)
	{
		point.x = point.x / 20 * 20;
		point.y = point.y / 20 * 20;
	}

	if (m_adding)
	{
		if (m_current == Convert(m_shapes.back().front()))
		{
			m_adding = false;
			UpdateShapes();
			return;
		}
	}
	else
	{
		for (size_t i = 0; i < m_shapes.size(); ++i)
			for (size_t j = 0; j < m_shapes[i].size(); ++j)
				if (HitPoint(point, Convert(m_shapes[i][j])))
				{
					m_dragging = true, m_dragShape = (int)i, m_dragPoint = (int)j;
					return;
				}

		m_shapes.emplace_back();
		m_adding = true;
	}
	m_shapes.back().push_back(Convert(point));
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_dragging = false;
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_dragging = false;
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_CONTROL)
	{
		point.x = point.x / 20 * 20;
		point.y = point.y / 20 * 20;
	}

	m_status.inPoly = false;

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
		shape[m_dragPoint] = Convert(point);
		UpdateShapes();
	}
	else
		for (auto& poly : m_shapes)
			if (m_status.inPoly = poly.Contains(Convert(point)))
				break;

	m_status.mousePos = point;

	UpdateStatus();
}

void CChildView::OnClear()
{
	m_shapes.clear();

	CRect r;
	GetClientRect(r);

	const int d = 100;
	
	r.right -= r.right % d;
	r.bottom -= r.bottom % d;

	m_rootShape.clear();
	for (int x = 0; x < r.right; x += d)
		m_rootShape.push_back(Jig::Vec2(x, 0));

	for (int y = 0; y < r.bottom; y += d)
		m_rootShape.push_back(Jig::Vec2(r.right, y));

	for (int x = r.right; x > 0; x -= d)
		m_rootShape.push_back(Jig::Vec2(x, r.bottom));

	for (int y = r.bottom; y > 0; y -= d)
		m_rootShape.push_back(Jig::Vec2(0, y));

	UpdateShapes();
}

void CChildView::OnOptimise()
{
	m_optimise = !m_optimise;
	UpdateShapes();
}

void CChildView::OnUpdateOptimise(CCmdUI* p)
{
	p->SetCheck(m_optimise);
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
		CRect r(m_current, Convert(m_shapes.back().back()));
		r.NormalizeRect();
		r.InflateRect(10, 10);
		InvalidateRect(r);
	}
}

void CChildView::InvalidateShape(const Jig::Polygon& shape)
{
	CRect r = Convert(shape.GetBBox());
	r.InflateRect(10, 10);
	InvalidateRect(r);
}

CRect CChildView::Convert(const Jig::Rect& r) const
{
	return CRect((long)std::floor(r.m_p0.x), (long)std::floor(r.m_p0.y), (long)std::floor(r.m_p1.x), (long)std::floor(r.m_p1.y));
}

CPoint CChildView::Convert(const Jig::Vec2& p) const
{
	return CPoint((long)std::floor(p.x), (long)std::floor(p.y));
}

Jig::Vec2 CChildView::Convert(const CPoint& p) const
{
	return Jig::Vec2(p.x, p.y);
}

void CChildView::DrawShape(const Jig::Polygon& shape, CDC& dc, bool special) const
{
	if (shape.empty())
		return;

	dc.SelectStockObject(NULL_PEN);
	dc.BeginPath();

	auto i = shape.begin();
	dc.MoveTo(Convert(*i));
	while (++i != shape.end())
		dc.LineTo(Convert(*i));

	dc.EndPath();
	dc.SelectStockObject(BLACK_PEN);
	dc.SelectStockObject(special ? DKGRAY_BRUSH : GRAY_BRUSH);

	dc.StrokeAndFillPath();

	dc.SelectStockObject(BLACK_BRUSH);

	for (auto& p : shape)
	{
		CRect r(Convert(p), Convert(p));
		r.InflateRect(2, 2, 3, 3);
		dc.Rectangle(r);
	}
}

void CChildView::UpdateShapes()
{
	m_mesh.Init(m_rootShape);

	for (auto& shape : m_shapes)
	{
		shape.Update();
		if (!shape.IsSelfIntersecting())
			m_mesh.AddHole(shape);
	}

	if (m_optimise)
		m_mesh.DissolveRedundantEdges();

	UpdatePath();
	Invalidate();
}

void CChildView::UpdatePath()
{
	m_path = Jig::PathFinder(m_mesh, Convert(m_start), Convert(m_end)).Find(&m_status.pathLength);
	
	Invalidate();
	UpdateStatus();
}

void CChildView::UpdateStatus()
{
	static_cast<CMainFrame*>(::AfxGetMainWnd())->SetStatus(m_status);
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (cx && cy && m_shapes.empty())
		OnClear();
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	return true;
}

void CChildView::OnStart()
{
	::GetCursorPos(&m_start);
	ScreenToClient(&m_start);
	UpdatePath();
}

void CChildView::OnEnd()
{
	::GetCursorPos(&m_end);
	ScreenToClient(&m_end);
	UpdatePath();
}

