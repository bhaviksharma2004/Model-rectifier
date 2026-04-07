// =============================================================================
// XmlViewerDlg.cpp
// XML viewer with syntax highlighting for diff nodes.
// =============================================================================
#include "pch.h"
#include "resource.h"
#include "XmlViewerDlg.h"
#include <fstream>
#include <sstream>
#include <richedit.h>

BEGIN_MESSAGE_MAP(CXmlViewerDlg, CDialogEx)
    ON_WM_SIZE()
END_MESSAGE_MAP()

CXmlViewerDlg::CXmlViewerDlg(CWnd* pParent)
    : CDialogEx(IDD_XMLVIEWER_DIALOG, pParent) {}

void CXmlViewerDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RICHEDIT_XML, m_richEdit);
}

void CXmlViewerDlg::SetFile(const std::filesystem::path& xmlPath, const CString& title) {
    m_xmlPath = xmlPath;
    m_title = title;
}

void CXmlViewerDlg::SetDiffs(
    const std::vector<ModelCompare::KeyDiffEntry>& missing,
    const std::vector<ModelCompare::KeyDiffEntry>& extra) {
    m_missing = missing;
    m_extra = extra;
}

void CXmlViewerDlg::SetScrollToKey(const std::string& compositeKey, bool isMissing) {
    m_scrollToKey = compositeKey;
    m_scrollToIsMissing = isMissing;
}

void CXmlViewerDlg::SetSearchTarget(const CString& targetSearch) {
    m_targetSearch = targetSearch;
}

BOOL CXmlViewerDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    SetWindowText(m_title);

    // Configure rich edit
    m_richEdit.SetReadOnly(TRUE);
    CHARFORMAT cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    wcscpy_s(cf.szFaceName, _T("Consolas"));
    cf.yHeight = 200; // 10pt
    m_richEdit.SetDefaultCharFormat(cf);
    m_richEdit.SetBackgroundColor(FALSE, RGB(30, 30, 30));

    // Set default text color to light gray
    CHARFORMAT cfDefault = {};
    cfDefault.cbSize = sizeof(cfDefault);
    cfDefault.dwMask = CFM_COLOR;
    cfDefault.crTextColor = RGB(212, 212, 212);
    m_richEdit.SetSel(0, -1);
    m_richEdit.SetSelectionCharFormat(cfDefault);
    m_richEdit.SetSel(0, 0);

    LoadAndHighlight();

    // Resize to reasonable size
    CRect screenRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
    int w = (int)(screenRect.Width() * 0.6);
    int h = (int)(screenRect.Height() * 0.8);
    SetWindowPos(nullptr,
        (screenRect.Width() - w) / 2, (screenRect.Height() - h) / 2,
        w, h, SWP_NOZORDER);

    // Return FALSE to prevent MFC from automatically selecting all text
    return FALSE;
}

void CXmlViewerDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (m_richEdit.GetSafeHwnd() && cx > 0 && cy > 0) {
        m_richEdit.MoveWindow(0, 0, cx, cy);
    }
}

// ---------------------------------------------------------------------------
// LoadAndHighlight: Load XML, set text, highlight diff lines
// ---------------------------------------------------------------------------
void CXmlViewerDlg::LoadAndHighlight() {
    std::ifstream fs(m_xmlPath, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        m_richEdit.SetWindowText(_T("Error: Could not open file."));
        return;
    }

    // Read all lines, decode from UTF-8 to display Unicode properly
    std::vector<CString> lines;
    std::string line;
    while (std::getline(fs, line)) {
        lines.push_back(CString(CA2W(line.c_str(), CP_UTF8)));
    }
    fs.close();

    // Build full text
    CString fullText;
    for (const auto& l : lines) {
        fullText += l + _T("\r\n");
    }

    // Stop redraw
    m_richEdit.SetRedraw(FALSE);
    m_richEdit.SetWindowText(fullText);

    // Set all text to light gray on dark bg
    m_richEdit.SetSel(0, -1);
    CHARFORMAT cfAll = {};
    cfAll.cbSize = sizeof(cfAll);
    cfAll.dwMask = CFM_COLOR;
    cfAll.crTextColor = RGB(212, 212, 212);
    m_richEdit.SetSelectionCharFormat(cfAll);

    // Apply XML syntax highlighting
    ApplySyntaxHighlighting();

    // Highlight diff lines
    int scrollToLine = -1;

    // Highlight EXTRA vals (blue) — these exist in the right file
    for (const auto& entry : m_extra) {
        int ln = FindValLine(lines, entry.groupId, entry.specId, entry.valId);
        if (ln >= 0) {
            HighlightLine(ln, RGB(70, 100, 160));
            if (!m_scrollToKey.empty() && entry.compositeKey == m_scrollToKey && !m_scrollToIsMissing) {
                scrollToLine = ln;
            }
        }
    }

    // Highlight MISSING vals (tomato) — find parent spec to indicate where it should be
    for (const auto& entry : m_missing) {
        // For missing vals, highlight the parent spec line
        int ln = FindValLine(lines, entry.groupId, entry.specId, "");
        if (ln >= 0) {
            HighlightLine(ln, RGB(140, 70, 60));
            if (!m_scrollToKey.empty() && entry.compositeKey == m_scrollToKey && m_scrollToIsMissing) {
                scrollToLine = ln;
            }
        }
    }

    // Remove global unselect that resets caret position, instead we rely on OnInitDialog return FALSE

    // Scroll and highlight target search string if provided
    if (!m_targetSearch.IsEmpty()) {
        FINDTEXTEX ft;
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = -1;
        ft.lpstrText = m_targetSearch.GetString();

        long nPos = m_richEdit.FindText(FR_DOWN | FR_MATCHCASE, &ft);
        if (nPos != -1) {
            // Highlight specific text
            m_richEdit.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMax);
            CHARFORMAT2 cfHighlight = {};
            cfHighlight.cbSize = sizeof(cfHighlight);
            cfHighlight.dwMask = CFM_BACKCOLOR | CFM_COLOR;
            cfHighlight.crBackColor = RGB(255, 255, 0); // Yellow background
            cfHighlight.crTextColor = RGB(0, 0, 0);     // Black text
            m_richEdit.SetSelectionCharFormat(cfHighlight);
            
            // Unselect without moving caret to top
            m_richEdit.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMin);
            
            // Restore redraw and scroll
            m_richEdit.SetRedraw(TRUE);
            m_richEdit.Invalidate();
            m_richEdit.SendMessage(EM_SCROLLCARET, 0, 0);
            m_richEdit.LineScroll(-5, 0);
            return;
        } 
    } 
    
    // Fallback to row scroll
    if (scrollToLine >= 0) {
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.Invalidate();
        ScrollToLine(scrollToLine);
    } else if (!m_extra.empty() || !m_missing.empty()) {
        int firstDiff = -1;
        if (!m_extra.empty()) {
            firstDiff = FindValLine(lines, m_extra[0].groupId, m_extra[0].specId, m_extra[0].valId);
        }
        if (firstDiff < 0 && !m_missing.empty()) {
            firstDiff = FindValLine(lines, m_missing[0].groupId, m_missing[0].specId, "");
        }
        
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.Invalidate();
        if (firstDiff >= 0) ScrollToLine(firstDiff);
    } else {
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.Invalidate();
    }
}


// ---------------------------------------------------------------------------
// FindValLine: Find line number of a val (or spec if valId empty) in the XML
// ---------------------------------------------------------------------------
int CXmlViewerDlg::FindValLine(const std::vector<CString>& lines,
    const std::string& groupId, const std::string& specId, const std::string& valId)
{
    CString gidPattern;
    gidPattern.Format(_T("group_ID=\"%s\""), CString(groupId.c_str()).GetString());
    CString sidPattern;
    sidPattern.Format(_T("spec_ID=\"%s\""), CString(specId.c_str()).GetString());

    bool inGroup = false, inSpec = false;
    int specLine = -1;

    for (int i = 0; i < (int)lines.size(); i++) {
        const CString& ln = lines[i];

        if (ln.Find(_T("<group")) >= 0 && ln.Find(gidPattern) >= 0) {
            inGroup = true;
            continue;
        }
        if (inGroup && ln.Find(_T("</group>")) >= 0) {
            inGroup = false;
            inSpec = false;
            continue;
        }
        if (inGroup && ln.Find(_T("<spec")) >= 0 && ln.Find(sidPattern) >= 0) {
            inSpec = true;
            specLine = i;
            if (valId.empty()) return i; // Looking for spec line only
            continue;
        }
        if (inSpec && ln.Find(_T("</spec>")) >= 0) {
            inSpec = false;
            continue;
        }
        if (inSpec && !valId.empty() && ln.Find(_T("<val")) >= 0) {
            CString vidPattern;
            vidPattern.Format(_T("val_id=\"%s\""), CString(valId.c_str()).GetString());
            if (ln.Find(vidPattern) >= 0) return i;
        }
    }
    return specLine; // fallback to spec line
}

// ---------------------------------------------------------------------------
// HighlightLine: Set background color for a specific line
// ---------------------------------------------------------------------------
void CXmlViewerDlg::HighlightLine(int lineIndex, COLORREF bgColor) {
    int startChar = m_richEdit.LineIndex(lineIndex);
    if (startChar < 0) return;
    int lineLen = m_richEdit.LineLength(startChar);
    m_richEdit.SetSel(startChar, startChar + lineLen);

    CHARFORMAT2 cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BACKCOLOR | CFM_COLOR;
    cf.crBackColor = bgColor;
    cf.crTextColor = RGB(255, 255, 255);
    m_richEdit.SetSelectionCharFormat(cf);
}

// ---------------------------------------------------------------------------
// ScrollToLine: Scroll the rich edit to show a specific line
// ---------------------------------------------------------------------------
void CXmlViewerDlg::ScrollToLine(int lineIndex) {
    int charIdx = m_richEdit.LineIndex(lineIndex);
    if (charIdx >= 0) {
        m_richEdit.SetSel(charIdx, charIdx);
        m_richEdit.SendMessage(EM_SCROLLCARET, 0, 0);
        // Scroll up a bit so the line isn't at the very top
        m_richEdit.LineScroll(-5, 0);
    }
}

// ---------------------------------------------------------------------------
// ApplySyntaxHighlighting: Fast custom syntax highlighter for XML in RichEdit
// ---------------------------------------------------------------------------
void CXmlViewerDlg::ApplySyntaxHighlighting() {
    CString text;
    m_richEdit.GetWindowText(text);
    // RichEdit uses \r for indexing but GetWindowText might return \r\n
    text.Replace(_T("\r\n"), _T("\n"));

    int len = text.GetLength();
    int i = 0;
    
    // VS Code Dark+ style colors
    CHARFORMAT2 cfTag = { sizeof(CHARFORMAT2) };
    cfTag.dwMask = CFM_COLOR; cfTag.crTextColor = RGB(86, 156, 214);
    
    CHARFORMAT2 cfAttr = { sizeof(CHARFORMAT2) };
    cfAttr.dwMask = CFM_COLOR; cfAttr.crTextColor = RGB(156, 220, 254);
    
    CHARFORMAT2 cfStr = { sizeof(CHARFORMAT2) };
    cfStr.dwMask = CFM_COLOR; cfStr.crTextColor = RGB(206, 145, 120);

    bool inTag = false;
    
    while (i < len) {
        TCHAR c = text[i];
        if (!inTag) {
            if (c == _T('<')) {
                inTag = true;
                int tagStart = i;
                int nameEnd = i + 1;
                if (nameEnd < len && text[nameEnd] == _T('/')) nameEnd++;
                while (nameEnd < len && iswalnum((unsigned short)text[nameEnd])) nameEnd++;
                
                m_richEdit.SetSel(tagStart, nameEnd);
                m_richEdit.SetSelectionCharFormat(cfTag);
                i = nameEnd;
            } else {
                i++;
            }
        } else {
            if (c == _T('>')) {
                m_richEdit.SetSel(i, i + 1);
                m_richEdit.SetSelectionCharFormat(cfTag);
                inTag = false;
                i++;
            } else if (c == _T('/') && i + 1 < len && text[i+1] == _T('>')) {
                m_richEdit.SetSel(i, i + 2);
                m_richEdit.SetSelectionCharFormat(cfTag);
                inTag = false;
                i += 2;
            } else if (iswalpha((unsigned short)c) || c == _T('_')) {
                // Attribute name
                int attrStart = i;
                while (i < len && (iswalnum((unsigned short)text[i]) || text[i] == _T('_'))) i++;
                m_richEdit.SetSel(attrStart, i);
                m_richEdit.SetSelectionCharFormat(cfAttr);
            } else if (c == _T('"') || c == _T('\'')) {
                // String value
                TCHAR quoteChar = c;
                int strStart = i;
                i++;
                while (i < len && text[i] != quoteChar) i++;
                if (i < len) i++;
                m_richEdit.SetSel(strStart, i);
                m_richEdit.SetSelectionCharFormat(cfStr);
            } else {
                i++;
            }
        }
    }
}
