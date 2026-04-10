#include "pch.h"
#include "TabSpecValueCompareDlg.h"

BEGIN_MESSAGE_MAP(CTabSpecValueCompareDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CTabSpecValueCompareDlg::CTabSpecValueCompareDlg(CWnd* pParent)
    : CDialogEx(IDD_TAB_SPEC_VALUE_COMPARE, pParent) {
    m_brushBg.CreateSolidBrush(RGB(243, 243, 248)); // Match CLR_DIALOG_BG
}

BOOL CTabSpecValueCompareDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CTabSpecValueCompareDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
}

HBRUSH CTabSpecValueCompareDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    return m_brushBg;
}

BOOL CTabSpecValueCompareDlg::OnEraseBkgnd(CDC* pDC) {
    CRect rect;
    GetClientRect(&rect);
    pDC->FillSolidRect(&rect, RGB(243, 243, 248));
    return TRUE;
}
