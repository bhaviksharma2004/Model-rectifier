#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>


namespace tinyxml2 { class XMLDocument; class XMLElement; }

namespace ModelCompare {

enum class ValidationIssueType {
    
    DuplicateGroupId,
    DuplicateSpecId,
    DuplicateValId,

    MissingRequiredAttribute,
    OrphanedElement,
    UnrecognizedTag,
    InvalidDataFormat
};

struct ValidationIssue {
    ValidationIssueType type;
    std::string description;        
    std::string groupId;
    std::string groupName;
    std::string specId;
    std::string specName;
    std::string valId;
    std::string valName;
    std::string elementTag;         
    int lineNumber = -1;    

    bool IsDuplicate() const {
        return type == ValidationIssueType::DuplicateGroupId
            || type == ValidationIssueType::DuplicateSpecId
            || type == ValidationIssueType::DuplicateValId;
    }
};

struct FileValidationResult {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;
    
    bool        isCorrupt = false;
    std::string corruptionDetail;       
    
    std::vector<ValidationIssue> issues;

    std::vector<CString> cachedLines;
    CString              cachedFullText;

    bool HasIssues() const {
        return isCorrupt || !issues.empty();
    }
};

struct ModelValidationReport {
    std::wstring modelPath;
    std::wstring modelName;
    std::vector<FileValidationResult> fileResults;

    size_t totalFilesScanned    = 0;
    size_t totalFilesWithIssues = 0;
    size_t totalIssues          = 0;
};

struct ValidationReport {
    ModelValidationReport leftReport;
    ModelValidationReport rightReport;
};

class XmlValidationEngine {
public:   
    static ValidationReport ValidateModels(
        const std::filesystem::path& leftRoot,
        const std::filesystem::path& rightRoot);

private:
    
    static ModelValidationReport ValidateSingleModel(
        const std::filesystem::path& modelRoot);

    static FileValidationResult ValidateFile(
        const std::filesystem::path& filePath,
        const std::filesystem::path& modelRoot);
 
    
    static void CheckDuplicateIds(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);
   
    static void CheckMissingRequiredAttributes(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    
    
    static void CheckOrphanedElements(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    
    static void CheckUnrecognizedTags(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    
    static void CheckInvalidDataFormats(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    
    static int GetElementLineNumber(const tinyxml2::XMLElement* elem);
    static std::string SafeAttr(const tinyxml2::XMLElement* elem, const char* name);

    
    static void ScanForUnrecognizedTags(
        const tinyxml2::XMLElement* elem,
        FileValidationResult& result);
};

} 
