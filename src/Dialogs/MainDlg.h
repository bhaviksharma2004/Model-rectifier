#pragma once

#include "Engine/DiffTypes.h"
#include "Engine/ValueCompareEngine.h"
#include <afxbutton.h>
#include "TabSpecIdCompareDlg.h"
#include "TabXmlValidationDlg.h"
#include "TabSpecValueCompareDlg.h"
#include <memory>
#include <vector>
#include <thread>

#define WM_COMPARE_COMPLETE  (WM_USER + 100)
#define WM_VALIDATE_COMPLETE (WM_USER + 101)
#define WM_VALUE_COMPARE_COMPLETE (WM_USER + 102)


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

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

    afx_msg void OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg LRESULT OnCompareComplete(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnValidateComplete(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnValueCompareComplete(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

private:
    CEdit       m_editLeftPath;
    CEdit       m_editRightPath;
    CButton     m_btnBrowseLeft;
    CButton     m_btnBrowseRight;
    CButton     m_btnCompare;
    CStatic     m_staticSummary;
    CStatic     m_staticChecksLegend;

    CTabCtrl    m_tabMain;
    CTabSpecIdCompareDlg m_tabSpecId;
    CTabXmlValidationDlg m_tabXmlVal;
    CTabSpecValueCompareDlg m_tabSpecVal;

    std::shared_ptr<ModelCompare::ModelDiffReport> m_report;
    std::shared_ptr<ModelCompare::ModelValidationReport> m_validationReport;
    std::shared_ptr<ModelCompare::ValueDiffReport> m_valueReport;

    CString BrowseForFolder(const CString& title);
    void RepositionControls(int cx, int cy);
    void EnableCompareUI(bool enable);
    void UpdateSummary(const CString& text);
    void RunCompare();
    void RunValidation();
    void RunValueCompare();

    std::thread m_workerThread;
    bool m_bClosing = false;

    CFont  m_uiFont;
    CFont  m_headerFont;
    CFont  m_legendFont;

    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;
    CBrush m_brushEditBg;

};
