
// WinUIDlg.h : header file
//

#pragma once
#include "..\CForth.Core\ForthOS.h"

// CWinUIDlg dialog
class CWinUIDlg : public CDialogEx
{
// Construction
public:
	CWinUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FORTH};

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	public:
	afx_msg void OnBnClickedOk();

	ForthOS *myOS;
	CString commands;
	long memSize;
	long retStackSize;
	CString output;
	CString stack;
	CString memDump;
	CEdit editMemDump;

	
	// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
