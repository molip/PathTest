#pragma once

#include "Shape.h"

class CChildView : public CWnd
{
public:
	CChildView();
	virtual ~CChildView();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	CPoint Snap(CPoint point) const;

	CPoint LogToDev(const CPoint& p) const;
	CPoint DevToLog(const CPoint& p) const;
	bool Colinear(const CPoint& p, const CPoint& q, const CPoint& r) const;

protected:
	void InvalidateCurrent();

	std::vector<Shape> m_shapes;
	bool m_adding;
	CPoint m_current;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnClear();
};

