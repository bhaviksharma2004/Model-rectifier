// =============================================================================
// XmlViewerDlg.h
// Modal dialog that displays XML content with highlighted diff nodes.
// =============================================================================
#pragma once

#include "Engine/DiffTypes.h"
#include <vector>
#include <string>
#include <filesystem>

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
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    CRichEditCtrl m_richEdit;
    std::filesystem::path m_xmlPath;
    CString m_title;

    // Diff data
    std::vector<ModelCompare::KeyDiffEntry> m_missing;
    std::vector<ModelCompare::KeyDiffEntry> m_extra;
    std::string m_scrollToKey;
    bool m_scrollToIsMissing = false;
    CString m_targetSearch;

    void LoadAndHighlight();
    void ApplySyntaxHighlighting();
    int FindValLine(const std::vector<CString>& lines,
                    const std::string& groupId,
                    const std::string& specId,
                    const std::string& valId);
    void HighlightLine(int lineIndex, COLORREF bgColor);
    void ScrollToLine(int lineIndex);
};
