#include "pch.h"
#include "XmlParser.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>

namespace ModelCompare {

static std::string SafeAttr(const tinyxml2::XMLElement* elem, const char* name) {
    const char* val = elem->Attribute(name);
    return val ? val : "";
}

static std::string CleanName(const std::string& raw) {
    auto pos = raw.find("$$");
    if (pos != std::string::npos) {
        std::string cleaned = raw.substr(0, pos);
        while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t')) {
            cleaned.pop_back();
        }
        return cleaned;
    }
    return raw;
}

static bool LoadDocument(const std::filesystem::path& filePath,
                         tinyxml2::XMLDocument& doc,
                         std::string& errorMessage)
{
    std::ifstream fs(filePath, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        errorMessage = "Failed to open file: " + filePath.u8string();
        return false;
    }

    std::ostringstream ss;
    ss << fs.rdbuf();
    std::string content = ss.str();
    fs.close();

    if (doc.Parse(content.c_str(), content.size()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "XML parse error in " + filePath.u8string()
                     + ": " + doc.ErrorStr();
        return false;
    }
    return true;
}

static tinyxml2::XMLElement* FindDataRoot(tinyxml2::XMLDocument& doc,
                                          const std::filesystem::path& filePath,
                                          std::string& errorMessage)
{
    auto* dataElem = doc.FirstChildElement("data");
    if (!dataElem) {
        errorMessage = "Missing <data> root element in " + filePath.u8string();
    }
    return dataElem;
}

XmlParser::ParseResult XmlParser::Parse(const std::filesystem::path& filePath) {
    ParseResult result;

    tinyxml2::XMLDocument doc;
    if (!LoadDocument(filePath, doc, result.errorMessage)) {
        return result;
    }

    auto* dataElem = FindDataRoot(doc, filePath, result.errorMessage);
    if (!dataElem) return result;

    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string groupId   = SafeAttr(groupElem, "group_ID");
        std::string groupName = CleanName(SafeAttr(groupElem, "group_name"));
        if (groupId.empty()) continue;

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string specId   = SafeAttr(specElem, "spec_ID");
            std::string specName = CleanName(SafeAttr(specElem, "spec_name"));
            if (specId.empty()) continue;

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string valId   = SafeAttr(valElem, "val_id");
                std::string valName = CleanName(SafeAttr(valElem, "val_name"));
                if (valId.empty()) continue;

                XmlNodeInfo node;
                node.groupId     = groupId;
                node.groupName   = groupName;
                node.specId      = specId;
                node.specName    = specName;
                node.valId       = valId;
                node.valName     = valName;
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

HierarchicalParseResult XmlParser::ParseHierarchical(
    const std::filesystem::path& filePath)
{
    HierarchicalParseResult result;

    tinyxml2::XMLDocument doc;
    if (!LoadDocument(filePath, doc, result.errorMessage)) {
        return result;
    }

    auto* dataElem = FindDataRoot(doc, filePath, result.errorMessage);
    if (!dataElem) return result;

    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string groupId = SafeAttr(groupElem, "group_ID");
        if (groupId.empty()) continue;

        ParsedGroup& group = result.groups[groupId];
        group.groupId   = groupId;
        group.groupName = CleanName(SafeAttr(groupElem, "group_name"));

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string specId = SafeAttr(specElem, "spec_ID");
            if (specId.empty()) continue;

            ParsedSpec& spec = group.specs[specId];
            spec.specId   = specId;
            spec.specName = CleanName(SafeAttr(specElem, "spec_name"));

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string valId = SafeAttr(valElem, "val_id");
                if (valId.empty()) continue;

                ParsedVal& val = spec.vals[valId];
                val.valId   = valId;
                val.valName = CleanName(SafeAttr(valElem, "val_name"));
            }
        }
    }

    result.success = true;
    return result;
}

}