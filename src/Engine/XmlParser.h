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


struct ParsedVal {
    std::string valId;
    std::string valName;
};

struct ParsedSpec {
    std::string specId;
    std::string specName;
    std::unordered_map<std::string, ParsedVal> vals;
};

struct ParsedGroup {
    std::string groupId;
    std::string groupName;
    std::unordered_map<std::string, ParsedSpec> specs;
};

struct HierarchicalParseResult {
    bool        success = false;
    std::string errorMessage;
    std::unordered_map<std::string, ParsedGroup> groups;
};


class XmlParser {
public:
    struct ParseResult {
        bool        success = false;
        std::string errorMessage;
        std::vector<XmlNodeInfo> nodes;
    };

    static ParseResult Parse(const std::filesystem::path& filePath);

    static HierarchicalParseResult ParseHierarchical(
        const std::filesystem::path& filePath);
};

}
