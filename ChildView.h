#pragma once

#include "Jig/Polygon.h"

class CChildView : public CWnd
{
public:
	CChildView();
	virtual ~CChildView();

	struct Status
	{
		CPoint mousePos;
		bool inPoly;
		double pathLength;
	};

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	CPoint Snap(CPoint point) const;

	CRect Convert(const Jig::Rect& r) const;
	CPoint Convert(const Jig::Vec2& p) const;
	Jig::Vec2 Convert(const CPoint& p) const;

	CPoint LogToDev(const CPoint& p) const;
	CPoint DevToLog(const CPoint& p) const;
	bool Colinear(const CPoint& p, const CPoint& q, const CPoint& r) const;
	void InvalidateCurrent();
	void InvalidateShape(const Jig::Polygon& shape);
	bool HitPoint(CPoint p, CPoint q) const;
	void DrawShape(const Jig::Polygon& shape, CDC& dc, bool special) const;
	void UpdateShapes();
	void UpdatePath();
	void UpdateStatus();

	std::vector<Jig::Polygon> m_shapes;
	Jig::Polygon m_rootShape;
	Jig::EdgeMesh m_mesh;

	bool m_adding, m_dragging;
	CPoint m_current;
	bool m_optimise;

	int m_dragShape, m_dragPoint;

	CPoint m_start, m_end;
	std::vector<Jig::Vec2> m_path;

	Status m_status;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnClear();
	afx_msg void OnOptimise();
	afx_msg void OnStart();
	afx_msg void OnEnd();
	afx_msg void OnUpdateOptimise(CCmdUI* p);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

