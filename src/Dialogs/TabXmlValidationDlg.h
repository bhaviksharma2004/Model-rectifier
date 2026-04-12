// =============================================================================
// TabXmlValidationDlg.h
// Read-only diagnostic tab for identifying XML corruption and logical duplicates.
//
// Layout:
//   LEFT PANEL (25%):  Two stacked file lists — Left Model (top 50%) and
//                      Right Model (bottom 50%), each with fixed header and
//                      independent scrollbar.
//   RIGHT PANEL (75%): Issue list with columns: #, Type, Description, View.
//                      "View All in XML" button at top-right.
//                      Corrupt-file overlay (centered label) when applicable.
//
// No apply/save/modification features — strictly read-only diagnostics.
// =============================================================================
#pragma once

#include "resource.h"
#include "Engine/XmlValidationEngine.h"
#include <memory>
#include <vector>

class CTabXmlValidationDlg : public CDialogEx {
public:
    CTabXmlValidationDlg(CWnd* pParent = nullptr);
    ~CTabXmlValidationDlg();
    enum { IDD = IDD_TAB_XML_VALIDATION };

    /// Called from MainDlg after validation completes on background thread.
    void SetValidationReport(std::shared_ptr<ModelCompare::ValidationReport> report);

    /// Re-proportion column widths after resize.
    void ResizeListColumns();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // ── Message handlers ──
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBnClickedViewAll();

    afx_msg void OnLeftFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRightFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnIssueListClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnIssueListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLeftFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRightFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnIssueListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLeftFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRightFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

    DECLARE_MESSAGE_MAP()

private:
    // ── Controls ──
    CStatic    m_staticLeftHeader;
    CStatic    m_staticRightHeader;
    CListCtrl  m_listLeftFiles;
    CListCtrl  m_listRightFiles;
    CStatic    m_staticIssuesHeader;
    CListCtrl  m_listIssues;
    CButton    m_btnViewAll;
    CButton    m_btnCorruptInfo;
    CStatic    m_staticBoundary;      // Border frame for corrupt-file overlay
    CImageList m_imageListIssues;     // Row height spacer for issue list

    // ── Data ──
    std::shared_ptr<ModelCompare::ValidationReport> m_report;

    enum class SelectedModel { None, Left, Right };
    SelectedModel m_selectedModel = SelectedModel::None;
    int m_selectedFileIndex = -1;     // Index into the appropriate ModelValidationReport.fileResults

    // ── Methods ──
    void SetupListColumns();
    void PopulateFileLists();
    void PopulateIssueList(SelectedModel model, int fileIndex);
    void ClearIssueList();
    void ClearFileSelection(SelectedModel exceptModel);

    /// Open XmlViewerDlg with cached content and optional scroll-to-issue.
    /// If scrollToIssue is nullptr, highlights ALL issues (View All mode).
    void OpenXmlViewer(const ModelCompare::FileValidationResult& fileResult,
                       const ModelCompare::ValidationIssue* scrollToIssue = nullptr);

    /// Get the currently selected file result, or nullptr if none.
    const ModelCompare::FileValidationResult* GetSelectedFileResult() const;

    /// Get the ModelValidationReport for the given model side.
    const ModelCompare::ModelValidationReport* GetModelReport(SelectedModel model) const;

    // ── Fonts ──
    CFont  m_uiFont;
    CFont  m_headerFont;

    // ── Brushes ──
    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;

    // ── Color palette — matches TabSpecIdCompareDlg ──
    static constexpr COLORREF CLR_DIALOG_BG      = RGB(243, 243, 248);
    static constexpr COLORREF CLR_PANEL_BG       = RGB(235, 237, 245);
    static constexpr COLORREF CLR_TEXT_PRIMARY    = RGB(30, 30, 40);
    static constexpr COLORREF CLR_TEXT_SECONDARY  = RGB(90, 95, 110);
    static constexpr COLORREF CLR_HEADER_TEXT     = RGB(25, 60, 140);
    static constexpr COLORREF CLR_ACCENT_BLUE     = RGB(13, 110, 253);
    static constexpr COLORREF CLR_ACCENT_RED      = RGB(220, 53, 69);

    // Issue highlight colors for dark-mode XmlViewer popup
    static constexpr COLORREF CLR_HIGHLIGHT_WARNING_DK   = RGB(102, 26, 26);   // #661A1A
    static constexpr COLORREF CLR_HIGHLIGHT_DUPLICATE_DK  = RGB(102, 80, 0);    // #665000
};
