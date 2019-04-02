// TextEntryDlg.cpp : implementation file
//

#include "TextEntryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define IDC_LABEL			10
#define IDC_EDITCTL			11

/////////////////////////////////////////////////////////////////////////////
// CTextEntryDlg

CTextEntryDlg::CTextEntryDlg()
{
}

CTextEntryDlg::~CTextEntryDlg()
{
}


BEGIN_MESSAGE_MAP(CTextEntryDlg, CWnd)
	//{{AFX_MSG_MAP(CTextEntryDlg)
	ON_WM_ERASEBKGND()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, OnOK)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_EN_CHANGE(IDC_EDITCTL, OnEditboxChanged)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTextEntryDlg message handlers

int CTextEntryDlg::Show(CWnd *pParent, LPCTSTR pszTitle, LPCTSTR pszPrompt, LPCTSTR pszDefault, bool bPassword)
{
	// register window class:
	CString strClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(NULL, IDC_ARROW), 
												CBrush(::GetSysColor(COLOR_BTNFACE)));

	if (!pParent)
		pParent = AfxGetApp()->GetMainWnd();
	
    if (!CreateEx(WS_EX_DLGMODALFRAME, strClass, pszTitle, WS_SYSMENU | WS_POPUP | 
						WS_BORDER | WS_CAPTION, 0,0, 348,100 + GetSystemMetrics(SM_CYCAPTION), 
						pParent ? pParent->GetSafeHwnd() : NULL, NULL))
	{
		return 0;
	}

	CenterWindow();

	if (!m_ctlLabel.Create(pszPrompt, WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFT, 
			CRect(6,6,340,35), this))
	{
		return 0;
	}
	
	if (!m_ctlOK.Create(_T("OK"), WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON,
			CRect(188, 62, 258, 85), this, IDOK))
	{
		return 0;
	}

	if (!m_ctlCancel.Create(_T("Cancel"), WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_PUSHBUTTON,
			CRect(265, 62, 335, 85), this, IDCANCEL))
	{
		return 0;
	}

	DWORD dwEditStyles = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
	if (bPassword)
		dwEditStyles |= ES_PASSWORD;

	if (!m_ctlEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), pszDefault, dwEditStyles, CRect(6,36,335,57), this, IDC_EDITCTL))
		return 0;

	m_Font.CreatePointFont(80, "MS Sans Serif");

	m_ctlLabel.SetFont(&m_Font, false);
	m_ctlOK.SetFont(&m_Font, false);
	m_ctlCancel.SetFont(&m_Font, false);
	m_ctlEdit.SetFont(&m_Font, false);

	ShowWindow(SW_SHOW);

	//RedrawWindow(NULL, NULL, RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);

	return DoModal(pParent);
}

BOOL CTextEntryDlg::OnEraseBkgnd(CDC* pDC) 
{
	CBrush brBkgd(::GetSysColor(COLOR_BTNFACE));
	CRect rcFillArea;

	pDC->GetClipBox(&rcFillArea);
	pDC->FillRect(&rcFillArea, &brBkgd);
	
	return true;
	// return CWnd::OnEraseBkgnd(pDC); // not calling base class, which would make the 
									   // the background
}

int CTextEntryDlg::DoModal(CWnd *pParent)
{
	pParent->EnableWindow(false);
	EnableWindow(true);

	// enter modal loop
	DWORD dwFlags = MLF_SHOWONIDLE;
	if (GetStyle() & DS_NOIDLEMSG)
		dwFlags |= MLF_NOIDLEMSG;

	m_ctlEdit.SetFocus();
	
	// should the OK button be enabled or disabled?
	CString str;
	m_ctlEdit.GetWindowText(str);
	m_ctlOK.EnableWindow(str != "");

	int nReturn = RunModalLoop(MLF_NOIDLEMSG);

	if (m_hWnd != NULL)
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

	pParent->SetFocus();
	if (GetParent() != NULL && ::GetActiveWindow() == m_hWnd)
		::SetActiveWindow(pParent->m_hWnd);

	if (::IsWindow(m_hWnd))
		DestroyWindow();

	pParent->EnableWindow(true);

	return nReturn;
}

void CTextEntryDlg::OnEditboxChanged()
{
	CString str;
	m_ctlEdit.GetWindowText(str);
	m_ctlOK.EnableWindow(str != "");
}

void CTextEntryDlg::OnClose() 
{
	m_ctlEdit.GetWindowText(m_strText);
	EndModalLoop(IDCANCEL);
}

void CTextEntryDlg::OnOK()
{
	m_ctlEdit.GetWindowText(m_strText);
	EndModalLoop(IDOK);
}

void CTextEntryDlg::OnCancel()
{
	m_ctlEdit.GetWindowText(m_strText);
	EndModalLoop(IDCANCEL);
}

BOOL CTextEntryDlg::PreTranslateMessage(MSG* pMsg) 
{
	// for modeless processing (or modal)
	ASSERT(m_hWnd != NULL);

	// allow tooltip messages to be filtered
	if (CWnd::PreTranslateMessage(pMsg))
		return TRUE;

	// don't translate dialog messages when in Shift+F1 help mode
	CFrameWnd* pFrameWnd = GetTopLevelFrame();
	if (pFrameWnd != NULL && pFrameWnd->m_bHelpMode)
		return FALSE;

	// fix around for VK_ESCAPE in a multiline Edit that is on a Dialog
	// that doesn't have a cancel or the cancel is disabled.
	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_CANCEL) &&
		(::GetWindowLong(pMsg->hwnd, GWL_STYLE) & ES_MULTILINE))
	{
		HWND hItem = ::GetDlgItem(m_hWnd, IDCANCEL);
		EndModalLoop(IDCANCEL);
		return TRUE;
	}
	// filter both messages to dialog and from children
	return PreTranslateInput(pMsg);
	
	return CWnd::PreTranslateMessage(pMsg);
}

LPCTSTR CTextEntryDlg::GetText()
{
	return (LPCTSTR)m_strText;
}
