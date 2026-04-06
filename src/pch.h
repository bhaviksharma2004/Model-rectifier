// =============================================================================
// pch.h — Precompiled header for the ModelCompare project.
//
// Includes MFC framework headers and standard library headers used
// throughout the project. Engine layer headers do NOT depend on MFC
// but benefit from PCH compilation speed.
// =============================================================================
#pragma once

// MFC Framework
#include "framework.h"

// C++ Standard Library (used by Engine layer)
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <thread>
#include <future>
#include <fstream>
#include <sstream>
