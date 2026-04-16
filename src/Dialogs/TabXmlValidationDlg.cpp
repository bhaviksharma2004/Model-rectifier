
#include "pch.h"
#include "TabXmlValidationDlg.h"
#include "XmlViewerDlg.h"
#include <UxTheme.h>
#include <cctype>

#include "Theme.h"

#define FILE_LIST_COL_W          260

#define COL_KEY_W                150
#define COL_DESC_W               350
#define COL_VIEW_W               70

#define LAYOUT_LEFT_PANEL_RATIO  0.20
#define VIEW_ALL_BTN_W           120

#define MIN_FILE_LIST_WIDTH      150
#define CORRUPT_BTN_W            220
#define CORRUPT_BTN_H            40
#define CORRUPT_DESC_W           400
#define CORRUPT_DESC_H           60

#define FILE_LIST_MARGIN         2

enum IssueCol {
    COL_KEY = 0,
    COL_DESC,
    COL_VIEW
};

BEGIN_MESSAGE_MAP(CTabXmlValidationDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_BTN_VAL_VIEW_ALL, &CTabXmlValidationDlg::OnBnClickedViewAll)

    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnFileListItemChanged)

    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VALISSUES,      &CTabXmlValidationDlg::OnIssueListCustomDraw)

    ON_NOTIFY(NM_CLICK,       IDC_LIST_VALISSUES, &CTabXmlValidationDlg::OnIssueListClick)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_VALISSUES, &CTabXmlValidationDlg::OnIssueListGetInfoTip)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_VALFILES_LEFT,  &CTabXmlValidationDlg::OnFileListGetInfoTip)
END_MESSAGE_MAP()





CTabXmlValidationDlg::CTabXmlValidationDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_XML_VALIDATION, pParent) {}

CTabXmlValidationDlg::~CTabXmlValidationDlg() {}





void CTabXmlValidationDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_VALFILES_LEFT_HEADER,  m_staticFileHeader);
    DDX_Control(pDX, IDC_LIST_VALFILES_LEFT,            m_listFiles);
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
    lf.lfHeight = Theme::Get()->FontSizeDefault();
    m_uiFont.CreateFontIndirect(&lf);

    LOGFONT lfHeader = {};
    wcscpy_s(lfHeader.lfFaceName, _T("Segoe UI Semibold"));
    lfHeader.lfHeight = Theme::Get()->FontSizeHeader();
    lfHeader.lfWeight = FW_SEMIBOLD;
    m_headerFont.CreateFontIndirect(&lfHeader);

    
    m_listFiles.SetFont(&m_uiFont);
    m_listIssues.SetFont(&m_uiFont);
    m_staticFileHeader.SetFont(&m_headerFont);
    m_staticIssuesHeader.SetFont(&m_headerFont);

    
    m_brushDialogBg.CreateSolidBrush(Theme::Get()->DialogBg());
    m_brushPanelBg.CreateSolidBrush(Theme::Get()->PanelBg());

    
    m_btnViewAll.SetFont(&m_uiFont);
    m_btnViewAll.ModifyStyle(0, BS_OWNERDRAW);
    m_btnViewAll.EnableWindow(FALSE);

    m_btnCorruptInfo.SetFont(&m_uiFont);
    m_btnCorruptInfo.ModifyStyle(0, BS_OWNERDRAW);

    
    m_staticBoundary.Create(_T(""), WS_CHILD | SS_GRAYFRAME, CRect(0,0,0,0), this, 2002);

    
    m_staticCorruptDesc.Create(_T(""), WS_CHILD | SS_CENTER | SS_NOPREFIX,
        CRect(0, 0, 0, 0), this, 2003);
    m_staticCorruptDesc.SetFont(&m_uiFont);

    
    m_staticFileHeader.SetWindowText(_T("  Model Files"));
    m_staticIssuesHeader.SetWindowText(_T("  Validation Issues"));

    
    SetupListColumns();

    
    m_imageListIssues.Create(1, 40, ILC_COLOR32, 1, 1);
    m_listIssues.SetImageList(&m_imageListIssues, LVSIL_SMALL);

    
    ModifyStyle(0, WS_CLIPCHILDREN);
    ::SetWindowTheme(m_listFiles.GetSafeHwnd(), L"Explorer", NULL);

    m_listFiles.ModifyStyle(0, LVS_SINGLESEL);
    m_listIssues.ModifyStyle(0, LVS_SINGLESEL);

    return TRUE;
}

void CTabXmlValidationDlg::SetupListColumns() {
    
    DWORD fStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_listFiles.SetExtendedStyle(m_listFiles.GetExtendedStyle() | fStyle);
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, FILE_LIST_COL_W);

    
    DWORD iStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
                 | LVS_EX_DOUBLEBUFFER  | LVS_EX_INFOTIP;
    m_listIssues.SetExtendedStyle(m_listIssues.GetExtendedStyle() | iStyle);
    m_listIssues.InsertColumn(COL_KEY,     _T("Composite Key"), LVCFMT_LEFT,   COL_KEY_W);
    m_listIssues.InsertColumn(COL_DESC,    _T("Description"),   LVCFMT_LEFT,   COL_DESC_W);
    m_listIssues.InsertColumn(COL_VIEW,    _T("View"),          LVCFMT_CENTER, COL_VIEW_W);
}

void CTabXmlValidationDlg::SetValidationReport(
    std::shared_ptr<ModelCompare::ModelValidationReport> report)
{
    m_report = report;
    m_selectedFileIndex = -1;
    PopulateFileList();
    ClearIssueList();
    PopulateIssueList(-1);
}

void CTabXmlValidationDlg::PopulateFileList() {
    m_listFiles.DeleteAllItems();
    if (!m_report) return;

    
    for (int i = 0; i < (int)m_report->fileResults.size(); i++) {
        const auto& fr = m_report->fileResults[i];
        CString name;
        name.Format(_T("%d. %s"), i + 1,
            CString(fr.relativePath.filename().wstring().c_str()).GetString());
        int idx = m_listFiles.InsertItem(i, name);
        m_listFiles.SetItemData(idx, (DWORD_PTR)i);
    }
    
    ResizeListColumns();
}

void CTabXmlValidationDlg::PopulateIssueList(int fileIndex) {
    ClearIssueList();
    m_selectedFileIndex = fileIndex;

    if (!m_report || fileIndex < 0 || fileIndex >= (int)m_report->fileResults.size()) {
        m_btnViewAll.EnableWindow(FALSE);
        m_btnCorruptInfo.ShowWindow(SW_HIDE);
        m_staticCorruptDesc.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_HIDE);
        m_listIssues.ShowWindow(SW_SHOW);
        m_staticIssuesHeader.SetWindowText(_T("  Validation Issues"));
        return;
    }

    const auto& fr = m_report->fileResults[fileIndex];

    
    CString header;
    header.Format(_T("  Validation Issues - %s"),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());
    m_staticIssuesHeader.SetWindowText(header);

    
    if (fr.isCorrupt) {
        m_listIssues.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_SHOW);

        CString corruptText = _T("Corrupted XML File");
        m_btnCorruptInfo.SetWindowText(corruptText);

        
        CString descText;
        if (fr.corruptionLineNumber > 0) {
            descText.Format(_T("Line %d: %s"), fr.corruptionLineNumber,
                CString(fr.corruptionDetail.c_str()).GetString());
        } else {
            descText = CString(fr.corruptionDetail.c_str());
        }
        m_staticCorruptDesc.SetWindowText(descText);

        m_btnViewAll.EnableWindow(TRUE);
        m_btnCorruptInfo.ShowWindow(SW_SHOW);
        m_btnCorruptInfo.BringWindowToTop();
        m_staticCorruptDesc.ShowWindow(SW_SHOW);

        
        CRect rcClient; GetClientRect(&rcClient);
        OnSize(0, rcClient.Width(), rcClient.Height());
        return;
    }

    
    m_btnCorruptInfo.ShowWindow(SW_HIDE);
    m_staticCorruptDesc.ShowWindow(SW_HIDE);
    m_staticBoundary.ShowWindow(SW_HIDE);
    m_listIssues.ShowWindow(SW_SHOW);

    for (int i = 0; i < (int)fr.issues.size(); i++) {
        const auto& issue = fr.issues[i];

        
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

void CTabXmlValidationDlg::OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNM = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if ((pNM->uChanged & LVIF_STATE) && (pNM->uNewState & LVIS_SELECTED)) {
        int fi = (int)m_listFiles.GetItemData(pNM->iItem);
        PopulateIssueList(fi);
    } else if ((pNM->uChanged & LVIF_STATE) && m_listFiles.GetSelectedCount() == 0) {
        m_selectedFileIndex = -1;
        PopulateIssueList(-1);
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
    if (!m_report || m_selectedFileIndex < 0
        || m_selectedFileIndex >= (int)m_report->fileResults.size())
        return nullptr;
    return &m_report->fileResults[m_selectedFileIndex];
}

void CTabXmlValidationDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (cx <= 0 || cy <= 0) return;

    int fileListW = (int)((cx - Theme::Get()->LayoutGap()) * LAYOUT_LEFT_PANEL_RATIO);
    if (fileListW < MIN_FILE_LIST_WIDTH) fileListW = MIN_FILE_LIST_WIDTH;

    int issueX = fileListW + Theme::Get()->LayoutGap();
    int issueW = cx - issueX;

    HDWP hDwp = ::BeginDeferWindowPos(8);
    if (!hDwp) return;

    
    hDwp = ::DeferWindowPos(hDwp, m_staticFileHeader.GetSafeHwnd(), NULL,
        0, 0, fileListW, Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listFiles.GetSafeHwnd(), NULL,
        0, Theme::Get()->LayoutHeaderHeight(), fileListW, cy - Theme::Get()->LayoutHeaderHeight(),
        SWP_NOZORDER | SWP_NOACTIVATE);

    
    int btnViewAllX = cx - VIEW_ALL_BTN_W;
    hDwp = ::DeferWindowPos(hDwp, m_btnViewAll.GetSafeHwnd(), NULL,
        btnViewAllX, 0, VIEW_ALL_BTN_W, Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);

    int issuesHeaderW = btnViewAllX - issueX - Theme::Get()->LayoutGap();
    if (issuesHeaderW < 10) issuesHeaderW = 10;
    hDwp = ::DeferWindowPos(hDwp, m_staticIssuesHeader.GetSafeHwnd(), NULL,
        issueX, 0, issuesHeaderW, Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);

    hDwp = ::DeferWindowPos(hDwp, m_listIssues.GetSafeHwnd(), NULL,
        issueX, Theme::Get()->LayoutHeaderHeight(), issueW, cy - Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);

    
    if (m_staticBoundary.GetSafeHwnd()) {
        hDwp = ::DeferWindowPos(hDwp, m_staticBoundary.GetSafeHwnd(), NULL,
            issueX, Theme::Get()->LayoutHeaderHeight(), issueW, cy - Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ::EndDeferWindowPos(hDwp);

    
    if (m_btnCorruptInfo.GetSafeHwnd() && m_btnCorruptInfo.IsWindowVisible()) {
        int btnW = CORRUPT_BTN_W;
        int btnH = CORRUPT_BTN_H;
        int panelH = cy - Theme::Get()->LayoutHeaderHeight();
        int btnX = issueX + (issueW - btnW) / 2;
        int btnY = Theme::Get()->LayoutHeaderHeight() + (panelH - btnH - CORRUPT_DESC_H - 10) / 2;
        m_btnCorruptInfo.SetWindowPos(&wndTop, btnX, btnY, btnW, btnH, SWP_NOACTIVATE);

        
        if (m_staticCorruptDesc.GetSafeHwnd()) {
            int descX = issueX + (issueW - CORRUPT_DESC_W) / 2;
            int descY = btnY + btnH + 10;
            m_staticCorruptDesc.SetWindowPos(NULL, descX, descY, CORRUPT_DESC_W, CORRUPT_DESC_H,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    ResizeListColumns();
}

void CTabXmlValidationDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listIssues.GetSafeHwnd()) return;

    
    CRect fileRect;
    m_listFiles.GetClientRect(&fileRect);
    m_listFiles.SetColumnWidth(0, fileRect.Width() - FILE_LIST_MARGIN);

    
    CRect issueRect;
    m_listIssues.GetClientRect(&issueRect);
    int totalWidth = issueRect.Width();

    if (totalWidth > 0) {
        
        const double colRatios[3] = { 0.30, 0.60, 0.10 };
        int w[3], sum = 0;
        for (int i = 0; i < 2; ++i) {
            w[i] = (int)(totalWidth * colRatios[i]);
            sum += w[i];
        }
        w[2] = totalWidth - sum - 3;
        for (int i = 0; i < 3; ++i)
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

void CTabXmlValidationDlg::OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;
    CString text = m_listFiles.GetItemText(pInfoTip->iItem, 0);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}





HBRUSH CTabXmlValidationDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    switch (nCtlColor) {
    case CTLCOLOR_DLG:
        pDC->SetBkColor(Theme::Get()->DialogBg());
        return m_brushDialogBg;

    case CTLCOLOR_STATIC: {
        UINT ctrlId = pWnd->GetDlgCtrlID();
        pDC->SetBkMode(TRANSPARENT);
        if (ctrlId == IDC_STATIC_VALFILES_LEFT_HEADER
            || ctrlId == IDC_STATIC_VALISSUES_HEADER)
        {
            pDC->SetTextColor(Theme::Get()->HeaderText());
            return m_brushPanelBg;
        }
        if (ctrlId == 2003) {
            pDC->SetTextColor(Theme::Get()->TextSecondary());
            return m_brushDialogBg;
        }
        pDC->SetTextColor(Theme::Get()->TextPrimary());
        return m_brushDialogBg;
    }

    case CTLCOLOR_BTN:
        pDC->SetBkColor(Theme::Get()->DialogBg());
        return m_brushDialogBg;
    }
    return hbr;
}

BOOL CTabXmlValidationDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(&rc, Theme::Get()->DialogBg());
    return TRUE;
}

void CTabXmlValidationDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF baseColor, pressedColor;
    int cornerRadius = 8;
    bool isOurs = true;

    switch (nIDCtl) {
    case IDC_BTN_VAL_VIEW_ALL:
        baseColor = Theme::Get()->BtnViewXmlBg();
        pressedColor = Theme::Get()->BtnViewXmlPrsd();
        cornerRadius = 8;
        break;
    case IDC_BTN_VAL_CORRUPT_INFO:
        baseColor = Theme::Get()->AccentRed();
        pressedColor = Theme::Get()->AccentRedPressed();
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

    pDC->FillSolidRect(&rect, Theme::Get()->DialogBg());
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





BOOL CTabXmlValidationDlg::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_MOUSEMOVE) {
        if (pMsg->hwnd == m_listFiles.GetSafeHwnd()) {
            CPoint pt = pMsg->pt; m_listFiles.ScreenToClient(&pt);
            LVHITTESTINFO hit = {pt}; m_listFiles.HitTest(&hit);
            if (hit.iItem != m_hoverFiles.index) {
                int oldIdx = m_hoverFiles.index;
                m_hoverFiles.index = hit.iItem;
                if (oldIdx != -1) m_listFiles.RedrawItems(oldIdx, oldIdx);
                if (hit.iItem != -1) m_listFiles.RedrawItems(hit.iItem, hit.iItem);
                
                if (!m_hoverFiles.isTracking) {
                    TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_listFiles.GetSafeHwnd(), 0 };
                    TrackMouseEvent(&tme);
                    m_hoverFiles.isTracking = true;
                }
            }
        } else if (pMsg->hwnd == m_listIssues.GetSafeHwnd()) {
            CPoint pt = pMsg->pt; m_listIssues.ScreenToClient(&pt);
            LVHITTESTINFO hit = {pt}; m_listIssues.HitTest(&hit);
            if (hit.iItem != m_hoverIssues.index) {
                int oldIdx = m_hoverIssues.index;
                m_hoverIssues.index = hit.iItem;
                if (oldIdx != -1) m_listIssues.RedrawItems(oldIdx, oldIdx);
                if (hit.iItem != -1) m_listIssues.RedrawItems(hit.iItem, hit.iItem);
                
                if (!m_hoverIssues.isTracking) {
                    TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_listIssues.GetSafeHwnd(), 0 };
                    TrackMouseEvent(&tme);
                    m_hoverIssues.isTracking = true;
                }
            }
        }
    } else if (pMsg->message == WM_MOUSELEAVE) {
        if (pMsg->hwnd == m_listFiles.GetSafeHwnd()) {
            m_hoverFiles.fadeIndex = m_hoverFiles.index;
            m_hoverFiles.index = -1;
            m_hoverFiles.fadeStep = 0;
            m_hoverFiles.isTracking = false;
            SetTimer(1, 30, NULL);
        } else if (pMsg->hwnd == m_listIssues.GetSafeHwnd()) {
            m_hoverIssues.fadeIndex = m_hoverIssues.index;
            m_hoverIssues.index = -1;
            m_hoverIssues.fadeStep = 0;
            m_hoverIssues.isTracking = false;
            SetTimer(2, 30, NULL);
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

void CTabXmlValidationDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1 && m_hoverFiles.fadeIndex != -1) {
        if (m_hoverFiles.fadeStep < 5) {
            m_hoverFiles.fadeStep++;
            m_listFiles.RedrawItems(m_hoverFiles.fadeIndex, m_hoverFiles.fadeIndex);
        } else {
            KillTimer(1);
            m_hoverFiles.fadeIndex = -1;
        }
    } else if (nIDEvent == 2 && m_hoverIssues.fadeIndex != -1) {
        if (m_hoverIssues.fadeStep < 5) {
            m_hoverIssues.fadeStep++;
            m_listIssues.RedrawItems(m_hoverIssues.fadeIndex, m_hoverIssues.fadeIndex);
        } else {
            KillTimer(2);
            m_hoverIssues.fadeIndex = -1;
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CTabXmlValidationDlg::OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT: {
        pCD->nmcd.uItemState &= ~CDIS_SELECTED;

        int row = (int)pCD->nmcd.dwItemSpec;
        bool isSelected = (m_listFiles.GetItemState(row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        bool isHovered = (row == m_hoverFiles.index);
        bool isFading = (row == m_hoverFiles.fadeIndex && m_hoverFiles.fadeStep < 5 && m_hoverFiles.fadeIndex != -1);
        
        COLORREF defaultBg = (row % 2 == 0) ? Theme::Get()->AltRowBg() : Theme::Get()->NormalRowBg();
        COLORREF bg = defaultBg;
        
        if (isSelected) {
            bg = Theme::Get()->SelectionBg();
        } else if (isHovered) {
            bg = Theme::Get()->HoverBg();
        } else if (isFading) {
            float t = m_hoverFiles.fadeStep / 5.0f;
            bg = RGB(
                GetRValue(Theme::Get()->HoverBg()) + (GetRValue(defaultBg) - GetRValue(Theme::Get()->HoverBg())) * t,
                GetGValue(Theme::Get()->HoverBg()) + (GetGValue(defaultBg) - GetGValue(Theme::Get()->HoverBg())) * t,
                GetBValue(Theme::Get()->HoverBg()) + (GetBValue(defaultBg) - GetBValue(Theme::Get()->HoverBg())) * t
            );
        }
        
        pCD->clrTextBk = bg;
        pCD->clrText = Theme::Get()->TextPrimary();
        *pResult = CDRF_DODEFAULT;
        break;
    }
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
        pCD->nmcd.uItemState &= ~CDIS_SELECTED;
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        return;
    }
    if (pCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
        pCD->nmcd.uItemState &= ~CDIS_SELECTED;
        int row = (int)pCD->nmcd.dwItemSpec;
        int displayIdx = (int)pCD->nmcd.lItemlParam;
        int subItem = pCD->iSubItem;

        bool isSelected = (m_listIssues.GetItemState(row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        bool isHovered = (row == m_hoverIssues.index);
        bool isFading = (row == m_hoverIssues.fadeIndex && m_hoverIssues.fadeStep < 5 && m_hoverIssues.fadeIndex != -1);
        
        if (subItem == COL_VIEW) { isSelected = false; isHovered = false; isFading = false; }

        COLORREF defaultBg = Theme::Get()->DialogBg();
        COLORREF bg = defaultBg;
        
        if (isSelected) {
            bg = Theme::Get()->SelectionBg();
        } else if (isHovered) {
            bg = Theme::Get()->HoverBg();
        } else if (isFading) {
            float t = m_hoverIssues.fadeStep / 5.0f;
            bg = RGB(
                GetRValue(Theme::Get()->HoverBg()) + (GetRValue(defaultBg) - GetRValue(Theme::Get()->HoverBg())) * t,
                GetGValue(Theme::Get()->HoverBg()) + (GetGValue(defaultBg) - GetGValue(Theme::Get()->HoverBg())) * t,
                GetBValue(Theme::Get()->HoverBg()) + (GetBValue(defaultBg) - GetBValue(Theme::Get()->HoverBg())) * t
            );
        }

        COLORREF txt = Theme::Get()->TextPrimary();

        if (displayIdx >= 0 && subItem == COL_VIEW) {
            
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
            CRect rect;
            m_listIssues.GetSubItemRect(row, subItem, LVIR_BOUNDS, rect);

            pDC->FillSolidRect(&rect, bg);
            rect.DeflateRect(12, 6);

            COLORREF btnColor = Theme::Get()->AccentBlue();
            CBrush brush(btnColor);
            CPen pen(PS_SOLID, 1, btnColor);
            CBrush* pOldBrush = pDC->SelectObject(&brush);
            CPen* pOldPen = pDC->SelectObject(&pen);
            pDC->RoundRect(&rect, CPoint(8, 8));

            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255, 255, 255));

            LOGFONT lfIcon = {0};
            wcscpy_s(lfIcon.lfFaceName, _T("Segoe MDL2 Assets"));
            lfIcon.lfHeight = Theme::Get()->FontSizeIcon();
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

        if (subItem == COL_KEY) {
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
            CRect rect;
            m_listIssues.GetSubItemRect(row, subItem, LVIR_LABEL, rect);
            pDC->FillSolidRect(&rect, bg);
            rect.DeflateRect(6, 4);

            CString text = m_listIssues.GetItemText(row, subItem);
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(txt);
            CFont* pOldFont = pDC->SelectObject(m_listIssues.GetFont());
            pDC->DrawText(text, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
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
