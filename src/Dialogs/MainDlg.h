// =============================================================================
// MainDlg.h — Redesigned two-panel layout with premium UI enhancements.
//
// LEFT:  File list (filename only, color-coded by status)
// RIGHT: ID mismatches with per-row View/Apply buttons + Status column
// =============================================================================
#pragma once

#include "Engine/DiffTypes.h"
#include <afxbutton.h>    // CMFCButton
#include <memory>
#include <vector>

#define WM_COMPARE_COMPLETE  (WM_USER + 100)

// Merged diff entry for display (combines missing + extra)
struct DisplayDiffEntry {
    bool isMissingInRight;  // true = deleted from right, false = added to right
    ModelCompare::KeyDiffEntry entry;
};

class CMainDlg : public CDialogEx {
public:
    CMainDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_MODELCOMPARE_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // --- Command Handlers ---
    afx_msg void OnBnClickedBrowseLeft();
    afx_msg void OnBnClickedBrowseRight();
    afx_msg void OnBnClickedCompare();
    afx_msg void OnBnClickedApplyAll();
    afx_msg void OnBnClickedViewXml();
    afx_msg void OnBnClickedFileAction();

    // --- Layout & Sizing ---
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

    // --- List Notifications ---
    afx_msg void OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListClick(NMHDR* pNMHDR, LRESULT* pResult);

    // --- Tooltip Handlers (Part 1.1) ---
    afx_msg void OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);

    // --- Premium Styling (Part 3) ---
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

    // --- Async Compare ---
    afx_msg LRESULT OnCompareComplete(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // =========================================================================
    // Controls
    // =========================================================================
    CEdit       m_editLeftPath;
    CEdit       m_editRightPath;
    CButton     m_btnBrowseLeft;
    CButton     m_btnBrowseRight;
    CButton     m_btnCompare;       // Premium styled custom button
    CStatic     m_staticSummary;
    CStatic     m_staticFilesHeader;
    CListCtrl   m_listFiles;
    CStatic     m_staticMismatchHeader;
    CButton     m_btnApplyAll;
    CButton     m_btnViewXml;
    CButton     m_btnFileAction;
    CStatic     m_staticBoundary;
    CListCtrl   m_listMismatches;
    CImageList  m_imageListMismatch;

    // =========================================================================
    // Data
    // =========================================================================
    std::unique_ptr<ModelCompare::ModelDiffReport> m_report;
    int m_selectedFileIndex = -1;
    std::vector<DisplayDiffEntry> m_displayDiffs;

    // =========================================================================
    // Helpers
    // =========================================================================
    CString BrowseForFolder(const CString& title);
    void SetupListColumns();
    void ResizeListColumns();
    void PopulateFileList();
    void PopulateMismatchList(int fileIndex);
    void ClearMismatchList();
    void RepositionControls(int cx, int cy);
    void EnableCompareUI(bool enable);
    void UpdateSummary(const CString& text);
    void RunCompare();
    void OpenXmlViewer(const std::string& scrollToKey = "", bool isMissing = false, const CString& targetSearch = _T(""));
    void ApplySingleDiff(int displayIndex);

    // Path helpers
    std::filesystem::path GetLeftFilePath(int fileIndex) const;
    std::filesystem::path GetRightFilePath(int fileIndex) const;

    // =========================================================================
    // UI Fonts & Brushes
    // =========================================================================
    CFont  m_uiFont;            // General UI font (Segoe UI)
    CFont  m_headerFont;        // Bold header font (Segoe UI Semibold, 11pt)

    // Premium Light-Mode Color Palette
    // Inspired by Visual Studio / modern Office light themes
    CBrush m_brushDialogBg;     // Main dialog background
    CBrush m_brushPanelBg;      // Panel/header background accent
    CBrush m_brushEditBg;       // Edit control background

    // Color constants — Premium Light Theme
    static constexpr COLORREF CLR_DIALOG_BG     = RGB(243, 243, 248);   // Soft blue-gray background
    static constexpr COLORREF CLR_PANEL_BG      = RGB(235, 237, 245);   // Subtle panel tint
    static constexpr COLORREF CLR_EDIT_BG       = RGB(255, 255, 255);   // Clean white inputs
    static constexpr COLORREF CLR_EDIT_BORDER   = RGB(190, 198, 212);   // Soft blue-gray border
    static constexpr COLORREF CLR_TEXT_PRIMARY   = RGB(30, 30, 40);     // Near-black text
    static constexpr COLORREF CLR_TEXT_SECONDARY = RGB(90, 95, 110);    // Muted secondary
    static constexpr COLORREF CLR_HEADER_TEXT    = RGB(25, 60, 140);    // Deep blue headers
    static constexpr COLORREF CLR_ACCENT_BLUE    = RGB(13, 110, 253);   // #0D6EFD — Compare button
    static constexpr COLORREF CLR_ACCENT_GREEN   = RGB(25, 135, 84);    // #198754 — Apply All
    static constexpr COLORREF CLR_ACCENT_TEAL    = RGB(13, 202, 240);   // #0DCAF0 — View XML
    static constexpr COLORREF CLR_ACCENT_RED     = RGB(220, 53, 69);    // #DC3545 — Remove
    static constexpr COLORREF CLR_SUMMARY_BG     = RGB(230, 240, 255);  // Light blue summary bar

    // Semantic row colors (kept from original for diff highlighting)
    static constexpr COLORREF CLR_DELETED_BG   = RGB(255, 215, 215);
    static constexpr COLORREF CLR_DELETED_TXT  = RGB(170, 0, 0);
    static constexpr COLORREF CLR_ADDED_BG     = RGB(215, 255, 215);
    static constexpr COLORREF CLR_ADDED_TXT    = RGB(0, 110, 0);
    static constexpr COLORREF CLR_MODIFIED_BG  = RGB(255, 245, 210);
    static constexpr COLORREF CLR_MODIFIED_TXT = RGB(170, 120, 0);
    static constexpr COLORREF CLR_MISSING_BG   = RGB(255, 230, 225);
    static constexpr COLORREF CLR_MISSING_TXT  = RGB(170, 45, 25);
    static constexpr COLORREF CLR_EXTRA_BG     = RGB(225, 253, 238);
    static constexpr COLORREF CLR_EXTRA_TXT    = RGB(0, 115, 60);
};
