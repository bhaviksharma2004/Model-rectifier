#include "pch.h"
#include "XmlValidationEngine.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>
#include <unordered_set>
#include <cctype>

namespace ModelCompare {

std::string XmlValidationEngine::SafeAttr(
    const tinyxml2::XMLElement* elem, const char* name)
{
    const char* val = elem->Attribute(name);
    return val ? val : "";
}

int XmlValidationEngine::GetElementLineNumber(
    const tinyxml2::XMLElement* elem)
{
    return elem ? elem->GetLineNum() - 1 : -1;   
}



static bool IsNumericString(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}





static std::vector<std::filesystem::path> EnumerateXmlFiles(
    const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(root)) return files;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().wstring();
            
            if (ext == L".xml" || ext == L".XML" || ext == L".Xml") {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}





ModelValidationReport XmlValidationEngine::ValidateModel(
    const std::filesystem::path& modelRoot)
{
    return ValidateSingleModel(modelRoot);
}

ModelValidationReport XmlValidationEngine::ValidateSingleModel(
    const std::filesystem::path& modelRoot)
{
    ModelValidationReport report;
    report.modelPath = modelRoot.wstring();
    report.modelName = modelRoot.filename().wstring();

    auto xmlFiles = EnumerateXmlFiles(modelRoot);
    report.totalFilesScanned = xmlFiles.size();

    for (const auto& filePath : xmlFiles) {
        auto fileResult = ValidateFile(filePath, modelRoot);

        if (fileResult.HasIssues()) {
            report.totalFilesWithIssues++;
            report.totalIssues += fileResult.isCorrupt
                ? 1
                : fileResult.issues.size();
            report.fileResults.push_back(std::move(fileResult));
        }
    }
    return report;
}

FileValidationResult XmlValidationEngine::ValidateFile(
    const std::filesystem::path& filePath,
    const std::filesystem::path& modelRoot)
{
    FileValidationResult result;
    result.absolutePath = filePath;
    result.relativePath = std::filesystem::relative(filePath, modelRoot);

    std::string rawUtf8;
    {
        std::ifstream fs(filePath, std::ios::in | std::ios::binary);
        if (!fs.is_open()) {
            result.isCorrupt = true;
            result.corruptionDetail = "Failed to open file: " + filePath.u8string();
            return result;
        }

        std::ostringstream ss;
        ss << fs.rdbuf();
        rawUtf8 = ss.str();
        fs.close();
    }

    {
        std::istringstream lineStream(rawUtf8);
        std::string line;
        while (std::getline(lineStream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            result.cachedLines.push_back(CString(CA2W(line.c_str(), CP_UTF8)));
        }

        size_t totalChars = 0;
        for (const auto& ln : result.cachedLines)
            totalChars += (size_t)ln.GetLength() + 2;

        result.cachedFullText.Preallocate((int)totalChars + 1);
        for (const auto& ln : result.cachedLines) {
            result.cachedFullText.Append(ln);
            result.cachedFullText.Append(_T("\r\n"));
        }
    }

    tinyxml2::XMLDocument doc;
    if (doc.Parse(rawUtf8.c_str(), rawUtf8.size()) != tinyxml2::XML_SUCCESS) {
        result.isCorrupt = true;
        result.corruptionDetail = doc.ErrorStr();
        result.corruptionLineNumber = doc.ErrorLineNum();
        return result;
    }

    CheckDuplicateIds(doc, result);
    CheckMissingRequiredAttributes(doc, result);
    CheckHierarchyMismatches(doc, result);
    CheckUnrecognizedTags(doc, result);
    CheckInvalidDataFormats(doc, result);
    return result;
}

void XmlValidationEngine::CheckDuplicateIds(
    tinyxml2::XMLDocument& doc,
    FileValidationResult& result)
{
    auto* dataElem = doc.FirstChildElement("data");
    if (!dataElem) return;
    {
        struct GroupInfo { int line; };
        std::unordered_map<std::string, std::vector<GroupInfo>> groupIdMap;

        for (auto* groupElem = dataElem->FirstChildElement("group");
             groupElem; groupElem = groupElem->NextSiblingElement("group"))
        {
            std::string gid = SafeAttr(groupElem, "group_ID");
            if (gid.empty()) continue;
            groupIdMap[gid].push_back({ GetElementLineNumber(groupElem) });
        }

        for (const auto& [id, occurrences] : groupIdMap) {
            if (occurrences.size() <= 1) continue;
            for (const auto& occ : occurrences) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::DuplicateGroupId;
                issue.groupId     = id;
                issue.elementTag  = "group";
                issue.lineNumber  = occ.line;
                issue.description = "Duplicate group_ID '" + id + "'";
                result.issues.push_back(std::move(issue));
            }
        }
    }

    
    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");

        struct SpecInfo { int line; };
        std::unordered_map<std::string, std::vector<SpecInfo>> specIdMap;

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid = SafeAttr(specElem, "spec_ID");
            if (sid.empty()) continue;
            specIdMap[sid].push_back({ GetElementLineNumber(specElem) });
        }

        for (const auto& [id, occurrences] : specIdMap) {
            if (occurrences.size() <= 1) continue;
            for (const auto& occ : occurrences) {
                ValidationIssue issue;  
                issue.type        = ValidationIssueType::DuplicateSpecId;
                issue.groupId     = gid;
                issue.specId      = id;
                issue.elementTag  = "spec";
                issue.lineNumber  = occ.line;
                issue.description = "Duplicate spec_ID '" + id + "'";
                result.issues.push_back(std::move(issue));
            }
        }
    }

    
    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid   = SafeAttr(specElem, "spec_ID");

            struct ValInfo { int line; };
            std::unordered_map<std::string, std::vector<ValInfo>> valIdMap;

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string vid = SafeAttr(valElem, "val_id");
                if (vid.empty()) continue;
                valIdMap[vid].push_back({ GetElementLineNumber(valElem) });
            }

            for (const auto& [id, occurrences] : valIdMap) {
                if (occurrences.size() <= 1) continue;
                for (const auto& occ : occurrences) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::DuplicateValId;
                    issue.groupId     = gid;
                    issue.specId      = sid;
                    issue.valId       = id;
                    issue.elementTag  = "val";
                    issue.lineNumber  = occ.line;
                    issue.description = "Duplicate val_id '" + id + "'";
                    result.issues.push_back(std::move(issue));
                }
            }
        }
    }
}


void XmlValidationEngine::CheckMissingRequiredAttributes(
    tinyxml2::XMLDocument& doc,
    FileValidationResult& result)
{
    auto* dataElem = doc.FirstChildElement("data");
    if (!dataElem) return;

    
    struct RequiredAttr { const char* tag; const char* attr; };
    

    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");

        
        const char* groupRequiredAttrs[] = { "group_ID", "group_name" };
        for (const char* attr : groupRequiredAttrs) {
            if (!groupElem->Attribute(attr)) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::MissingRequiredAttribute;
                issue.groupId     = gid;
                issue.elementTag  = "group";
                issue.lineNumber  = GetElementLineNumber(groupElem);
                issue.description = std::string("Missing required attribute '")
                                  + attr + "' in <group>";
                result.issues.push_back(std::move(issue));
            }
        }

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid   = SafeAttr(specElem, "spec_ID");

            
            const char* specRequiredAttrs[] = { "spec_ID", "spec_name" };
            for (const char* attr : specRequiredAttrs) {
                if (!specElem->Attribute(attr)) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::MissingRequiredAttribute;
                    issue.groupId     = gid;
                    issue.specId      = sid;
                    issue.elementTag  = "spec";
                    issue.lineNumber  = GetElementLineNumber(specElem);
                    issue.description = std::string("Missing required attribute '")
                                      + attr + "' in <spec>";
                    result.issues.push_back(std::move(issue));
                }
            }

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string vid   = SafeAttr(valElem, "val_id");

                
                const char* valRequiredAttrs[] = { "val_id", "val_name" };
                for (const char* attr : valRequiredAttrs) {
                    if (!valElem->Attribute(attr)) {
                        ValidationIssue issue;
                        issue.type        = ValidationIssueType::MissingRequiredAttribute;
                        issue.groupId     = gid;
                        issue.specId      = sid;
                        issue.valId       = vid;
                        issue.elementTag  = "val";
                        issue.lineNumber  = GetElementLineNumber(valElem);
                        issue.description = std::string("Missing required attribute '")
                                          + attr + "' in <val>";
                        result.issues.push_back(std::move(issue));
                    }
                }
            }
        }
    }
}


void XmlValidationEngine::CheckHierarchyMismatches(
    tinyxml2::XMLDocument& doc,
    FileValidationResult& result)
{
    auto* dataElem = doc.FirstChildElement("data");
    if (!dataElem) return;

    
    for (auto* child = dataElem->FirstChildElement();
         child; child = child->NextSiblingElement())
    {
        std::string tag = child->Name() ? child->Name() : "";

        if (tag == "spec") {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::HierarchyMismatch;
            issue.specId      = SafeAttr(child, "spec_ID");
            issue.elementTag  = "spec";
            issue.lineNumber  = GetElementLineNumber(child);
            issue.description = "Hierarchy mismatch: <spec> found outside expected parent <group>";
            result.issues.push_back(std::move(issue));
        }
        else if (tag == "val") {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::HierarchyMismatch;
            issue.valId       = SafeAttr(child, "val_id");
            issue.elementTag  = "val";
            issue.lineNumber  = GetElementLineNumber(child);
            issue.description = "Hierarchy mismatch: <val> found outside expected parent <spec>";
            result.issues.push_back(std::move(issue));
        }
    }

    
    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");

        for (auto* child = groupElem->FirstChildElement();
             child; child = child->NextSiblingElement())
        {
            std::string tag = child->Name() ? child->Name() : "";

            if (tag == "val") {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::HierarchyMismatch;
                issue.groupId     = gid;
                issue.valId       = SafeAttr(child, "val_id");
                issue.elementTag  = "val";
                issue.lineNumber  = GetElementLineNumber(child);
                issue.description = "Hierarchy mismatch: <val> found outside expected parent <spec>";
                result.issues.push_back(std::move(issue));
            }
        }
    }
}


void XmlValidationEngine::CheckUnrecognizedTags(
    tinyxml2::XMLDocument& doc,
    FileValidationResult& result)
{
    auto* root = doc.RootElement();
    if (!root) return;

    
    ScanForUnrecognizedTags(root, result);
}

void XmlValidationEngine::ScanForUnrecognizedTags(
    const tinyxml2::XMLElement* elem,
    FileValidationResult& result)
{
    
    static const std::unordered_set<std::string> allowedTags = {
        "data", "group", "spec", "val"
    };

    std::string tag = elem->Name() ? elem->Name() : "";

    if (!tag.empty() && allowedTags.find(tag) == allowedTags.end()) {
        ValidationIssue issue;
        issue.type        = ValidationIssueType::UnrecognizedTag;
        issue.elementTag  = tag;
        issue.lineNumber  = GetElementLineNumber(elem);
        issue.description = "Unrecognized element tag '<" + tag + ">'";

        
        auto* parent = elem->Parent() ? elem->Parent()->ToElement() : nullptr;
        if (parent) {
            std::string parentTag = parent->Name() ? parent->Name() : "";
            if (parentTag == "spec") {
                issue.specId   = SafeAttr(parent, "spec_ID");
                
                auto* gp = parent->Parent() ? parent->Parent()->ToElement() : nullptr;
                if (gp && std::string(gp->Name() ? gp->Name() : "") == "group") {
                    issue.groupId   = SafeAttr(gp, "group_ID");
                }
            }
            else if (parentTag == "group") {
                issue.groupId   = SafeAttr(parent, "group_ID");
            }
        }

        result.issues.push_back(std::move(issue));
    }

    
    for (auto* child = elem->FirstChildElement();
         child; child = child->NextSiblingElement())
    {
        ScanForUnrecognizedTags(child, result);
    }
}


void XmlValidationEngine::CheckInvalidDataFormats(
    tinyxml2::XMLDocument& doc,
    FileValidationResult& result)
{
    auto* dataElem = doc.FirstChildElement("data");
    if (!dataElem) return;

    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");

        if (groupElem->Attribute("group_ID") && gid.empty()) {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::InvalidDataFormat;
            issue.elementTag  = "group";
            issue.lineNumber  = GetElementLineNumber(groupElem);
            issue.description = "group_ID is empty";
            result.issues.push_back(std::move(issue));
        }
        else if (!gid.empty() && !IsNumericString(gid)) {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::InvalidDataFormat;
            issue.groupId     = gid;
            issue.elementTag  = "group";
            issue.lineNumber  = GetElementLineNumber(groupElem);
            issue.description = "Invalid data format: Attribute 'group_ID' expects numeric value, found '"
                              + gid + "'";
            result.issues.push_back(std::move(issue));
        }

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid   = SafeAttr(specElem, "spec_ID");

            if (specElem->Attribute("spec_ID") && sid.empty()) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::InvalidDataFormat;
                issue.groupId     = gid;
                issue.elementTag  = "spec";
                issue.lineNumber  = GetElementLineNumber(specElem);
                issue.description = "spec_ID is empty";
                result.issues.push_back(std::move(issue));
            }
            else if (!sid.empty() && !IsNumericString(sid)) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::InvalidDataFormat;
                issue.groupId     = gid;
                issue.specId      = sid;
                issue.elementTag  = "spec";
                issue.lineNumber  = GetElementLineNumber(specElem);
                issue.description = "Invalid data format: Attribute 'spec_ID' expects numeric value, found '"
                                  + sid + "'";
                result.issues.push_back(std::move(issue));
            }

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string vid   = SafeAttr(valElem, "val_id");

                if (valElem->Attribute("val_id") && vid.empty()) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::InvalidDataFormat;
                    issue.groupId     = gid;
                    issue.specId      = sid;
                    issue.elementTag  = "val";
                    issue.lineNumber  = GetElementLineNumber(valElem);
                    issue.description = "val_id is empty";
                    result.issues.push_back(std::move(issue));
                }
                else if (!vid.empty() && !IsNumericString(vid)) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::InvalidDataFormat;
                    issue.groupId     = gid;
                    issue.specId      = sid;
                    issue.valId       = vid;
                    issue.elementTag  = "val";
                    issue.lineNumber  = GetElementLineNumber(valElem);
                    issue.description = "Invalid data format: Attribute 'val_id' expects numeric value, found '"
                                      + vid + "'";
                    result.issues.push_back(std::move(issue));
                }
            }
        }
    }
}

} 
