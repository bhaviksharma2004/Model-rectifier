// =============================================================================
// XmlViewerDlg.h
// Production-ready modal dialog that displays XML content with highlighted diff
// nodes. Features: dark mode, ClearType fonts, batch syntax highlighting,
// horizontal touchpad scroll, DWM dark scrollbars, exception-safe loading.
// =============================================================================
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

// Validation highlight — maps a line number to a background color
struct ValidationHighlight {
    int lineNumber;     // 0-based line index
    COLORREF bgColor;
};

class CXmlViewerDlg : public CDialogEx {
public:
    CXmlViewerDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_XMLVIEWER_DIALOG };

    // ── Existing API (used by TabSpecIdCompareDlg) ──
    void SetFile(const std::filesystem::path& xmlPath, const std::filesystem::path& leftPath, const CString& title);
    void SetDiffs(
        const std::vector<ModelCompare::KeyDiffEntry>& missing,
        const std::vector<ModelCompare::KeyDiffEntry>& extra
    );
    void SetScrollToKey(const std::string& compositeKey, bool isMissing);
    void SetSearchTarget(const CString& targetSearch);

    // ── New API for validation tab (cached content, no disk I/O) ──
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

    // ── Cached content for validation tab (no disk I/O path) ──
    bool m_useCachedContent = false;
    CString m_cachedFullText;
    std::vector<CString> m_cachedLines;
    std::vector<ValidationHighlight> m_validationHighlights;
    int m_scrollToLineNumber = -1;

    // Dark mode color constants
    static constexpr COLORREF CLR_BG_DARK    = RGB(30, 30, 30);    // #1E1E1E
    static constexpr COLORREF CLR_TEXT_LIGHT  = RGB(212, 212, 212); // #D4D4D4

    // VS Code Dark+ syntax colors
    static constexpr COLORREF CLR_TAG        = RGB(86, 156, 214);   // Blue — tags
    static constexpr COLORREF CLR_ATTR       = RGB(156, 220, 254);  // Cyan — attributes
    static constexpr COLORREF CLR_STRING     = RGB(206, 145, 120);  // Orange — strings
    static constexpr COLORREF CLR_COMMENT    = RGB(106, 153, 85);   // Green — comments

    // Diff highlight colors
    static constexpr COLORREF CLR_DIFF_EXTRA_BG   = RGB(45, 95, 55);   // Dark Forest Green
    static constexpr COLORREF CLR_DIFF_MISSING_BG = RGB(120, 45, 45);  // Dark Crimson Red

    // Core methods
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
