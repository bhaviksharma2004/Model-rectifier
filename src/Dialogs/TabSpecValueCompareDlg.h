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

    afx_msg void OnSize(UINT nType, int cx, int cy);
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

    int m_selectedFileIndex = -1;
    std::shared_ptr<ModelCompare::ValueDiffReport> m_report;

    static constexpr COLORREF CLR_DIALOG_BG     = RGB(243, 243, 248);
    static constexpr COLORREF CLR_TEXT_PRIMARY   = RGB(30, 30, 40);
    static constexpr COLORREF CLR_HEADER_TEXT    = RGB(25, 60, 140);
    static constexpr COLORREF CLR_MODIFIED_BG    = RGB(255, 245, 210);
    static constexpr COLORREF CLR_MODIFIED_TXT   = RGB(170, 120, 0);
};
