
// PathTest.h : main header file for the PathTest application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CPathTestApp:
// See PathTest.cpp for the implementation of this class
//

class CPathTestApp : public CWinApp
{
public:
	CPathTestApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CPathTestApp theApp;
