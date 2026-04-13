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





ValidationReport XmlValidationEngine::ValidateModels(
    const std::filesystem::path& leftRoot,
    const std::filesystem::path& rightRoot)
{
    ValidationReport report;
    report.leftReport  = ValidateSingleModel(leftRoot);
    report.rightReport = ValidateSingleModel(rightRoot);
    return report;
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
    {
        std::ifstream fs(filePath, std::ios::in | std::ios::binary);
        if (!fs.is_open()) {
            result.isCorrupt = true;
            result.corruptionDetail = "Failed to open file: " + filePath.u8string();
            return result;
        }

        std::string raw;
        while (std::getline(fs, raw)) {
            if (!raw.empty() && raw.back() == '\r') raw.pop_back();
            result.cachedLines.push_back(CString(CA2W(raw.c_str(), CP_UTF8)));
        }
        fs.close();

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
    {  
        CStringA utf8Content(result.cachedFullText);
        if (doc.Parse(utf8Content.GetString(), utf8Content.GetLength()) != tinyxml2::XML_SUCCESS) {
            result.isCorrupt = true;
            result.corruptionDetail = doc.ErrorStr();       
            return result;
        }
    }

    CheckDuplicateIds(doc, result);
    CheckMissingRequiredAttributes(doc, result);
    CheckOrphanedElements(doc, result);
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
        struct GroupInfo { std::string name; int line; };
        std::unordered_map<std::string, std::vector<GroupInfo>> groupIdMap;

        for (auto* groupElem = dataElem->FirstChildElement("group");
             groupElem; groupElem = groupElem->NextSiblingElement("group"))
        {
            std::string gid = SafeAttr(groupElem, "group_ID");
            if (gid.empty()) continue;
            std::string gname = SafeAttr(groupElem, "group_name");
            groupIdMap[gid].push_back({ gname, GetElementLineNumber(groupElem) });
        }

        for (const auto& [id, occurrences] : groupIdMap) {
            if (occurrences.size() <= 1) continue;
            for (const auto& occ : occurrences) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::DuplicateGroupId;
                issue.groupId     = id;
                issue.groupName   = occ.name;
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
        std::string gname = SafeAttr(groupElem, "group_name");

        struct SpecInfo { std::string name; int line; };
        std::unordered_map<std::string, std::vector<SpecInfo>> specIdMap;

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid = SafeAttr(specElem, "spec_ID");
            if (sid.empty()) continue;
            std::string sname = SafeAttr(specElem, "spec_name");
            specIdMap[sid].push_back({ sname, GetElementLineNumber(specElem) });
        }

        for (const auto& [id, occurrences] : specIdMap) {
            if (occurrences.size() <= 1) continue;
            for (const auto& occ : occurrences) {
                ValidationIssue issue;  
                issue.type        = ValidationIssueType::DuplicateSpecId;
                issue.groupId     = gid;
                issue.groupName   = gname;
                issue.specId      = id;
                issue.specName    = occ.name;
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
        std::string gname = SafeAttr(groupElem, "group_name");

        for (auto* specElem = groupElem->FirstChildElement("spec");
             specElem; specElem = specElem->NextSiblingElement("spec"))
        {
            std::string sid   = SafeAttr(specElem, "spec_ID");
            std::string sname = SafeAttr(specElem, "spec_name");

            struct ValInfo { std::string name; int line; };
            std::unordered_map<std::string, std::vector<ValInfo>> valIdMap;

            for (auto* valElem = specElem->FirstChildElement("val");
                 valElem; valElem = valElem->NextSiblingElement("val"))
            {
                std::string vid = SafeAttr(valElem, "val_id");
                if (vid.empty()) continue;
                std::string vname = SafeAttr(valElem, "val_name");
                valIdMap[vid].push_back({ vname, GetElementLineNumber(valElem) });
            }

            for (const auto& [id, occurrences] : valIdMap) {
                if (occurrences.size() <= 1) continue;
                for (const auto& occ : occurrences) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::DuplicateValId;
                    issue.groupId     = gid;
                    issue.groupName   = gname;
                    issue.specId      = sid;
                    issue.specName    = sname;
                    issue.valId       = id;
                    issue.valName     = occ.name;
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
        std::string gname = SafeAttr(groupElem, "group_name");

        
        const char* groupRequiredAttrs[] = { "group_ID", "group_name" };
        for (const char* attr : groupRequiredAttrs) {
            if (!groupElem->Attribute(attr)) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::MissingRequiredAttribute;
                issue.groupId     = gid;
                issue.groupName   = gname;
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
            std::string sname = SafeAttr(specElem, "spec_name");

            
            const char* specRequiredAttrs[] = { "spec_ID", "spec_name" };
            for (const char* attr : specRequiredAttrs) {
                if (!specElem->Attribute(attr)) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::MissingRequiredAttribute;
                    issue.groupId     = gid;
                    issue.groupName   = gname;
                    issue.specId      = sid;
                    issue.specName    = sname;
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
                std::string vname = SafeAttr(valElem, "val_name");

                
                const char* valRequiredAttrs[] = { "val_id", "val_name" };
                for (const char* attr : valRequiredAttrs) {
                    if (!valElem->Attribute(attr)) {
                        ValidationIssue issue;
                        issue.type        = ValidationIssueType::MissingRequiredAttribute;
                        issue.groupId     = gid;
                        issue.groupName   = gname;
                        issue.specId      = sid;
                        issue.specName    = sname;
                        issue.valId       = vid;
                        issue.valName     = vname;
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


void XmlValidationEngine::CheckOrphanedElements(
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
            issue.type        = ValidationIssueType::OrphanedElement;
            issue.specId      = SafeAttr(child, "spec_ID");
            issue.specName    = SafeAttr(child, "spec_name");
            issue.elementTag  = "spec";
            issue.lineNumber  = GetElementLineNumber(child);
            issue.description = "Orphaned element: <spec> found outside expected parent <group>";
            result.issues.push_back(std::move(issue));
        }
        else if (tag == "val") {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::OrphanedElement;
            issue.valId       = SafeAttr(child, "val_id");
            issue.valName     = SafeAttr(child, "val_name");
            issue.elementTag  = "val";
            issue.lineNumber  = GetElementLineNumber(child);
            issue.description = "Orphaned element: <val> found outside expected parent <spec>";
            result.issues.push_back(std::move(issue));
        }
    }

    
    for (auto* groupElem = dataElem->FirstChildElement("group");
         groupElem; groupElem = groupElem->NextSiblingElement("group"))
    {
        std::string gid   = SafeAttr(groupElem, "group_ID");
        std::string gname = SafeAttr(groupElem, "group_name");

        for (auto* child = groupElem->FirstChildElement();
             child; child = child->NextSiblingElement())
        {
            std::string tag = child->Name() ? child->Name() : "";

            if (tag == "val") {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::OrphanedElement;
                issue.groupId     = gid;
                issue.groupName   = gname;
                issue.valId       = SafeAttr(child, "val_id");
                issue.valName     = SafeAttr(child, "val_name");
                issue.elementTag  = "val";
                issue.lineNumber  = GetElementLineNumber(child);
                issue.description = "Orphaned element: <val> found outside expected parent <spec>";
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
                issue.specName = SafeAttr(parent, "spec_name");
                
                auto* gp = parent->Parent() ? parent->Parent()->ToElement() : nullptr;
                if (gp && std::string(gp->Name() ? gp->Name() : "") == "group") {
                    issue.groupId   = SafeAttr(gp, "group_ID");
                    issue.groupName = SafeAttr(gp, "group_name");
                }
            }
            else if (parentTag == "group") {
                issue.groupId   = SafeAttr(parent, "group_ID");
                issue.groupName = SafeAttr(parent, "group_name");
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
        std::string gname = SafeAttr(groupElem, "group_name");

        
        if (!gid.empty() && !IsNumericString(gid)) {
            ValidationIssue issue;
            issue.type        = ValidationIssueType::InvalidDataFormat;
            issue.groupId     = gid;
            issue.groupName   = gname;
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
            std::string sname = SafeAttr(specElem, "spec_name");

            
            if (!sid.empty() && !IsNumericString(sid)) {
                ValidationIssue issue;
                issue.type        = ValidationIssueType::InvalidDataFormat;
                issue.groupId     = gid;
                issue.groupName   = gname;
                issue.specId      = sid;
                issue.specName    = sname;
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
                std::string vname = SafeAttr(valElem, "val_name");

                
                if (!vid.empty() && !IsNumericString(vid)) {
                    ValidationIssue issue;
                    issue.type        = ValidationIssueType::InvalidDataFormat;
                    issue.groupId     = gid;
                    issue.groupName   = gname;
                    issue.specId      = sid;
                    issue.specName    = sname;
                    issue.valId       = vid;
                    issue.valName     = vname;
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
