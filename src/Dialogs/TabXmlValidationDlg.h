#pragma once
#include "resource.h"

class CTabXmlValidationDlg : public CDialogEx {
public:
    CTabXmlValidationDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_TAB_XML_VALIDATION };

protected:
    virtual BOOL OnInitDialog() override;
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    CBrush m_brushBg;
};
