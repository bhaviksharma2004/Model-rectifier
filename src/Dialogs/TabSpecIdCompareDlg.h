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

};
