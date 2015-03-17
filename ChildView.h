#pragma once

#include "Jig/Polygon.h"

class CChildView : public CWnd
{
public:
	CChildView();
	virtual ~CChildView();

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
	void DrawShape(const Jig::Polygon& shape, CDC& dc) const;
	void UpdateShapes();

	std::vector<Jig::Polygon> m_shapes;
	Jig::Polygon m_rootShape;
	Jig::EdgeMesh m_mesh;

	bool m_adding, m_dragging;
	CPoint m_current;
	bool m_optimise;

	int m_dragShape, m_dragPoint;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnClear();
	afx_msg void OnOptimise();
	afx_msg void OnUpdateOptimise(CCmdUI* p);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

