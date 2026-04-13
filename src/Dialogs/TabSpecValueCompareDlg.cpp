#include "pch.h"
#include "TabSpecValueCompareDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CTabSpecValueCompareDlg, CDialogEx)
    ON_WM_SIZE()
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
    lf.lfHeight = -14;
    m_uiFont.CreateFontIndirect(&lf);

    lf.lfWeight = FW_BOLD;
    m_headerFont.CreateFontIndirect(&lf);

    m_staticFilesHeader.SetFont(&m_headerFont);
    m_staticMismatchHeader.SetFont(&m_headerFont);
    m_listFiles.SetFont(&m_uiFont);
    m_listMismatches.SetFont(&m_uiFont);

    m_brushDialogBg.CreateSolidBrush(CLR_DIALOG_BG);

    m_listFiles.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_listMismatches.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

    SetupListColumns();
    return TRUE;
}

void CTabSpecValueCompareDlg::SetupListColumns() {
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 200);
    m_listFiles.InsertColumn(1, _T("Diffs"), LVCFMT_RIGHT, 50);

    m_listMismatches.InsertColumn(0, _T("Composite Key"), LVCFMT_LEFT, 160);
    m_listMismatches.InsertColumn(1, _T("Group"), LVCFMT_LEFT, 100);
    m_listMismatches.InsertColumn(2, _T("Spec"), LVCFMT_LEFT, 120);
    m_listMismatches.InsertColumn(3, _T("Val Name"), LVCFMT_LEFT, 120);
    m_listMismatches.InsertColumn(4, _T("Property"), LVCFMT_LEFT, 100);
    m_listMismatches.InsertColumn(5, _T("Left Value"), LVCFMT_LEFT, 120);
    m_listMismatches.InsertColumn(6, _T("Right Value"), LVCFMT_LEFT, 120);
}

void CTabSpecValueCompareDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listMismatches.GetSafeHwnd()) return;

    CRect rectFiles;
    m_listFiles.GetClientRect(&rectFiles);
    int totalW = rectFiles.Width();
    if (totalW > 0) {
        m_listFiles.SetColumnWidth(0, totalW - 60);
        m_listFiles.SetColumnWidth(1, 50);
    }

    CRect rectMiss;
    m_listMismatches.GetClientRect(&rectMiss);
    int w = rectMiss.Width();
    if (w > 0) {
        int scrollBarW = GetSystemMetrics(SM_CXVSCROLL);
        int available = w - scrollBarW;
        int proportions[] = {18, 12, 14, 14, 12, 15, 15};
        int totalProp = 0;
        for (int p : proportions) totalProp += p;
        for (int i = 0; i < 7; ++i) {
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

    const double LEFT_PANEL_RATIO = 0.20;
    const int M = 10;
    const int HEADER_H = 22;
    const int GAP = 6;

    int fileListW = (int)((cx - GAP) * LEFT_PANEL_RATIO);
    if (fileListW < 150) fileListW = 150;

    int rightX = fileListW + GAP;
    int rightW = cx - rightX;
    if (rightW < 100) rightW = 100;

    int listH = cy - HEADER_H;
    if (listH < 50) listH = 50;

    HDWP hDwp = ::BeginDeferWindowPos(4);
    if (!hDwp) return;

    hDwp = ::DeferWindowPos(hDwp, m_staticFilesHeader.GetSafeHwnd(), NULL,
        0, 0, fileListW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listFiles.GetSafeHwnd(), NULL,
        0, HEADER_H, fileListW, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    hDwp = ::DeferWindowPos(hDwp, m_staticMismatchHeader.GetSafeHwnd(), NULL,
        rightX, 0, rightW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listMismatches.GetSafeHwnd(), NULL,
        rightX, HEADER_H, rightW, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    ::EndDeferWindowPos(hDwp);
    ResizeListColumns();
}

HBRUSH CTabSpecValueCompareDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        UINT id = pWnd->GetDlgCtrlID();
        if (id == IDC_STATIC_VC_FILES_HEADER || id == IDC_STATIC_VC_MISMATCH_HEADER) {
            pDC->SetTextColor(CLR_HEADER_TEXT);
        } else {
            pDC->SetTextColor(CLR_TEXT_PRIMARY);
        }
        return m_brushDialogBg;
    }
    return hbr;
}

BOOL CTabSpecValueCompareDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rect;
    GetClientRect(&rect);
    pDC->FillSolidRect(&rect, CLR_DIALOG_BG);
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
    } else if (m_listFiles.GetSelectedCount() == 0) {
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

void CTabSpecValueCompareDlg::OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pLVCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT:
        pLVCD->clrTextBk = (pLVCD->nmcd.dwItemSpec % 2 == 0) ? RGB(250, 250, 255) : RGB(255, 255, 255);
        pLVCD->clrText = CLR_TEXT_PRIMARY;
        *pResult = CDRF_DODEFAULT;
        break;
    }
}

void CTabSpecValueCompareDlg::OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pLVCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT:
        pLVCD->clrTextBk = (pLVCD->nmcd.dwItemSpec % 2 == 0) ? RGB(250, 250, 255) : RGB(255, 255, 255);
        pLVCD->clrText = CLR_TEXT_PRIMARY;
        *pResult = CDRF_DODEFAULT;
        break;
    }
}
