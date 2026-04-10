#include "pch.h"
#include "XmlApplyEngine.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>
#include <cstdlib>

namespace ModelCompare {

static bool LoadDoc(const std::filesystem::path& path,
                    tinyxml2::XMLDocument& doc,
                    std::string& errorMsg)
{
    std::ifstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        errorMsg = "Failed to open file: " + path.u8string();
        return false;
    }
    std::ostringstream ss;
    ss << fs.rdbuf();
    std::string content = ss.str();
    fs.close();

    if (doc.Parse(content.c_str(), content.size()) != tinyxml2::XML_SUCCESS) {
        errorMsg = "XML parse error: " + std::string(doc.ErrorStr());
        return false;
    }
    return true;
}

bool XmlApplyEngine::SaveDocument(
    const std::filesystem::path& filePath, tinyxml2::XMLDocument& doc)
{
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    std::ofstream ofs(filePath, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) return false;
    ofs.write(printer.CStr(), printer.CStrSize() - 1);
    ofs.close();
    return true;
}

static const char* SafeAttr(const tinyxml2::XMLElement* e, const char* n) {
    const char* v = e->Attribute(n);
    return v ? v : "";
}

static int ParseNumericId(const char* idStr) {
    if (!idStr || !*idStr) return 0;
    return std::atoi(idStr);
}

static tinyxml2::XMLElement* FindGroup(tinyxml2::XMLElement* data,
                                       const std::string& gid)
{
    for (auto* g = data->FirstChildElement("group"); g;
         g = g->NextSiblingElement("group"))
    {
        if (gid == SafeAttr(g, "group_ID")) return g;
    }
    return nullptr;
}

static tinyxml2::XMLElement* FindSpec(tinyxml2::XMLElement* group,
                                      const std::string& sid)
{
    for (auto* s = group->FirstChildElement("spec"); s;
         s = s->NextSiblingElement("spec"))
    {
        if (sid == SafeAttr(s, "spec_ID")) return s;
    }
    return nullptr;
}

static tinyxml2::XMLElement* FindVal(tinyxml2::XMLElement* spec,
                                     const std::string& vid)
{
    for (auto* v = spec->FirstChildElement("val"); v;
         v = v->NextSiblingElement("val"))
    {
        if (vid == SafeAttr(v, "val_id")) return v;
    }
    return nullptr;
}

static void InsertSorted(tinyxml2::XMLElement* parent,
                         tinyxml2::XMLNode* newNode,
                         const char* childTag,
                         const char* idAttr,
                         int newId)
{
    tinyxml2::XMLElement* insertAfter = nullptr;

    for (auto* sibling = parent->FirstChildElement(childTag); sibling;
         sibling = sibling->NextSiblingElement(childTag))
    {
        int siblingId = ParseNumericId(SafeAttr(sibling, idAttr));
        if (siblingId <= newId) {
            insertAfter = sibling;
        } else {
            break;
        }
    }

    if (insertAfter) {
        parent->InsertAfterChild(insertAfter, newNode);
    } else {
        auto* firstChild = parent->FirstChild();
        if (firstChild) {
            parent->InsertFirstChild(newNode);
        } else {
            parent->InsertEndChild(newNode);
        }
    }
}

XmlApplyEngine::ApplyResult XmlApplyEngine::AddMissing(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile,
    const KeyDiffEntry& entry)
{
    ApplyResult result;

    tinyxml2::XMLDocument leftDoc, rightDoc;
    if (!LoadDoc(leftFile, leftDoc, result.errorMessage)) return result;
    if (!LoadDoc(rightFile, rightDoc, result.errorMessage)) return result;

    auto* leftData  = leftDoc.FirstChildElement("data");
    auto* rightData = rightDoc.FirstChildElement("data");
    if (!leftData || !rightData) {
        result.errorMessage = "Missing <data> root element";
        return result;
    }

    switch (entry.level) {
    case DiffLevel::Group: {
        auto* srcGroup = FindGroup(leftData, entry.groupId);
        if (!srcGroup) {
            result.errorMessage = "Group " + entry.groupId + " not found in left";
            return result;
        }
        auto* cloned = srcGroup->DeepClone(&rightDoc);
        InsertSorted(rightData, cloned, "group", "group_ID",
                     ParseNumericId(entry.groupId.c_str()));
        break;
    }
    case DiffLevel::Spec: {
        auto* srcGroup = FindGroup(leftData, entry.groupId);
        if (!srcGroup) {
            result.errorMessage = "Group " + entry.groupId + " not found in left";
            return result;
        }
        auto* srcSpec = FindSpec(srcGroup, entry.specId);
        if (!srcSpec) {
            result.errorMessage = "Spec " + entry.specId + " not found in left";
            return result;
        }
        auto* rightGroup = FindGroup(rightData, entry.groupId);
        if (!rightGroup) {
            result.errorMessage = "Group " + entry.groupId + " not found in right";
            return result;
        }
        auto* cloned = srcSpec->DeepClone(&rightDoc);
        InsertSorted(rightGroup, cloned, "spec", "spec_ID",
                     ParseNumericId(entry.specId.c_str()));
        break;
    }
    case DiffLevel::Val: {
        auto* srcGroup = FindGroup(leftData, entry.groupId);
        if (!srcGroup) {
            result.errorMessage = "Group " + entry.groupId + " not found in left";
            return result;
        }
        auto* srcSpec = FindSpec(srcGroup, entry.specId);
        if (!srcSpec) {
            result.errorMessage = "Spec " + entry.specId + " not found in left";
            return result;
        }
        auto* srcVal = FindVal(srcSpec, entry.valId);
        if (!srcVal) {
            result.errorMessage = "Val " + entry.valId + " not found in left";
            return result;
        }
        auto* rightGroup = FindGroup(rightData, entry.groupId);
        if (!rightGroup) {
            result.errorMessage = "Group " + entry.groupId + " not found in right";
            return result;
        }
        auto* rightSpec = FindSpec(rightGroup, entry.specId);
        if (!rightSpec) {
            result.errorMessage = "Spec " + entry.specId + " not found in right";
            return result;
        }
        auto* cloned = srcVal->DeepClone(&rightDoc);
        InsertSorted(rightSpec, cloned, "val", "val_id",
                     ParseNumericId(entry.valId.c_str()));
        break;
    }
    }

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

XmlApplyEngine::ApplyResult XmlApplyEngine::RemoveExtra(
    const std::filesystem::path& rightFile,
    const KeyDiffEntry& entry)
{
    ApplyResult result;

    tinyxml2::XMLDocument rightDoc;
    if (!LoadDoc(rightFile, rightDoc, result.errorMessage)) return result;

    auto* rightData = rightDoc.FirstChildElement("data");
    if (!rightData) {
        result.errorMessage = "Missing <data> root element";
        return result;
    }

    switch (entry.level) {
    case DiffLevel::Group: {
        auto* group = FindGroup(rightData, entry.groupId);
        if (!group) {
            result.errorMessage = "Group " + entry.groupId + " not found";
            return result;
        }
        rightData->DeleteChild(group);
        break;
    }
    case DiffLevel::Spec: {
        auto* group = FindGroup(rightData, entry.groupId);
        if (!group) {
            result.errorMessage = "Group " + entry.groupId + " not found";
            return result;
        }
        auto* spec = FindSpec(group, entry.specId);
        if (!spec) {
            result.errorMessage = "Spec " + entry.specId + " not found";
            return result;
        }
        group->DeleteChild(spec);
        break;
    }
    case DiffLevel::Val: {
        auto* group = FindGroup(rightData, entry.groupId);
        if (!group) {
            result.errorMessage = "Group " + entry.groupId + " not found";
            return result;
        }
        auto* spec = FindSpec(group, entry.specId);
        if (!spec) {
            result.errorMessage = "Spec " + entry.specId + " not found";
            return result;
        }
        auto* val = FindVal(spec, entry.valId);
        if (!val) {
            result.errorMessage = "Val " + entry.valId + " not found";
            return result;
        }
        spec->DeleteChild(val);
        break;
    }
    }

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

XmlApplyEngine::ApplyResult XmlApplyEngine::ApplyAllDiffs(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile,
    const std::vector<KeyDiffEntry>& missingInRight,
    const std::vector<KeyDiffEntry>& extraInRight)
{
    ApplyResult result;

    tinyxml2::XMLDocument leftDoc, rightDoc;
    if (!LoadDoc(leftFile, leftDoc, result.errorMessage)) return result;
    if (!LoadDoc(rightFile, rightDoc, result.errorMessage)) return result;

    auto* leftData  = leftDoc.FirstChildElement("data");
    auto* rightData = rightDoc.FirstChildElement("data");
    if (!leftData || !rightData) {
        result.errorMessage = "Missing <data> root element";
        return result;
    }

    for (const auto& entry : extraInRight) {
        switch (entry.level) {
        case DiffLevel::Group: {
            auto* group = FindGroup(rightData, entry.groupId);
            if (group) rightData->DeleteChild(group);
            break;
        }
        case DiffLevel::Spec: {
            auto* group = FindGroup(rightData, entry.groupId);
            if (!group) continue;
            auto* spec = FindSpec(group, entry.specId);
            if (spec) group->DeleteChild(spec);
            break;
        }
        case DiffLevel::Val: {
            auto* group = FindGroup(rightData, entry.groupId);
            if (!group) continue;
            auto* spec = FindSpec(group, entry.specId);
            if (!spec) continue;
            auto* val = FindVal(spec, entry.valId);
            if (val) spec->DeleteChild(val);
            break;
        }
        }
    }

    for (const auto& entry : missingInRight) {
        switch (entry.level) {
        case DiffLevel::Group: {
            auto* srcGroup = FindGroup(leftData, entry.groupId);
            if (!srcGroup) continue;
            auto* cloned = srcGroup->DeepClone(&rightDoc);
            InsertSorted(rightData, cloned, "group", "group_ID",
                         ParseNumericId(entry.groupId.c_str()));
            break;
        }
        case DiffLevel::Spec: {
            auto* srcGroup = FindGroup(leftData, entry.groupId);
            if (!srcGroup) continue;
            auto* srcSpec = FindSpec(srcGroup, entry.specId);
            if (!srcSpec) continue;
            auto* rightGroup = FindGroup(rightData, entry.groupId);
            if (!rightGroup) continue;
            auto* cloned = srcSpec->DeepClone(&rightDoc);
            InsertSorted(rightGroup, cloned, "spec", "spec_ID",
                         ParseNumericId(entry.specId.c_str()));
            break;
        }
        case DiffLevel::Val: {
            auto* srcGroup = FindGroup(leftData, entry.groupId);
            if (!srcGroup) continue;
            auto* srcSpec = FindSpec(srcGroup, entry.specId);
            if (!srcSpec) continue;
            auto* srcVal = FindVal(srcSpec, entry.valId);
            if (!srcVal) continue;
            auto* rightGroup = FindGroup(rightData, entry.groupId);
            if (!rightGroup) continue;
            auto* rightSpec = FindSpec(rightGroup, entry.specId);
            if (!rightSpec) continue;
            auto* cloned = srcVal->DeepClone(&rightDoc);
            InsertSorted(rightSpec, cloned, "val", "val_id",
                         ParseNumericId(entry.valId.c_str()));
            break;
        }
        }
    }

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

}
