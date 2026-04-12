// =============================================================================
// XmlValidationEngine.h
// Read-only diagnostic engine that scans XML files for structural issues.
//
// Detects two categories of issues:
//   1. Duplicate IDs — group_ID, spec_ID, val_id within their hierarchy scope
//   2. Syntax Warnings — missing attributes, orphaned elements,
//      unrecognized tags, invalid data formats
//
// Extensibility:
//   Each check is a standalone static method. To add a new check:
//     1. Add a new enum value to ValidationIssueType
//     2. Add a new CheckXxx() method
//     3. Call it from ValidateFile()
//
// Caching:
//   File content (lines + fullText) is cached per-file during validation
//   to enable instant XML viewer popup without re-reading from disk.
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declaration — avoid pulling tinyxml2 into every translation unit
namespace tinyxml2 { class XMLDocument; class XMLElement; }

namespace ModelCompare {

// ─────────────────────────────────────────────────────────────────────────────
// Issue classification — each enum value maps to one check method
// ─────────────────────────────────────────────────────────────────────────────
enum class ValidationIssueType {
    // Duplicate ID checks
    DuplicateGroupId,
    DuplicateSpecId,
    DuplicateValId,

    // Syntax warning checks
    MissingRequiredAttribute,
    OrphanedElement,
    UnrecognizedTag,
    InvalidDataFormat
};

// ─────────────────────────────────────────────────────────────────────────────
// Single validation issue — carries full context for UI display and viewer nav
// ─────────────────────────────────────────────────────────────────────────────
struct ValidationIssue {
    ValidationIssueType type;
    std::string description;        // Human-readable description for UI

    // Hierarchy context (populated as available)
    std::string groupId;
    std::string groupName;
    std::string specId;
    std::string specName;
    std::string valId;
    std::string valName;
    std::string elementTag;         // The tag that has the issue (e.g., "spec")

    int         lineNumber = -1;    // 0-based line index for XML viewer scrolling

    // Convenience: is this a duplicate-type issue?
    bool IsDuplicate() const {
        return type == ValidationIssueType::DuplicateGroupId
            || type == ValidationIssueType::DuplicateSpecId
            || type == ValidationIssueType::DuplicateValId;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-file validation result with cached content for instant viewer popup
// ─────────────────────────────────────────────────────────────────────────────
struct FileValidationResult {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;

    // Corruption state — if true, tinyxml2 could not parse the file
    bool        isCorrupt = false;
    std::string corruptionDetail;       // tinyxml2 ErrorStr()

    // All detected issues (empty if corrupt)
    std::vector<ValidationIssue> issues;

    // Cached file content for instant XML viewer popup
    std::vector<CString> cachedLines;
    CString              cachedFullText;

    bool HasIssues() const {
        return isCorrupt || !issues.empty();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-model validation report
// ─────────────────────────────────────────────────────────────────────────────
struct ModelValidationReport {
    std::wstring modelPath;
    std::wstring modelName;
    std::vector<FileValidationResult> fileResults;

    size_t totalFilesScanned    = 0;
    size_t totalFilesWithIssues = 0;
    size_t totalIssues          = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Combined report for both Left and Right models
// ─────────────────────────────────────────────────────────────────────────────
struct ValidationReport {
    ModelValidationReport leftReport;
    ModelValidationReport rightReport;
};

// ─────────────────────────────────────────────────────────────────────────────
// Validation engine — stateless, all methods are static
// ─────────────────────────────────────────────────────────────────────────────
class XmlValidationEngine {
public:
    /// Validate all XML files in both model directories.
    /// Designed to run on a background thread (no MFC dependencies).
    static ValidationReport ValidateModels(
        const std::filesystem::path& leftRoot,
        const std::filesystem::path& rightRoot);

private:
    // ── Orchestration ──
    static ModelValidationReport ValidateSingleModel(
        const std::filesystem::path& modelRoot);

    static FileValidationResult ValidateFile(
        const std::filesystem::path& filePath,
        const std::filesystem::path& modelRoot);

    // ── Individual checks — one method per concern ──

    /// Check for duplicate group_ID within <data>,
    /// duplicate spec_ID within each <group>,
    /// and duplicate val_id within each <spec>.
    static void CheckDuplicateIds(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    /// Check that required attributes are present:
    /// group: group_ID, group_name
    /// spec:  spec_ID, spec_name
    /// val:   val_id, val_name
    static void CheckMissingRequiredAttributes(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    /// Check that elements follow the strict hierarchy:
    /// <data> -> <group> -> <spec> -> <val>
    static void CheckOrphanedElements(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    /// Check for tags not in the allowlist: {data, group, spec, val}
    static void CheckUnrecognizedTags(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    /// Check that ID attributes (group_ID, spec_ID, val_id) are numeric
    static void CheckInvalidDataFormats(
        tinyxml2::XMLDocument& doc,
        FileValidationResult& result);

    // ── Utilities ──
    static int GetElementLineNumber(const tinyxml2::XMLElement* elem);
    static std::string SafeAttr(const tinyxml2::XMLElement* elem, const char* name);

    /// Recursively scan all elements for unrecognized tags
    static void ScanForUnrecognizedTags(
        const tinyxml2::XMLElement* elem,
        FileValidationResult& result);
};

} // namespace ModelCompare
