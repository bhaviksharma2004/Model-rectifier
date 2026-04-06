// =============================================================================
// StructuralIdComparator.cpp
// Implements the structural ID comparison strategy.
//
// Algorithm:
//   1. Parse both XML files using XmlParser
//   2. Build an unordered_set of composite keys for each file
//   3. Compute set difference in both directions
//   4. Return FileDiffResult with missing/extra entries
// =============================================================================
#include "pch.h"
#include "StructuralIdComparator.h"
#include "XmlParser.h"

#include <unordered_set>
#include <unordered_map>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Helper: Build a lookup map from composite key -> XmlNodeInfo
// ---------------------------------------------------------------------------
static std::unordered_map<std::string, const XmlNodeInfo*>
BuildKeyMap(const std::vector<XmlNodeInfo>& nodes) {
    std::unordered_map<std::string, const XmlNodeInfo*> map;
    map.reserve(nodes.size());
    for (const auto& node : nodes) {
        if (node.HasValidIds()) {
            map[node.CompositeKey()] = &node;
        }
    }
    return map;
}

// ---------------------------------------------------------------------------
// Helper: Convert XmlNodeInfo to a KeyDiffEntry for display
// ---------------------------------------------------------------------------
static KeyDiffEntry ToKeyDiffEntry(const XmlNodeInfo& node) {
    KeyDiffEntry entry;
    entry.compositeKey = node.CompositeKey();
    entry.groupId      = node.groupId;
    entry.groupName    = node.groupName;
    entry.specId       = node.specId;
    entry.specName     = node.specName;
    entry.valId        = node.valId;
    entry.valName      = node.valName;
    return entry;
}

// ---------------------------------------------------------------------------
// CompareFiles: Main comparison logic
// ---------------------------------------------------------------------------
FileDiffResult StructuralIdComparator::CompareFiles(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile)
{
    FileDiffResult result;

    // Parse both files
    auto leftParsed  = XmlParser::Parse(leftFile);
    auto rightParsed = XmlParser::Parse(rightFile);

    // If either file fails to parse, treat as modified with error info
    if (!leftParsed.success || !rightParsed.success) {
        result.status = FileDiffResult::Status::Modified;
        return result;
    }

    // Build key maps for O(1) lookup
    auto leftMap  = BuildKeyMap(leftParsed.nodes);
    auto rightMap = BuildKeyMap(rightParsed.nodes);

    result.leftKeyCount  = leftMap.size();
    result.rightKeyCount = rightMap.size();

    // Find keys in Left that are missing from Right
    for (const auto& [key, nodePtr] : leftMap) {
        if (rightMap.find(key) == rightMap.end()) {
            result.missingInRight.push_back(ToKeyDiffEntry(*nodePtr));
        }
    }

    // Find keys in Right that are extra (not in Left)
    for (const auto& [key, nodePtr] : rightMap) {
        if (leftMap.find(key) == leftMap.end()) {
            result.extraInRight.push_back(ToKeyDiffEntry(*nodePtr));
        }
    }

    // Determine status
    if (result.missingInRight.empty() && result.extraInRight.empty()) {
        result.status = FileDiffResult::Status::Identical;
    } else {
        result.status = FileDiffResult::Status::Modified;
    }

    // Sort entries by composite key for consistent display
    auto sortByKey = [](const KeyDiffEntry& a, const KeyDiffEntry& b) {
        return a.compositeKey < b.compositeKey;
    };
    std::sort(result.missingInRight.begin(), result.missingInRight.end(), sortByKey);
    std::sort(result.extraInRight.begin(),   result.extraInRight.end(),   sortByKey);

    return result;
}

} // namespace ModelCompare
