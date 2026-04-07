// =============================================================================
// StructuralIdComparator.cpp
// Implements the structural ID comparison strategy with hierarchical diffing.
//
// Algorithm (top-down):
//   1. Parse both XML files using ParseHierarchical()
//   2. Compare group_IDs — emit DiffLevel::Group for unmatched groups
//   3. For matching groups, compare spec_IDs — emit DiffLevel::Spec
//   4. For matching specs, compare val_ids — emit DiffLevel::Val
//   5. Repeat in reverse direction for "extra in Right" diffs
//
// This ensures exactly ONE diff entry per structural mismatch at the
// highest applicable hierarchy level, avoiding duplicate rows.
// =============================================================================
#include "pch.h"
#include "StructuralIdComparator.h"
#include "XmlParser.h"

#include <algorithm>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Helper: Detect missing entries from `source` that are absent in `target`
//         at the group→spec→val hierarchy, emitting at the highest level.
// ---------------------------------------------------------------------------
static void DetectMissing(
    const std::unordered_map<std::string, ParsedGroup>& source,
    const std::unordered_map<std::string, ParsedGroup>& target,
    std::vector<KeyDiffEntry>& output)
{
    for (const auto& [groupId, srcGroup] : source) {
        auto targetGroupIt = target.find(groupId);

        // Entire group is missing from target
        if (targetGroupIt == target.end()) {
            KeyDiffEntry entry;
            entry.level      = DiffLevel::Group;
            entry.groupId    = groupId;
            entry.groupName  = srcGroup.groupName;
            entry.childCount = static_cast<int>(srcGroup.specs.size());
            entry.compositeKey = KeyDiffEntry::MakeKey(DiffLevel::Group, groupId);
            output.push_back(std::move(entry));
            continue;
        }

        // Group exists in both — compare specs within it
        const auto& targetSpecs = targetGroupIt->second.specs;

        for (const auto& [specId, srcSpec] : srcGroup.specs) {
            auto targetSpecIt = targetSpecs.find(specId);

            // Entire spec is missing from target's group
            if (targetSpecIt == targetSpecs.end()) {
                KeyDiffEntry entry;
                entry.level      = DiffLevel::Spec;
                entry.groupId    = groupId;
                entry.groupName  = srcGroup.groupName;
                entry.specId     = specId;
                entry.specName   = srcSpec.specName;
                entry.childCount = static_cast<int>(srcSpec.vals.size());
                entry.compositeKey = KeyDiffEntry::MakeKey(DiffLevel::Spec, groupId, specId);
                output.push_back(std::move(entry));
                continue;
            }

            // Spec exists in both — compare vals within it
            const auto& targetVals = targetSpecIt->second.vals;

            for (const auto& [valId, srcVal] : srcSpec.vals) {
                if (targetVals.find(valId) == targetVals.end()) {
                    KeyDiffEntry entry;
                    entry.level      = DiffLevel::Val;
                    entry.groupId    = groupId;
                    entry.groupName  = srcGroup.groupName;
                    entry.specId     = specId;
                    entry.specName   = srcSpec.specName;
                    entry.valId      = valId;
                    entry.valName    = srcVal.valName;
                    entry.childCount = 0;
                    entry.compositeKey = KeyDiffEntry::MakeKey(DiffLevel::Val, groupId, specId, valId);
                    output.push_back(std::move(entry));
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// CompareFiles: Main comparison logic — hierarchical top-down diff detection
// ---------------------------------------------------------------------------
FileDiffResult StructuralIdComparator::CompareFiles(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile)
{
    FileDiffResult result;

    auto leftParsed  = XmlParser::ParseHierarchical(leftFile);
    auto rightParsed = XmlParser::ParseHierarchical(rightFile);

    if (!leftParsed.success || !rightParsed.success) {
        result.status = FileDiffResult::Status::Modified;
        return result;
    }

    // Count total leaf keys for summary statistics
    size_t leftCount = 0, rightCount = 0;
    for (const auto& [_, g] : leftParsed.groups)
        for (const auto& [__, s] : g.specs)
            leftCount += s.vals.size();
    for (const auto& [_, g] : rightParsed.groups)
        for (const auto& [__, s] : g.specs)
            rightCount += s.vals.size();

    result.leftKeyCount  = leftCount;
    result.rightKeyCount = rightCount;

    // Detect missing in Right (present in Left, absent in Right)
    DetectMissing(leftParsed.groups, rightParsed.groups, result.missingInRight);

    // Detect extra in Right (present in Right, absent in Left)
    DetectMissing(rightParsed.groups, leftParsed.groups, result.extraInRight);

    // Determine status
    if (result.missingInRight.empty() && result.extraInRight.empty()) {
        result.status = FileDiffResult::Status::Identical;
    } else {
        result.status = FileDiffResult::Status::Modified;
    }

    // Sort entries by composite key for consistent display order
    auto sortByKey = [](const KeyDiffEntry& a, const KeyDiffEntry& b) {
        return a.compositeKey < b.compositeKey;
    };
    std::sort(result.missingInRight.begin(), result.missingInRight.end(), sortByKey);
    std::sort(result.extraInRight.begin(),   result.extraInRight.end(),   sortByKey);

    return result;
}

} // namespace ModelCompare
