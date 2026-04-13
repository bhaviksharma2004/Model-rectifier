#include "pch.h"
#include "resource.h"
#include "XmlViewerDlg.h"
#include <fstream>
#include <sstream>
#include <richedit.h>
#include <dwmapi.h>
#include <CommCtrl.h>

#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(CXmlViewerDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_MOUSEHWHEEL()
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_XMLLOAD_DEFERRED, &CXmlViewerDlg::OnDeferredLoad)
    ON_MESSAGE(WM_XMLDESELECT, &CXmlViewerDlg::OnDeferredDeselect)
END_MESSAGE_MAP()


CXmlViewerDlg::CXmlViewerDlg(CWnd* pParent)
    : CDialogEx(IDD_XMLVIEWER_DIALOG, pParent) {}


void CXmlViewerDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RICHEDIT_XML, m_richEdit);
}


void CXmlViewerDlg::SetFile(const std::filesystem::path& xmlPath, const std::filesystem::path& leftPath, const CString& title) {
    m_xmlPath = xmlPath;
    m_leftPath = leftPath;
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

void CXmlViewerDlg::SetCachedContent(const CString& fullText,
                                     const std::vector<CString>& lines,
                                     const CString& title) {
    m_useCachedContent = true;
    m_cachedFullText = fullText;
    m_cachedLines = lines;
    m_title = title;
}

void CXmlViewerDlg::SetValidationHighlights(
    const std::vector<ValidationHighlight>& highlights) {
    m_validationHighlights = highlights;
}

void CXmlViewerDlg::SetScrollToLine(int lineNumber) {
    m_scrollToLineNumber = lineNumber;
}


BOOL CXmlViewerDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    SetWindowText(m_title);

    ModifyStyle(WS_MINIMIZEBOX, 0, SWP_FRAMECHANGED);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu) {
        pSysMenu->RemoveMenu(SC_MINIMIZE, MF_BYCOMMAND);
    }


    m_brushDarkBg.CreateSolidBrush(CLR_BG_DARK);


    BOOL darkMode = TRUE;
    HRESULT hr = ::DwmSetWindowAttribute(GetSafeHwnd(), 20, &darkMode, sizeof(darkMode));
    if (FAILED(hr)) {
        ::DwmSetWindowAttribute(GetSafeHwnd(), 19, &darkMode, sizeof(darkMode));
    }

    hr = ::DwmSetWindowAttribute(m_richEdit.GetSafeHwnd(), 20, &darkMode, sizeof(darkMode));
    if (FAILED(hr)) {
        ::DwmSetWindowAttribute(m_richEdit.GetSafeHwnd(), 19, &darkMode, sizeof(darkMode));
    }

    ::SetWindowTheme(m_richEdit.GetSafeHwnd(), L"DarkMode_Explorer", NULL);


    LOGFONT lf = {};
    lf.lfHeight = -14;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    wcscpy_s(lf.lfFaceName, L"Consolas");

    m_codeFont.CreateFontIndirect(&lf);


    m_richEdit.SetReadOnly(TRUE);
    m_richEdit.SetFont(&m_codeFont);
    m_richEdit.SetBackgroundColor(FALSE, CLR_BG_DARK);

    CHARFORMAT2 cfDefault = {};
    cfDefault.cbSize = sizeof(cfDefault);
    cfDefault.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_BACKCOLOR;
    wcscpy_s(cfDefault.szFaceName, lf.lfFaceName);
    cfDefault.yHeight = 210;
    cfDefault.crTextColor = CLR_TEXT_LIGHT;
    cfDefault.crBackColor = CLR_BG_DARK;
    cfDefault.dwEffects = 0;
    m_richEdit.SetDefaultCharFormat(cfDefault);


    m_richEdit.SetWindowText(_T("  Loading XML..."));
    m_richEdit.SetSel(0, 0);


    CRect screenRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
    int w = (int)(screenRect.Width() * 0.6);
    int h = (int)(screenRect.Height() * 0.8);
    SetWindowPos(nullptr,
        (screenRect.Width() - w) / 2 + screenRect.left,
        (screenRect.Height() - h) / 2 + screenRect.top,
        w, h, SWP_NOZORDER | SWP_FRAMECHANGED);


    PostMessage(WM_XMLLOAD_DEFERRED, 0, 0);

    return FALSE;
}

LRESULT CXmlViewerDlg::OnDeferredLoad(WPARAM, LPARAM) {
    CWaitCursor wait;
    LoadAndHighlight();

    PostMessage(WM_XMLDESELECT, 0, 0);
    return 0;
}

LRESULT CXmlViewerDlg::OnDeferredDeselect(WPARAM, LPARAM) {
    m_richEdit.HideCaret();
    return 0;
}

HBRUSH CXmlViewerDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    if (nCtlColor == CTLCOLOR_DLG) {
        pDC->SetBkColor(CLR_BG_DARK);
        return (HBRUSH)m_brushDarkBg;
    }
    return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CXmlViewerDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
    if (m_richEdit.GetSafeHwnd() && cx > 0 && cy > 0) {
        m_richEdit.MoveWindow(0, 0, cx, cy);
    }
}

void CXmlViewerDlg::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt) {
    if (m_richEdit.GetSafeHwnd()) {
        int ticks = abs(zDelta) / WHEEL_DELTA;
        if (ticks < 1) ticks = 1;

        WPARAM scrollCmd = (zDelta > 0) ? SB_LINERIGHT : SB_LINELEFT;
        for (int i = 0; i < ticks * 3; i++) {
            m_richEdit.SendMessage(WM_HSCROLL, scrollCmd, 0);
        }
    }
}

void CXmlViewerDlg::LoadAndHighlight() {
    try {
        std::vector<CString> lines;
        CString fullText;

        if (m_useCachedContent) {
            
            lines = m_cachedLines;
            fullText = m_cachedFullText;
        } else {
            
            std::ifstream fs(m_xmlPath, std::ios::in | std::ios::binary);
            if (!fs.is_open()) {
                ShowError(_T("File Open Error"),
                    CString(_T("Could not open file:\n")) + CString(m_xmlPath.wstring().c_str()));
                m_richEdit.SetWindowText(_T("Error: Could not open file."));
                return;
            }

            {
                std::string raw;
                while (std::getline(fs, raw)) {
                    if (!raw.empty() && raw.back() == '\r') {
                        raw.pop_back();
                    }
                    lines.push_back(CString(CA2W(raw.c_str(), CP_UTF8)));
                }
                fs.close();
            }

            if (!m_missing.empty() && !m_leftPath.empty() && std::filesystem::exists(m_leftPath)) {
                std::vector<CString> leftLines;
                std::ifstream lfs(m_leftPath, std::ios::in | std::ios::binary);
                if (lfs.is_open()) {
                    std::string raw;
                    while (std::getline(lfs, raw)) {
                        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
                        leftLines.push_back(CString(CA2W(raw.c_str(), CP_UTF8)));
                    }
                    lfs.close();

                    for (const auto& entry : m_missing) {
                        int leftLn = FindValLine(leftLines, entry.groupId, entry.specId, entry.valId);
                        if (leftLn >= 0) {
                            auto snippet = ExtractBlock(leftLines, leftLn);
                            if (!snippet.empty()) {
                                int parentStartLn = -1;
                                std::string targetId = "";
                                
                                if (entry.level == ModelCompare::DiffLevel::Val) {
                                    parentStartLn = FindValLine(lines, entry.groupId, entry.specId, "");
                                    targetId = entry.valId;
                                } else if (entry.level == ModelCompare::DiffLevel::Spec) {
                                    parentStartLn = FindValLine(lines, entry.groupId, "", "");
                                    targetId = entry.specId;
                                } else {
                                    for (int i = 0; i < (int)lines.size(); i++) {
                                        if (lines[i].Find(_T("<data")) >= 0) {
                                            parentStartLn = i; break;
                                        }
                                    }
                                    targetId = entry.groupId;
                                }
                                
                                if (parentStartLn >= 0 && parentStartLn < (int)lines.size()) {
                                    int sortedInsertLn = GetSortedInsertLine(lines, parentStartLn, entry.level, targetId);
                                    lines.insert(lines.begin() + sortedInsertLn, snippet.begin(), snippet.end());
                                }
                            }
                        }
                    }
                }
            }

            
            size_t totalChars = 0;
            for (const auto& l : lines) {
                totalChars += (size_t)l.GetLength() + 2;
            }
            fullText.Preallocate((int)totalChars + 1);
            for (const auto& l : lines) {
                fullText.Append(l);
                fullText.Append(_T("\r\n"));
            }
        }

        
        m_richEdit.SetRedraw(FALSE);
        m_richEdit.SetWindowText(fullText);

        m_richEdit.SetSel(0, -1);
        CHARFORMAT2 cfAll = {};
        cfAll.cbSize = sizeof(cfAll);
        cfAll.dwMask = CFM_COLOR | CFM_BACKCOLOR;
        cfAll.crTextColor = CLR_TEXT_LIGHT;
        cfAll.crBackColor = CLR_BG_DARK;
        cfAll.dwEffects = 0;
        m_richEdit.SetSelectionCharFormat(cfAll);

        
        {
            CString reText;
            m_richEdit.GetWindowText(reText);
            reText.Replace(_T("\r\n"), _T("\n"));

            std::vector<ColorRange> syntaxRanges;
            syntaxRanges.reserve(lines.size() * 4);
            CollectSyntaxRanges(reText, syntaxRanges);
            ApplyColorRanges(syntaxRanges);
        }

        int scrollToLine = -1;

        if (m_useCachedContent) {
            
            for (const auto& vh : m_validationHighlights) {
                if (vh.lineNumber >= 0 && vh.lineNumber < (int)lines.size()) {
                    HighlightBlock(vh.lineNumber, vh.lineNumber, vh.bgColor);
                }
            }
            scrollToLine = m_scrollToLineNumber;
        } else {
            
            for (const auto& entry : m_extra) {
                int ln = FindValLine(lines, entry.groupId, entry.specId, entry.valId);
                if (ln >= 0) {
                    int endLn = FindBlockEnd(lines, ln);
                    HighlightBlock(ln, endLn, CLR_DIFF_EXTRA_BG);
                    if (!m_scrollToKey.empty() && entry.compositeKey == m_scrollToKey && !m_scrollToIsMissing) {
                        scrollToLine = ln;
                    }
                }
            }

            for (const auto& entry : m_missing) {
                int ln = FindValLine(lines, entry.groupId, entry.specId, entry.valId);
                if (ln >= 0) {
                    int endLn = FindBlockEnd(lines, ln);
                    HighlightBlock(ln, endLn, CLR_DIFF_MISSING_BG);
                    if (!m_scrollToKey.empty() && entry.compositeKey == m_scrollToKey && m_scrollToIsMissing) {
                        scrollToLine = ln;
                    }
                }
            }
        }

        m_richEdit.SetSel(0, 0);
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.Invalidate();
        m_richEdit.UpdateWindow();

        if (scrollToLine >= 0) {
            ScrollToLine(scrollToLine);
        } else if (!m_useCachedContent && (!m_extra.empty() || !m_missing.empty())) {
            int firstDiff = -1;
            if (!m_extra.empty()) {
                firstDiff = FindValLine(lines, m_extra[0].groupId, m_extra[0].specId, m_extra[0].valId);
            }
            if (firstDiff < 0 && !m_missing.empty()) {
                firstDiff = FindValLine(lines, m_missing[0].groupId, m_missing[0].specId, "");
            }
            if (firstDiff >= 0) ScrollToLine(firstDiff);
        }

    } catch (CMemoryException* e) {
        e->Delete();
        ShowError(_T("Memory Error"),
            _T("Insufficient memory to load the XML file.\nThe file may be too large."));
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.SetWindowText(_T("Error: Out of memory."));
    } catch (CException* e) {
        TCHAR errMsg[512] = {};
        e->GetErrorMessage(errMsg, 512);
        e->Delete();
        ShowError(_T("MFC Error"), CString(_T("An error occurred:\n")) + errMsg);
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.SetWindowText(_T("Error: Failed to load file."));
    } catch (const std::exception& e) {
        ShowError(_T("Runtime Error"),
            CString(_T("An unexpected error occurred:\n")) + CString(e.what()));
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.SetWindowText(_T("Error: Failed to load file."));
    } catch (...) {
        ShowError(_T("Unknown Error"),
            _T("An unexpected error occurred while loading the XML file."));
        m_richEdit.SetRedraw(TRUE);
        m_richEdit.SetWindowText(_T("Error: Failed to load file."));
    }
}


void CXmlViewerDlg::CollectSyntaxRanges(const CString& text, std::vector<ColorRange>& ranges) {
    int len = text.GetLength();
    int i = 0;
    bool inTag = false;

    while (i < len) {
        TCHAR c = text[i];

        if (!inTag) {
            if (c == _T('<')) {
                if (i + 3 < len && text[i+1] == _T('!') && text[i+2] == _T('-') && text[i+3] == _T('-')) {
                    int commentStart = i;
                    i += 4;
                    while (i + 2 < len) {
                        if (text[i] == _T('-') && text[i+1] == _T('-') && text[i+2] == _T('>')) {
                            i += 3;
                            break;
                        }
                        i++;
                    }
                    ranges.push_back({ commentStart, i, CLR_COMMENT });
                    continue;
                }

                if (i + 1 < len && text[i+1] == _T('?')) {
                    int piStart = i;
                    i += 2;
                    while (i + 1 < len) {
                        if (text[i] == _T('?') && text[i+1] == _T('>')) {
                            i += 2;
                            break;
                        }
                        i++;
                    }
                    ranges.push_back({ piStart, i, CLR_TAG });
                    continue;
                }

                inTag = true;
                int tagStart = i;
                int nameEnd = i + 1;

                if (nameEnd < len && text[nameEnd] == _T('/')) nameEnd++;

                while (nameEnd < len && (iswalnum((unsigned short)text[nameEnd]) || text[nameEnd] == _T('_') || text[nameEnd] == _T(':'))) {
                    nameEnd++;
                }

                ranges.push_back({ tagStart, nameEnd, CLR_TAG });
                i = nameEnd;
            } else {
                i++;
            }
        } else {
            if (c == _T('>')) {
                ranges.push_back({ i, i + 1, CLR_TAG });
                inTag = false;
                i++;
            } else if (c == _T('/') && i + 1 < len && text[i+1] == _T('>')) {
                ranges.push_back({ i, i + 2, CLR_TAG });
                inTag = false;
                i += 2;
            } else if (iswalpha((unsigned short)c) || c == _T('_')) {
                int attrStart = i;
                while (i < len && (iswalnum((unsigned short)text[i]) || text[i] == _T('_') || text[i] == _T(':'))) i++;
                ranges.push_back({ attrStart, i, CLR_ATTR });
            } else if (c == _T('"') || c == _T('\'')) {
                TCHAR quoteChar = c;
                int strStart = i;
                i++;
                while (i < len && text[i] != quoteChar) i++;
                if (i < len) i++;
                ranges.push_back({ strStart, i, CLR_STRING });
            } else {
                i++;
            }
        }
    }
}


void CXmlViewerDlg::ApplyColorRanges(const std::vector<ColorRange>& ranges) {
    CHARFORMAT2 cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.dwEffects = 0;

    for (const auto& r : ranges) {
        m_richEdit.SetSel(r.start, r.end);
        cf.crTextColor = r.color;
        m_richEdit.SetSelectionCharFormat(cf);
    }
}


int CXmlViewerDlg::FindValLine(const std::vector<CString>& lines,
    const std::string& groupId, const std::string& specId, const std::string& valId)
{
    CString gidPattern;
    gidPattern.Format(_T("group_ID=\"%s\""), CString(groupId.c_str()).GetString());
    CString sidPattern;
    sidPattern.Format(_T("spec_ID=\"%s\""), CString(specId.c_str()).GetString());

    bool inGroup = false, inSpec = false;
    int specLine = -1;
    int groupLine = -1;

    for (int i = 0; i < (int)lines.size(); i++) {
        const CString& ln = lines[i];

        if (ln.Find(_T("<group")) >= 0 && ln.Find(gidPattern) >= 0) {
            inGroup = true;
            groupLine = i;
            if (specId.empty()) return i;
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
            if (valId.empty()) return i;
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
    if (specLine != -1) return specLine;
    return groupLine;
}


int CXmlViewerDlg::GetSortedInsertLine(const std::vector<CString>& lines, int parentStartLine, 
                                       ModelCompare::DiffLevel level, const std::string& targetId) {
    if (parentStartLine < 0 || parentStartLine >= (int)lines.size()) return parentStartLine;
    
    CString parentStartStr = lines[parentStartLine];
    if (parentStartStr.Find(_T("/>")) >= 0) return parentStartLine + 1;

    int tagStart = parentStartStr.Find(_T('<'));
    if (tagStart < 0) return parentStartLine + 1;

    int spaceIdx = parentStartStr.Find(_T(' '), tagStart);
    int closeIdx = parentStartStr.Find(_T('>'), tagStart);
    int tagEnd = spaceIdx;
    if (spaceIdx < 0 || (closeIdx >= 0 && closeIdx < spaceIdx)) tagEnd = closeIdx;
    if (tagEnd <= tagStart + 1) return parentStartLine + 1;

    CString parentTagName = parentStartStr.Mid(tagStart + 1, tagEnd - (tagStart + 1));
    CString parentClosingTag;
    parentClosingTag.Format(_T("</%s>"), parentTagName.GetString());

    CString siblingTagName;
    CString idAttr;
    if (level == ModelCompare::DiffLevel::Group) { siblingTagName = _T("<group"); idAttr = _T("group_ID=\""); }
    else if (level == ModelCompare::DiffLevel::Spec) { siblingTagName = _T("<spec"); idAttr = _T("spec_ID=\""); }
    else { siblingTagName = _T("<val"); idAttr = _T("val_id=\""); }

    long targetVal = 0;
    try { targetVal = std::stol(targetId); } catch(...) {}

    int depth = 1;
    for (int i = parentStartLine + 1; i < (int)lines.size(); i++) {
        CString ln = lines[i];

        
        CString openPat; openPat.Format(_T("<%s"), parentTagName.GetString());
        if (ln.Find(openPat) >= 0 && ln.Find(_T("/>")) < 0) depth++;
        if (ln.Find(parentClosingTag) >= 0) {
            depth--;
            if (depth == 0) return i; 
        }

        
        if (depth == 1 && ln.Find(siblingTagName) >= 0) {
            int attrStart = ln.Find(idAttr);
            if (attrStart >= 0) {
                attrStart += idAttr.GetLength();
                int attrEnd = ln.Find(_T('"'), attrStart);
                if (attrEnd > attrStart) {
                    CString idStr = ln.Mid(attrStart, attrEnd - attrStart);
                    long sibVal = 0;
                    try { sibVal = std::stol(std::wstring(idStr.GetString())); } catch(...) {}
                    
                    if (sibVal > targetVal) {
                        return i; 
                    }
                }
            }
        }
    }
    return parentStartLine + 1;
}


int CXmlViewerDlg::FindBlockEnd(const std::vector<CString>& lines, int startLine) {
    if (startLine < 0 || startLine >= (int)lines.size()) return startLine;

    CString firstLine = lines[startLine];
    if (firstLine.Find(_T("/>")) >= 0) return startLine;

    int tagStart = firstLine.Find(_T('<'));
    if (tagStart < 0) return startLine;

    int spaceIdx = firstLine.Find(_T(' '), tagStart);
    int closeIdx = firstLine.Find(_T('>'), tagStart);
    int tagEnd = spaceIdx;
    if (spaceIdx < 0 || (closeIdx >= 0 && closeIdx < spaceIdx)) tagEnd = closeIdx;
    if (tagEnd <= tagStart + 1) return startLine;

    CString tagName = firstLine.Mid(tagStart + 1, tagEnd - (tagStart + 1));
    CString closingTag;
    closingTag.Format(_T("</%s>"), tagName.GetString());

    int depth = 1;
    for (int i = startLine + 1; i < (int)lines.size(); i++) {
        CString ln = lines[i];
        CString openPat; openPat.Format(_T("<%s"), tagName.GetString());
        if (ln.Find(openPat) >= 0 && ln.Find(_T("/>")) < 0) depth++;
        if (ln.Find(closingTag) >= 0) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return startLine;
}

std::vector<CString> CXmlViewerDlg::ExtractBlock(const std::vector<CString>& lines, int startLine) {
    std::vector<CString> block;
    int endLine = FindBlockEnd(lines, startLine);
    if (startLine >= 0 && endLine >= startLine && endLine < (int)lines.size()) {
        for (int i = startLine; i <= endLine; i++) {
            block.push_back(lines[i]);
        }
    }
    return block;
}

void CXmlViewerDlg::HighlightBlock(int startLine, int endLine, COLORREF bgColor) {
    if (startLine < 0 || endLine < startLine) return;
    
    int startChar = m_richEdit.LineIndex(startLine);
    if (startChar < 0) return;
    
    int lastLineStart = m_richEdit.LineIndex(endLine);
    int endChar = lastLineStart + m_richEdit.LineLength(lastLineStart);
    
    m_richEdit.SetSel(startChar, endChar);

    CHARFORMAT2 cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BACKCOLOR | CFM_COLOR;
    cf.crBackColor = bgColor;
    cf.crTextColor = RGB(255, 255, 255);
    cf.dwEffects = 0;
    m_richEdit.SetSelectionCharFormat(cf);
}


void CXmlViewerDlg::ScrollToLine(int lineIndex) {
    int charIdx = m_richEdit.LineIndex(lineIndex);
    if (charIdx < 0) return;

    m_richEdit.SetSel(charIdx, charIdx);


    CRect rc;
    m_richEdit.GetClientRect(&rc);

    CDC* pDC = m_richEdit.GetDC();
    int lineHeight = 18;
    if (pDC) {
        CFont* pOldFont = pDC->SelectObject(&m_codeFont);
        TEXTMETRIC tm = {};
        pDC->GetTextMetrics(&tm);
        lineHeight = tm.tmHeight + tm.tmExternalLeading;
        pDC->SelectObject(pOldFont);
        m_richEdit.ReleaseDC(pDC);
    }

    int visibleLines = rc.Height() / max(lineHeight, 1);
    

    int targetTopLine = lineIndex - (visibleLines / 2);
    if (targetTopLine < 0) targetTopLine = 0;


    int currentTopLine = m_richEdit.GetFirstVisibleLine();
    int linesToScroll = targetTopLine - currentTopLine;

    if (linesToScroll != 0) {
        m_richEdit.LineScroll(linesToScroll, 0);
    }
}


void CXmlViewerDlg::ShowError(const CString& title, const CString& detail) {
    HRESULT hr = ::TaskDialog(
        GetSafeHwnd(), NULL,
        _T("XML Viewer"), title.GetString(), detail.GetString(),
        TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);

    if (FAILED(hr)) {
        AfxMessageBox(title + _T("\n\n") + detail, MB_ICONERROR | MB_OK);
    }
}
