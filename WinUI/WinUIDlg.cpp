
// WinUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WinUI.h"
#include "WinUIDlg.h"
#include "afxdialogex.h"
#include "..\CForth.Core\ForthOS.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWinUIDlg dialog



CWinUIDlg::CWinUIDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWinUIDlg::IDD, pParent)
	, memSize(8192)
	, retStackSize(256)
	, commands(_T("include ..\\4th\\ForthOS.4th"))
	, output(_T(""))
	, stack(_T(""))
	, memDump(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	myOS = NULL;
}

void CWinUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_COMMANDS, commands);
	DDX_Text(pDX, IDC_MEMORY, memSize);
	DDX_Text(pDX, IDC_RETSTACK, retStackSize);
	DDX_Text(pDX, IDC_OUTPUT, output);
	DDX_Text(pDX, IDC_STACK, stack);
	DDX_Text(pDX, IDC_MEMDUMP, memDump);
	DDX_Control(pDX, IDC_MEMDUMP, editMemDump);
}

BEGIN_MESSAGE_MAP(CWinUIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CWinUIDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CWinUIDlg message handlers

BOOL CWinUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	CFont *pFont = new CFont();
	pFont->CreatePointFont(80, _T("Courier New"));
	editMemDump.SetFont(pFont);
	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWinUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWinUIDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	if (commands.CompareNoCase(_T("bye")) == 0)
	{
		EndDialog(IDOK);
		return;
	}

	if (commands.CompareNoCase(_T("reset")) == 0)
	{
		delete myOS;
		myOS = NULL;
		commands.Empty();
	}

	if (myOS == NULL)
	{
		myOS = new ForthOS(this->memSize);
		myOS->BootStrap();
	}

	myOS->ParseInput(commands);

	output = myOS->output;
	output.Append(_T(" ok"));
	myOS->output.Empty();
	myOS->Dump(memDump);
	myOS->DumpStack(stack);
	UpdateData(FALSE);
}
