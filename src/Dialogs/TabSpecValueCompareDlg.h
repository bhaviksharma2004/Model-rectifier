#pragma once
#include "resource.h"
#include "Engine/ValueCompareEngine.h"
#include <memory>
#include <vector>

class CTabSpecValueCompareDlg : public CDialogEx {
public:
    CTabSpecValueCompareDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_TAB_SPEC_VALUE_COMPARE };

    void SetReport(std::shared_ptr<ModelCompare::ValueDiffReport> report);
    void ResizeListColumns();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    afx_msg void OnFileListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFileListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMismatchListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void SetupListColumns();
    void PopulateFileList();
    void PopulateMismatchList(int fileIndex);
    void ClearMismatchList();

    CStatic     m_staticFilesHeader;
    CListCtrl   m_listFiles;
    CStatic     m_staticMismatchHeader;
    CListCtrl   m_listMismatches;

    CFont       m_uiFont;
    CFont       m_headerFont;

    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;

    int m_selectedFileIndex = -1;
    std::shared_ptr<ModelCompare::ValueDiffReport> m_report;

    struct HoverState {
        int index = -1;
        int fadeIndex = -1;
        int fadeStep = 0;
        bool isTracking = false;
    };
    HoverState m_hoverFiles;
    HoverState m_hoverMiss;

};
