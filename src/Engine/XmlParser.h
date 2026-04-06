// =============================================================================
// XmlParser.h
// Parses LAI XML configuration files and extracts structured node information.
//
// Understands the <data> -> <group> -> <spec> -> <val> hierarchy.
// Extracts ALL attributes from each level for maximum extensibility.
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <vector>
#include <string>

namespace ModelCompare {

class XmlParser {
public:
    // Result of parsing a single XML file
    struct ParseResult {
        bool        success = false;
        std::string errorMessage;

        // All <val> nodes with their full parent hierarchy context
        std::vector<XmlNodeInfo> nodes;
    };

    // Parse an XML file and extract all <val> nodes with hierarchy context.
    // Nodes missing any required ID (group_ID, spec_ID, val_id) are skipped.
    static ParseResult Parse(const std::filesystem::path& filePath);
};

} // namespace ModelCompare
