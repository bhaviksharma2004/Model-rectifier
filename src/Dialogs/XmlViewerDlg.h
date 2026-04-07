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

// Internal message: deferred content loading after dialog is visible
#define WM_XMLLOAD_DEFERRED  (WM_USER + 201)
// Internal message: deferred deselection after load completes
#define WM_XMLDESELECT       (WM_USER + 202)

// ---------------------------------------------------------------------------
// ColorRange — Describes a span of text to be colored during batch formatting.
// ---------------------------------------------------------------------------
struct ColorRange {
    int start;
    int end;
    COLORREF color;
};

class CXmlViewerDlg : public CDialogEx {
public:
    CXmlViewerDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_XMLVIEWER_DIALOG };

    // Set the XML file to display and the diffs to highlight
    void SetFile(const std::filesystem::path& xmlPath, const CString& title);
    void SetDiffs(
        const std::vector<ModelCompare::KeyDiffEntry>& missing,
        const std::vector<ModelCompare::KeyDiffEntry>& extra
    );
    void SetScrollToKey(const std::string& compositeKey, bool isMissing);
    void SetSearchTarget(const CString& targetSearch);

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // Message handlers
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnDeferredLoad(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDeferredDeselect(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CRichEditCtrl m_richEdit;
    CFont         m_codeFont;       // ClearType anti-aliased programming font
    CBrush        m_brushDarkBg;    // Dark background brush for dialog
    std::filesystem::path m_xmlPath;
    CString m_title;

    // Diff data
    std::vector<ModelCompare::KeyDiffEntry> m_missing;
    std::vector<ModelCompare::KeyDiffEntry> m_extra;
    std::string m_scrollToKey;
    bool m_scrollToIsMissing = false;
    CString m_targetSearch;

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
    void HighlightLine(int lineIndex, COLORREF bgColor);
    void ScrollToLine(int lineIndex);

    // Error reporting
    void ShowError(const CString& title, const CString& detail);
};
