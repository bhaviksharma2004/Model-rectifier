// =============================================================================
// MainDlg.cpp — Redesigned two-panel layout implementation.
// =============================================================================
#include "pch.h"
#include "resource.h"
#include "MainDlg.h"
#include "XmlViewerDlg.h"
#include "Engine/CompareEngine.h"
#include "Engine/StructuralIdComparator.h"
#include "Engine/XmlApplyEngine.h"
#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Column indices for the mismatch list
enum MismatchCol { COL_KEY = 0, COL_GROUP, COL_SPEC, COL_VALNAME, COL_VIEW, COL_APPLY };

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LEFT,  &CMainDlg::OnBnClickedBrowseLeft)
    ON_BN_CLICKED(IDC_BTN_BROWSE_RIGHT, &CMainDlg::OnBnClickedBrowseRight)
    ON_BN_CLICKED(IDC_BTN_COMPARE,      &CMainDlg::OnBnClickedCompare)
    ON_BN_CLICKED(IDC_BTN_APPLY_ALL,    &CMainDlg::OnBnClickedApplyAll)
    ON_BN_CLICKED(IDC_BTN_VIEW_XML,     &CMainDlg::OnBnClickedViewXml)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES,      &CMainDlg::OnFileListItemChanged)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_FILES,      &CMainDlg::OnFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_MISMATCHES, &CMainDlg::OnMismatchListCustomDraw)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_MISMATCHES, &CMainDlg::OnMismatchListClick)
    ON_MESSAGE(WM_COMPARE_COMPLETE, &CMainDlg::OnCompareComplete)
END_MESSAGE_MAP()

CMainDlg::CMainDlg(CWnd* pParent)
    : CDialogEx(IDD_MODELCOMPARE_DIALOG, pParent) {}

void CMainDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_LEFT_PATH,        m_editLeftPath);
    DDX_Control(pDX, IDC_EDIT_RIGHT_PATH,       m_editRightPath);
    DDX_Control(pDX, IDC_BTN_BROWSE_LEFT,       m_btnBrowseLeft);
    DDX_Control(pDX, IDC_BTN_BROWSE_RIGHT,      m_btnBrowseRight);
    DDX_Control(pDX, IDC_BTN_COMPARE,           m_btnCompare);
    DDX_Control(pDX, IDC_STATIC_SUMMARY,        m_staticSummary);
    DDX_Control(pDX, IDC_STATIC_FILES_HEADER,   m_staticFilesHeader);
    DDX_Control(pDX, IDC_LIST_FILES,            m_listFiles);
    DDX_Control(pDX, IDC_STATIC_MISMATCH_HEADER,m_staticMismatchHeader);
    DDX_Control(pDX, IDC_BTN_APPLY_ALL,         m_btnApplyAll);
    DDX_Control(pDX, IDC_BTN_VIEW_XML,          m_btnViewXml);
    DDX_Control(pDX, IDC_LIST_MISMATCHES,       m_listMismatches);
}

BOOL CMainDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    SetWindowText(_T("LAI Model Compare Tool"));
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), TRUE);
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), FALSE);

    SetupListColumns();
    UpdateSummary(_T("Select two model folders and click Compare."));
    m_staticFilesHeader.SetWindowText(_T("Diff Files"));
    m_staticMismatchHeader.SetWindowText(_T("ID Mismatches"));
    m_btnApplyAll.EnableWindow(FALSE);
    m_btnViewXml.EnableWindow(FALSE);

    // Create a modern UI font (Segoe UI)
    LOGFONT lf = {};
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
    wcscpy_s(lf.lfFaceName, _T("Segoe UI"));
    lf.lfHeight = -14; // Slightly larger for better readability
    m_uiFont.CreateFontIndirect(&lf);

    // Apply font to controls
    m_listFiles.SetFont(&m_uiFont);
    m_listMismatches.SetFont(&m_uiFont);
    m_staticSummary.SetFont(&m_uiFont);
    m_staticFilesHeader.SetFont(&m_uiFont);
    m_staticMismatchHeader.SetFont(&m_uiFont);
    m_editLeftPath.SetFont(&m_uiFont);
    m_editRightPath.SetFont(&m_uiFont);

    // Apply Explorer Theme to lists
    ::SetWindowTheme(m_listFiles.GetSafeHwnd(), L"Explorer", NULL);
    ::SetWindowTheme(m_listMismatches.GetSafeHwnd(), L"Explorer", NULL);

    // Size window to fill ~85% of screen with no wasted space
    CRect workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int w = (int)(workArea.Width() * 0.85);
    int h = (int)(workArea.Height() * 0.85);
    SetWindowPos(nullptr,
        (workArea.Width() - w) / 2 + workArea.left,
        (workArea.Height() - h) / 2 + workArea.top,
        w, h, SWP_NOZORDER);

    return TRUE;
}

void CMainDlg::SetupListColumns() {
    // File list: single column
    DWORD fStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    m_listFiles.SetExtendedStyle(m_listFiles.GetExtendedStyle() | fStyle);
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 260);

    // Mismatch list: Key, Group, Spec, Val Name, View, Apply
    DWORD mStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
    m_listMismatches.SetExtendedStyle(m_listMismatches.GetExtendedStyle() | mStyle);
    m_listMismatches.InsertColumn(COL_KEY,     _T("Composite Key"), LVCFMT_LEFT,   165);
    m_listMismatches.InsertColumn(COL_GROUP,   _T("Group"),         LVCFMT_LEFT,   130);
    m_listMismatches.InsertColumn(COL_SPEC,    _T("Spec"),          LVCFMT_LEFT,   150);
    m_listMismatches.InsertColumn(COL_VALNAME, _T("Val Name"),      LVCFMT_LEFT,   150);
    m_listMismatches.InsertColumn(COL_VIEW,    _T("View"),          LVCFMT_CENTER, 50);
    m_listMismatches.InsertColumn(COL_APPLY,   _T("Apply"),         LVCFMT_CENTER, 55);
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

// =============================================================================
// Compare
// =============================================================================
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
    UpdateSummary(_T("Comparing..."));
    m_listFiles.DeleteAllItems();
    ClearMismatchList();

    RunCompare();
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
    PopulateFileList();
    EnableCompareUI(true);
    return 0;
}

// =============================================================================
// File List
// =============================================================================
void CMainDlg::PopulateFileList() {
    m_listFiles.DeleteAllItems();
    ClearMismatchList();
    m_selectedFileIndex = -1;
    if (!m_report) return;

    for (int i = 0; i < (int)m_report->fileResults.size(); i++) {
        const auto& fr = m_report->fileResults[i];
        CString name(fr.relativePath.filename().wstring().c_str());
        int idx = m_listFiles.InsertItem(i, name);
        m_listFiles.SetItemData(idx, (DWORD_PTR)i);
    }
}

void CMainDlg::OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNM = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if (!(pNM->uChanged & LVIF_STATE) || !(pNM->uNewState & LVIS_SELECTED)) return;
    int fi = (int)m_listFiles.GetItemData(pNM->iItem);
    PopulateMismatchList(fi);
}

// =============================================================================
// Mismatch List
// =============================================================================
void CMainDlg::PopulateMismatchList(int fileIndex) {
    ClearMismatchList();
    m_selectedFileIndex = fileIndex;

    if (!m_report || fileIndex < 0 || fileIndex >= (int)m_report->fileResults.size()) {
        m_btnApplyAll.EnableWindow(FALSE);
        m_btnViewXml.EnableWindow(FALSE);
        return;
    }

    const auto& fr = m_report->fileResults[fileIndex];

    // Update header
    CString header;
    header.Format(_T("ID Mismatches \u2014 %s"),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());
    m_staticMismatchHeader.SetWindowText(header);

    // For deleted/added files, just show status
    if (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight ||
        fr.status == ModelCompare::FileDiffResult::Status::AddedInRight) {
        CString statusMsg = (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight)
            ? _T("File exists only in Reference (Left) model")
            : _T("File exists only in Target (Right) model");
        int idx = m_listMismatches.InsertItem(0, statusMsg);
        m_listMismatches.SetItemData(idx, (DWORD_PTR)-1);
        m_btnApplyAll.EnableWindow(FALSE);
        m_btnViewXml.EnableWindow(FALSE);
        return;
    }

    // Build merged display list
    for (const auto& e : fr.missingInRight) {
        m_displayDiffs.push_back({ true, e });
    }
    for (const auto& e : fr.extraInRight) {
        m_displayDiffs.push_back({ false, e });
    }

    // Populate list
    for (int i = 0; i < (int)m_displayDiffs.size(); i++) {
        const auto& d = m_displayDiffs[i];
        int idx = m_listMismatches.InsertItem(i, CString(d.entry.compositeKey.c_str()));
        m_listMismatches.SetItemText(idx, COL_GROUP,   CString(d.entry.groupName.c_str()));
        m_listMismatches.SetItemText(idx, COL_SPEC,    CString(d.entry.specName.c_str()));
        m_listMismatches.SetItemText(idx, COL_VALNAME,  CString(d.entry.valName.c_str()));
        m_listMismatches.SetItemText(idx, COL_VIEW,    _T("\xD83D\xDD0D"));  // magnifying glass
        m_listMismatches.SetItemText(idx, COL_APPLY,   _T("\u2714"));  // checkmark
        m_listMismatches.SetItemData(idx, (DWORD_PTR)i);
    }

    m_btnApplyAll.EnableWindow(!m_displayDiffs.empty());
    m_btnViewXml.EnableWindow(TRUE);
}

void CMainDlg::ClearMismatchList() {
    m_listMismatches.DeleteAllItems();
    m_displayDiffs.clear();
}

// =============================================================================
// Click on View/Apply columns in mismatch list
// =============================================================================
void CMainDlg::OnMismatchListClick(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNMItem = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    *pResult = 0;

    LVHITTESTINFO hitInfo = {};
    hitInfo.pt = pNMItem->ptAction;
    m_listMismatches.SubItemHitTest(&hitInfo);

    if (hitInfo.iItem < 0) return;
    int displayIdx = (int)m_listMismatches.GetItemData(hitInfo.iItem);
    if (displayIdx < 0 || displayIdx >= (int)m_displayDiffs.size()) return;

    if (hitInfo.iSubItem == COL_VIEW) {
        const auto& d = m_displayDiffs[displayIdx];
        CString searchStr;
        if (!d.entry.valId.empty()) {
            searchStr.Format(_T("val_id=\"%s\""), CString(d.entry.valId.c_str()).GetString());
        }
        OpenXmlViewer(d.entry.compositeKey, d.isMissingInRight, searchStr);
    } else if (hitInfo.iSubItem == COL_APPLY) {
        ApplySingleDiff(displayIdx);
    }
}

// =============================================================================
// View in XML
// =============================================================================
void CMainDlg::OnBnClickedViewXml() {
    OpenXmlViewer();
}

void CMainDlg::OpenXmlViewer(const std::string& scrollToKey, bool isMissing, const CString& targetSearch) {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    // Show the Right file
    auto rightPath = GetRightFilePath(m_selectedFileIndex);
    if (rightPath.empty() || !std::filesystem::exists(rightPath)) {
        AfxMessageBox(_T("Right model file not found."), MB_ICONWARNING);
        return;
    }

    CString title;
    title.Format(_T("XML Viewer \u2014 %s (Right Model)"),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());

    CXmlViewerDlg dlg(this);
    dlg.SetFile(rightPath, title);
    dlg.SetDiffs(fr.missingInRight, fr.extraInRight);
    if (!scrollToKey.empty()) {
        dlg.SetScrollToKey(scrollToKey, isMissing);
    }
    if (!targetSearch.IsEmpty()) {
        dlg.SetSearchTarget(targetSearch);
    }
    dlg.DoModal();
}

// =============================================================================
// Apply
// =============================================================================
void CMainDlg::OnBnClickedApplyAll() {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    CString msg;
    msg.Format(_T("Apply all %zu diff(s) to the Right model file?\n\n%s"),
        fr.DiffCount(),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto leftPath = GetLeftFilePath(m_selectedFileIndex);
    auto rightPath = GetRightFilePath(m_selectedFileIndex);

    auto result = ModelCompare::XmlApplyEngine::ApplyAllDiffs(
        leftPath, rightPath, fr.missingInRight, fr.extraInRight);

    if (result.success) {
        AfxMessageBox(_T("Applied successfully. Re-comparing..."), MB_ICONINFORMATION);
        RunCompare();
    } else {
        CString err;
        err.Format(_T("Apply failed: %s"), CString(result.errorMessage.c_str()).GetString());
        AfxMessageBox(err, MB_ICONERROR);
    }
}

void CMainDlg::ApplySingleDiff(int displayIndex) {
    if (displayIndex < 0 || displayIndex >= (int)m_displayDiffs.size()) return;
    if (m_selectedFileIndex < 0 || !m_report) return;

    const auto& d = m_displayDiffs[displayIndex];
    CString action = d.isMissingInRight ? _T("ADD missing") : _T("REMOVE extra");
    CString msg;
    msg.Format(_T("%s ID: %s ?"), action.GetString(),
        CString(d.entry.compositeKey.c_str()).GetString());

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto leftPath = GetLeftFilePath(m_selectedFileIndex);
    auto rightPath = GetRightFilePath(m_selectedFileIndex);

    ModelCompare::XmlApplyEngine::ApplyResult result;
    if (d.isMissingInRight) {
        result = ModelCompare::XmlApplyEngine::AddMissingVal(leftPath, rightPath, d.entry);
    } else {
        result = ModelCompare::XmlApplyEngine::RemoveExtraVal(rightPath, d.entry);
    }

    if (result.success) {
        AfxMessageBox(_T("Applied. Re-comparing..."), MB_ICONINFORMATION);
        RunCompare();
    } else {
        CString err;
        err.Format(_T("Apply failed: %s"), CString(result.errorMessage.c_str()).GetString());
        AfxMessageBox(err, MB_ICONERROR);
    }
}

// =============================================================================
// Path helpers
// =============================================================================
std::filesystem::path CMainDlg::GetLeftFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->leftModelPath) / m_report->fileResults[fileIndex].relativePath;
}

std::filesystem::path CMainDlg::GetRightFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->rightModelPath) / m_report->fileResults[fileIndex].relativePath;
}

// =============================================================================
// UI Helpers
// =============================================================================
void CMainDlg::EnableCompareUI(bool enable) {
    m_btnBrowseLeft.EnableWindow(enable);
    m_btnBrowseRight.EnableWindow(enable);
    m_btnCompare.EnableWindow(enable);
}

void CMainDlg::UpdateSummary(const CString& text) {
    m_staticSummary.SetWindowText(text);
}

void CMainDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI) {
    lpMMI->ptMinTrackSize.x = 900;
    lpMMI->ptMinTrackSize.y = 500;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CMainDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (cx > 0 && cy > 0) RepositionControls(cx, cy);
}

// =============================================================================
// Layout — compact top, two panels below filling all space
// =============================================================================
void CMainDlg::RepositionControls(int cx, int cy) {
    if (!m_editLeftPath.GetSafeHwnd()) return;

    const int M = 8;        // margin
    const int ROW_H = 20;   // row height
    const int BTN_W = 60;   // browse button width
    const int GAP = 4;
    const int LBL_W = 128;
    const int CMP_W = 100;  // compare button width

    int editW = cx - M - LBL_W - GAP - BTN_W - GAP - CMP_W - M;
    if (editW < 100) editW = 100;
    int y = M;

    // Row 1: Left path
    CWnd* pLL = GetDlgItem(IDC_STATIC_LEFT_LABEL);
    if (pLL) pLL->MoveWindow(M, y + 2, LBL_W, ROW_H);
    m_editLeftPath.MoveWindow(M + LBL_W + GAP, y, editW, ROW_H);
    m_btnBrowseLeft.MoveWindow(M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H);
    int compareX = cx - M - CMP_W;
    int compareY = y;
    y += ROW_H + GAP;

    // Row 2: Right path
    CWnd* pRL = GetDlgItem(IDC_STATIC_RIGHT_LABEL);
    if (pRL) pRL->MoveWindow(M, y + 2, LBL_W, ROW_H);
    m_editRightPath.MoveWindow(M + LBL_W + GAP, y, editW, ROW_H);
    m_btnBrowseRight.MoveWindow(M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H);
    y += ROW_H + GAP;

    // Compare button: spans both rows, right-aligned
    int compareH = (y - compareY) - GAP;
    m_btnCompare.MoveWindow(compareX, compareY, CMP_W, compareH);

    // Summary
    m_staticSummary.MoveWindow(M, y, cx - 2 * M, 16);
    y += 16 + GAP;

    // Two panels below: file list (~25%) | mismatch list (~75%)
    int panelH = cy - y - M;
    if (panelH < 100) panelH = 100;
    int fileListW = (int)((cx - 2 * M - GAP) * 0.25);
    if (fileListW < 150) fileListW = 150;
    int mismatchX = M + fileListW + GAP;
    int mismatchW = cx - mismatchX - M;

    // File list panel
    m_staticFilesHeader.MoveWindow(M, y, fileListW, 16);
    m_listFiles.MoveWindow(M, y + 16 + 2, fileListW, panelH - 18);

    // Mismatch panel
    m_staticMismatchHeader.MoveWindow(mismatchX, y, mismatchW, 16);
    int btnY = y + 16 + 2;
    int applyAllW = 130;
    int viewXmlW = 90;
    m_btnApplyAll.MoveWindow(mismatchX, btnY, applyAllW, 24);
    m_btnViewXml.MoveWindow(mismatchX + applyAllW + GAP, btnY, viewXmlW, 24);

    int listY = btnY + 24 + 2;
    int listH = panelH - (listY - y) - 2;
    if (listH < 50) listH = 50;
    m_listMismatches.MoveWindow(mismatchX, listY, mismatchW, listH);

    ResizeListColumns();
}

void CMainDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listMismatches.GetSafeHwnd()) return;

    // File List: 1 column fills the whole space
    CRect fileRect;
    m_listFiles.GetClientRect(&fileRect);
    m_listFiles.SetColumnWidth(0, fileRect.Width());

    // Mismatch List: Proportional + right fill
    CRect mmRect;
    m_listMismatches.GetClientRect(&mmRect);
    int totalWidth = mmRect.Width();

    // Fixed widths for View/Apply buttons
    int colViewW = 50;
    int colApplyW = 55;
    int remainingWidth = totalWidth - colViewW - colApplyW;
    
    if (remainingWidth > 0) {
        int colKeyW = (int)(remainingWidth * 0.35);
        int colGroupW = (int)(remainingWidth * 0.25);
        int colSpecW = (int)(remainingWidth * 0.20);
        int colValNameW = remainingWidth - colKeyW - colGroupW - colSpecW;

        m_listMismatches.SetColumnWidth(COL_KEY, colKeyW);
        m_listMismatches.SetColumnWidth(COL_GROUP, colGroupW);
        m_listMismatches.SetColumnWidth(COL_SPEC, colSpecW);
        m_listMismatches.SetColumnWidth(COL_VALNAME, colValNameW);
        m_listMismatches.SetColumnWidth(COL_VIEW, colViewW);
        m_listMismatches.SetColumnWidth(COL_APPLY, colApplyW);
    }
}

// =============================================================================
// Custom Draw — File List
// =============================================================================
void CMainDlg::OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        if (!m_report) return;
        int idx = (int)pCD->nmcd.lItemlParam;
        if (idx < 0 || idx >= (int)m_report->fileResults.size()) return;

        switch (m_report->fileResults[idx].status) {
            case ModelCompare::FileDiffResult::Status::DeletedInRight:
                pCD->clrTextBk = CLR_DELETED_BG; pCD->clrText = CLR_DELETED_TXT; break;
            case ModelCompare::FileDiffResult::Status::AddedInRight:
                pCD->clrTextBk = CLR_ADDED_BG; pCD->clrText = CLR_ADDED_TXT; break;
            case ModelCompare::FileDiffResult::Status::Modified:
                pCD->clrTextBk = CLR_MODIFIED_BG; pCD->clrText = CLR_MODIFIED_TXT; break;
            default: break;
        }
    }
}

// =============================================================================
// Custom Draw — Mismatch List (tomato for missing, blue for extra)
// =============================================================================
void CMainDlg::OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        int idx = (int)pCD->nmcd.lItemlParam;
        if (idx >= 0 && idx < (int)m_displayDiffs.size()) {
            if (m_displayDiffs[idx].isMissingInRight) {
                pCD->clrTextBk = CLR_MISSING_BG;
                pCD->clrText = CLR_MISSING_TXT;
            } else {
                pCD->clrTextBk = CLR_EXTRA_BG;
                pCD->clrText = CLR_EXTRA_TXT;
            }
        }
    }
}
