#include "pch.h"
#include "resource.h"
#include "MainDlg.h"
#include "Engine/CompareEngine.h"
#include "Engine/StructuralIdComparator.h"
#include "Engine/XmlValidationEngine.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define APP_INITIAL_WIDTH  1000
#define APP_INITIAL_HEIGHT 600
#define APP_MIN_WIDTH      1000
#define APP_MIN_HEIGHT     600

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LEFT,  &CMainDlg::OnBnClickedBrowseLeft)
    ON_BN_CLICKED(IDC_BTN_BROWSE_RIGHT, &CMainDlg::OnBnClickedBrowseRight)
    ON_BN_CLICKED(IDC_BTN_COMPARE,      &CMainDlg::OnBnClickedCompare)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_DRAWITEM()
    ON_WM_CLOSE()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CMainDlg::OnTcnSelchangeTabMain)
    ON_MESSAGE(WM_COMPARE_COMPLETE, &CMainDlg::OnCompareComplete)
    ON_MESSAGE(WM_VALIDATE_COMPLETE, &CMainDlg::OnValidateComplete)
END_MESSAGE_MAP()

CMainDlg::CMainDlg(CWnd* pParent)
    : CDialogEx(IDD_MODELCOMPARE_DIALOG, pParent) {}

void CMainDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_LEFT_PATH,         m_editLeftPath);
    DDX_Control(pDX, IDC_EDIT_RIGHT_PATH,        m_editRightPath);
    DDX_Control(pDX, IDC_BTN_BROWSE_LEFT,        m_btnBrowseLeft);
    DDX_Control(pDX, IDC_BTN_BROWSE_RIGHT,       m_btnBrowseRight);
    DDX_Control(pDX, IDC_BTN_COMPARE,            m_btnCompare);
    DDX_Control(pDX, IDC_STATIC_SUMMARY,         m_staticSummary);
    DDX_Control(pDX, IDC_TAB_MAIN,               m_tabMain);
}

BOOL CMainDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    SetWindowText(_T("LAI Model Compare Tool"));
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), TRUE);
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), FALSE);

    LOGFONT lf = {};
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
    wcscpy_s(lf.lfFaceName, _T("Segoe UI"));
    lf.lfHeight = -14;
    m_uiFont.CreateFontIndirect(&lf);

    m_tabMain.SetFont(&m_uiFont);
    m_staticSummary.SetFont(&m_uiFont);
    m_editLeftPath.SetFont(&m_uiFont);
    m_editRightPath.SetFont(&m_uiFont);
    m_btnBrowseLeft.SetFont(&m_uiFont);
    m_btnBrowseRight.SetFont(&m_uiFont);

    m_brushDialogBg.CreateSolidBrush(CLR_DIALOG_BG);
    m_brushEditBg.CreateSolidBrush(CLR_EDIT_BG);

    m_btnCompare.SetFont(&m_uiFont);
    m_btnCompare.ModifyStyle(0, BS_OWNERDRAW);

    m_tabMain.InsertItem(0, _T("Spec ID Compare"));
    m_tabMain.InsertItem(1, _T("XML File Validation"));
    m_tabMain.InsertItem(2, _T("Spec Value Compare"));

    m_tabSpecId.Create(IDD_TAB_SPEC_ID_COMPARE, &m_tabMain);
    m_tabXmlVal.Create(IDD_TAB_XML_VALIDATION, &m_tabMain);
    m_tabSpecVal.Create(IDD_TAB_SPEC_VALUE_COMPARE, &m_tabMain);

    m_tabSpecId.ShowWindow(SW_SHOW);
    m_tabXmlVal.ShowWindow(SW_HIDE);
    m_tabSpecVal.ShowWindow(SW_HIDE);

    UpdateSummary(_T("Select two model folders and click Compare."));

    BOOL bShadow = TRUE;
    DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_NCRENDERING_POLICY, &bShadow, sizeof(bShadow));

    SetWindowPos(nullptr, 0, 0, APP_INITIAL_WIDTH, APP_INITIAL_HEIGHT, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    return TRUE;
}

void CMainDlg::OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult) {
    *pResult = 0;
    int sel = m_tabMain.GetCurSel();
    m_tabSpecId.ShowWindow(sel == 0 ? SW_SHOW : SW_HIDE);
    m_tabXmlVal.ShowWindow(sel == 1 ? SW_SHOW : SW_HIDE);
    m_tabSpecVal.ShowWindow(sel == 2 ? SW_SHOW : SW_HIDE);

    // Dynamic button text based on active tab
    if (sel == 1)
        m_btnCompare.SetWindowText(_T("Validate\nModels"));
    else
        m_btnCompare.SetWindowText(_T("Compare\nModels"));
    
    CRect rcClient;
    GetClientRect(&rcClient);
    RepositionControls(rcClient.Width(), rcClient.Height());
}

CString CMainDlg::BrowseForFolder(const CString& title) {
    CFolderPickerDialog dlg(nullptr, 0, this);
    dlg.m_ofn.lpstrTitle = title;
    if (dlg.DoModal() == IDOK) return dlg.GetPathName();
    return _T("");
}

void CMainDlg::OnBnClickedBrowseLeft() {
    CString p = BrowseForFolder(_T("Select Left Model (Reference)"));
    if (!p.IsEmpty()) m_editLeftPath.SetWindowText(p);
}

void CMainDlg::OnBnClickedBrowseRight() {
    CString p = BrowseForFolder(_T("Select Right Model (Target)"));
    if (!p.IsEmpty()) m_editRightPath.SetWindowText(p);
}

void CMainDlg::OnBnClickedCompare() {
    CString lp, rp;
    m_editLeftPath.GetWindowText(lp);
    m_editRightPath.GetWindowText(rp);
    if (lp.IsEmpty() || rp.IsEmpty()) {
        AfxMessageBox(_T("Please select both model folders."), MB_ICONWARNING);
        return;
    }
    if (!std::filesystem::exists((LPCWSTR)lp) || !std::filesystem::exists((LPCWSTR)rp)) {
        AfxMessageBox(_T("One or both folders do not exist."), MB_ICONERROR);
        return;
    }

    EnableCompareUI(false);

    int activeTab = m_tabMain.GetCurSel();
    if (activeTab == 1) {
        // XML Validation tab
        UpdateSummary(_T("Validating..."));
        RunValidation();
    } else {
        // Compare tabs
        UpdateSummary(_T("Comparing..."));
        m_tabSpecId.SetReport(nullptr);
        RunCompare();
    }
}

void CMainDlg::RunCompare() {
    CString lp, rp;
    m_editLeftPath.GetWindowText(lp);
    m_editRightPath.GetWindowText(rp);
    std::wstring lw = (LPCWSTR)lp, rw = (LPCWSTR)rp;
    HWND hWnd = GetSafeHwnd();

    std::thread([lw, rw, hWnd]() {
        auto engine = std::make_unique<ModelCompare::CompareEngine>(
            std::make_unique<ModelCompare::StructuralIdComparator>());
        auto* report = new ModelCompare::ModelDiffReport(
            engine->CompareModels(std::filesystem::path(lw), std::filesystem::path(rw)));
        ::PostMessage(hWnd, WM_COMPARE_COMPLETE, 0, (LPARAM)report);
    }).detach();
}

LRESULT CMainDlg::OnCompareComplete(WPARAM, LPARAM lParam) {
    m_report.reset(reinterpret_cast<ModelCompare::ModelDiffReport*>(lParam));
    CString s;
    s.Format(_T("%zu file(s) with differences  |  %zu scanned  |  %zu missing  |  %zu extra"),
        m_report->totalFilesWithDiffs, m_report->totalFilesScanned,
        m_report->totalMissingIds, m_report->totalExtraIds);
    UpdateSummary(s);
    
    m_tabSpecId.SetReport(m_report);

    EnableCompareUI(true);
    return 0;
}

void CMainDlg::RunValidation() {
    CString lp, rp;
    m_editLeftPath.GetWindowText(lp);
    m_editRightPath.GetWindowText(rp);
    HWND hWnd = GetSafeHwnd();

    std::thread([lw = std::wstring((LPCWSTR)lp),
                 rw = std::wstring((LPCWSTR)rp), hWnd]() {
        auto* report = new ModelCompare::ValidationReport(
            ModelCompare::XmlValidationEngine::ValidateModels(
                std::filesystem::path(lw),
                std::filesystem::path(rw)));
        ::PostMessage(hWnd, WM_VALIDATE_COMPLETE, 0, (LPARAM)report);
    }).detach();
}

LRESULT CMainDlg::OnValidateComplete(WPARAM, LPARAM lParam) {
    m_validationReport.reset(
        reinterpret_cast<ModelCompare::ValidationReport*>(lParam));

    size_t totalIssues = m_validationReport->leftReport.totalIssues
                       + m_validationReport->rightReport.totalIssues;
    size_t totalFilesWithIssues = m_validationReport->leftReport.totalFilesWithIssues
                                + m_validationReport->rightReport.totalFilesWithIssues;
    size_t totalScanned = m_validationReport->leftReport.totalFilesScanned
                        + m_validationReport->rightReport.totalFilesScanned;

    CString s;
    s.Format(_T("%zu issue(s) found  |  %zu file(s) with issues  |  %zu file(s) scanned"),
        totalIssues, totalFilesWithIssues, totalScanned);
    UpdateSummary(s);

    m_tabXmlVal.SetValidationReport(m_validationReport);
    EnableCompareUI(true);
    return 0;
}

void CMainDlg::EnableCompareUI(bool enable) {
    m_btnBrowseLeft.EnableWindow(enable);
    m_btnBrowseRight.EnableWindow(enable);
    m_btnCompare.EnableWindow(enable);
}

void CMainDlg::UpdateSummary(const CString& text) {
    m_staticSummary.SetWindowText(text);
}

void CMainDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI) {
    lpMMI->ptMinTrackSize.x = APP_MIN_WIDTH;
    lpMMI->ptMinTrackSize.y = APP_MIN_HEIGHT;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CMainDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (cx > 0 && cy > 0) RepositionControls(cx, cy);
}

void CMainDlg::RepositionControls(int cx, int cy) {
    if (!m_editLeftPath.GetSafeHwnd()) return;

    const int M       = 10;
    const int ROW_H   = 24;
    const int GAP     = 6;
    const int LBL_W   = 150;
    const int BTN_W   = 65;
    const int CMP_W   = 90;
    const int CMP_H   = 50;

    HDWP hDwp = ::BeginDeferWindowPos(8);
    if (!hDwp) return;

    int cmpX = cx - M - CMP_W;
    int inputAreaH = ROW_H * 2 + GAP;
    int cmpY = M + (inputAreaH - CMP_H) / 2;
    int maxInputRight = cmpX - 12;
    int editW = maxInputRight - (M + LBL_W + GAP + GAP + BTN_W);
    if (editW < 100) editW = 100;
    int y = M;

    CWnd* pLL = GetDlgItem(IDC_STATIC_LEFT_LABEL);
    if (pLL) hDwp = ::DeferWindowPos(hDwp, pLL->GetSafeHwnd(), NULL, M, y + 3, LBL_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_editLeftPath.GetSafeHwnd(), NULL, M + LBL_W + GAP, y, editW, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_btnBrowseLeft.GetSafeHwnd(), NULL, M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    y += ROW_H + GAP;

    CWnd* pRL = GetDlgItem(IDC_STATIC_RIGHT_LABEL);
    if (pRL) hDwp = ::DeferWindowPos(hDwp, pRL->GetSafeHwnd(), NULL, M, y + 3, LBL_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_editRightPath.GetSafeHwnd(), NULL, M + LBL_W + GAP, y, editW, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_btnBrowseRight.GetSafeHwnd(), NULL, M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    y += ROW_H + GAP;

    hDwp = ::DeferWindowPos(hDwp, m_btnCompare.GetSafeHwnd(), NULL, cmpX, cmpY, CMP_W, CMP_H, SWP_NOZORDER | SWP_NOACTIVATE);

    int summaryH = 22;
    hDwp = ::DeferWindowPos(hDwp, m_staticSummary.GetSafeHwnd(), NULL, M, y, cx - 2 * M, summaryH, SWP_NOZORDER | SWP_NOACTIVATE);
    y += summaryH + GAP;

    int tabH = cy - y - M;
    if (tabH < 100) tabH = 100;
    
    hDwp = ::DeferWindowPos(hDwp, m_tabMain.GetSafeHwnd(), NULL, M, y, cx - 2 * M, tabH, SWP_NOZORDER | SWP_NOACTIVATE);
    ::EndDeferWindowPos(hDwp);

    CRect rcTab;
    m_tabMain.GetClientRect(&rcTab);
    m_tabMain.AdjustRect(FALSE, &rcTab);
    
    int contentX = rcTab.left + 2;
    int contentY = rcTab.top + 2;
    int contentW = rcTab.Width() - 4;
    int contentH = rcTab.Height() - 4;

    if (m_tabSpecId.GetSafeHwnd()) m_tabSpecId.SetWindowPos(NULL, contentX, contentY, contentW, contentH, SWP_NOZORDER | SWP_NOACTIVATE);
    if (m_tabXmlVal.GetSafeHwnd()) m_tabXmlVal.SetWindowPos(NULL, contentX, contentY, contentW, contentH, SWP_NOZORDER | SWP_NOACTIVATE);
    if (m_tabSpecVal.GetSafeHwnd()) m_tabSpecVal.SetWindowPos(NULL, contentX, contentY, contentW, contentH, SWP_NOZORDER | SWP_NOACTIVATE);
}

HBRUSH CMainDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    switch (nCtlColor) {
    case CTLCOLOR_DLG: pDC->SetBkColor(CLR_DIALOG_BG); return (HBRUSH)m_brushDialogBg;
    case CTLCOLOR_STATIC: {
        UINT ctrlId = pWnd->GetDlgCtrlID(); pDC->SetBkMode(TRANSPARENT);
        if (ctrlId == IDC_STATIC_SUMMARY) { pDC->SetTextColor(CLR_TEXT_SECONDARY); return (HBRUSH)m_brushDialogBg; }
        pDC->SetTextColor(CLR_TEXT_PRIMARY); return (HBRUSH)m_brushDialogBg;
    }
    case CTLCOLOR_EDIT: pDC->SetBkColor(CLR_EDIT_BG); pDC->SetTextColor(CLR_TEXT_PRIMARY); return (HBRUSH)m_brushEditBg;
    case CTLCOLOR_BTN: pDC->SetBkColor(CLR_DIALOG_BG); return (HBRUSH)m_brushDialogBg;
    }
    return hbr;
}

BOOL CMainDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rc; GetClientRect(&rc); pDC->FillSolidRect(&rc, CLR_DIALOG_BG); return TRUE;
}

void CMainDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    if (nIDCtl != IDC_BTN_COMPARE) { CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct); return; }

    COLORREF bgColor = CLR_ACCENT_BLUE, pressedColor = RGB(0, 90, 210);
    int cornerRadius = 14;

    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC); CRect rect = lpDrawItemStruct->rcItem; UINT state = lpDrawItemStruct->itemState;
    pDC->FillSolidRect(&rect, CLR_DIALOG_BG);

    COLORREF textColor = RGB(255, 255, 255);
    if (state & ODS_DISABLED) { bgColor = RGB(200, 200, 200); textColor = RGB(150, 150, 150); }
    else if (state & ODS_SELECTED) bgColor = pressedColor;

    CBrush brush(bgColor); CPen pen(PS_SOLID, 1, bgColor);
    CBrush* pOldBrush = pDC->SelectObject(&brush); CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->RoundRect(&rect, CPoint(cornerRadius, cornerRadius));

    CString text; GetDlgItem(nIDCtl)->GetWindowText(text);
    pDC->SetBkMode(TRANSPARENT); pDC->SetTextColor(textColor);
    CFont* pOldFont = pDC->SelectObject(GetDlgItem(nIDCtl)->GetFont());

    CRect textRect = rect;
    pDC->DrawText(text, textRect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
    int offsetY = (rect.Height() - textRect.Height()) / 2;
    textRect = rect; textRect.top += offsetY;
    pDC->DrawText(text, textRect, DT_CENTER | DT_WORDBREAK);

    pDC->SelectObject(pOldFont); pDC->SelectObject(pOldBrush); pDC->SelectObject(pOldPen);
    if (state & ODS_FOCUS) { CRect focusRect = rect; focusRect.DeflateRect(3, 3); pDC->DrawFocusRect(&focusRect); }
}

void CMainDlg::OnClose() {
    CDialogEx::OnCancel();
}
