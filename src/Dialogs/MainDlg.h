





#pragma once

#include "Engine/DiffTypes.h"
#include <afxbutton.h>
#include "TabSpecIdCompareDlg.h"
#include "TabXmlValidationDlg.h"
#include "TabSpecValueCompareDlg.h"
#include <memory>
#include <vector>

#define WM_COMPARE_COMPLETE  (WM_USER + 100)
#define WM_VALIDATE_COMPLETE (WM_USER + 101)


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
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()

private:

    CEdit       m_editLeftPath;
    CEdit       m_editRightPath;
    CButton     m_btnBrowseLeft;
    CButton     m_btnBrowseRight;
    CButton     m_btnCompare;
    CStatic     m_staticSummary;

    CTabCtrl    m_tabMain;
    CTabSpecIdCompareDlg m_tabSpecId;
    CTabXmlValidationDlg m_tabXmlVal;
    CTabSpecValueCompareDlg m_tabSpecVal;

    std::shared_ptr<ModelCompare::ModelDiffReport> m_report;
    std::shared_ptr<ModelCompare::ValidationReport> m_validationReport;

    CString BrowseForFolder(const CString& title);
    void RepositionControls(int cx, int cy);
    void EnableCompareUI(bool enable);
    void UpdateSummary(const CString& text);
    void RunCompare();
    void RunValidation();

    CFont  m_uiFont;
    CFont  m_headerFont;

    CBrush m_brushDialogBg;
    CBrush m_brushPanelBg;
    CBrush m_brushEditBg;


    static constexpr COLORREF CLR_DIALOG_BG     = RGB(243, 243, 248);
    static constexpr COLORREF CLR_EDIT_BG       = RGB(255, 255, 255);
    static constexpr COLORREF CLR_TEXT_PRIMARY  = RGB(30, 30, 40);
    static constexpr COLORREF CLR_TEXT_SECONDARY = RGB(90, 95, 110);
    static constexpr COLORREF CLR_ACCENT_BLUE   = RGB(13, 110, 253);
};
