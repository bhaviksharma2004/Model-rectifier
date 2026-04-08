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


CModelCompareApp theApp;


BEGIN_MESSAGE_MAP(CModelCompareApp, CWinApp)
END_MESSAGE_MAP()


CModelCompareApp::CModelCompareApp() {

}


BOOL CModelCompareApp::InitInstance() {
    CWinApp::InitInstance();


    AfxInitRichEdit2();


    INITCOMMONCONTROLSEX initCtrls;
    initCtrls.dwSize = sizeof(initCtrls);
    initCtrls.dwICC  = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&initCtrls);




    CMainDlg dlg;
    m_pMainWnd = &dlg;

    INT_PTR nResponse = dlg.DoModal();


    return FALSE;
}
