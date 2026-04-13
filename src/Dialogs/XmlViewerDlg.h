





#pragma once

#include "Engine/DiffTypes.h"
#include <vector>
#include <string>
#include <filesystem>


#define WM_XMLLOAD_DEFERRED  (WM_USER + 201)
#define WM_XMLDESELECT       (WM_USER + 202)


struct ColorRange {
    int start;
    int end;
    COLORREF color;
};


struct ValidationHighlight {
    int lineNumber;     
    COLORREF bgColor;
};

class CXmlViewerDlg : public CDialogEx {
public:
    CXmlViewerDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_XMLVIEWER_DIALOG };

    
    void SetFile(const std::filesystem::path& xmlPath, const std::filesystem::path& leftPath, const CString& title);
    void SetDiffs(
        const std::vector<ModelCompare::KeyDiffEntry>& missing,
        const std::vector<ModelCompare::KeyDiffEntry>& extra
    );
    void SetScrollToKey(const std::string& compositeKey, bool isMissing);
    void SetSearchTarget(const CString& targetSearch);

    
    void SetCachedContent(const CString& fullText,
                          const std::vector<CString>& lines,
                          const CString& title);
    void SetValidationHighlights(const std::vector<ValidationHighlight>& highlights);
    void SetScrollToLine(int lineNumber);

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;


    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnDeferredLoad(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDeferredDeselect(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CRichEditCtrl m_richEdit;
    CFont         m_codeFont;
    CBrush        m_brushDarkBg;
    std::filesystem::path m_xmlPath;
    std::filesystem::path m_leftPath;
    CString m_title;


    std::vector<ModelCompare::KeyDiffEntry> m_missing;
    std::vector<ModelCompare::KeyDiffEntry> m_extra;
    std::string m_scrollToKey;
    bool m_scrollToIsMissing = false;
    CString m_targetSearch;

    
    bool m_useCachedContent = false;
    CString m_cachedFullText;
    std::vector<CString> m_cachedLines;
    std::vector<ValidationHighlight> m_validationHighlights;
    int m_scrollToLineNumber = -1;


    
    void LoadAndHighlight();
    void CollectSyntaxRanges(const CString& text, std::vector<ColorRange>& ranges);
    void ApplyColorRanges(const std::vector<ColorRange>& ranges);

    int FindValLine(const std::vector<CString>& lines,
                    const std::string& groupId,
                    const std::string& specId,
                    const std::string& valId);
    
    int FindBlockEnd(const std::vector<CString>& lines, int startLine);
    std::vector<CString> ExtractBlock(const std::vector<CString>& lines, int startLine);
    void HighlightBlock(int startLine, int endLine, COLORREF bgColor);
    
    int GetSortedInsertLine(const std::vector<CString>& lines, int parentStartLine, 
                            ModelCompare::DiffLevel level, const std::string& targetId);
    
    void ScrollToLine(int lineIndex);


    void ShowError(const CString& title, const CString& detail);
};
