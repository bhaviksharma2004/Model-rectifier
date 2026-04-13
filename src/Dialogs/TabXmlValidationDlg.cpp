



#include "pch.h"
#include "TabXmlValidationDlg.h"
#include "XmlViewerDlg.h"
#include <UxTheme.h>
#include <cctype>

enum IssueCol {
    COL_KEY = 0,
    COL_GROUP,
    COL_SPEC,
    COL_VALNAME,
    COL_TYPE,
    COL_DESC,
    COL_VIEW
};

static CString ExtractEnglishName(const std::string& fullName) {
    if (fullName.empty()) return _T("missing");
    size_t pos = fullName.find("$$");
    std::string eng = (pos != std::string::npos) ? fullName.substr(0, pos) : fullName;
    
    
    while (!eng.empty() && std::isspace(static_cast<unsigned char>(eng.back()))) {
        eng.pop_back();
    }
    return CString(eng.c_str());
}





BEGIN_MESSAGE_MAP(CTabXmlValidationDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_BTN_VAL_VIEW_ALL, &CTabXmlValidationDlg::OnBnClickedViewAll)

    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnLeftFileListItemChanged)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VALFILES_RIGHT, &CTabXmlValidationDlg::OnRightFileListItemChanged)

    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnLeftFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VALFILES_RIGHT, &CTabXmlValidationDlg::OnRightFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VALISSUES,      &CTabXmlValidationDlg::OnIssueListCustomDraw)

    ON_NOTIFY(NM_CLICK,       IDC_LIST_VALISSUES, &CTabXmlValidationDlg::OnIssueListClick)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_VALISSUES, &CTabXmlValidationDlg::OnIssueListGetInfoTip)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnLeftFileListGetInfoTip)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_VALFILES_RIGHT, &CTabXmlValidationDlg::OnRightFileListGetInfoTip)
END_MESSAGE_MAP()





CTabXmlValidationDlg::CTabXmlValidationDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_XML_VALIDATION, pParent) {}

CTabXmlValidationDlg::~CTabXmlValidationDlg() {}





void CTabXmlValidationDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_VALFILES_LEFT_HEADER,  m_staticLeftHeader);
    DDX_Control(pDX, IDC_STATIC_VALFILES_RIGHT_HEADER, m_staticRightHeader);
    DDX_Control(pDX, IDC_LIST_VALFILES_LEFT,            m_listLeftFiles);
    DDX_Control(pDX, IDC_LIST_VALFILES_RIGHT,           m_listRightFiles);
    DDX_Control(pDX, IDC_STATIC_VALISSUES_HEADER,       m_staticIssuesHeader);
    DDX_Control(pDX, IDC_LIST_VALISSUES,                m_listIssues);
    DDX_Control(pDX, IDC_BTN_VAL_VIEW_ALL,              m_btnViewAll);
    DDX_Control(pDX, IDC_BTN_VAL_CORRUPT_INFO,          m_btnCorruptInfo);
}





BOOL CTabXmlValidationDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    
    LOGFONT lf = {};
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
    wcscpy_s(lf.lfFaceName, _T("Segoe UI"));
    lf.lfHeight = -14;
    m_uiFont.CreateFontIndirect(&lf);

    LOGFONT lfHeader = {};
    wcscpy_s(lfHeader.lfFaceName, _T("Segoe UI Semibold"));
    lfHeader.lfHeight = -15;
    lfHeader.lfWeight = FW_SEMIBOLD;
    m_headerFont.CreateFontIndirect(&lfHeader);

    
    m_listLeftFiles.SetFont(&m_uiFont);
    m_listRightFiles.SetFont(&m_uiFont);
    m_listIssues.SetFont(&m_uiFont);
    m_staticLeftHeader.SetFont(&m_headerFont);
    m_staticRightHeader.SetFont(&m_headerFont);
    m_staticIssuesHeader.SetFont(&m_headerFont);

    
    m_brushDialogBg.CreateSolidBrush(CLR_DIALOG_BG);
    m_brushPanelBg.CreateSolidBrush(CLR_PANEL_BG);

    
    m_btnViewAll.SetFont(&m_uiFont);
    m_btnViewAll.ModifyStyle(0, BS_OWNERDRAW);
    m_btnViewAll.EnableWindow(FALSE);

    m_btnCorruptInfo.SetFont(&m_uiFont);
    m_btnCorruptInfo.ModifyStyle(0, BS_OWNERDRAW);

    
    m_staticBoundary.Create(_T(""), WS_CHILD | SS_GRAYFRAME, CRect(0,0,0,0), this, 2002);

    
    m_staticLeftHeader.SetWindowText(_T("  Left Model"));
    m_staticRightHeader.SetWindowText(_T("  Right Model"));
    m_staticIssuesHeader.SetWindowText(_T("  Validation Issues"));

    
    SetupListColumns();

    
    m_imageListIssues.Create(1, 48, ILC_COLOR32, 1, 1);
    m_listIssues.SetImageList(&m_imageListIssues, LVSIL_SMALL);

    
    ModifyStyle(0, WS_CLIPCHILDREN);
    ::SetWindowTheme(m_listLeftFiles.GetSafeHwnd(), L"Explorer", NULL);
    ::SetWindowTheme(m_listRightFiles.GetSafeHwnd(), L"Explorer", NULL);
    ::SetWindowTheme(m_listIssues.GetSafeHwnd(), L"Explorer", NULL);

    return TRUE;
}





void CTabXmlValidationDlg::SetupListColumns() {
    
    DWORD fStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_listLeftFiles.SetExtendedStyle(m_listLeftFiles.GetExtendedStyle() | fStyle);
    m_listLeftFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 260);

    m_listRightFiles.SetExtendedStyle(m_listRightFiles.GetExtendedStyle() | fStyle);
    m_listRightFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 260);

    
    DWORD iStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
                 | LVS_EX_DOUBLEBUFFER  | LVS_EX_INFOTIP;
    m_listIssues.SetExtendedStyle(m_listIssues.GetExtendedStyle() | iStyle);
    m_listIssues.InsertColumn(COL_KEY,     _T("Composite Key"), LVCFMT_LEFT,   150);
    m_listIssues.InsertColumn(COL_GROUP,   _T("Group"),         LVCFMT_LEFT,   110);
    m_listIssues.InsertColumn(COL_SPEC,    _T("Spec"),          LVCFMT_LEFT,   120);
    m_listIssues.InsertColumn(COL_VALNAME, _T("Val Name"),      LVCFMT_LEFT,   120);
    m_listIssues.InsertColumn(COL_TYPE,    _T("Type"),          LVCFMT_LEFT,   100);
    m_listIssues.InsertColumn(COL_DESC,    _T("Description"),   LVCFMT_LEFT,   250);
    m_listIssues.InsertColumn(COL_VIEW,    _T("View"),          LVCFMT_CENTER, 70);
}





void CTabXmlValidationDlg::SetValidationReport(
    std::shared_ptr<ModelCompare::ValidationReport> report)
{
    m_report = report;
    m_selectedModel = SelectedModel::None;
    m_selectedFileIndex = -1;
    PopulateFileLists();
    ClearIssueList();
    PopulateIssueList(SelectedModel::None, -1);
}





void CTabXmlValidationDlg::PopulateFileLists() {
    m_listLeftFiles.DeleteAllItems();
    m_listRightFiles.DeleteAllItems();
    if (!m_report) return;

    
    for (int i = 0; i < (int)m_report->leftReport.fileResults.size(); i++) {
        const auto& fr = m_report->leftReport.fileResults[i];
        CString name;
        name.Format(_T("%d. %s"), i + 1,
            CString(fr.relativePath.filename().wstring().c_str()).GetString());
        int idx = m_listLeftFiles.InsertItem(i, name);
        m_listLeftFiles.SetItemData(idx, (DWORD_PTR)i);
    }

    
    for (int i = 0; i < (int)m_report->rightReport.fileResults.size(); i++) {
        const auto& fr = m_report->rightReport.fileResults[i];
        CString name;
        name.Format(_T("%d. %s"), i + 1,
            CString(fr.relativePath.filename().wstring().c_str()).GetString());
        int idx = m_listRightFiles.InsertItem(i, name);
        m_listRightFiles.SetItemData(idx, (DWORD_PTR)i);
    }
    
    ResizeListColumns();
}





void CTabXmlValidationDlg::PopulateIssueList(SelectedModel model, int fileIndex) {
    ClearIssueList();
    m_selectedModel = model;
    m_selectedFileIndex = fileIndex;

    const auto* modelReport = GetModelReport(model);
    if (!modelReport || fileIndex < 0 || fileIndex >= (int)modelReport->fileResults.size()) {
        m_btnViewAll.EnableWindow(FALSE);
        m_btnCorruptInfo.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_HIDE);
        m_listIssues.ShowWindow(SW_SHOW);
        m_staticIssuesHeader.SetWindowText(_T("  Validation Issues"));
        return;
    }

    const auto& fr = modelReport->fileResults[fileIndex];

    
    CString header;
    header.Format(_T("  Validation Issues - %s"),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());
    m_staticIssuesHeader.SetWindowText(header);

    
    if (fr.isCorrupt) {
        m_listIssues.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_SHOW);

        CString corruptText = _T("Corrupted XML File");
        m_btnCorruptInfo.SetWindowText(corruptText);

        
        CString tip = CString(fr.corruptionDetail.c_str());
        m_btnCorruptInfo.SetWindowText(corruptText);

        m_btnViewAll.EnableWindow(TRUE);  
        m_btnCorruptInfo.ShowWindow(SW_SHOW);
        m_btnCorruptInfo.BringWindowToTop();

        
        CRect rcClient; GetClientRect(&rcClient);
        OnSize(0, rcClient.Width(), rcClient.Height());
        return;
    }

    
    m_btnCorruptInfo.ShowWindow(SW_HIDE);
    m_staticBoundary.ShowWindow(SW_HIDE);
    m_listIssues.ShowWindow(SW_SHOW);

    for (int i = 0; i < (int)fr.issues.size(); i++) {
        const auto& issue = fr.issues[i];

        CString groupStr = ExtractEnglishName(issue.groupName);
        CString specStr = ExtractEnglishName(issue.specName);
        CString valStr = ExtractEnglishName(issue.valName);

        
        std::string keyParts = "";
        if (!issue.groupId.empty()) keyParts += "G:" + issue.groupId;
        if (!issue.specId.empty()) {
            if (!keyParts.empty()) keyParts += "|";
            keyParts += "S:" + issue.specId;
        }
        if (!issue.valId.empty()) {
            if (!keyParts.empty()) keyParts += "|";
            keyParts += "V:" + issue.valId;
        }
        CString compositeKey = CString(keyParts.c_str());

        int idx = m_listIssues.InsertItem(i, compositeKey);

        m_listIssues.SetItemText(idx, COL_GROUP, groupStr);
        m_listIssues.SetItemText(idx, COL_SPEC, specStr);
        m_listIssues.SetItemText(idx, COL_VALNAME, valStr);

        
        CString typeStr;
        if (issue.IsDuplicate())
            typeStr = _T("Duplicate ID");
        else
            typeStr = _T("Warning");
        m_listIssues.SetItemText(idx, COL_TYPE, typeStr);

        
        m_listIssues.SetItemText(idx, COL_DESC,
            CString(issue.description.c_str()));

        
        m_listIssues.SetItemText(idx, COL_VIEW, _T("O"));

        
        m_listIssues.SetItemData(idx, (DWORD_PTR)i);
    }

    m_btnViewAll.EnableWindow(!fr.issues.empty());
    
    ResizeListColumns();
}

void CTabXmlValidationDlg::ClearIssueList() {
    m_listIssues.DeleteAllItems();
}





void CTabXmlValidationDlg::OnLeftFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNM = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if (!(pNM->uChanged & LVIF_STATE) || !(pNM->uNewState & LVIS_SELECTED)) return;

    
    ClearFileSelection(SelectedModel::Left);

    int fi = (int)m_listLeftFiles.GetItemData(pNM->iItem);
    PopulateIssueList(SelectedModel::Left, fi);
}

void CTabXmlValidationDlg::OnRightFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNM = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if (!(pNM->uChanged & LVIF_STATE) || !(pNM->uNewState & LVIS_SELECTED)) return;

    
    ClearFileSelection(SelectedModel::Right);

    int fi = (int)m_listRightFiles.GetItemData(pNM->iItem);
    PopulateIssueList(SelectedModel::Right, fi);
}

void CTabXmlValidationDlg::ClearFileSelection(SelectedModel exceptModel) {
    if (exceptModel != SelectedModel::Left) {
        int sel = m_listLeftFiles.GetNextItem(-1, LVNI_SELECTED);
        if (sel >= 0) m_listLeftFiles.SetItemState(sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
    if (exceptModel != SelectedModel::Right) {
        int sel = m_listRightFiles.GetNextItem(-1, LVNI_SELECTED);
        if (sel >= 0) m_listRightFiles.SetItemState(sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}





void CTabXmlValidationDlg::OnIssueListClick(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNMItem = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    *pResult = 0;

    LVHITTESTINFO hitInfo = {};
    hitInfo.pt = pNMItem->ptAction;
    m_listIssues.SubItemHitTest(&hitInfo);

    if (hitInfo.iItem < 0 || hitInfo.iSubItem != COL_VIEW) return;

    const auto* fileResult = GetSelectedFileResult();
    if (!fileResult || fileResult->isCorrupt) return;

    int issueIdx = (int)m_listIssues.GetItemData(hitInfo.iItem);
    if (issueIdx < 0 || issueIdx >= (int)fileResult->issues.size()) return;

    OpenXmlViewer(*fileResult, &fileResult->issues[issueIdx]);
}





void CTabXmlValidationDlg::OnBnClickedViewAll() {
    const auto* fileResult = GetSelectedFileResult();
    if (!fileResult) return;

    OpenXmlViewer(*fileResult, nullptr);  
}





void CTabXmlValidationDlg::OpenXmlViewer(
    const ModelCompare::FileValidationResult& fileResult,
    const ModelCompare::ValidationIssue* scrollToIssue)
{
    CString title;
    title.Format(_T("XML Viewer - %s"),
        CString(fileResult.relativePath.filename().wstring().c_str()).GetString());

    CXmlViewerDlg dlg(this);

    
    dlg.SetCachedContent(fileResult.cachedFullText, fileResult.cachedLines, title);

    
    std::vector<ValidationHighlight> highlights;

    if (scrollToIssue) {
        
        if (scrollToIssue->lineNumber >= 0) {
            COLORREF color = scrollToIssue->IsDuplicate()
                ? CLR_HIGHLIGHT_DUPLICATE_DK
                : CLR_HIGHLIGHT_WARNING_DK;
            highlights.push_back({ scrollToIssue->lineNumber, color });
        }
        dlg.SetScrollToLine(scrollToIssue->lineNumber);
    } else {
        
        for (const auto& issue : fileResult.issues) {
            if (issue.lineNumber >= 0) {
                COLORREF color = issue.IsDuplicate()
                    ? CLR_HIGHLIGHT_DUPLICATE_DK
                    : CLR_HIGHLIGHT_WARNING_DK;
                highlights.push_back({ issue.lineNumber, color });
            }
        }
    }

    dlg.SetValidationHighlights(highlights);
    dlg.DoModal();
}





const ModelCompare::FileValidationResult* CTabXmlValidationDlg::GetSelectedFileResult() const {
    const auto* modelReport = GetModelReport(m_selectedModel);
    if (!modelReport || m_selectedFileIndex < 0
        || m_selectedFileIndex >= (int)modelReport->fileResults.size())
        return nullptr;
    return &modelReport->fileResults[m_selectedFileIndex];
}

const ModelCompare::ModelValidationReport* CTabXmlValidationDlg::GetModelReport(
    SelectedModel model) const
{
    if (!m_report) return nullptr;
    switch (model) {
    case SelectedModel::Left:  return &m_report->leftReport;
    case SelectedModel::Right: return &m_report->rightReport;
    default: return nullptr;
    }
}





void CTabXmlValidationDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (cx <= 0 || cy <= 0) return;

    const double LEFT_PANEL_RATIO = 0.25;
    const int HDR_H    = 22;
    const int GAP      = 6;
    const int VIEW_ALL_W = 120;

    int fileListW = (int)((cx - GAP) * LEFT_PANEL_RATIO);
    if (fileListW < 150) fileListW = 150;

    int issueX = fileListW + GAP;
    int issueW = cx - issueX;

    
    int halfH = (cy - HDR_H * 2 - GAP) / 2;  
    if (halfH < 50) halfH = 50;

    HDWP hDwp = ::BeginDeferWindowPos(10);
    if (!hDwp) return;

    
    hDwp = ::DeferWindowPos(hDwp, m_staticLeftHeader.GetSafeHwnd(), NULL,
        0, 0, fileListW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listLeftFiles.GetSafeHwnd(), NULL,
        0, HDR_H, fileListW, halfH, SWP_NOZORDER | SWP_NOACTIVATE);

    
    int rightHeaderY = HDR_H + halfH + GAP;
    hDwp = ::DeferWindowPos(hDwp, m_staticRightHeader.GetSafeHwnd(), NULL,
        0, rightHeaderY, fileListW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listRightFiles.GetSafeHwnd(), NULL,
        0, rightHeaderY + HDR_H, fileListW, cy - rightHeaderY - HDR_H,
        SWP_NOZORDER | SWP_NOACTIVATE);

    
    int btnViewAllX = cx - VIEW_ALL_W;
    hDwp = ::DeferWindowPos(hDwp, m_btnViewAll.GetSafeHwnd(), NULL,
        btnViewAllX, 0, VIEW_ALL_W, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    int issuesHeaderW = btnViewAllX - issueX - GAP;
    if (issuesHeaderW < 10) issuesHeaderW = 10;
    hDwp = ::DeferWindowPos(hDwp, m_staticIssuesHeader.GetSafeHwnd(), NULL,
        issueX, 0, issuesHeaderW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    hDwp = ::DeferWindowPos(hDwp, m_listIssues.GetSafeHwnd(), NULL,
        issueX, HDR_H, issueW, cy - HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    
    if (m_staticBoundary.GetSafeHwnd()) {
        hDwp = ::DeferWindowPos(hDwp, m_staticBoundary.GetSafeHwnd(), NULL,
            issueX, HDR_H, issueW, cy - HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ::EndDeferWindowPos(hDwp);

    
    if (m_btnCorruptInfo.GetSafeHwnd() && m_btnCorruptInfo.IsWindowVisible()) {
        int btnW = 220;
        int btnH = 40;
        int btnX = issueX + (issueW - btnW) / 2;
        int btnY = HDR_H + (cy - HDR_H - btnH) / 2;
        m_btnCorruptInfo.SetWindowPos(&wndTop, btnX, btnY, btnW, btnH, SWP_NOACTIVATE);
    }

    ResizeListColumns();
}

void CTabXmlValidationDlg::ResizeListColumns() {
    if (!m_listLeftFiles.GetSafeHwnd() || !m_listRightFiles.GetSafeHwnd()
        || !m_listIssues.GetSafeHwnd()) return;

    
    CRect leftRect;
    m_listLeftFiles.GetClientRect(&leftRect);
    m_listLeftFiles.SetColumnWidth(0, leftRect.Width() - 2);

    CRect rightRect;
    m_listRightFiles.GetClientRect(&rightRect);
    m_listRightFiles.SetColumnWidth(0, rightRect.Width() - 2);

    
    CRect issueRect;
    m_listIssues.GetClientRect(&issueRect);
    int totalWidth = issueRect.Width();

    if (totalWidth > 0) {
        
        const double colRatios[7] = { 0.18, 0.13, 0.14, 0.14, 0.11, 0.22, 0.08 };
        int w[7], sum = 0;
        for (int i = 0; i < 6; ++i) {
            w[i] = (int)(totalWidth * colRatios[i]);
            sum += w[i];
        }
        w[6] = totalWidth - sum - 3;
        for (int i = 0; i < 7; ++i)
            m_listIssues.SetColumnWidth(i, w[i]);
    }
}





void CTabXmlValidationDlg::OnIssueListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;

    CPoint pt; GetCursorPos(&pt); m_listIssues.ScreenToClient(&pt);
    LVHITTESTINFO hitInfo = {}; hitInfo.pt = pt; m_listIssues.SubItemHitTest(&hitInfo);
    int subItem = hitInfo.iSubItem >= 0 ? hitInfo.iSubItem : 0;
    CString text = m_listIssues.GetItemText(pInfoTip->iItem, subItem);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}

void CTabXmlValidationDlg::OnLeftFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;
    CString text = m_listLeftFiles.GetItemText(pInfoTip->iItem, 0);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}

void CTabXmlValidationDlg::OnRightFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;
    CString text = m_listRightFiles.GetItemText(pInfoTip->iItem, 0);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}






HBRUSH CTabXmlValidationDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    switch (nCtlColor) {
    case CTLCOLOR_DLG:
        pDC->SetBkColor(CLR_DIALOG_BG);
        return m_brushDialogBg;

    case CTLCOLOR_STATIC: {
        UINT ctrlId = pWnd->GetDlgCtrlID();
        pDC->SetBkMode(TRANSPARENT);
        if (ctrlId == IDC_STATIC_VALFILES_LEFT_HEADER
            || ctrlId == IDC_STATIC_VALFILES_RIGHT_HEADER
            || ctrlId == IDC_STATIC_VALISSUES_HEADER)
        {
            pDC->SetTextColor(CLR_HEADER_TEXT);
            return m_brushPanelBg;
        }
        pDC->SetTextColor(CLR_TEXT_PRIMARY);
        return m_brushDialogBg;
    }

    case CTLCOLOR_BTN:
        pDC->SetBkColor(CLR_DIALOG_BG);
        return m_brushDialogBg;
    }
    return hbr;
}

BOOL CTabXmlValidationDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(&rc, CLR_DIALOG_BG);
    return TRUE;
}

void CTabXmlValidationDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF baseColor, pressedColor;
    int cornerRadius = 8;
    bool isOurs = true;

    switch (nIDCtl) {
    case IDC_BTN_VAL_VIEW_ALL:
        baseColor = RGB(80, 140, 220);
        pressedColor = RGB(55, 110, 190);
        break;
    case IDC_BTN_VAL_CORRUPT_INFO:
        baseColor = RGB(220, 53, 69);
        pressedColor = RGB(190, 40, 55);
        cornerRadius = 12;
        break;
    default:
        isOurs = false;
        break;
    }

    if (!isOurs) {
        CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
        return;
    }

    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;
    UINT state = lpDrawItemStruct->itemState;

    pDC->FillSolidRect(&rect, CLR_DIALOG_BG);
    COLORREF bgColor = baseColor;
    COLORREF textColor = RGB(255, 255, 255);

    if (state & ODS_DISABLED) {
        bgColor = RGB(200, 200, 200);
        textColor = RGB(150, 150, 150);
    } else if (state & ODS_SELECTED) {
        bgColor = pressedColor;
    }

    CBrush brush(bgColor);
    CPen pen(PS_SOLID, 1, bgColor);
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->RoundRect(&rect, CPoint(cornerRadius, cornerRadius));

    CString text;
    GetDlgItem(nIDCtl)->GetWindowText(text);
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(textColor);
    CFont* pOldFont = pDC->SelectObject(GetDlgItem(nIDCtl)->GetFont());
    pDC->DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);
    pDC->SelectObject(pOldBrush);
    pDC->SelectObject(pOldPen);

    if (state & ODS_FOCUS) {
        CRect focusRect = rect;
        focusRect.DeflateRect(3, 3);
        pDC->DrawFocusRect(&focusRect);
    }
}





void CTabXmlValidationDlg::OnLeftFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW; return; }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        
        pCD->clrTextBk = CLR_DIALOG_BG;
        pCD->clrText = CLR_TEXT_PRIMARY;
    }
}

void CTabXmlValidationDlg::OnRightFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW; return; }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        pCD->clrTextBk = CLR_DIALOG_BG;
        pCD->clrText = CLR_TEXT_PRIMARY;
    }
}

void CTabXmlValidationDlg::OnIssueListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        return;
    }
    if (pCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
        int row = (int)pCD->nmcd.dwItemSpec;
        int displayIdx = (int)pCD->nmcd.lItemlParam;
        int subItem = pCD->iSubItem;

        COLORREF bg = CLR_DIALOG_BG;
        COLORREF txt = CLR_TEXT_PRIMARY;

        if (displayIdx >= 0 && subItem == COL_VIEW) {
            
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
            CRect rect;
            m_listIssues.GetSubItemRect(row, subItem, LVIR_BOUNDS, rect);

            pDC->FillSolidRect(&rect, bg);
            rect.DeflateRect(12, 6);

            COLORREF btnColor = CLR_ACCENT_BLUE;
            CBrush brush(btnColor);
            CPen pen(PS_SOLID, 1, btnColor);
            CBrush* pOldBrush = pDC->SelectObject(&brush);
            CPen* pOldPen = pDC->SelectObject(&pen);
            pDC->RoundRect(&rect, CPoint(8, 8));

            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255, 255, 255));

            LOGFONT lfIcon = {0};
            wcscpy_s(lfIcon.lfFaceName, _T("Segoe MDL2 Assets"));
            lfIcon.lfHeight = -16;
            CFont iconFont;
            iconFont.CreateFontIndirect(&lfIcon);

            CFont* pOldFont = pDC->SelectObject(&iconFont);
            pDC->DrawText(_T("\xE890"), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            pDC->SelectObject(pOldFont);
            pDC->SelectObject(pOldBrush);
            pDC->SelectObject(pOldPen);

            *pResult = CDRF_SKIPDEFAULT;
            return;
        }

        if (displayIdx >= 0 && subItem == COL_DESC) {
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
            CRect rect;
            m_listIssues.GetSubItemRect(row, subItem, LVIR_BOUNDS, rect);
            pDC->FillSolidRect(&rect, bg);
            rect.DeflateRect(6, 4);
            
            CString text = m_listIssues.GetItemText(row, subItem);
            pDC->SetBkColor(bg);
            pDC->SetTextColor(txt);
            CFont* pOldFont = pDC->SelectObject(m_listIssues.GetFont());
            pDC->DrawText(text, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS | DT_EDITCONTROL);
            pDC->SelectObject(pOldFont);
            *pResult = CDRF_SKIPDEFAULT;
            return;
        }

        pCD->clrTextBk = bg;
        pCD->clrText = txt;
        *pResult = CDRF_DODEFAULT;
        return;
    }
}
