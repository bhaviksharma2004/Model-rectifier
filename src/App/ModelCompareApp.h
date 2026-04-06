// =============================================================================
// ModelCompareApp.h
// CWinApp-derived application class for the ModelCompare tool.
// =============================================================================
#pragma once

#include "resource.h"

class CModelCompareApp : public CWinApp {
public:
    CModelCompareApp();

    BOOL InitInstance() override;

    DECLARE_MESSAGE_MAP()
};

// The single global application object
extern CModelCompareApp theApp;
