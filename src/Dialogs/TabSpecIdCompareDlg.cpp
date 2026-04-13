#include "pch.h"
#include "TabSpecIdCompareDlg.h"
#include "XmlViewerDlg.h"
#include "Engine/XmlApplyEngine.h"
#include <UxTheme.h>
#include <cctype>
#include "Theme.h"

enum MismatchCol {
    COL_KEY = 0,
    COL_GROUP,
    COL_SPEC,
    COL_VALNAME,
    COL_STATUS,
    COL_APPLY,
    COL_VIEW
};

static CString ExtractEnglishName(const std::string& fullName) {
    if (fullName.empty()) return _T("missing");
    size_t pos = fullName.find("$$");
    std::string eng = (pos != std::string::npos) ? fullName.substr(0, pos) : fullName;
    while (!eng.empty() && std::isspace(static_cast<unsigned char>(eng.back()))) eng.pop_back();
    return CString(eng.c_str());
}

BEGIN_MESSAGE_MAP(CTabSpecIdCompareDlg, CDialogEx)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_APPLY_ALL,    &CTabSpecIdCompareDlg::OnBnClickedApplyAll)
    ON_BN_CLICKED(IDC_BTN_APPLY_SELECTION, &CTabSpecIdCompareDlg::OnBnClickedApplySelection)
    ON_BN_CLICKED(IDC_BTN_VIEW_XML,     &CTabSpecIdCompareDlg::OnBnClickedViewXml)
    ON_BN_CLICKED(IDC_BTN_FILE_ACTION,  &CTabSpecIdCompareDlg::OnBnClickedFileAction)
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_DRAWITEM()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES,      &CTabSpecIdCompareDlg::OnFileListItemChanged)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_FILES,      &CTabSpecIdCompareDlg::OnFileListCustomDraw)
    ON_NOTIFY(NM_CUSTOMDRAW,   IDC_LIST_MISMATCHES, &CTabSpecIdCompareDlg::OnMismatchListCustomDraw)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MISMATCHES, &CTabSpecIdCompareDlg::OnLvnItemchangedListMismatches)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_MISMATCHES, &CTabSpecIdCompareDlg::OnMismatchListClick)
    ON_NOTIFY(LVN_GETINFOTIP,  IDC_LIST_FILES,      &CTabSpecIdCompareDlg::OnFileListGetInfoTip)
    ON_NOTIFY(LVN_GETINFOTIP,  IDC_LIST_MISMATCHES, &CTabSpecIdCompareDlg::OnMismatchListGetInfoTip)
END_MESSAGE_MAP()

CTabSpecIdCompareDlg::CTabSpecIdCompareDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_SPEC_ID_COMPARE, pParent) {}

CTabSpecIdCompareDlg::~CTabSpecIdCompareDlg() {}

void CTabSpecIdCompareDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_FILES_HEADER,    m_staticFilesHeader);
    DDX_Control(pDX, IDC_LIST_FILES,             m_listFiles);
    DDX_Control(pDX, IDC_STATIC_MISMATCH_HEADER, m_staticMismatchHeader);
    DDX_Control(pDX, IDC_BTN_APPLY_ALL,          m_btnApplyAll);
    DDX_Control(pDX, IDC_BTN_APPLY_SELECTION,    m_btnApplySelection);
    DDX_Control(pDX, IDC_BTN_VIEW_XML,           m_btnViewXml);
    DDX_Control(pDX, IDC_BTN_FILE_ACTION,        m_btnFileAction);
    DDX_Control(pDX, IDC_LIST_MISMATCHES,        m_listMismatches);
}

BOOL CTabSpecIdCompareDlg::OnInitDialog() {
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
    m_listMismatches.SetFont(&m_uiFont);
    m_staticFilesHeader.SetFont(&m_headerFont);
    m_staticMismatchHeader.SetFont(&m_headerFont);

    m_brushDialogBg.CreateSolidBrush(Theme::Get()->DialogBg());
    m_brushPanelBg.CreateSolidBrush(Theme::Get()->PanelBg());

    m_btnApplyAll.SetFont(&m_uiFont);
    m_btnApplyAll.ModifyStyle(0, BS_OWNERDRAW);
    m_btnApplySelection.SetFont(&m_uiFont);
    m_btnApplySelection.ModifyStyle(0, BS_OWNERDRAW);
    m_btnViewXml.SetFont(&m_uiFont);
    m_btnViewXml.ModifyStyle(0, BS_OWNERDRAW);
    m_btnFileAction.SetFont(&m_uiFont);
    m_btnFileAction.ModifyStyle(0, BS_OWNERDRAW);

    m_staticBoundary.Create(_T(""), WS_CHILD | SS_GRAYFRAME, CRect(0,0,0,0), this, 2001);

    SetupListColumns();
    m_staticFilesHeader.SetWindowText(_T("  Diff Files"));
    m_staticMismatchHeader.SetWindowText(_T("  ID Mismatches"));
    m_btnApplyAll.EnableWindow(FALSE);
    m_btnApplySelection.EnableWindow(FALSE);
    m_btnViewXml.EnableWindow(FALSE);

    m_imageListMismatch.Create(1, 40, ILC_COLOR32, 1, 1);
    m_listMismatches.SetImageList(&m_imageListMismatch, LVSIL_SMALL);

    ModifyStyle(0, WS_CLIPCHILDREN);
    ::SetWindowTheme(m_listFiles.GetSafeHwnd(), L"Explorer", NULL);
    ::SetWindowTheme(m_listMismatches.GetSafeHwnd(), L"Explorer", NULL);

    return TRUE;
}

void CTabSpecIdCompareDlg::SetReport(std::shared_ptr<ModelCompare::ModelDiffReport> report) {
    m_report = report;
    PopulateFileList();
}

void CTabSpecIdCompareDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (cx > 0 && cy > 0) {
        const double LEFT_RATIO = 0.20;
        const int HEADER_H = Theme::Get()->LayoutHeaderHeight();
        const int GAP = Theme::Get()->LayoutGap();
        
        int fileListW = (int)((cx - GAP) * LEFT_RATIO);
        if (fileListW < 150) fileListW = 150;
        
        int mismatchX = fileListW + GAP;
        int mismatchW = cx - mismatchX;

        HDWP hDwp = ::BeginDeferWindowPos(8);
        if (!hDwp) return;

        hDwp = ::DeferWindowPos(hDwp, m_staticFilesHeader.GetSafeHwnd(), NULL,
            0, 0, fileListW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
        hDwp = ::DeferWindowPos(hDwp, m_listFiles.GetSafeHwnd(), NULL,
            0, HEADER_H, fileListW, cy - HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);

        int applyAllW = 120;
        int applySelW = 110;
        int viewXmlW  = 90;
        int btnViewX  = cx - viewXmlW;
        int btnApplyX = btnViewX - GAP - applyAllW;
        int btnApplySelX = btnApplyX - GAP - applySelW;
        
        hDwp = ::DeferWindowPos(hDwp, m_btnApplySelection.GetSafeHwnd(), NULL,
            btnApplySelX, 0, applySelW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
        hDwp = ::DeferWindowPos(hDwp, m_btnApplyAll.GetSafeHwnd(), NULL,
            btnApplyX, 0, applyAllW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
        hDwp = ::DeferWindowPos(hDwp, m_btnViewXml.GetSafeHwnd(), NULL,
            btnViewX, 0, viewXmlW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);

        int mismatchHeaderW = btnApplySelX - mismatchX - GAP;
        if (mismatchHeaderW < 10) mismatchHeaderW = 10;
        hDwp = ::DeferWindowPos(hDwp, m_staticMismatchHeader.GetSafeHwnd(), NULL,
            mismatchX, 0, mismatchHeaderW, HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);

        hDwp = ::DeferWindowPos(hDwp, m_listMismatches.GetSafeHwnd(), NULL,
            mismatchX, HEADER_H, mismatchW, cy - HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);

        if (m_staticBoundary.GetSafeHwnd()) {
            hDwp = ::DeferWindowPos(hDwp, m_staticBoundary.GetSafeHwnd(), NULL,
                mismatchX, HEADER_H, mismatchW, cy - HEADER_H, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        ::EndDeferWindowPos(hDwp);

        if (m_btnFileAction.GetSafeHwnd()) {
            int btnW = 160;
            int btnH = 34;
            int btnX = mismatchX + (mismatchW - btnW) / 2;
            int btnY = HEADER_H + (cy - HEADER_H - btnH) / 2;
            m_btnFileAction.SetWindowPos(&wndTop, btnX, btnY, btnW, btnH, SWP_NOACTIVATE);
        }

        ResizeListColumns();
    }
}

void CTabSpecIdCompareDlg::ResizeListColumns() {
    if (!m_listFiles.GetSafeHwnd() || !m_listMismatches.GetSafeHwnd()) return;

    CRect fileRect;
    m_listFiles.GetClientRect(&fileRect);
    m_listFiles.SetColumnWidth(0, fileRect.Width() - 2);

    CRect mmRect;
    m_listMismatches.GetClientRect(&mmRect);
    int totalWidth = mmRect.Width();

    const double colRatios[7] = { 0.18, 0.18, 0.16, 0.18, 0.09, 0.14, 0.07 };

    if (totalWidth > 0) {
        int w[7], sum = 0;
        for (int i=0; i<6; ++i) { w[i] = (int)(totalWidth * colRatios[i]); sum += w[i]; }
        w[6] = totalWidth - sum - 3;
        for (int i=0; i<7; ++i) m_listMismatches.SetColumnWidth(i, w[i]);
    }
}

void CTabSpecIdCompareDlg::SetupListColumns() {
    DWORD fStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_listFiles.SetExtendedStyle(m_listFiles.GetExtendedStyle() | fStyle);
    m_listFiles.InsertColumn(0, _T("File Name"), LVCFMT_LEFT, 260);

    DWORD mStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
                 | LVS_EX_DOUBLEBUFFER  | LVS_EX_INFOTIP | LVS_EX_CHECKBOXES;
    m_listMismatches.SetExtendedStyle(m_listMismatches.GetExtendedStyle() | mStyle);
    m_listMismatches.InsertColumn(COL_KEY,     _T("Composite Key"), LVCFMT_LEFT,   150);
    m_listMismatches.InsertColumn(COL_GROUP,   _T("Group"),         LVCFMT_LEFT,   110);
    m_listMismatches.InsertColumn(COL_SPEC,    _T("Spec"),          LVCFMT_LEFT,   120);
    m_listMismatches.InsertColumn(COL_VALNAME, _T("Val Name"),      LVCFMT_LEFT,   120);
    m_listMismatches.InsertColumn(COL_STATUS,  _T("Status(Left)"),  LVCFMT_LEFT,   170);
    m_listMismatches.InsertColumn(COL_APPLY,   _T("Action"),        LVCFMT_CENTER, 120);
    m_listMismatches.InsertColumn(COL_VIEW,    _T("View(Right)"),   LVCFMT_CENTER, 130);
}

void CTabSpecIdCompareDlg::PopulateFileList() {
    m_listFiles.DeleteAllItems();
    PopulateMismatchList(-1);
    if (!m_report) return;

    int displayIndex = 0;
    for (int i = 0; i < (int)m_report->fileResults.size(); i++) {
        const auto& fr = m_report->fileResults[i];
        if (!fr.HasDifferences()) continue;

        CString name;
        name.Format(_T("%d. %s"), displayIndex + 1, CString(fr.relativePath.filename().wstring().c_str()).GetString());
        int idx = m_listFiles.InsertItem(displayIndex, name);
        m_listFiles.SetItemData(idx, (DWORD_PTR)i);
        displayIndex++;
    }
    
    ResizeListColumns();
}

void CTabSpecIdCompareDlg::OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pNM = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if (!(pNM->uChanged & LVIF_STATE) || !(pNM->uNewState & LVIS_SELECTED)) return;
    int fi = (int)m_listFiles.GetItemData(pNM->iItem);
    PopulateMismatchList(fi);
}

void CTabSpecIdCompareDlg::PopulateMismatchList(int fileIndex) {
    ClearMismatchList();
    m_selectedFileIndex = fileIndex;

    if (!m_report || fileIndex < 0 || fileIndex >= (int)m_report->fileResults.size()) {
        m_btnApplyAll.EnableWindow(FALSE);
        m_btnApplySelection.EnableWindow(FALSE);
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
    header.Format(_T("  ID Mismatches - %s"), CString(fr.relativePath.filename().wstring().c_str()).GetString());
    m_staticMismatchHeader.SetWindowText(header);

    if (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight ||
        fr.status == ModelCompare::FileDiffResult::Status::AddedInRight) {
        m_listMismatches.ShowWindow(SW_HIDE);
        m_staticBoundary.ShowWindow(SW_SHOW);
        CString actionText = (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight)
            ? _T("Add File -> Right") : _T("Delete File -> Right");
        m_btnFileAction.SetWindowText(actionText);
        m_btnApplyAll.EnableWindow(FALSE);
        m_btnApplySelection.EnableWindow(FALSE);
        m_btnViewXml.EnableWindow(FALSE);
        m_btnFileAction.ShowWindow(SW_SHOW);
        m_btnFileAction.BringWindowToTop();
        
        CRect rcClient; GetClientRect(&rcClient);
        OnSize(0, rcClient.Width(), rcClient.Height());
        return;
    }

    m_btnFileAction.ShowWindow(SW_HIDE);
    m_staticBoundary.ShowWindow(SW_HIDE);
    m_listMismatches.ShowWindow(SW_SHOW);

    for (const auto& e : fr.missingInRight) m_displayDiffs.push_back({ true, e });
    for (const auto& e : fr.extraInRight) m_displayDiffs.push_back({ false, e });

    for (int i = 0; i < (int)m_displayDiffs.size(); i++) {
        const auto& d = m_displayDiffs[i];
        int idx = m_listMismatches.InsertItem(i, CString(d.entry.compositeKey.c_str()));
        m_listMismatches.SetItemText(idx, COL_GROUP, ExtractEnglishName(d.entry.groupName));

        CString specText;
        if (d.entry.level == ModelCompare::DiffLevel::Group) specText.Format(_T("(%d specs)"), d.entry.childCount);
        else specText = ExtractEnglishName(d.entry.specName);
        m_listMismatches.SetItemText(idx, COL_SPEC, specText);

        CString valText;
        if (d.entry.level == ModelCompare::DiffLevel::Group) valText = _T("-");
        else if (d.entry.level == ModelCompare::DiffLevel::Spec) valText.Format(_T("(%d vals)"), d.entry.childCount);
        else valText = ExtractEnglishName(d.entry.valName);
        m_listMismatches.SetItemText(idx, COL_VALNAME, valText);

        m_listMismatches.SetItemText(idx, COL_STATUS, d.isMissingInRight ? _T("Added") : _T("Deleted"));
        m_listMismatches.SetItemText(idx, COL_APPLY, d.isMissingInRight ? _T("Add->Right") : _T("Delete->Right"));
        m_listMismatches.SetItemText(idx, COL_VIEW, _T("O"));
        m_listMismatches.SetItemData(idx, (DWORD_PTR)i);
    }
    m_btnApplyAll.EnableWindow(!m_displayDiffs.empty());
    m_btnApplySelection.EnableWindow(!m_displayDiffs.empty());
    m_btnViewXml.EnableWindow(TRUE);
    
    ResizeListColumns();
}

void CTabSpecIdCompareDlg::ClearMismatchList() {
    m_listMismatches.DeleteAllItems();
    m_displayDiffs.clear();
}

void CTabSpecIdCompareDlg::OnMismatchListClick(NMHDR* pNMHDR, LRESULT* pResult) {
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
        if (d.entry.level == ModelCompare::DiffLevel::Group) searchStr.Format(_T("group_ID=\"%s\""), CString(d.entry.groupId.c_str()).GetString());
        else if (d.entry.level == ModelCompare::DiffLevel::Spec) searchStr.Format(_T("spec_ID=\"%s\""), CString(d.entry.specId.c_str()).GetString());
        else if (d.entry.level == ModelCompare::DiffLevel::Val) searchStr.Format(_T("val_id=\"%s\""), CString(d.entry.valId.c_str()).GetString());
        OpenXmlViewer(d.entry.compositeKey, d.isMissingInRight, searchStr);
    } else if (hitInfo.iSubItem == COL_APPLY) {
        ApplySingleDiff(displayIdx);
    }
}

void CTabSpecIdCompareDlg::OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;
    CString text = m_listFiles.GetItemText(pInfoTip->iItem, 0);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}

void CTabSpecIdCompareDlg::OnMismatchListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pInfoTip = reinterpret_cast<NMLVGETINFOTIP*>(pNMHDR);
    *pResult = 0;
    if (pInfoTip->iItem < 0) return;
    CPoint pt; GetCursorPos(&pt); m_listMismatches.ScreenToClient(&pt);
    LVHITTESTINFO hitInfo = {}; hitInfo.pt = pt; m_listMismatches.SubItemHitTest(&hitInfo);
    int subItem = hitInfo.iSubItem >= 0 ? hitInfo.iSubItem : 0;
    CString text = m_listMismatches.GetItemText(pInfoTip->iItem, subItem);
    _tcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, text, _TRUNCATE);
}

void CTabSpecIdCompareDlg::OnBnClickedViewXml() {
    OpenXmlViewer();
}

void CTabSpecIdCompareDlg::OpenXmlViewer(const std::string& scrollToKey, bool isMissing, const CString& targetSearch) {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    auto rightPath = GetRightFilePath(m_selectedFileIndex);
    auto leftPath = GetLeftFilePath(m_selectedFileIndex);
    if (rightPath.empty() || !std::filesystem::exists(rightPath)) {
        AfxMessageBox(_T("Right model file not found."), MB_ICONWARNING);
        return;
    }

    CString title;
    title.Format(_T("XML Viewer - %s (Right Model)"), CString(fr.relativePath.filename().wstring().c_str()).GetString());

    CXmlViewerDlg dlg(this);
    dlg.SetFile(rightPath, leftPath, title);
    dlg.SetDiffs(fr.missingInRight, fr.extraInRight);
    if (!scrollToKey.empty()) dlg.SetScrollToKey(scrollToKey, isMissing);
    if (!targetSearch.IsEmpty()) dlg.SetSearchTarget(targetSearch);
    dlg.DoModal();
}

void CTabSpecIdCompareDlg::OnBnClickedApplyAll() {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    CString msg; msg.Format(_T("Apply all %zu diff(s) to the Right model file?\n\n%s"), 
        fr.DiffCount(), CString(fr.relativePath.filename().wstring().c_str()).GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto result = ModelCompare::XmlApplyEngine::ApplyAllDiffs(GetLeftFilePath(m_selectedFileIndex), GetRightFilePath(m_selectedFileIndex), fr.missingInRight, fr.extraInRight);
    if (result.success) HandleApplySuccess(fr.missingInRight, fr.extraInRight);
    else { CString err; err.Format(_T("Apply failed: %s"), CString(result.errorMessage.c_str()).GetString()); AfxMessageBox(err, MB_ICONERROR); }
}

void CTabSpecIdCompareDlg::OnBnClickedApplySelection() {
    if (m_selectedFileIndex < 0 || !m_report) return;
    std::vector<ModelCompare::KeyDiffEntry> missingToApply, extraToApply;
    for (int i = 0; i < m_listMismatches.GetItemCount(); i++) {
        if (m_listMismatches.GetCheck(i)) {
            int displayIdx = (int)m_listMismatches.GetItemData(i);
            if (displayIdx >= 0 && displayIdx < (int)m_displayDiffs.size()) {
                const auto& d = m_displayDiffs[displayIdx];
                if (d.isMissingInRight) missingToApply.push_back(d.entry);
                else extraToApply.push_back(d.entry);
            }
        }
    }
    int count = static_cast<int>(missingToApply.size() + extraToApply.size());
    if (count == 0) { AfxMessageBox(_T("Please check at least one diff to apply."), MB_ICONWARNING); return; }
    
    CString msg; msg.Format(_T("Apply %d selected diff(s) to the Right model file?\n\n%s"), count, CString(m_report->fileResults[m_selectedFileIndex].relativePath.filename().wstring().c_str()).GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto result = ModelCompare::XmlApplyEngine::ApplyAllDiffs(GetLeftFilePath(m_selectedFileIndex), GetRightFilePath(m_selectedFileIndex), missingToApply, extraToApply);
    if (result.success) HandleApplySuccess(missingToApply, extraToApply);
    else { CString err; err.Format(_T("Apply failed: %s"), CString(result.errorMessage.c_str()).GetString()); AfxMessageBox(err, MB_ICONERROR); }
}

void CTabSpecIdCompareDlg::ApplySingleDiff(int displayIndex) {
    if (displayIndex < 0 || displayIndex >= (int)m_displayDiffs.size()) return;
    if (m_selectedFileIndex < 0 || !m_report) return;

    const auto& d = m_displayDiffs[displayIndex];
    CString msg; msg.Format(_T("%s ID: %s ?"), d.isMissingInRight ? _T("ADD missing") : _T("REMOVE extra"), CString(d.entry.compositeKey.c_str()).GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    ModelCompare::XmlApplyEngine::ApplyResult result;
    if (d.isMissingInRight) result = ModelCompare::XmlApplyEngine::AddMissing(GetLeftFilePath(m_selectedFileIndex), GetRightFilePath(m_selectedFileIndex), d.entry);
    else result = ModelCompare::XmlApplyEngine::RemoveExtra(GetRightFilePath(m_selectedFileIndex), d.entry);

    if (result.success) {
        std::vector<ModelCompare::KeyDiffEntry> appliedMissing, appliedExtra;
        if (d.isMissingInRight) appliedMissing.push_back(d.entry); else appliedExtra.push_back(d.entry);
        HandleApplySuccess(appliedMissing, appliedExtra);
    } else { CString err; err.Format(_T("Apply failed: %s"), CString(result.errorMessage.c_str()).GetString()); AfxMessageBox(err, MB_ICONERROR); }
}

void CTabSpecIdCompareDlg::OnBnClickedFileAction() {
    if (m_selectedFileIndex < 0 || !m_report) return;
    const auto& fr = m_report->fileResults[m_selectedFileIndex];

    CString actionText; m_btnFileAction.GetWindowText(actionText);
    CString msg; msg.Format(_T("Are you sure you want to %s?\n\nFile: %s"), actionText.GetString(), CString(fr.relativePath.filename().wstring().c_str()).GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto leftPath = GetLeftFilePath(m_selectedFileIndex);
    auto rightPath = GetRightFilePath(m_selectedFileIndex);
    bool bSuccess = false; CString errText;

    try {
        if (fr.status == ModelCompare::FileDiffResult::Status::DeletedInRight) {
            std::filesystem::create_directories(rightPath.parent_path());
            std::filesystem::copy_file(leftPath, rightPath, std::filesystem::copy_options::overwrite_existing);
            bSuccess = true;
        } else if (fr.status == ModelCompare::FileDiffResult::Status::AddedInRight) {
            if (std::filesystem::exists(rightPath)) std::filesystem::remove(rightPath);
            bSuccess = true;
        }
    } catch (const std::exception& e) { errText = e.what(); }

    if (bSuccess) {
        
        GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_COMPARE, BN_CLICKED), 0);
    } else {
        CString err; err.Format(_T("Apply failed: %s"), errText.GetString());
        AfxMessageBox(err, MB_ICONERROR);
    }
}

void CTabSpecIdCompareDlg::HandleApplySuccess(std::vector<ModelCompare::KeyDiffEntry> appliedMissing, std::vector<ModelCompare::KeyDiffEntry> appliedExtra) {
    if (m_selectedFileIndex < 0 || !m_report || m_selectedFileIndex >= (int)m_report->fileResults.size()) return;
    auto& fr = m_report->fileResults[m_selectedFileIndex];

    for (const auto& elem : appliedMissing) {
        auto it = std::remove_if(fr.missingInRight.begin(), fr.missingInRight.end(), [&](const ModelCompare::KeyDiffEntry& d) { return d.compositeKey == elem.compositeKey; });
        fr.missingInRight.erase(it, fr.missingInRight.end());
    }
    for (const auto& elem : appliedExtra) {
        auto it = std::remove_if(fr.extraInRight.begin(), fr.extraInRight.end(), [&](const ModelCompare::KeyDiffEntry& d) { return d.compositeKey == elem.compositeKey; });
        fr.extraInRight.erase(it, fr.extraInRight.end());
    }

    if (fr.missingInRight.empty() && fr.extraInRight.empty()) fr.status = ModelCompare::FileDiffResult::Status::Identical;

    int savedFileIndex = m_selectedFileIndex;
    PopulateFileList();
    m_selectedFileIndex = -1;
    int lvIndex = -1;
    for (int i = 0; i < m_listFiles.GetItemCount(); i++) {
        if ((int)m_listFiles.GetItemData(i) == savedFileIndex) { lvIndex = i; m_selectedFileIndex = savedFileIndex; break; }
    }
    if (lvIndex >= 0) {
        m_listFiles.SetItemState(lvIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_listFiles.EnsureVisible(lvIndex, FALSE);
        PopulateMismatchList(m_selectedFileIndex);
    } else { ClearMismatchList(); PopulateMismatchList(-1); }
}

std::filesystem::path CTabSpecIdCompareDlg::GetLeftFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->leftModelPath) / m_report->fileResults[fileIndex].relativePath;
}

std::filesystem::path CTabSpecIdCompareDlg::GetRightFilePath(int fileIndex) const {
    if (!m_report || fileIndex < 0) return {};
    return std::filesystem::path(m_report->rightModelPath) / m_report->fileResults[fileIndex].relativePath;
}

void CTabSpecIdCompareDlg::OnLvnItemchangedListMismatches(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR); *pResult = 0;
    if (m_bUpdatingCheckState) return;
    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState ^ pNMLV->uOldState) & LVIS_SELECTED) {
        bool isSelected = (pNMLV->uNewState & LVIS_SELECTED) != 0;
        bool isChecked = (m_listMismatches.GetCheck(pNMLV->iItem) != FALSE);
        if (isChecked != isSelected) {
            m_bUpdatingCheckState = true;
            m_listMismatches.SetCheck(pNMLV->iItem, isSelected ? TRUE : FALSE);
            m_bUpdatingCheckState = false;
        }
    }
}

HBRUSH CTabSpecIdCompareDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    switch (nCtlColor) {
    case CTLCOLOR_DLG: pDC->SetBkColor(Theme::Get()->DialogBg()); return m_brushDialogBg;
    case CTLCOLOR_STATIC: {
        UINT ctrlId = pWnd->GetDlgCtrlID(); pDC->SetBkMode(TRANSPARENT);
        if (ctrlId == IDC_STATIC_FILES_HEADER || ctrlId == IDC_STATIC_MISMATCH_HEADER) {
            pDC->SetTextColor(Theme::Get()->HeaderText()); return m_brushPanelBg;
        }
        pDC->SetTextColor(Theme::Get()->TextPrimary()); return m_brushDialogBg;
    }
    case CTLCOLOR_BTN: pDC->SetBkColor(Theme::Get()->DialogBg()); return m_brushDialogBg;
    }
    return hbr;
}

BOOL CTabSpecIdCompareDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rc; GetClientRect(&rc); pDC->FillSolidRect(&rc, Theme::Get()->DialogBg()); return TRUE;
}

void CTabSpecIdCompareDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF baseColor, pressedColor; int cornerRadius = 8; bool isOurs = true;
    switch (nIDCtl) {
    case IDC_BTN_APPLY_SELECTION: baseColor = Theme::Get()->BtnApplySelBg(); pressedColor = Theme::Get()->BtnApplySelPrsd(); break;
    case IDC_BTN_APPLY_ALL: baseColor = Theme::Get()->BtnApplyAllBg(); pressedColor = Theme::Get()->BtnApplyAllPrsd(); break;
    case IDC_BTN_VIEW_XML: baseColor = Theme::Get()->BtnViewXmlBg(); pressedColor = Theme::Get()->BtnViewXmlPrsd(); break;
    case IDC_BTN_FILE_ACTION: baseColor = Theme::Get()->AccentBlue(); pressedColor = Theme::Get()->AccentBluePressed(); cornerRadius = Theme::Get()->BtnActionRadius(); break;
    default: isOurs = false; break;
    }
    if (!isOurs) { CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct); return; }

    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC); CRect rect = lpDrawItemStruct->rcItem; UINT state = lpDrawItemStruct->itemState;
    pDC->FillSolidRect(&rect, Theme::Get()->DialogBg());
    COLORREF bgColor = baseColor, textColor = RGB(255, 255, 255);
    if (state & ODS_DISABLED) { bgColor = RGB(200, 200, 200); textColor = RGB(150, 150, 150); }
    else if (state & ODS_SELECTED) bgColor = pressedColor;

    CBrush brush(bgColor); CPen pen(PS_SOLID, 1, bgColor);
    CBrush* pOldBrush = pDC->SelectObject(&brush); CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->RoundRect(&rect, CPoint(cornerRadius, cornerRadius));

    CString text; GetDlgItem(nIDCtl)->GetWindowText(text);
    pDC->SetBkMode(TRANSPARENT); pDC->SetTextColor(textColor);
    CFont* pOldFont = pDC->SelectObject(GetDlgItem(nIDCtl)->GetFont());
    pDC->DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    pDC->SelectObject(pOldFont); pDC->SelectObject(pOldBrush); pDC->SelectObject(pOldPen);
    if (state & ODS_FOCUS) { CRect focusRect = rect; focusRect.DeflateRect(3, 3); pDC->DrawFocusRect(&focusRect); }
}

void CTabSpecIdCompareDlg::OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR); *pResult = CDRF_DODEFAULT;
    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW; return; }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        if (!m_report) return; int idx = (int)pCD->nmcd.lItemlParam; if (idx < 0 || idx >= (int)m_report->fileResults.size()) return;
        switch (m_report->fileResults[idx].status) {
        case ModelCompare::FileDiffResult::Status::DeletedInRight: pCD->clrTextBk = Theme::Get()->DeletedBg(); pCD->clrText = Theme::Get()->DeletedTxt(); break;
        case ModelCompare::FileDiffResult::Status::AddedInRight: pCD->clrTextBk = Theme::Get()->AddedBg(); pCD->clrText = Theme::Get()->AddedTxt(); break;
        case ModelCompare::FileDiffResult::Status::Modified: pCD->clrTextBk = Theme::Get()->ModifiedBg(); pCD->clrText = Theme::Get()->ModifiedTxt(); break;
        default: break;
        }
    }
}

void CTabSpecIdCompareDlg::OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR); *pResult = CDRF_DODEFAULT;
    if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW; return; }
    if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) { *pResult = CDRF_NOTIFYSUBITEMDRAW; return; }
    if (pCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
        int row = (int)pCD->nmcd.dwItemSpec, displayIdx = (int)pCD->nmcd.lItemlParam, subItem = pCD->iSubItem;
        COLORREF bg = Theme::Get()->DialogBg(), txt = Theme::Get()->TextPrimary();
        if (displayIdx >= 0 && (subItem == COL_APPLY || subItem == COL_VIEW)) {
            CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc); CRect rect; m_listMismatches.GetSubItemRect(row, subItem, LVIR_BOUNDS, rect);
            pDC->FillSolidRect(&rect, bg); rect.DeflateRect(12, 6);
            COLORREF btnColor = (subItem == COL_APPLY) ? (m_displayDiffs[displayIdx].isMissingInRight ? Theme::Get()->AccentGreen() : Theme::Get()->AccentRed()) : Theme::Get()->AccentBlue();
            CBrush brush(btnColor); CPen pen(PS_SOLID, 1, btnColor); CBrush* pOldBrush = pDC->SelectObject(&brush); CPen* pOldPen = pDC->SelectObject(&pen);
            pDC->RoundRect(&rect, CPoint(8, 8)); pDC->SetBkMode(TRANSPARENT); pDC->SetTextColor(RGB(255, 255, 255));
            
            CFont iconFont;
            CFont dynamicFont;
            CFont* pOldFont = nullptr;
            CString textToDraw;
            
            if (subItem == COL_VIEW) {
                LOGFONT lfIcon = {0};
                wcscpy_s(lfIcon.lfFaceName, _T("Segoe MDL2 Assets"));
                lfIcon.lfHeight = Theme::Get()->FontSizeIcon();
                iconFont.CreateFontIndirect(&lfIcon);
                pOldFont = pDC->SelectObject(&iconFont);
                textToDraw = _T("\xE890");
            } else {
                textToDraw = m_listMismatches.GetItemText(row, subItem);
                LOGFONT lf; m_listMismatches.GetFont()->GetLogFont(&lf);
                int buttonWidth = rect.Width(); if (buttonWidth < 85) { double scale = (double)buttonWidth / 85.0; if (scale < 0.65) scale = 0.65; lf.lfHeight = (LONG)(lf.lfHeight * scale); }
                dynamicFont.CreateFontIndirect(&lf); pOldFont = pDC->SelectObject(&dynamicFont);
            }
            
            pDC->DrawText(textToDraw, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            pDC->SelectObject(pOldFont); pDC->SelectObject(pOldBrush); pDC->SelectObject(pOldPen); *pResult = CDRF_SKIPDEFAULT; return;
        }
        pCD->clrTextBk = bg; pCD->clrText = txt; *pResult = CDRF_DODEFAULT; return;
    }
}
