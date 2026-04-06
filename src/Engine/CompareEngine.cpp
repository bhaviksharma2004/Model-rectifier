// =============================================================================
// CompareEngine.cpp
// Implementation of the directory-level comparison orchestrator.
// =============================================================================
#include "pch.h"
#include "CompareEngine.h"

#include <algorithm>
#include <set>

namespace fs = std::filesystem;

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
CompareEngine::CompareEngine(std::unique_ptr<IComparisonStrategy> strategy)
    : m_strategy(std::move(strategy))
{
}

// ---------------------------------------------------------------------------
// SetStrategy: Swap comparison strategy at runtime
// ---------------------------------------------------------------------------
void CompareEngine::SetStrategy(std::unique_ptr<IComparisonStrategy> strategy) {
    m_strategy = std::move(strategy);
}

// ---------------------------------------------------------------------------
// EnumerateXmlFiles: Recursively find all .xml files under a directory
// ---------------------------------------------------------------------------
std::vector<fs::path> CompareEngine::EnumerateXmlFiles(const fs::path& root) {
    std::vector<fs::path> files;

    if (!fs::exists(root) || !fs::is_directory(root)) {
        return files;
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        // Only include .xml files (case-insensitive check)
        auto ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

        if (ext == L".xml") {
            files.push_back(entry.path());
        }
    }

    return files;
}

// ---------------------------------------------------------------------------
// MakeRelative: Compute relative path from root directory
// ---------------------------------------------------------------------------
fs::path CompareEngine::MakeRelative(const fs::path& file, const fs::path& root) {
    return fs::relative(file, root);
}

// ---------------------------------------------------------------------------
// CompareModels: Main comparison entry point
// ---------------------------------------------------------------------------
ModelDiffReport CompareEngine::CompareModels(
    const fs::path& leftModelRoot,
    const fs::path& rightModelRoot,
    ProgressCallback progressCb)
{
    ModelDiffReport report;
    report.leftModelPath  = leftModelRoot.wstring();
    report.rightModelPath = rightModelRoot.wstring();

    // Extract model names from the directory name
    report.leftModelName  = leftModelRoot.filename().wstring();
    report.rightModelName = rightModelRoot.filename().wstring();

    // --- Step 1: Enumerate all XML files in both directories ---
    auto leftFiles  = EnumerateXmlFiles(leftModelRoot);
    auto rightFiles = EnumerateXmlFiles(rightModelRoot);

    // --- Step 2: Build relative path maps for matching ---
    // Key: relative path (lowercase for case-insensitive matching)
    // Value: absolute path
    std::map<std::wstring, fs::path> leftMap, rightMap;

    for (const auto& f : leftFiles) {
        auto rel = MakeRelative(f, leftModelRoot).wstring();
        std::wstring key = rel;
        std::transform(key.begin(), key.end(), key.begin(), ::towlower);
        leftMap[key] = f;
    }

    for (const auto& f : rightFiles) {
        auto rel = MakeRelative(f, rightModelRoot).wstring();
        std::wstring key = rel;
        std::transform(key.begin(), key.end(), key.begin(), ::towlower);
        rightMap[key] = f;
    }

    // Collect all unique relative paths
    std::set<std::wstring> allKeys;
    for (const auto& [k, _] : leftMap)  allKeys.insert(k);
    for (const auto& [k, _] : rightMap) allKeys.insert(k);

    report.totalFilesScanned = allKeys.size();

    // --- Step 3: Compare each file pair ---
    size_t current = 0;
    for (const auto& key : allKeys) {
        ++current;

        bool inLeft  = leftMap.count(key)  > 0;
        bool inRight = rightMap.count(key) > 0;

        // Progress callback
        if (progressCb) {
            progressCb(current, allKeys.size(), key);
        }

        FileDiffResult fileResult;

        if (inLeft && !inRight) {
            // File exists only in Left — DELETED in Right
            fileResult.relativePath = MakeRelative(leftMap[key], leftModelRoot);
            fileResult.status = FileDiffResult::Status::DeletedInRight;

        } else if (!inLeft && inRight) {
            // File exists only in Right — ADDED in Right
            fileResult.relativePath = MakeRelative(rightMap[key], rightModelRoot);
            fileResult.status = FileDiffResult::Status::AddedInRight;

        } else {
            // File exists in both — delegate to comparison strategy
            fileResult = m_strategy->CompareFiles(leftMap[key], rightMap[key]);
            fileResult.relativePath = MakeRelative(leftMap[key], leftModelRoot);
        }

        // Only include files with differences
        if (fileResult.HasDifferences()) {
            report.totalMissingIds += fileResult.missingInRight.size();
            report.totalExtraIds   += fileResult.extraInRight.size();
            report.fileResults.push_back(std::move(fileResult));
        }
    }

    report.totalFilesWithDiffs = report.fileResults.size();

    // Sort results: Deleted first, then Modified, then Added
    std::sort(report.fileResults.begin(), report.fileResults.end(),
        [](const FileDiffResult& a, const FileDiffResult& b) {
            if (a.status != b.status) return a.status < b.status;
            return a.relativePath < b.relativePath;
        });

    return report;
}

} // namespace ModelCompare
