#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

namespace ModelCompare {

struct ValueDiffProperty {
    std::string propertyName;
    std::string leftValue;
    std::string rightValue;
};

struct ValueDiffEntry {
    std::string compositeKey;
    std::string groupName;
    std::string specName;
    std::string valName;
    std::vector<ValueDiffProperty> differingProperties;
};

struct FileValueDiffResult {
    std::filesystem::path relativePath;
    std::vector<ValueDiffEntry> differences;

    bool HasDifferences() const {
        return !differences.empty();
    }
};

struct ValueDiffReport {
    std::wstring leftModelName;
    std::wstring rightModelName;
    std::wstring leftModelPath;
    std::wstring rightModelPath;

    std::vector<FileValueDiffResult> fileResults;
    std::vector<std::string> errors;

    size_t totalFilesScanned = 0;
    size_t totalFilesWithDiffs = 0;
    size_t totalValueDifferences = 0;
};

class ValueCompareEngine {
public:
    static ValueDiffReport CompareModels(const std::filesystem::path& leftPath,
                                         const std::filesystem::path& rightPath);

private:
    static void ScanDirectory(const std::filesystem::path& rootPath,
                              std::unordered_map<std::wstring, std::filesystem::path>& fileMap);
    
    static FileValueDiffResult CompareSingleFile(const std::filesystem::path& leftFile,
                                                 const std::filesystem::path& rightFile,
                                                 const std::filesystem::path& relativePath);
};

} // namespace ModelCompare
