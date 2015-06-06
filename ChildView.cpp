#include "stdafx.h"
#include "PathTest.h"
#include "ChildView.h"
#include "MainFrm.h"
#include "MemoryDC.h"

#include "Jig/Geometry.h"
#include "Jig/GetVisiblePoints.h"
#include "Jig/Triangulator.h"
#include "Jig/PathFinder.h"

#include "libKernel/Debug.h"
#include "libKernel/Serial.h"

#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CChildView::CChildView() : m_adding(false), m_current{}, m_dragShape(-1), m_dragPoint(-1), m_optimise(false), m_showVisible(false), m_start{}, m_end{}, m_status{}, m_visibleFrom{}
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
	ON_COMMAND(ID_CLEARSQUARES, OnClearSquares)
	ON_COMMAND(ID_OPTIMISE, OnOptimise)
	ON_COMMAND(ID_START, OnStart)
	ON_COMMAND(ID_END, OnEnd)
	ON_UPDATE_COMMAND_UI(ID_OPTIMISE, OnUpdateOptimise)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_TEST, &CChildView::OnTest)
	ON_COMMAND(ID_SHOWVISIBLE, &CChildView::OnShowVisible)
	ON_UPDATE_COMMAND_UI(ID_SHOWVISIBLE, &CChildView::OnUpdateShowVisible)
	ON_COMMAND(ID_FILE_SAVE, &CChildView::OnFileSave)
	ON_COMMAND(ID_FILE_OPEN, &CChildView::OnFileOpen)
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

	if (m_showVisible)
	{
		CPen pen(PS_SOLID, 1, 0x00c0ff);
		dc.SelectObject(&pen);
		for (auto& p : m_visible)
		{
			dc.MoveTo(m_visibleFrom);
			dc.LineTo(Convert(*p));
		}
		dc.SelectStockObject(BLACK_PEN);
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

CRect CChildView::GetRect() const
{
	CRect r;
	GetClientRect(r);

	const int d = 100;

	r.right -= r.right % d;
	r.bottom -= r.bottom % d;

	return r;
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_showVisible)
		return;

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
			m_dragState = DragState::MovingShapePoint, m_dragShape = (int)i, m_dragPoint = vert;
			break;
		}
	}
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_showVisible)
		return;

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
		if (HitPoint(point, m_start))
		{
			m_dragState = DragState::MovingStartPoint;
			return;
		}

		if (HitPoint(point, m_end))
		{
			m_dragState = DragState::MovingEndPoint;
			return;
		}

		for (size_t i = 0; i < m_shapes.size(); ++i)
			for (size_t j = 0; j < m_shapes[i].size(); ++j)
				if (HitPoint(point, Convert(m_shapes[i][j])))
				{
					m_dragState = DragState::MovingShapePoint, m_dragShape = (int)i, m_dragPoint = (int)j;
					return;
				}

		m_shapes.emplace_back();
		m_adding = true;
	}
	m_shapes.back().push_back(Convert(point));
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_dragState = DragState::None;
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_dragState = DragState::None;
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_CONTROL)
	{
		point.x = point.x / 20 * 20;
		point.y = point.y / 20 * 20;
	}

	m_status.inPoly = false;

	if (m_showVisible)
	{
#if 1		
		InvalidateVisible();
		m_visibleFrom = point;
		m_visible = Jig::GetVisiblePoints(m_mesh, Convert(point));
		InvalidateVisible();
#else
		m_visible.clear();

		for (auto& vert : m_mesh.GetVerts())
		{
			if (HitPoint(Convert(vert), point))
			{
				m_visibleFrom = Convert(vert);
				m_visible = vert.visible;
				InvalidateVisible();
			}
		}
#endif
	}
	else if (m_adding)
	{
		InvalidateCurrent();
		m_current = Snap(point);
		InvalidateCurrent();
	}
	else if (m_dragState == DragState::MovingStartPoint)
	{
		m_start = point;
		UpdatePath();
	}
	else if (m_dragState == DragState::MovingEndPoint)
	{
		m_end = point;
		UpdatePath();
	}
	else if (m_dragState == DragState::MovingShapePoint)
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

	const CRect r = GetRect();
	const int dx = 100, dy = 100;
	//const int dx = r.right, dy = r.bottom;

	m_rootShape.clear();

	for (int x = 0; x < r.right; x += dx)
		m_rootShape.push_back(Jig::Vec2(x, 0));

	for (int y = 0; y < r.bottom; y += dy)
		m_rootShape.push_back(Jig::Vec2(r.right, y));

	for (int x = r.right; x > 0; x -= dx)
		m_rootShape.push_back(Jig::Vec2(x, r.bottom));

	for (int y = r.bottom; y > 0; y -= dy)
		m_rootShape.push_back(Jig::Vec2(0, y));

	UpdateShapes();
}

void CChildView::OnClearSquares()
{
	OnClear();

	const CRect r = GetRect();

	int s = 50;
	for (int x = s/2; x < r.right - s; x += s)
		for (int y = s / 2; y < r.bottom - s; y += s)
		{
			Jig::Polygon poly;
			poly.push_back(Jig::Vec2(x, y));
			poly.push_back(Jig::Vec2(x + s/2, y));
			poly.push_back(Jig::Vec2(x + s/2, y + s/2));
			poly.push_back(Jig::Vec2(x, y + s/2));
			m_shapes.push_back(poly);
		}

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
}

CPoint CChildView::DevToLog(const CPoint& p) const
{
	return p;
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

void CChildView::InvalidateVisible()
{
	if (m_visible.empty())
		return;

	Jig::Rect r(*m_visible.front());
	for (auto* p : m_visible)
		r.GrowTo(*p);

	r.GrowTo(Convert(m_visibleFrom));

	InvalidateRect(Convert(r));
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
	Jig::Triangulator triangulator(m_rootShape);

	for (auto& shape : m_shapes)
	{
		shape.Update();
		if (!shape.IsSelfIntersecting())
			triangulator.AddHole(shape);
	}

	m_mesh = triangulator.Go();

	if (m_optimise)
		m_mesh.DissolveRedundantEdges();

	m_mesh.UpdateVisible();

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

void CChildView::Save(Kernel::Serial::SaveNode& node) const
{
	Jig::Vec2 start = Convert(m_start), end = Convert(m_end);

	node.SaveType("start", start);
	node.SaveType("end", end);
	node.SaveClass("root", m_rootShape);
	node.SaveCntr("shapes", m_shapes, Kernel::Serial::ClassSaver());
}

void CChildView::Load(const Kernel::Serial::LoadNode& node)
{
	m_rootShape.clear();
	m_shapes.clear();

	Jig::Vec2 start, end;

	node.LoadType("start", start);
	node.LoadType("end", end);
	node.LoadClass("root", m_rootShape);
	node.LoadCntr("shapes", m_shapes, Kernel::Serial::ClassLoader());

	m_start = Convert(start);
	m_end = Convert(end);

	UpdateShapes();
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

void CChildView::OnTest()
{
	CWaitCursor wc;

	ULONGLONG start = ::GetTickCount64();

	std::wostringstream oss;

#if 1
	::srand((int)start);
	int n = 0;

	while (::GetTickCount64() < start + 1000)
	{
		const CRect r = GetRect();

		CPoint startPoint(::rand() % (r.right), ::rand() % (r.bottom));
		CPoint endPoint(::rand() % (r.right), ::rand() % (r.bottom));

		Jig::PathFinder(m_mesh, Convert(startPoint), Convert(endPoint)).Find();

		++n;
	}
	oss << n << L" paths/s\n";

#else 
	for (int i = 0; i < 1000; ++i)
		UpdateShapes();

	oss << ::GetTickCount64() - start << L" ms";
#endif

	::AfxMessageBox(oss.str().c_str());
}

void CChildView::OnShowVisible()
{
	m_showVisible = !m_showVisible;
	Invalidate();
}

void CChildView::OnUpdateShowVisible(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_showVisible);
}

void CChildView::OnFileSave()
{
	Kernel::Serial::SaveClass("test.pathtest", *this);
}

void CChildView::OnFileOpen()
{
	Kernel::Serial::LoadClass("test.pathtest", *this);
}
