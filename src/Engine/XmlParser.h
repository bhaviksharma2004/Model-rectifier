// =============================================================================
// XmlParser.h
// Parses LAI XML configuration files and extracts structured node information.
//
// Two parsing modes:
//   1. Flat — Parse() returns a flat list of <val> nodes (used by XML viewer).
//   2. Hierarchical — ParseHierarchical() preserves the group→spec→val tree
//      (used by the structural comparator for level-aware diff detection).
//
// Understands the <data> -> <group> -> <spec> -> <val> hierarchy.
// Extracts ALL attributes from each level for maximum extensibility.
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Hierarchical parse result types — preserve the XML tree structure.
// Used by StructuralIdComparator for top-down comparison.
// ---------------------------------------------------------------------------
struct ParsedVal {
    std::string valId;
    std::string valName;
};

struct ParsedSpec {
    std::string specId;
    std::string specName;
    std::unordered_map<std::string, ParsedVal> vals;  // keyed by val_id
};

struct ParsedGroup {
    std::string groupId;
    std::string groupName;
    std::unordered_map<std::string, ParsedSpec> specs;  // keyed by spec_ID
};

struct HierarchicalParseResult {
    bool        success = false;
    std::string errorMessage;
    std::unordered_map<std::string, ParsedGroup> groups;  // keyed by group_ID
};

// ---------------------------------------------------------------------------
// XmlParser — Dual-mode XML parser for LAI configuration files.
// ---------------------------------------------------------------------------
class XmlParser {
public:
    // Result of flat parsing (all <val> nodes with full parent context)
    struct ParseResult {
        bool        success = false;
        std::string errorMessage;
        std::vector<XmlNodeInfo> nodes;
    };

    // Flat parse: extract all <val> nodes with hierarchy context.
    // Nodes missing any required ID (group_ID, spec_ID, val_id) are skipped.
    static ParseResult Parse(const std::filesystem::path& filePath);

    // Hierarchical parse: preserve the group→spec→val tree structure.
    // Groups/specs/vals missing their primary ID are skipped.
    static HierarchicalParseResult ParseHierarchical(
        const std::filesystem::path& filePath);
};

} // namespace ModelCompare
