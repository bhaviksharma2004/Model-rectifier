#include "pch.h"
#include "TabXmlValidationDlg.h"

BEGIN_MESSAGE_MAP(CTabXmlValidationDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CTabXmlValidationDlg::CTabXmlValidationDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_XML_VALIDATION, pParent) {
    m_brushBg.CreateSolidBrush(RGB(243, 243, 248)); // Match CLR_DIALOG_BG
}

BOOL CTabXmlValidationDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CTabXmlValidationDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
}

HBRUSH CTabXmlValidationDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    return m_brushBg;
}

BOOL CTabXmlValidationDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rect;
    GetClientRect(&rect);
    pDC->FillSolidRect(&rect, RGB(243, 243, 248));
    return TRUE;
}
