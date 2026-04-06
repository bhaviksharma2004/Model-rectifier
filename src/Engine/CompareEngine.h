// =============================================================================
// CompareEngine.h
// Orchestrates the comparison of two model directories.
//
// Responsibilities:
//   - Recursively enumerates .xml files in both model directories
//   - Matches files by their relative path
//   - Detects added/deleted files
//   - Delegates file-level comparison to the active IComparisonStrategy
//   - Aggregates results into a ModelDiffReport
//
// Thread safety:
//   CompareModels() is designed to run on a background thread.
//   It does NOT access any MFC objects.
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include "IComparisonStrategy.h"

#include <filesystem>
#include <memory>
#include <functional>

namespace ModelCompare {

class CompareEngine {
public:
    // Progress callback: (currentFileIndex, totalFiles, currentFileName)
    using ProgressCallback = std::function<
        void(size_t current, size_t total, const std::wstring& currentFile)
    >;

    explicit CompareEngine(std::unique_ptr<IComparisonStrategy> strategy);

    // Compare two model directories and return a report.
    // Only files with differences are included in the report.
    ModelDiffReport CompareModels(
        const std::filesystem::path& leftModelRoot,
        const std::filesystem::path& rightModelRoot,
        ProgressCallback progressCb = nullptr
    );

    // Swap the comparison strategy at runtime
    void SetStrategy(std::unique_ptr<IComparisonStrategy> strategy);

private:
    std::unique_ptr<IComparisonStrategy> m_strategy;

    // Recursively enumerate all .xml files under a directory
    static std::vector<std::filesystem::path>
    EnumerateXmlFiles(const std::filesystem::path& root);

    // Compute relative path from root
    static std::filesystem::path
    MakeRelative(const std::filesystem::path& file,
                 const std::filesystem::path& root);
};

} // namespace ModelCompare
