#include "pch.h"
#include "ValueCompareEngine.h"
#include "XmlParser.h"
#include <algorithm>

namespace ModelCompare {

ValueDiffReport ValueCompareEngine::CompareModels(const std::filesystem::path& leftPath,
                                                  const std::filesystem::path& rightPath)
{
    ValueDiffReport report;
    report.leftModelPath = leftPath.wstring();
    report.rightModelPath = rightPath.wstring();
    
    // Extract Model Names (simple folder name extraction)
    if (leftPath.has_filename()) report.leftModelName = leftPath.filename().wstring();
    if (rightPath.has_filename()) report.rightModelName = rightPath.filename().wstring();

    if (!std::filesystem::exists(leftPath) || !std::filesystem::exists(rightPath)) {
        report.errors.push_back("One or both model paths do not exist.");
        return report;
    }

    std::unordered_map<std::wstring, std::filesystem::path> leftFiles;
    std::unordered_map<std::wstring, std::filesystem::path> rightFiles;

    ScanDirectory(leftPath, leftFiles);
    ScanDirectory(rightPath, rightFiles);

    // Only compare files that exist in both models
    for (const auto& [relStr, leftFilePath] : leftFiles) {
        auto itRight = rightFiles.find(relStr);
        if (itRight != rightFiles.end()) {
            report.totalFilesScanned++;
            
            auto rightFilePath = itRight->second;
            std::filesystem::path relPath(relStr);
            
            FileValueDiffResult fileResult = CompareSingleFile(leftFilePath, rightFilePath, relPath);
            if (fileResult.HasDifferences()) {
                report.fileResults.push_back(std::move(fileResult));
                report.totalFilesWithDiffs++;
                for (const auto& entry : report.fileResults.back().differences)
                    report.totalValueDifferences += entry.differingProperties.size();
            }
        }
    }

    return report;
}

void ValueCompareEngine::ScanDirectory(const std::filesystem::path& rootPath,
                                       std::unordered_map<std::wstring, std::filesystem::path>& fileMap)
{
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file() && 
                _wcsicmp(entry.path().extension().wstring().c_str(), L".xml") == 0) 
            {
                std::filesystem::path relPath = std::filesystem::relative(entry.path(), rootPath);
                
                // Keep the relative path exactly as read for comparison
                fileMap[relPath.wstring()] = entry.path();
            }
        }
    }
    catch (const std::exception&) {
        // Ignore errors during directory iteration
    }
}

FileValueDiffResult ValueCompareEngine::CompareSingleFile(const std::filesystem::path& leftFile,
                                                          const std::filesystem::path& rightFile,
                                                          const std::filesystem::path& relativePath)
{
    FileValueDiffResult result;
    result.relativePath = relativePath;

    auto leftParse = XmlParser::Parse(leftFile);
    auto rightParse = XmlParser::Parse(rightFile);

    if (!leftParse.success || !rightParse.success) {
        return result; // Skip invalid XML files
    }

    // Map right nodes by composite key for fast lookup
    std::unordered_map<std::string, const XmlNodeInfo*> rightMap;
    for (const auto& node : rightParse.nodes) {
        rightMap[node.CompositeKey()] = &node;
    }

    // Compare left nodes against right nodes
    for (const auto& leftNode : leftParse.nodes) {
        std::string key = leftNode.CompositeKey();
        
        auto it = rightMap.find(key);
        if (it != rightMap.end()) {
            const XmlNodeInfo* rightNode = it->second;

            ValueDiffEntry diffEntry;
            diffEntry.compositeKey = key;
            diffEntry.groupName = leftNode.groupName;
            diffEntry.specName = leftNode.specName;
            diffEntry.valName = leftNode.valName;

            // Gather all attributes from both nodes to compare
            // To be comprehensive, check all keys from left and right
            std::vector<std::string> allKeys;
            for (const auto& [attrName, attrVal] : leftNode.attributes) {
                allKeys.push_back(attrName);
            }
            for (const auto& [attrName, attrVal] : rightNode->attributes) {
                if (std::find(allKeys.begin(), allKeys.end(), attrName) == allKeys.end()) {
                    allKeys.push_back(attrName);
                }
            }

            for (const auto& attrName : allKeys) {
                std::string leftVal = "";
                std::string rightVal = "";

                auto itLeftAttr = leftNode.attributes.find(attrName);
                if (itLeftAttr != leftNode.attributes.end()) leftVal = itLeftAttr->second;

                auto itRightAttr = rightNode->attributes.find(attrName);
                if (itRightAttr != rightNode->attributes.end()) rightVal = itRightAttr->second;

                if (leftVal != rightVal) {
                    ValueDiffProperty prop;
                    prop.propertyName = attrName;
                    prop.leftValue = leftVal;
                    prop.rightValue = rightVal;
                    diffEntry.differingProperties.push_back(prop);
                }
            }

            if (!diffEntry.differingProperties.empty()) {
                result.differences.push_back(std::move(diffEntry));
            }
        }
    }

    return result;
}

} // namespace ModelCompare
