#include "pch.h"
#include "TabSpecValueCompareDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Theme.h"

// UI Metrics & Dimensions
#define FILE_LIST_COL_FILE_W     200
#define FILE_LIST_COL_DIFFS_W    50

#define MISS_COL_KEY_W           160
#define MISS_COL_GROUP_W         100
#define MISS_COL_SPEC_W          120
#define MISS_COL_VALNAME_W       120
#define MISS_COL_PROP_W          100
#define MISS_COL_LEFT_W          120
#define MISS_COL_RIGHT_W         120

#define DIFFS_COL_OFFSET         60

#define PROP_COMPOSITE_KEY       18
#define PROP_GROUP               12
#define PROP_SPEC                14
#define PROP_VAL_NAME            14
#define PROP_PROPERTY            12
#define PROP_LEFT_VAL            15
#define PROP_RIGHT_VAL           15
#define TOTAL_PROP_COUNT         7

#define LAYOUT_LEFT_PANEL_RATIO  0.20
#define MIN_FILE_LIST_WIDTH      150
#define MIN_RIGHT_PANEL_WIDTH    100
#define MIN_LIST_HEIGHT          50

BEGIN_MESSAGE_MAP(CTabSpecValueCompareDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VC_FILES, &CTabSpecValueCompareDlg::OnFileListItemChanged)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VC_FILES, &CTabSpecValueCompareDlg::OnFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VC_MISMATCHES, &CTabSpecValueCompareDlg::OnMismatchListCustomDraw)
END_MESSAGE_MAP()

CTabSpecValueCompareDlg::CTabSpecValueCompareDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_SPEC_VALUE_COMPARE, pParent) {}

void CTabSpecValueCompareDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_VC_FILES_HEADER,    m_staticFilesHeader);
    DDX_Control(pDX, IDC_LIST_VC_FILES,              m_listFiles);
    DDX_Control(pDX, IDC_STATIC_VC_MISMATCH_HEADER,  m_staticMismatchHeader);
    DDX_Control(pDX, IDC_LIST_VC_MISMATCHES,          m_listMismatches);
}

BOOL CTabSpecValueCompareDlg::OnInitDialog() {
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

    m_staticFilesHeader.SetFont(&m_headerFont);
    m_staticMismatchHeader.SetFont(&m_headerFont);
    m_listFiles.SetFont(&m_uiFont);
    m_listMismatches.SetFont(&m_uiFont);

    m_brushDialogBg.CreateSolidBrush(Theme::Get()->DialogBg());
    m_brushPanelBg.CreateSolidBrush(Theme::Get()->PanelBg());

    m_listFiles.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_listMismatches.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

    m_listFiles.ModifyStyle(0, LVS_SINGLESEL);
    m_listMismatches.ModifyStyle(0, LVS_SINGLESEL);

    SetupListColumns();
    return TRUE;
}

void CTabSpecValueCompareDlg::SetupListColumns() {
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, FILE_LIST_COL_FILE_W);
    m_listFiles.InsertColumn(1, _T("Diffs"), LVCFMT_RIGHT, FILE_LIST_COL_DIFFS_W);

    m_listMismatches.InsertColumn(0, _T("Composite Key"), LVCFMT_LEFT, MISS_COL_KEY_W);
    m_listMismatches.InsertColumn(1, _T("Group"), LVCFMT_LEFT, MISS_COL_GROUP_W);
    m_listMismatches.InsertColumn(2, _T("Spec"), LVCFMT_LEFT, MISS_COL_SPEC_W);
    m_listMismatches.InsertColumn(3, _T("Val Name"), LVCFMT_LEFT, MISS_COL_VALNAME_W);
    m_listMismatches.InsertColumn(4, _T("Property"), LVCFMT_LEFT, MISS_COL_PROP_W);
    m_listMismatches.InsertColumn(5, _T("Left Value"), LVCFMT_LEFT, MISS_COL_LEFT_W);
    m_listMismatches.InsertColumn(6, _T("Right Value"), LVCFMT_LEFT, MISS_COL_RIGHT_W);
}

void CTabSpecValueCompareDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listMismatches.GetSafeHwnd()) return;

    CRect rectFiles;
    m_listFiles.GetClientRect(&rectFiles);
    int totalW = rectFiles.Width();
    if (totalW > 0) {
        m_listFiles.SetColumnWidth(0, totalW - DIFFS_COL_OFFSET);
        m_listFiles.SetColumnWidth(1, FILE_LIST_COL_DIFFS_W);
    }

    CRect rectMiss;
    m_listMismatches.GetClientRect(&rectMiss);
    int w = rectMiss.Width();
    if (w > 0) {
        int scrollBarW = GetSystemMetrics(SM_CXVSCROLL);
        int available = w - scrollBarW;
        int proportions[] = {PROP_COMPOSITE_KEY, PROP_GROUP, PROP_SPEC, PROP_VAL_NAME, PROP_PROPERTY, PROP_LEFT_VAL, PROP_RIGHT_VAL};
        int totalProp = 0;
        for (int p : proportions) totalProp += p;
        for (int i = 0; i < TOTAL_PROP_COUNT; ++i) {
            m_listMismatches.SetColumnWidth(i, available * proportions[i] / totalProp);
        }
    }
}

void CTabSpecValueCompareDlg::SetReport(std::shared_ptr<ModelCompare::ValueDiffReport> report) {
    m_report = report;
    m_selectedFileIndex = -1;
    ClearMismatchList();
    PopulateFileList();
}

void CTabSpecValueCompareDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (!m_listFiles.GetSafeHwnd()) return;

    int fileListW = (int)((cx - Theme::Get()->LayoutGap()) * LAYOUT_LEFT_PANEL_RATIO);
    if (fileListW < MIN_FILE_LIST_WIDTH) fileListW = MIN_FILE_LIST_WIDTH;

    int rightX = fileListW + Theme::Get()->LayoutGap();
    int rightW = cx - rightX;
    if (rightW < MIN_RIGHT_PANEL_WIDTH) rightW = MIN_RIGHT_PANEL_WIDTH;

    int listH = cy - Theme::Get()->LayoutHeaderHeight();
    if (listH < MIN_LIST_HEIGHT) listH = MIN_LIST_HEIGHT;

    HDWP hDwp = ::BeginDeferWindowPos(4);
    if (!hDwp) return;

    hDwp = ::DeferWindowPos(hDwp, m_staticFilesHeader.GetSafeHwnd(), NULL,
        0, 0, fileListW, Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listFiles.GetSafeHwnd(), NULL,
        0, Theme::Get()->LayoutHeaderHeight(), fileListW, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    hDwp = ::DeferWindowPos(hDwp, m_staticMismatchHeader.GetSafeHwnd(), NULL,
        rightX, 0, rightW, Theme::Get()->LayoutHeaderHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listMismatches.GetSafeHwnd(), NULL,
        rightX, Theme::Get()->LayoutHeaderHeight(), rightW, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    ::EndDeferWindowPos(hDwp);
    ResizeListColumns();
}

HBRUSH CTabSpecValueCompareDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        UINT id = pWnd->GetDlgCtrlID();
        if (id == IDC_STATIC_VC_FILES_HEADER || id == IDC_STATIC_VC_MISMATCH_HEADER) {
            pDC->SetTextColor(Theme::Get()->HeaderText());
        } else {
            pDC->SetTextColor(Theme::Get()->TextPrimary());
        }
        return m_brushPanelBg;
    }
    return hbr;
}

BOOL CTabSpecValueCompareDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rect;
    GetClientRect(&rect);
    pDC->FillSolidRect(&rect, Theme::Get()->DialogBg());
    return TRUE;
}

void CTabSpecValueCompareDlg::PopulateFileList() {
    m_listFiles.SetRedraw(FALSE);
    m_listFiles.DeleteAllItems();

    if (m_report) {
        int idx = 0;
        for (const auto& f : m_report->fileResults) {
            CString relPath = f.relativePath.wstring().c_str();
            m_listFiles.InsertItem(idx, relPath);

            size_t totalPropDiffs = 0;
            for (const auto& d : f.differences)
                totalPropDiffs += d.differingProperties.size();

            CString diffCount;
            diffCount.Format(_T("%zu"), totalPropDiffs);
            m_listFiles.SetItemText(idx, 1, diffCount);
            idx++;
        }
    }
    m_listFiles.SetRedraw(TRUE);
}

void CTabSpecValueCompareDlg::OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED)) {
        if (pNMLV->iItem != m_selectedFileIndex) {
            m_selectedFileIndex = pNMLV->iItem;
            PopulateMismatchList(m_selectedFileIndex);
        }
    } else if ((pNMLV->uChanged & LVIF_STATE) && m_listFiles.GetSelectedCount() == 0 && m_selectedFileIndex != -1) {
        m_selectedFileIndex = -1;
        ClearMismatchList();
    }
    *pResult = 0;
}

void CTabSpecValueCompareDlg::ClearMismatchList() {
    m_listMismatches.SetRedraw(FALSE);
    m_listMismatches.DeleteAllItems();
    m_listMismatches.SetRedraw(TRUE);
}

void CTabSpecValueCompareDlg::PopulateMismatchList(int fileIndex) {
    ClearMismatchList();
    if (!m_report || fileIndex < 0 || fileIndex >= (int)m_report->fileResults.size()) return;

    m_listMismatches.SetRedraw(FALSE);

    const auto& f = m_report->fileResults[fileIndex];
    int rowIdx = 0;

    for (const auto& entry : f.differences) {
        for (const auto& prop : entry.differingProperties) {
            m_listMismatches.InsertItem(rowIdx, CString(entry.compositeKey.c_str()));
            m_listMismatches.SetItemText(rowIdx, 1, CString(entry.groupName.c_str()));
            m_listMismatches.SetItemText(rowIdx, 2, CString(entry.specName.c_str()));
            m_listMismatches.SetItemText(rowIdx, 3, CString(entry.valName.c_str()));
            m_listMismatches.SetItemText(rowIdx, 4, CString(prop.propertyName.c_str()));
            m_listMismatches.SetItemText(rowIdx, 5, CString(prop.leftValue.c_str()));
            m_listMismatches.SetItemText(rowIdx, 6, CString(prop.rightValue.c_str()));
            rowIdx++;
        }
    }

    m_listMismatches.SetRedraw(TRUE);
}

BOOL CTabSpecValueCompareDlg::PreTranslateMessage(MSG* pMsg) {
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
        } else if (pMsg->hwnd == m_listMismatches.GetSafeHwnd()) {
            CPoint pt = pMsg->pt; m_listMismatches.ScreenToClient(&pt);
            LVHITTESTINFO hit = {pt}; m_listMismatches.HitTest(&hit);
            if (hit.iItem != m_hoverMiss.index) {
                int oldIdx = m_hoverMiss.index;
                m_hoverMiss.index = hit.iItem;
                if (oldIdx != -1) m_listMismatches.RedrawItems(oldIdx, oldIdx);
                if (hit.iItem != -1) m_listMismatches.RedrawItems(hit.iItem, hit.iItem);
                
                if (!m_hoverMiss.isTracking) {
                    TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_listMismatches.GetSafeHwnd(), 0 };
                    TrackMouseEvent(&tme);
                    m_hoverMiss.isTracking = true;
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
        } else if (pMsg->hwnd == m_listMismatches.GetSafeHwnd()) {
            m_hoverMiss.fadeIndex = m_hoverMiss.index;
            m_hoverMiss.index = -1;
            m_hoverMiss.fadeStep = 0;
            m_hoverMiss.isTracking = false;
            SetTimer(2, 30, NULL);
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

void CTabSpecValueCompareDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1 && m_hoverFiles.fadeIndex != -1) {
        if (m_hoverFiles.fadeStep < 5) {
            m_hoverFiles.fadeStep++;
            m_listFiles.RedrawItems(m_hoverFiles.fadeIndex, m_hoverFiles.fadeIndex);
        } else {
            KillTimer(1);
            m_hoverFiles.fadeIndex = -1;
        }
    } else if (nIDEvent == 2 && m_hoverMiss.fadeIndex != -1) {
        if (m_hoverMiss.fadeStep < 5) {
            m_hoverMiss.fadeStep++;
            m_listMismatches.RedrawItems(m_hoverMiss.fadeIndex, m_hoverMiss.fadeIndex);
        } else {
            KillTimer(2);
            m_hoverMiss.fadeIndex = -1;
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CTabSpecValueCompareDlg::OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
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

void CTabSpecValueCompareDlg::OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT: {
        pCD->nmcd.uItemState &= ~CDIS_SELECTED;

        int row = (int)pCD->nmcd.dwItemSpec;
        bool isSelected = (m_listMismatches.GetItemState(row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        bool isHovered = (row == m_hoverMiss.index);
        bool isFading = (row == m_hoverMiss.fadeIndex && m_hoverMiss.fadeStep < 5 && m_hoverMiss.fadeIndex != -1);
        
        COLORREF defaultBg = (row % 2 == 0) ? Theme::Get()->AltRowBg() : Theme::Get()->NormalRowBg();
        COLORREF bg = defaultBg;
        
        if (isSelected) {
            bg = Theme::Get()->SelectionBg();
        } else if (isHovered) {
            bg = Theme::Get()->HoverBg();
        } else if (isFading) {
            float t = m_hoverMiss.fadeStep / 5.0f;
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
