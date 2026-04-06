// =============================================================================
// XmlParser.cpp
// Implementation of the LAI XML configuration file parser.
//
// Uses TinyXML2 for XML parsing. Reads the file via std::ifstream to properly
// handle Unicode file paths on Windows (TinyXML2's LoadFile uses fopen which
// doesn't support wide paths).
// =============================================================================
#include "pch.h"
#include "XmlParser.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Helper: safely read an attribute, returning empty string if not present.
// ---------------------------------------------------------------------------
static std::string SafeAttr(const tinyxml2::XMLElement* elem, const char* name) {
    const char* val = elem->Attribute(name);
    return val ? val : "";
}

// ---------------------------------------------------------------------------
// Helper: clean group/spec/val names by stripping the Korean suffix after "$$"
// ---------------------------------------------------------------------------
static std::string CleanName(const std::string& raw) {
    auto pos = raw.find("$$");
    if (pos != std::string::npos) {
        std::string cleaned = raw.substr(0, pos);
        // Trim trailing whitespace
        while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t')) {
            cleaned.pop_back();
        }
        return cleaned;
    }
    return raw;
}

// ---------------------------------------------------------------------------
// Parse: Main entry point
// ---------------------------------------------------------------------------
XmlParser::ParseResult XmlParser::Parse(const std::filesystem::path& filePath) {
    ParseResult result;

    // --- Read file content via std::ifstream (handles Unicode paths) ---
    std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
    if (!fileStream.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open file: " + filePath.u8string();
        return result;
    }

    std::ostringstream ss;
    ss << fileStream.rdbuf();
    std::string content = ss.str();
    fileStream.close();

    // --- Parse XML ---
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError xmlErr = doc.Parse(content.c_str(), content.size());
    if (xmlErr != tinyxml2::XML_SUCCESS) {
        result.success = false;
        result.errorMessage = "XML parse error in " + filePath.u8string()
                            + ": " + doc.ErrorStr();
        return result;
    }

    // --- Find root <data> element ---
    tinyxml2::XMLElement* dataElem = doc.FirstChildElement("data");
    if (!dataElem) {
        result.success = false;
        result.errorMessage = "Missing <data> root element in " + filePath.u8string();
        return result;
    }

    // --- Walk hierarchy: <data> -> <group> -> <spec> -> <val> ---
    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem != nullptr;
         groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string groupId   = SafeAttr(groupElem, "group_ID");
        std::string groupName = CleanName(SafeAttr(groupElem, "group_name"));

        // Skip groups without an ID
        if (groupId.empty()) continue;

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem != nullptr;
             specElem = specElem->NextSiblingElement("spec"))
        {
            std::string specId   = SafeAttr(specElem, "spec_ID");
            std::string specName = CleanName(SafeAttr(specElem, "spec_name"));

            // Skip specs without an ID
            if (specId.empty()) continue;

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem != nullptr;
                 valElem = valElem->NextSiblingElement("val"))
            {
                std::string valId   = SafeAttr(valElem, "val_id");
                std::string valName = CleanName(SafeAttr(valElem, "val_name"));

                // Skip vals without an ID (per business rules)
                if (valId.empty()) continue;

                // Build the full node info
                XmlNodeInfo node;
                node.groupId     = groupId;
                node.groupName   = groupName;
                node.specId      = specId;
                node.specName    = specName;
                node.valId       = valId;
                node.valName     = valName;

                // Capture data attributes for future extensibility
                node.value       = SafeAttr(valElem, "value");
                node.minVal      = SafeAttr(valElem, "min");
                node.maxVal      = SafeAttr(valElem, "max");
                node.valEditable = SafeAttr(valElem, "val_editable");
                node.valDatatype = SafeAttr(valElem, "val_datatype");
                node.paramId     = SafeAttr(valElem, "param_id");

                result.nodes.push_back(std::move(node));
            }
        }
    }

    result.success = true;
    return result;
}

} // namespace ModelCompare
