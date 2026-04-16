
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

    
    void SetValidationReport(std::shared_ptr<ModelCompare::ModelValidationReport> report);

    
    void ResizeListColumns();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedViewAll();

    afx_msg void OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnIssueListClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnIssueListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnIssueListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

    DECLARE_MESSAGE_MAP()

private:
    
    CStatic    m_staticFileHeader;
    CListCtrl  m_listFiles;
    CStatic    m_staticIssuesHeader;
    CListCtrl  m_listIssues;
    CButton    m_btnViewAll;
    CButton    m_btnCorruptInfo;
    CStatic    m_staticBoundary;
    CStatic    m_staticCorruptDesc;
    CImageList m_imageListIssues;

    
    std::shared_ptr<ModelCompare::ModelValidationReport> m_report;
    int m_selectedFileIndex = -1;

    struct HoverState {
        int index = -1;
        int fadeIndex = -1;
        int fadeStep = 0;
        bool isTracking = false;
    };
    HoverState m_hoverFiles;
    HoverState m_hoverIssues;

    
    void SetupListColumns();
    void PopulateFileList();
    void PopulateIssueList(int fileIndex);
    void ClearIssueList();

    
    
    void OpenXmlViewer(const ModelCompare::FileValidationResult& fileResult,
                       const ModelCompare::ValidationIssue* scrollToIssue = nullptr);

    
    const ModelCompare::FileValidationResult* GetSelectedFileResult() const;

    
    CFont  m_uiFont;
    CFont  m_headerFont;

    
    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;

    static constexpr COLORREF CLR_HIGHLIGHT_WARNING_DK   = RGB(102, 26, 26);
    static constexpr COLORREF CLR_HIGHLIGHT_DUPLICATE_DK  = RGB(102, 80, 0);
};
