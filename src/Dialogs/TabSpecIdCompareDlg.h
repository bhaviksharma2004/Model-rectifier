#pragma once

#include "Engine/DiffTypes.h"
#include "resource.h"
#include <afxbutton.h>
#include <memory>
#include <vector>

struct DisplayDiffEntry {
    bool isMissingInRight;
    ModelCompare::KeyDiffEntry entry;
};

class CTabSpecIdCompareDlg : public CDialogEx {
public:
    CTabSpecIdCompareDlg(CWnd* pParent = nullptr);
    virtual ~CTabSpecIdCompareDlg();
    enum { IDD = IDD_TAB_SPEC_ID_COMPARE };

    void SetReport(std::shared_ptr<ModelCompare::ModelDiffReport> report);
    void ResizeListColumns();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBnClickedApplyAll();
    afx_msg void OnBnClickedApplySelection();
    afx_msg void OnBnClickedViewXml();
    afx_msg void OnBnClickedFileAction();

    afx_msg void OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnLvnItemchangedListMismatches(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    CStatic     m_staticFilesHeader;
    CListCtrl   m_listFiles;
    CStatic     m_staticMismatchHeader;
    CButton     m_btnApplyAll;
    CButton     m_btnApplySelection;
    CButton     m_btnViewXml;
    CButton     m_btnFileAction;
    CStatic     m_staticBoundary;
    CListCtrl   m_listMismatches;
    CImageList  m_imageListMismatch;

    std::shared_ptr<ModelCompare::ModelDiffReport> m_report;
    int m_selectedFileIndex = -1;
    std::vector<DisplayDiffEntry> m_displayDiffs;
    bool m_bUpdatingCheckState = false;

    void SetupListColumns();
    void PopulateFileList();
    void PopulateMismatchList(int fileIndex);
    void ClearMismatchList();
    void OpenXmlViewer(const std::string& scrollToKey = "", bool isMissing = false, const CString& targetSearch = _T(""));
    void ApplySingleDiff(int displayIndex);
    void HandleApplySuccess(std::vector<ModelCompare::KeyDiffEntry> appliedMissing, 
                            std::vector<ModelCompare::KeyDiffEntry> appliedExtra);

    std::filesystem::path GetLeftFilePath(int fileIndex) const;
    std::filesystem::path GetRightFilePath(int fileIndex) const;

    CFont  m_uiFont;
    CFont  m_headerFont;

    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;

    static constexpr COLORREF CLR_DIALOG_BG     = RGB(243, 243, 248);
    static constexpr COLORREF CLR_PANEL_BG      = RGB(235, 237, 245);
    static constexpr COLORREF CLR_TEXT_PRIMARY   = RGB(30, 30, 40);
    static constexpr COLORREF CLR_TEXT_SECONDARY = RGB(90, 95, 110);
    static constexpr COLORREF CLR_HEADER_TEXT    = RGB(25, 60, 140);
    static constexpr COLORREF CLR_ACCENT_BLUE    = RGB(13, 110, 253);
    static constexpr COLORREF CLR_ACCENT_GREEN   = RGB(25, 135, 84);
    static constexpr COLORREF CLR_ACCENT_TEAL    = RGB(13, 202, 240);
    static constexpr COLORREF CLR_ACCENT_RED     = RGB(220, 53, 69);

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
