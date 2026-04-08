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

CompareEngine::CompareEngine(std::unique_ptr<IComparisonStrategy> strategy)
    : m_strategy(std::move(strategy))
{
}

void CompareEngine::SetStrategy(std::unique_ptr<IComparisonStrategy> strategy) {
    m_strategy = std::move(strategy);
}

std::vector<fs::path> CompareEngine::EnumerateXmlFiles(const fs::path& root) {
    std::vector<fs::path> files;

    if (!fs::exists(root) || !fs::is_directory(root)) {
        return files;
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

        if (ext == L".xml") {
            files.push_back(entry.path());
        }
    }

    return files;
}

fs::path CompareEngine::MakeRelative(const fs::path& file, const fs::path& root) {
    return fs::relative(file, root);
}

ModelDiffReport CompareEngine::CompareModels(
    const fs::path& leftModelRoot,
    const fs::path& rightModelRoot,
    ProgressCallback progressCb)
{
    ModelDiffReport report;
    report.leftModelPath  = leftModelRoot.wstring();
    report.rightModelPath = rightModelRoot.wstring();

    report.leftModelName  = leftModelRoot.filename().wstring();
    report.rightModelName = rightModelRoot.filename().wstring();

    auto leftFiles  = EnumerateXmlFiles(leftModelRoot);
    auto rightFiles = EnumerateXmlFiles(rightModelRoot);

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

    std::set<std::wstring> allKeys;
    for (const auto& [k, _] : leftMap)  allKeys.insert(k);
    for (const auto& [k, _] : rightMap) allKeys.insert(k);

    report.totalFilesScanned = allKeys.size();

    size_t current = 0;
    for (const auto& key : allKeys) {
        ++current;

        bool inLeft  = leftMap.count(key)  > 0;
        bool inRight = rightMap.count(key) > 0;

        if (progressCb) {
            progressCb(current, allKeys.size(), key);
        }

        FileDiffResult fileResult;

        if (inLeft && !inRight) {
            fileResult.relativePath = MakeRelative(leftMap[key], leftModelRoot);
            fileResult.status = FileDiffResult::Status::DeletedInRight;

        } else if (!inLeft && inRight) {
            fileResult.relativePath = MakeRelative(rightMap[key], rightModelRoot);
            fileResult.status = FileDiffResult::Status::AddedInRight;

        } else {
            fileResult = m_strategy->CompareFiles(leftMap[key], rightMap[key]);
            fileResult.relativePath = MakeRelative(leftMap[key], leftModelRoot);
        }

        if (fileResult.HasDifferences()) {
            report.totalMissingIds += fileResult.missingInRight.size();
            report.totalExtraIds   += fileResult.extraInRight.size();
            report.fileResults.push_back(std::move(fileResult));
        }
    }

    report.totalFilesWithDiffs = report.fileResults.size();

    std::sort(report.fileResults.begin(), report.fileResults.end(),
        [](const FileDiffResult& a, const FileDiffResult& b) {
            if (a.status != b.status) return a.status < b.status;
            return a.relativePath < b.relativePath;
        });

    return report;
}

}
