// =============================================================================
// DiffTypes.h
// Core data structures for the Model Compare engine.
//
// Design:
//   - DiffLevel indicates the hierarchy tier at which a diff was detected.
//   - XmlNodeInfo captures the FULL hierarchy context of each <val> node,
//     including all attributes. This enables future comparison strategies
//     to compare values, names, or any other attribute without parser changes.
//   - KeyDiffEntry represents a single structural mismatch at any hierarchy
//     level (group, spec, or val).
//   - FileDiffResult represents the comparison outcome for a single XML file.
//   - ModelDiffReport aggregates results across all files in two model dirs.
//
// Extensibility:
//   To add new comparison criteria, create a new IComparisonStrategy
//   implementation. The data structures here are intentionally rich enough
//   to support value-based, attribute-based, or any other comparison mode.
// =============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// DiffLevel — The hierarchy tier at which a structural mismatch was detected.
//
//   Group — An entire <group> is missing or extra.
//   Spec  — A <spec> within a matching group is missing or extra.
//   Val   — A <val> within a matching spec is missing or extra.
// ---------------------------------------------------------------------------
enum class DiffLevel {
    Group,
    Spec,
    Val
};

// ---------------------------------------------------------------------------
// XmlNodeInfo — Represents a single <val> node with its full parent context.
//
// Hierarchy: <data> -> <group> -> <spec> -> <val>
//
// All attributes are captured for extensibility. Current comparison only
// uses the ID fields, but future strategies may use value/min/max/etc.
//
// NOTE: This struct is used by the flat parser (XmlParser::Parse) and the
//       XML viewer. The hierarchical comparator uses ParsedGroup/Spec/Val.
// ---------------------------------------------------------------------------
struct XmlNodeInfo {
    // Group-level attributes
    std::string groupId;
    std::string groupName;

    // Spec-level attributes
    std::string specId;
    std::string specName;

    // Val-level attributes
    std::string valId;
    std::string valName;

    // Data attributes (captured for future comparison modes)
    std::string value;
    std::string minVal;
    std::string maxVal;
    std::string valEditable;
    std::string valDatatype;
    std::string paramId;

    // Generate the composite key used for structural ID comparison.
    // Format: "G:<group_ID>|S:<spec_ID>|V:<val_id>"
    std::string CompositeKey() const {
        return "G:" + groupId + "|S:" + specId + "|V:" + valId;
    }

    // Returns true if all three ID fields are non-empty
    bool HasValidIds() const {
        return !groupId.empty() && !specId.empty() && !valId.empty();
    }
};

// ---------------------------------------------------------------------------
// KeyDiffEntry — A single structural mismatch with hierarchy-aware context.
//
// The `level` field determines interpretation:
//   DiffLevel::Group → groupId/groupName populated; specId/valId empty
//   DiffLevel::Spec  → groupId + specId populated;  valId empty
//   DiffLevel::Val   → all three IDs populated
//
// The `childCount` field indicates how many children are encompassed:
//   Group-level → number of specs in the group
//   Spec-level  → number of vals in the spec
//   Val-level   → 0 (leaf node)
// ---------------------------------------------------------------------------
struct KeyDiffEntry {
    std::string compositeKey;
    std::string groupId;
    std::string groupName;
    std::string specId;
    std::string specName;
    std::string valId;
    std::string valName;
    DiffLevel   level      = DiffLevel::Val;
    int         childCount = 0;

    // Build a composite key appropriate to the diff level.
    static std::string MakeKey(DiffLevel lvl,
                               const std::string& gid,
                               const std::string& sid = "",
                               const std::string& vid = "")
    {
        switch (lvl) {
        case DiffLevel::Group: return "G:" + gid;
        case DiffLevel::Spec:  return "G:" + gid + "|S:" + sid;
        case DiffLevel::Val:   return "G:" + gid + "|S:" + sid + "|V:" + vid;
        }
        return {};
    }
};

// ---------------------------------------------------------------------------
// FileDiffResult — Comparison result for a single XML file pair.
//
// Status semantics:
//   Identical      — Both files exist and have the same ID set
//   Modified       — Both files exist but differ in ID structure
//   DeletedInRight — File exists ONLY in the Left (Reference) model
//   AddedInRight   — File exists ONLY in the Right (Target) model
// ---------------------------------------------------------------------------
struct FileDiffResult {
    std::filesystem::path relativePath;  // e.g., "Algo\Lens Over (Normal).xml"

    enum class Status {
        Identical,
        Modified,
        DeletedInRight,
        AddedInRight
    };
    Status status = Status::Identical;

    std::vector<KeyDiffEntry> missingInRight;  // IDs in Left but NOT in Right
    std::vector<KeyDiffEntry> extraInRight;    // IDs in Right but NOT in Left

    size_t leftKeyCount  = 0;
    size_t rightKeyCount = 0;

    bool HasDifferences() const {
        return status != Status::Identical;
    }

    size_t DiffCount() const {
        return missingInRight.size() + extraInRight.size();
    }
};

// ---------------------------------------------------------------------------
// ModelDiffReport — Full comparison report across two model directories.
// ---------------------------------------------------------------------------
struct ModelDiffReport {
    std::wstring leftModelName;
    std::wstring rightModelName;
    std::wstring leftModelPath;
    std::wstring rightModelPath;

    std::vector<FileDiffResult> fileResults;  // ONLY files with differences

    size_t totalFilesScanned   = 0;
    size_t totalFilesWithDiffs = 0;
    size_t totalMissingIds     = 0;
    size_t totalExtraIds       = 0;

    std::vector<std::string> errors;  // Parsing errors encountered
};

} // namespace ModelCompare
