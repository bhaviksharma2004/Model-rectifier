// =============================================================================
// MainDlg.h — Redesigned two-panel layout.
//
// LEFT:  File list (filename only, color-coded by status)
// RIGHT: ID mismatches with per-row View/Apply buttons
// =============================================================================
#pragma once

#include "Engine/DiffTypes.h"
#include <memory>
#include <vector>

#define WM_COMPARE_COMPLETE  (WM_USER + 100)

// Merged diff entry for display (combines missing + extra)
struct DisplayDiffEntry {
    bool isMissingInRight;  // true = tomato, false = blue
    ModelCompare::KeyDiffEntry entry;
};

class CMainDlg : public CDialogEx {
public:
    CMainDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_MODELCOMPARE_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnBnClickedBrowseLeft();
    afx_msg void OnBnClickedBrowseRight();
    afx_msg void OnBnClickedCompare();
    afx_msg void OnBnClickedApplyAll();
    afx_msg void OnBnClickedViewXml();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnCompareComplete(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // Controls
    CEdit     m_editLeftPath;
    CEdit     m_editRightPath;
    CButton   m_btnBrowseLeft;
    CButton   m_btnBrowseRight;
    CButton   m_btnCompare;
    CStatic   m_staticSummary;
    CStatic   m_staticFilesHeader;
    CListCtrl m_listFiles;
    CStatic   m_staticMismatchHeader;
    CButton   m_btnApplyAll;
    CButton   m_btnViewXml;
    CListCtrl m_listMismatches;

    // Data
    std::unique_ptr<ModelCompare::ModelDiffReport> m_report;
    int m_selectedFileIndex = -1;
    std::vector<DisplayDiffEntry> m_displayDiffs;

    // Helpers
    CString BrowseForFolder(const CString& title);
    void SetupListColumns();
    void PopulateFileList();
    void PopulateMismatchList(int fileIndex);
    void ClearMismatchList();
    void RepositionControls(int cx, int cy);
    void EnableCompareUI(bool enable);
    void UpdateSummary(const CString& text);
    void RunCompare();
    void OpenXmlViewer(const std::string& scrollToKey = "", bool isMissing = false);
    void ApplySingleDiff(int displayIndex);

    // Get paths to left/right files for the selected diff file
    std::filesystem::path GetLeftFilePath(int fileIndex) const;
    std::filesystem::path GetRightFilePath(int fileIndex) const;

    // UI Enhancements
    CFont m_uiFont;

    // Colors
    static constexpr COLORREF CLR_DELETED_BG   = RGB(255, 215, 215);
    static constexpr COLORREF CLR_DELETED_TXT  = RGB(170, 0, 0);
    static constexpr COLORREF CLR_ADDED_BG     = RGB(215, 255, 215);
    static constexpr COLORREF CLR_ADDED_TXT    = RGB(0, 110, 0);
    static constexpr COLORREF CLR_MODIFIED_BG  = RGB(255, 245, 210);
    static constexpr COLORREF CLR_MODIFIED_TXT = RGB(170, 120, 0);
    static constexpr COLORREF CLR_MISSING_BG   = RGB(255, 230, 225);
    static constexpr COLORREF CLR_MISSING_TXT  = RGB(170, 45, 25);
    static constexpr COLORREF CLR_EXTRA_BG     = RGB(225, 238, 255);
    static constexpr COLORREF CLR_EXTRA_TXT    = RGB(0, 75, 170);
};
