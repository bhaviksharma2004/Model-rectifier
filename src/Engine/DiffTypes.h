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


enum class DiffLevel {
    Group,
    Spec,
    Val
};

struct XmlNodeInfo {
    std::string groupId;
    std::string groupName;

    std::string specId;
    std::string specName;

    std::string valId;
    std::string valName;

    std::string value;
    std::string minVal;
    std::string maxVal;
    std::string valEditable;
    std::string valDatatype;
    std::string paramId;

    std::string CompositeKey() const {
        return "G:" + groupId + "|S:" + specId + "|V:" + valId;
    }

    bool HasValidIds() const {
        return !groupId.empty() && !specId.empty() && !valId.empty();
    }
};

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

struct FileDiffResult {
    std::filesystem::path relativePath;

    enum class Status {
        Identical,
        Modified,
        DeletedInRight,
        AddedInRight
    };
    Status status = Status::Identical;

    std::vector<KeyDiffEntry> missingInRight;
    std::vector<KeyDiffEntry> extraInRight;

    size_t leftKeyCount  = 0;
    size_t rightKeyCount = 0;

    bool HasDifferences() const {
        return status != Status::Identical;
    }

    size_t DiffCount() const {
        return missingInRight.size() + extraInRight.size();
    }
};

struct ModelDiffReport {
    std::wstring leftModelName;
    std::wstring rightModelName;
    std::wstring leftModelPath;
    std::wstring rightModelPath;

    std::vector<FileDiffResult> fileResults;

    size_t totalFilesScanned   = 0;
    size_t totalFilesWithDiffs = 0;
    size_t totalMissingIds     = 0;
    size_t totalExtraIds       = 0;

    std::vector<std::string> errors;
};

} 
