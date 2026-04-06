// =============================================================================
// ModelCompareApp.cpp
// Application entry point — creates and displays the main dialog.
// =============================================================================
#include "pch.h"
#include "resource.h"
#include "ModelCompareApp.h"
#include "Dialogs/MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// The one and only application object
// ---------------------------------------------------------------------------
CModelCompareApp theApp;

// ---------------------------------------------------------------------------
// Message Map
// ---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CModelCompareApp, CWinApp)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
CModelCompareApp::CModelCompareApp() {
    // Place all significant initialization in InitInstance
}

// ---------------------------------------------------------------------------
// InitInstance: Application initialization
// ---------------------------------------------------------------------------
BOOL CModelCompareApp::InitInstance() {
    CWinApp::InitInstance();

    // Initialize Rich Edit control (for XML viewer)
    AfxInitRichEdit2();

    // Initialize Common Controls (required for CListCtrl, etc.)
    INITCOMMONCONTROLSEX initCtrls;
    initCtrls.dwSize = sizeof(initCtrls);
    initCtrls.dwICC  = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&initCtrls);

    // Standard MFC initialization

    // Create and display the main dialog
    CMainDlg dlg;
    m_pMainWnd = &dlg;

    INT_PTR nResponse = dlg.DoModal();

    // Application is shutting down (dialog closed)
    return FALSE;
}
