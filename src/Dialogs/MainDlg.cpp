// =============================================================================
// MainDlg.cpp — Premium two-panel layout with 7 UI/UX upgrades.
//
// Features implemented:
//   1. Data Tooltips (LVN_GETINFOTIP) for both list controls
//   2. "Status" column in ID Mismatches grid
//   3. Dynamic Apply button text based on status
//   4. Column reordering: View placed to the right of Apply
//   5. Bold header fonts seamlessly attached to list controls
//   6. HDWP-based proportional OnSize layout (no overlap)
//   7. Premium light-mode color scheme with styled CMFCButton
// =============================================================================
#include "pch.h"
#include "resource.h"
#include "MainDlg.h"
#include "XmlViewerDlg.h"
#include "Engine/CompareEngine.h"
#include "Engine/StructuralIdComparator.h"
#include "Engine/XmlApplyEngine.h"
#include <UxTheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define APP_INITIAL_WIDTH  1000
#define APP_INITIAL_HEIGHT 600
#define APP_MIN_WIDTH      1000
#define APP_MIN_HEIGHT     600


enum MismatchCol {
    COL_KEY = 0,
    COL_GROUP,
    COL_SPEC,
    COL_VALNAME,
    COL_STATUS,
    COL_APPLY,
    COL_VIEW
};


BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LEFT,  &CMainDlg::OnBnClickedBrowseLeft)
    ON_BN_CLICKED(IDC_BTN_BROWSE_RIGHT, &CMainDlg::OnBnClickedBrowseRight)
    ON_BN_CLICKED(IDC_BTN_COMPARE,      &CMainDlg::OnBnClickedCompare)
    ON_BN_CLICKED(IDC_BTN_APPLY_ALL,    &CMainDlg::OnBnClickedApplyAll)
    ON_BN_CLICKED(IDC_BTN_VIEW_XML,     &CMainDlg::OnBnClickedViewXml)
    ON_BN_CLICKED(IDC_BTN_FILE_ACTION,  &CMainDlg::OnBnClickedFileAction)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_DRAWITEM()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES,      &CMainDlg::OnFileListItemChanged)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_FILES,      &CMainDlg::OnFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_MISMATCHES, &CMainDlg::OnMismatchListCustomDraw)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_MISMATCHES, &CMainDlg::OnMismatchListClick)

    ON_NOTIFY(LVN_GETINFOTIP,  IDC_LIST_FILES,      &CMainDlg::OnFileListGetInfoTip)
    ON_NOTIFY(LVN_GETINFOTIP,  IDC_LIST_MISMATCHES, &CMainDlg::OnMismatchListGetInfoTip)
    ON_MESSAGE(WM_COMPARE_COMPLETE, &CMainDlg::OnCompareComplete)
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
    DDX_Control(pDX, IDC_STATIC_FILES_HEADER,    m_staticFilesHeader);
    DDX_Control(pDX, IDC_LIST_FILES,             m_listFiles);
    DDX_Control(pDX, IDC_STATIC_MISMATCH_HEADER, m_staticMismatchHeader);
    DDX_Control(pDX, IDC_BTN_APPLY_ALL,          m_btnApplyAll);
    DDX_Control(pDX, IDC_BTN_VIEW_XML,           m_btnViewXml);
    DDX_Control(pDX, IDC_BTN_FILE_ACTION,        m_btnFileAction);
    DDX_Control(pDX, IDC_LIST_MISMATCHES,        m_listMismatches);
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


    LOGFONT lfHeader = {};
    wcscpy_s(lfHeader.lfFaceName, _T("Segoe UI Semibold"));
    lfHeader.lfHeight = -15;
    lfHeader.lfWeight = FW_SEMIBOLD;
    m_headerFont.CreateFontIndirect(&lfHeader);


    m_listFiles.SetFont(&m_uiFont);
    m_listMismatches.SetFont(&m_uiFont);
    m_staticSummary.SetFont(&m_uiFont);
    m_editLeftPath.SetFont(&m_uiFont);
    m_editRightPath.SetFont(&m_uiFont);
    m_btnBrowseLeft.SetFont(&m_uiFont);
    m_btnBrowseRight.SetFont(&m_uiFont);


    m_staticFilesHeader.SetFont(&m_headerFont);
    m_staticMismatchHeader.SetFont(&m_headerFont);


    m_brushDialogBg.CreateSolidBrush(CLR_DIALOG_BG);
    m_brushPanelBg.CreateSolidBrush(CLR_PANEL_BG);
    m_brushEditBg.CreateSolidBrush(CLR_EDIT_BG);


    m_btnCompare.SetFont(&m_uiFont);
    m_btnCompare.ModifyStyle(0, BS_OWNERDRAW);


    m_btnApplyAll.SetFont(&m_uiFont);
    m_btnApplyAll.ModifyStyle(0, BS_OWNERDRAW);
    m_btnViewXml.SetFont(&m_uiFont);
    m_btnViewXml.ModifyStyle(0, BS_OWNERDRAW);
    m_btnFileAction.SetFont(&m_uiFont);
    m_btnFileAction.ModifyStyle(0, BS_OWNERDRAW);


    m_staticBoundary.Create(_T(""), WS_CHILD | SS_GRAYFRAME, CRect(0,0,0,0), this, 2001);


    SetupListColumns();
    UpdateSummary(_T("Select two model folders and click Compare."));
    m_staticFilesHeader.SetWindowText(_T("  Diff Files"));
    m_staticMismatchHeader.SetWindowText(_T("  ID Mismatches"));
    m_btnApplyAll.EnableWindow(FALSE);
    m_btnViewXml.EnableWindow(FALSE);


    m_imageListMismatch.Create(1, 38, ILC_COLOR32, 1, 1);
    m_listMismatches.SetImageList(&m_imageListMismatch, LVSIL_SMALL);


    ModifyStyle(0, WS_CLIPCHILDREN);


    ::SetWindowTheme(m_listFiles.GetSafeHwnd(), L"Explorer", NULL);
    ::SetWindowTheme(m_listMismatches.GetSafeHwnd(), L"Explorer", NULL);


    BOOL bShadow = TRUE;
    DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_NCRENDERING_POLICY, &bShadow, sizeof(bShadow));


    SetWindowPos(nullptr, 0, 0, APP_INITIAL_WIDTH, APP_INITIAL_HEIGHT, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    return TRUE;
}


void CMainDlg::SetupListColumns() {
    DWORD fStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_listFiles.SetExtendedStyle(m_listFiles.GetExtendedStyle() | fStyle);
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 260);

    DWORD mStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
                 | LVS_EX_DOUBLEBUFFER  | LVS_EX_INFOTIP;
    m_listMismatches.SetExtendedStyle(m_listMismatches.GetExtendedStyle() | mStyle);
    m_listMismatches.InsertColumn(COL_KEY,     _T("Composite Key"), LVCFMT_LEFT,   150);
    m_listMismatches.InsertColumn(COL_GROUP,   _T("Group"),         LVCFMT_LEFT,   110);
    m_listMismatches.InsertColumn(COL_SPEC,    _T("Spec"),          LVCFMT_LEFT,   120);
    m_listMismatches.InsertColumn(COL_VALNAME, _T("Val Name"),      LVCFMT_LEFT,   120);
    m_listMismatches.InsertColumn(COL_STATUS,  _T("Status"),        LVCFMT_LEFT,   130);
    m_listMismatches.InsertColumn(COL_APPLY,   _T("Action"),        LVCFMT_CENTER, 120);
    m_listMismatches.InsertColumn(COL_VIEW,    _T("View"),          LVCFMT_CENTER, 50);
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
    

    m_listFiles.SetFocus();
    
    EnableCompareUI(false);
    UpdateSummary(_T("Comparing..."));
    m_listFiles.DeleteAllItems();
    PopulateMismatchList(-1);
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


void CMainDlg::PopulateFileList() {
    m_listFiles.DeleteAllItems();
    PopulateMismatchList(-1);
    if (!m_report) return;

    for (int i = 0; i < (int)m_report->fileResults.size(); i++) {
        const auto& fr = m_report->fileResults[i];
        CString name;
        name.Format(_T("%d. %s"), i + 1, CString(fr.relativePath.filename().wstring().c_str()).GetString());
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


void CMainDlg::PopulateMismatchList(int fileIndex) {
    ClearMismatchList();
    m_selectedFileIndex = fileIndex;

    if (!m_report || fileIndex < 0 || fileIndex >= (int)m_report->fileResults.size()) {
        m_btnApplyAll.EnableWindow(FALSE);
        m_btnViewXml.EnableWindow(FALSE);
        
        m_btnFileAction.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_HIDE);
        m_listMismatches.ShowWindow(SW_SHOW);
        if (m_staticMismatchHeader.GetSafeHwnd()) {
            m_staticMismatchHeader.SetWindowText(_T("  ID Mismatches"));
        }
        return;
    }

    const auto& fr = m_report->fileResults[fileIndex];


    CString header;
    header.Format(_T("  ID Mismatches \u2014 %s"),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());
    m_staticMismatchHeader.SetWindowText(header);


    if (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight ||
        fr.status == ModelCompare::FileDiffResult::Status::AddedInRight) {
        
        m_listMismatches.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_SHOW);

        CString actionText = (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight)
            ? _T("Add File -> Right")
            : _T("Delete File -> Right");
        
        m_btnFileAction.SetWindowText(actionText);

        m_btnApplyAll.EnableWindow(FALSE);
        m_btnViewXml.EnableWindow(FALSE);
        
        m_btnFileAction.ShowWindow(SW_SHOW);
        m_btnFileAction.BringWindowToTop();
        

        CRect rcClient;
        GetClientRect(&rcClient);
        RepositionControls(rcClient.Width(), rcClient.Height());
        
        return;
    }


    m_btnFileAction.ShowWindow(SW_HIDE);
    m_staticBoundary.ShowWindow(SW_HIDE);
    m_listMismatches.ShowWindow(SW_SHOW);


    for (const auto& e : fr.missingInRight) {
        m_displayDiffs.push_back({ true, e });
    }
    for (const auto& e : fr.extraInRight) {
        m_displayDiffs.push_back({ false, e });
    }


    for (int i = 0; i < (int)m_displayDiffs.size(); i++) {
        const auto& d = m_displayDiffs[i];
        int idx = m_listMismatches.InsertItem(i, CString(d.entry.compositeKey.c_str()));
        m_listMismatches.SetItemText(idx, COL_GROUP, CString(d.entry.groupName.c_str()));


        CString specText;
        switch (d.entry.level) {
        case ModelCompare::DiffLevel::Group:
            specText.Format(_T("(%d specs)"), d.entry.childCount);
            break;
        default:
            specText = CString(d.entry.specName.c_str());
            break;
        }
        m_listMismatches.SetItemText(idx, COL_SPEC, specText);


        CString valText;
        switch (d.entry.level) {
        case ModelCompare::DiffLevel::Group:
            valText = _T("\u2014");
            break;
        case ModelCompare::DiffLevel::Spec:
            valText.Format(_T("(%d vals)"), d.entry.childCount);
            break;
        default:
            valText = CString(d.entry.valName.c_str());
            break;
        }
        m_listMismatches.SetItemText(idx, COL_VALNAME, valText);


        m_listMismatches.SetItemText(idx, COL_STATUS,
            d.isMissingInRight ? _T("Deleted") : _T("Added"));


        CString actionText = d.isMissingInRight ? _T("Add->Right") : _T("Delete->Right");
        m_listMismatches.SetItemText(idx, COL_APPLY, actionText);


        m_listMismatches.SetItemText(idx, COL_VIEW, _T("\xD83D\xDD0D"));

        m_listMismatches.SetItemData(idx, (DWORD_PTR)i);
    }

    m_btnApplyAll.EnableWindow(!m_displayDiffs.empty());
    m_btnViewXml.EnableWindow(TRUE);
}

void CMainDlg::ClearMismatchList() {
    m_listMismatches.DeleteAllItems();
    m_displayDiffs.clear();
}


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
        switch (d.entry.level) {
        case ModelCompare::DiffLevel::Group:
            searchStr.Format(_T("group_ID=\"%s\""), CString(d.entry.groupId.c_str()).GetString());
            break;
        case ModelCompare::DiffLevel::Spec:
            searchStr.Format(_T("spec_ID=\"%s\""), CString(d.entry.specId.c_str()).GetString());
            break;
        case ModelCompare::DiffLevel::Val:
            searchStr.Format(_T("val_id=\"%s\""), CString(d.entry.valId.c_str()).GetString());
            break;
        }
        OpenXmlViewer(d.entry.compositeKey, d.isMissingInRight, searchStr);
    } else if (hitInfo.iSubItem == COL_APPLY) {
        ApplySingleDiff(displayIdx);
    }
}


void CMainDlg::OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;

    if (pInfoTip->iItem < 0) return;

    CString text = m_listFiles.GetItemText(pInfoTip->iItem, 0);


    CRect colRect;
    m_listFiles.GetSubItemRect(pInfoTip->iItem, 0, LVIR_BOUNDS, colRect);
    CDC* pDC = m_listFiles.GetDC();
    if (pDC) {
        CFont* pOldFont = pDC->SelectObject(m_listFiles.GetFont());
        CSize textSize = pDC->GetTextExtent(text);
        pDC->SelectObject(pOldFont);
        m_listFiles.ReleaseDC(pDC);

        if (textSize.cx <= colRect.Width()) return;
    }

    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}

void CMainDlg::OnMismatchListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;

    if (pInfoTip->iItem < 0) return;


    CPoint pt;
    GetCursorPos(&pt);
    m_listMismatches.ScreenToClient(&pt);

    LVHITTESTINFO hitInfo = {};
    hitInfo.pt = pt;
    m_listMismatches.SubItemHitTest(&hitInfo);

    int subItem = (hitInfo.iSubItem >= 0) ? hitInfo.iSubItem : 0;

    CString text = m_listMismatches.GetItemText(pInfoTip->iItem, subItem);
    if (text.IsEmpty()) return;


    CRect colRect;
    if (subItem == 0) {
        m_listMismatches.GetSubItemRect(pInfoTip->iItem, 0, LVIR_BOUNDS, colRect);
        int col1Width = m_listMismatches.GetColumnWidth(0);
        colRect.right = colRect.left + col1Width;
    } else {
        m_listMismatches.GetSubItemRect(pInfoTip->iItem, subItem, LVIR_BOUNDS, colRect);
    }

    CDC* pDC = m_listMismatches.GetDC();
    if (pDC) {
        CFont* pOldFont = pDC->SelectObject(m_listMismatches.GetFont());
        CSize textSize = pDC->GetTextExtent(text);
        pDC->SelectObject(pOldFont);
        m_listMismatches.ReleaseDC(pDC);

        if (textSize.cx <= colRect.Width() - 8) return;
    }


    CString tipText;
    LVCOLUMN col = {};
    TCHAR colName[128] = {};
    col.mask = LVCF_TEXT;
    col.pszText = colName;
    col.cchTextMax = 128;
    if (m_listMismatches.GetColumn(subItem, &col)) {
        tipText.Format(_T("%s: %s"), colName, text.GetString());
    } else {
        tipText = text;
    }

    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, tipText, _TRUNCATE);
}


void CMainDlg::OnBnClickedViewXml() {
    OpenXmlViewer();
}

void CMainDlg::OpenXmlViewer(const std::string& scrollToKey, bool isMissing, const CString& targetSearch) {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

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
        int savedIndex = m_selectedFileIndex;
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
        result = ModelCompare::XmlApplyEngine::AddMissing(leftPath, rightPath, d.entry);
    } else {
        result = ModelCompare::XmlApplyEngine::RemoveExtra(rightPath, d.entry);
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

void CMainDlg::OnBnClickedFileAction() {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    CString actionText;
    m_btnFileAction.GetWindowText(actionText);

    CString msg;
    msg.Format(_T("Are you sure you want to %s?\n\nFile: %s"),
        actionText.GetString(),
        CString(fr.relativePath.filename().wstring().c_str()).GetString());

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto leftPath = GetLeftFilePath(m_selectedFileIndex);
    auto rightPath = GetRightFilePath(m_selectedFileIndex);

    bool bSuccess = false;
    CString errText;

    if (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight) {

        try {
            std::filesystem::create_directories(rightPath.parent_path());
            std::filesystem::copy_file(leftPath, rightPath, std::filesystem::copy_options::overwrite_existing);
            bSuccess = true;
        } catch (const std::exception& e) {
            errText = e.what();
        }
    } else if (fr.status == ModelCompare::FileDiffResult::Status::AddedInRight) {

        try {
            if (std::filesystem::exists(rightPath)) {
                std::filesystem::remove(rightPath);
            }
            bSuccess = true;
        } catch (const std::exception& e) {
            errText = e.what();
        }
    }

    if (bSuccess) {
        AfxMessageBox(_T("Applied successfully. Re-comparing..."), MB_ICONINFORMATION);
        OnBnClickedCompare();
    } else {
        CString err;
        err.Format(_T("Apply failed: %s"), errText.GetString());
        AfxMessageBox(err, MB_ICONERROR);
    }
}


std::filesystem::path CMainDlg::GetLeftFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->leftModelPath) / m_report->fileResults[fileIndex].relativePath;
}

std::filesystem::path CMainDlg::GetRightFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->rightModelPath) / m_report->fileResults[fileIndex].relativePath;
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
    const int HDR_H   = 22;

    const double LEFT_PANEL_RATIO = 0.20;

    HDWP hDwp = ::BeginDeferWindowPos(16);
    if (!hDwp) return;


    int cmpX = cx - M - CMP_W;
    int inputAreaH = ROW_H * 2 + GAP;
    int cmpY = M + (inputAreaH - CMP_H) / 2;

    int maxInputRight = cmpX - 12;
    int editW = maxInputRight - (M + LBL_W + GAP + GAP + BTN_W);
    if (editW < 100) editW = 100;

    int y = M;


    CWnd* pLL = GetDlgItem(IDC_STATIC_LEFT_LABEL);
    if (pLL) {
        hDwp = ::DeferWindowPos(hDwp, pLL->GetSafeHwnd(), NULL,
            M, y + 3, LBL_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    hDwp = ::DeferWindowPos(hDwp, m_editLeftPath.GetSafeHwnd(), NULL,
        M + LBL_W + GAP, y, editW, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_btnBrowseLeft.GetSafeHwnd(), NULL,
        M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    y += ROW_H + GAP;


    CWnd* pRL = GetDlgItem(IDC_STATIC_RIGHT_LABEL);
    if (pRL) {
        hDwp = ::DeferWindowPos(hDwp, pRL->GetSafeHwnd(), NULL,
            M, y + 3, LBL_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    hDwp = ::DeferWindowPos(hDwp, m_editRightPath.GetSafeHwnd(), NULL,
        M + LBL_W + GAP, y, editW, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_btnBrowseRight.GetSafeHwnd(), NULL,
        M + LBL_W + GAP + editW + GAP, y, BTN_W, ROW_H, SWP_NOZORDER | SWP_NOACTIVATE);
    y += ROW_H + GAP;


    hDwp = ::DeferWindowPos(hDwp, m_btnCompare.GetSafeHwnd(), NULL,
        cmpX, cmpY, CMP_W, CMP_H, SWP_NOZORDER | SWP_NOACTIVATE);


    int summaryH = 22;
    hDwp = ::DeferWindowPos(hDwp, m_staticSummary.GetSafeHwnd(), NULL,
        M, y, cx - 2 * M, summaryH, SWP_NOZORDER | SWP_NOACTIVATE);
    y += summaryH + GAP;


    int panelH = cy - y - M;
    if (panelH < 100) panelH = 100;
    

    int fileListW = (int)((cx - 2 * M - GAP) * LEFT_PANEL_RATIO);
    if (fileListW < 150) fileListW = 150;
    
    int mismatchX = M + fileListW + GAP;
    int mismatchW = cx - mismatchX - M;


    hDwp = ::DeferWindowPos(hDwp, m_staticFilesHeader.GetSafeHwnd(), NULL,
        M, y, fileListW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_listFiles.GetSafeHwnd(), NULL,
        M, y + HDR_H, fileListW, panelH - HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);


    int applyAllW = 130;
    int viewXmlW  = 90;
    int btnViewX  = cx - M - viewXmlW;
    int btnApplyX = btnViewX - GAP - applyAllW;
    int btnY = y - 2;

    hDwp = ::DeferWindowPos(hDwp, m_btnApplyAll.GetSafeHwnd(), NULL,
        btnApplyX, btnY, applyAllW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    hDwp = ::DeferWindowPos(hDwp, m_btnViewXml.GetSafeHwnd(), NULL,
        btnViewX, btnY, viewXmlW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    int mismatchHeaderW = btnApplyX - mismatchX - GAP;
    if (mismatchHeaderW < 10) mismatchHeaderW = 10;
    hDwp = ::DeferWindowPos(hDwp, m_staticMismatchHeader.GetSafeHwnd(), NULL,
        mismatchX, y, mismatchHeaderW, HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    hDwp = ::DeferWindowPos(hDwp, m_listMismatches.GetSafeHwnd(), NULL,
        mismatchX, y + HDR_H, mismatchW, panelH - HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);

    if (m_staticBoundary.GetSafeHwnd()) {
        hDwp = ::DeferWindowPos(hDwp, m_staticBoundary.GetSafeHwnd(), NULL,
            mismatchX, y + HDR_H, mismatchW, panelH - HDR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ::EndDeferWindowPos(hDwp);


    if (m_btnFileAction.GetSafeHwnd()) {
        int btnW = 160;
        int btnH = 34;
        int btnX = mismatchX + (mismatchW - btnW) / 2;
        int btnY = y + HDR_H + (panelH - HDR_H - btnH) / 2;
        m_btnFileAction.SetWindowPos(&wndTop, btnX, btnY, btnW, btnH, SWP_NOACTIVATE);
    }


    ResizeListColumns();
}


void CMainDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listMismatches.GetSafeHwnd()) return;


    CRect fileRect;
    m_listFiles.GetClientRect(&fileRect);
    m_listFiles.SetColumnWidth(0, fileRect.Width());


    CRect mmRect;
    m_listMismatches.GetClientRect(&mmRect);
    int totalWidth = mmRect.Width();


    const double colRatios[7] = {
        0.18,
        0.18,
        0.16,
        0.18,
        0.09,
        0.14,
        0.07
    };

    if (totalWidth > 0) {
        int w0 = (int)(totalWidth * colRatios[COL_KEY]);
        int w1 = (int)(totalWidth * colRatios[COL_GROUP]);
        int w2 = (int)(totalWidth * colRatios[COL_SPEC]);
        int w3 = (int)(totalWidth * colRatios[COL_VALNAME]);
        int w4 = (int)(totalWidth * colRatios[COL_STATUS]);
        int w5 = (int)(totalWidth * colRatios[COL_APPLY]);

        int w6 = totalWidth - (w0 + w1 + w2 + w3 + w4 + w5);

        m_listMismatches.SetColumnWidth(COL_KEY,     w0);
        m_listMismatches.SetColumnWidth(COL_GROUP,   w1);
        m_listMismatches.SetColumnWidth(COL_SPEC,    w2);
        m_listMismatches.SetColumnWidth(COL_VALNAME, w3);
        m_listMismatches.SetColumnWidth(COL_STATUS,  w4);
        m_listMismatches.SetColumnWidth(COL_APPLY,   w5);
        m_listMismatches.SetColumnWidth(COL_VIEW,    w6);
    }
}


HBRUSH CMainDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    switch (nCtlColor) {
    case CTLCOLOR_DLG:
        pDC->SetBkColor(CLR_DIALOG_BG);
        return (HBRUSH)m_brushDialogBg;

    case CTLCOLOR_STATIC: {
        UINT ctrlId = pWnd->GetDlgCtrlID();
        pDC->SetBkMode(TRANSPARENT);


        if (ctrlId == IDC_STATIC_FILES_HEADER || ctrlId == IDC_STATIC_MISMATCH_HEADER) {
            pDC->SetTextColor(CLR_HEADER_TEXT);
            return (HBRUSH)m_brushPanelBg;
        }

        if (ctrlId == IDC_STATIC_SUMMARY) {
            pDC->SetTextColor(CLR_TEXT_SECONDARY);
            return (HBRUSH)m_brushDialogBg;
        }

        pDC->SetTextColor(CLR_TEXT_PRIMARY);
        return (HBRUSH)m_brushDialogBg;
    }

    case CTLCOLOR_EDIT:
        pDC->SetBkColor(CLR_EDIT_BG);
        pDC->SetTextColor(CLR_TEXT_PRIMARY);
        return (HBRUSH)m_brushEditBg;

    case CTLCOLOR_BTN:
        pDC->SetBkColor(CLR_DIALOG_BG);
        return (HBRUSH)m_brushDialogBg;

    default:
        break;
    }

    return hbr;
}


BOOL CMainDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, CLR_DIALOG_BG);
    return TRUE;
}


void CMainDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF baseColor;
    COLORREF pressedColor;
    int cornerRadius = 8;
    bool isOurs = true;

    switch (nIDCtl) {
    case IDC_BTN_COMPARE:
        baseColor = CLR_ACCENT_BLUE;
        pressedColor = RGB(0, 90, 210);
        cornerRadius = 14;
        break;
    case IDC_BTN_APPLY_ALL:
        baseColor = RGB(60, 170, 110);
        pressedColor = RGB(40, 130, 85);
        break;
    case IDC_BTN_VIEW_XML:
        baseColor = RGB(80, 140, 220);
        pressedColor = RGB(55, 110, 190);
        break;
    case IDC_BTN_FILE_ACTION:

        baseColor = RGB(13, 110, 253);
        pressedColor = RGB(10, 90, 210);
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

    UINT drawFlags = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    if (nIDCtl == IDC_BTN_COMPARE) {
        CRect textRect = rect;
        pDC->DrawText(text, textRect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        int offsetY = (rect.Height() - textRect.Height()) / 2;
        textRect = rect;
        textRect.top += offsetY;
        pDC->DrawText(text, textRect, DT_CENTER | DT_WORDBREAK);
    } else {
        pDC->DrawText(text, &rect, drawFlags);
    }

    pDC->SelectObject(pOldFont);
    pDC->SelectObject(pOldBrush);
    pDC->SelectObject(pOldPen);

    if (state & ODS_FOCUS) {
        CRect focusRect = rect;
        focusRect.DeflateRect(3, 3);
        pDC->DrawFocusRect(&focusRect);
    }
}


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


void CMainDlg::OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
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

        if (displayIdx >= 0 && displayIdx < (int)m_displayDiffs.size()) {
            if (m_displayDiffs[displayIdx].isMissingInRight) {
                bg = CLR_MISSING_BG;
                txt = CLR_MISSING_TXT;
            } else {
                bg = CLR_EXTRA_BG;
                txt = CLR_EXTRA_TXT;
            }
        }


        if (displayIdx >= 0 && (subItem == COL_APPLY || subItem == COL_VIEW)) {
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
            CRect rect;
            m_listMismatches.GetSubItemRect(row, subItem, LVIR_BOUNDS, rect);
            
            pDC->FillSolidRect(&rect, bg);
            
            rect.DeflateRect(12, 6);

            COLORREF btnColor = CLR_ACCENT_BLUE;
            if (subItem == COL_APPLY) {
                btnColor = m_displayDiffs[displayIdx].isMissingInRight ? CLR_ACCENT_GREEN : CLR_ACCENT_RED;
            }

            CBrush brush(btnColor);
            CPen pen(PS_SOLID, 1, btnColor);
            CBrush* pOldBrush = pDC->SelectObject(&brush);
            CPen* pOldPen = pDC->SelectObject(&pen);

            pDC->RoundRect(&rect, CPoint(8, 8));

            CString text = m_listMismatches.GetItemText(row, subItem);
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255, 255, 255));


            LOGFONT lf;
            m_listMismatches.GetFont()->GetLogFont(&lf);
            

            int buttonWidth = rect.Width();
            if (buttonWidth < 85) {
                double scale = (double)buttonWidth / 85.0;
                if (scale < 0.65) scale = 0.65;
                lf.lfHeight = (LONG)(lf.lfHeight * scale);
            }

            CFont dynamicFont;
            dynamicFont.CreateFontIndirect(&lf);
            CFont* pOldFont = pDC->SelectObject(&dynamicFont);

            if (subItem == COL_APPLY) {

                pDC->DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                pDC->DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            pDC->SelectObject(pOldFont);
            pDC->SelectObject(pOldBrush);
            pDC->SelectObject(pOldPen);

            *pResult = CDRF_SKIPDEFAULT;
            return;
        }


        pCD->clrTextBk = bg;
        pCD->clrText = txt;
        *pResult = CDRF_DODEFAULT;
        return;
    }
}
