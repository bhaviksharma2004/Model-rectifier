#pragma once

#include <afxwin.h>

class Theme {
public:
    static Theme* Get();

    // Dialog & Panel Colors
    COLORREF DialogBg() const { return RGB(243, 243, 248); }
    COLORREF PanelBg() const { return RGB(235, 237, 245); }
    COLORREF EditBg() const { return RGB(255, 255, 255); }
    COLORREF BgDark() const { return RGB(30, 30, 30); }
    COLORREF ListBg() const { return RGB(255, 255, 255); }

    // Text Colors
    COLORREF TextPrimary() const { return RGB(30, 30, 40); }
    COLORREF TextSecondary() const { return RGB(90, 95, 110); }
    COLORREF HeaderText() const { return RGB(25, 60, 140); }
    COLORREF TextLight() const { return RGB(212, 212, 212); }

    // Accent Colors
    COLORREF AccentBlue() const { return RGB(13, 110, 253); }
    COLORREF AccentBluePressed() const { return RGB(10, 90, 210); }
    COLORREF AccentTeal() const { return RGB(13, 202, 240); }
    COLORREF AccentGreen() const { return RGB(25, 135, 84); }
    COLORREF AccentGreenPressed() const { return RGB(20, 110, 65); }
    COLORREF AccentRed() const { return RGB(220, 53, 69); }
    COLORREF AccentRedPressed() const { return RGB(180, 40, 50); }

    // Action Button Colors
    COLORREF BtnApplySelBg() const { return RGB(60, 170, 110); }
    COLORREF BtnApplySelPrsd() const { return RGB(40, 130, 85); }
    COLORREF BtnApplyAllBg() const { return RGB(60, 170, 110); }
    COLORREF BtnApplyAllPrsd() const { return RGB(40, 130, 85); }
    COLORREF BtnViewXmlBg() const { return RGB(80, 140, 220); }
    COLORREF BtnViewXmlPrsd() const { return RGB(55, 110, 190); }

    // State Colors
    COLORREF DeletedBg() const { return RGB(255, 215, 215); }
    COLORREF DeletedTxt() const { return RGB(170, 0, 0); }
    COLORREF AddedBg() const { return RGB(215, 255, 215); }
    COLORREF AddedTxt() const { return RGB(0, 110, 0); }
    COLORREF ModifiedBg() const { return RGB(255, 245, 210); }
    COLORREF ModifiedTxt() const { return RGB(170, 120, 0); }
    COLORREF MissingBg() const { return RGB(255, 230, 225); }
    COLORREF MissingTxt() const { return RGB(170, 45, 25); }
    COLORREF ExtraBg() const { return RGB(225, 253, 238); }
    COLORREF ExtraTxt() const { return RGB(0, 115, 60); }
    COLORREF AltRowBg() const { return RGB(250, 250, 255); }
    COLORREF NormalRowBg() const { return RGB(255, 255, 255); }

    // Syntax Colors (XmlViewerDlg)
    COLORREF SyntaxTag() const { return RGB(86, 156, 214); }
    COLORREF SyntaxAttr() const { return RGB(156, 220, 254); }
    COLORREF SyntaxString() const { return RGB(206, 145, 120); }
    COLORREF SyntaxComment() const { return RGB(106, 153, 85); }
    COLORREF DiffExtraBg() const { return RGB(45, 95, 55); }
    COLORREF DiffMissingBg() const { return RGB(120, 45, 45); }

    // Global Font Sizes
    int FontSizeDefault() const { return -14; }
    int FontSizeHeader() const { return -15; }
    int FontSizeIcon() const { return -16; }

    // Common Metrics
    int LayoutMargin() const { return 10; }
    int LayoutGap() const { return 6; }
    int LayoutHeaderHeight() const { return 22; }
    int BtnActionRadius() const { return 12; }

private:
    Theme() = default;
    ~Theme() = default;
    static Theme* s_instance;
};
