// =============================================================================
// XmlApplyEngine.cpp
// Surgically modifies Right model XML to match Left model's ID structure.
// =============================================================================
#include "pch.h"
#include "XmlApplyEngine.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Helper: Load XML document from a wide-path file
// ---------------------------------------------------------------------------
static bool LoadDoc(const std::filesystem::path& path, tinyxml2::XMLDocument& doc) {
    std::ifstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) return false;
    std::ostringstream ss;
    ss << fs.rdbuf();
    std::string content = ss.str();
    fs.close();
    return doc.Parse(content.c_str(), content.size()) == tinyxml2::XML_SUCCESS;
}

// ---------------------------------------------------------------------------
// SaveDocument: Write XMLDocument to file via std::ofstream (Unicode safe)
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Helper: Safe attribute read
// ---------------------------------------------------------------------------
static const char* SafeAttr(const tinyxml2::XMLElement* e, const char* n) {
    const char* v = e->Attribute(n);
    return v ? v : "";
}

// ---------------------------------------------------------------------------
// Helper: Find group element by group_ID
// ---------------------------------------------------------------------------
static tinyxml2::XMLElement* FindGroup(tinyxml2::XMLElement* data, const std::string& gid) {
    for (auto* g = data->FirstChildElement("group"); g; g = g->NextSiblingElement("group")) {
        if (gid == SafeAttr(g, "group_ID")) return g;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Helper: Find spec element by spec_ID within a group
// ---------------------------------------------------------------------------
static tinyxml2::XMLElement* FindSpec(tinyxml2::XMLElement* group, const std::string& sid) {
    for (auto* s = group->FirstChildElement("spec"); s; s = s->NextSiblingElement("spec")) {
        if (sid == SafeAttr(s, "spec_ID")) return s;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Helper: Find val element by val_id within a spec
// ---------------------------------------------------------------------------
static tinyxml2::XMLElement* FindVal(tinyxml2::XMLElement* spec, const std::string& vid) {
    for (auto* v = spec->FirstChildElement("val"); v; v = v->NextSiblingElement("val")) {
        if (vid == SafeAttr(v, "val_id")) return v;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// AddMissingVal: Copy a val node from Left file into Right file
// ---------------------------------------------------------------------------
XmlApplyEngine::ApplyResult XmlApplyEngine::AddMissingVal(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile,
    const KeyDiffEntry& entry)
{
    ApplyResult result;

    tinyxml2::XMLDocument leftDoc, rightDoc;
    if (!LoadDoc(leftFile, leftDoc)) {
        result.errorMessage = "Failed to load left file";
        return result;
    }
    if (!LoadDoc(rightFile, rightDoc)) {
        result.errorMessage = "Failed to load right file";
        return result;
    }

    auto* leftData  = leftDoc.FirstChildElement("data");
    auto* rightData = rightDoc.FirstChildElement("data");
    if (!leftData || !rightData) {
        result.errorMessage = "Missing <data> root element";
        return result;
    }

    // Find the val in the left file
    auto* leftGroup = FindGroup(leftData, entry.groupId);
    if (!leftGroup) { result.errorMessage = "Group not found in left"; return result; }
    auto* leftSpec = FindSpec(leftGroup, entry.specId);
    if (!leftSpec) { result.errorMessage = "Spec not found in left"; return result; }
    auto* leftVal = FindVal(leftSpec, entry.valId);
    if (!leftVal) { result.errorMessage = "Val not found in left"; return result; }

    // Find or create group in right
    auto* rightGroup = FindGroup(rightData, entry.groupId);
    if (!rightGroup) {
        // Clone entire group from left
        auto* cloned = leftGroup->DeepClone(&rightDoc);
        rightData->InsertEndChild(cloned);
    } else {
        // Find or create spec in right
        auto* rightSpec = FindSpec(rightGroup, entry.specId);
        if (!rightSpec) {
            auto* cloned = leftSpec->DeepClone(&rightDoc);
            rightGroup->InsertEndChild(cloned);
        } else {
            // Clone just the val
            auto* cloned = leftVal->DeepClone(&rightDoc);
            rightSpec->InsertEndChild(cloned);
        }
    }

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// RemoveExtraVal: Remove a val node from Right file
// ---------------------------------------------------------------------------
XmlApplyEngine::ApplyResult XmlApplyEngine::RemoveExtraVal(
    const std::filesystem::path& rightFile,
    const KeyDiffEntry& entry)
{
    ApplyResult result;

    tinyxml2::XMLDocument rightDoc;
    if (!LoadDoc(rightFile, rightDoc)) {
        result.errorMessage = "Failed to load right file";
        return result;
    }

    auto* rightData = rightDoc.FirstChildElement("data");
    if (!rightData) { result.errorMessage = "Missing <data> root"; return result; }

    auto* group = FindGroup(rightData, entry.groupId);
    if (!group) { result.errorMessage = "Group not found"; return result; }
    auto* spec = FindSpec(group, entry.specId);
    if (!spec) { result.errorMessage = "Spec not found"; return result; }
    auto* val = FindVal(spec, entry.valId);
    if (!val) { result.errorMessage = "Val not found"; return result; }

    spec->DeleteChild(val);

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// ApplyAllDiffs: Apply all missing + extra diffs for a file
// ---------------------------------------------------------------------------
XmlApplyEngine::ApplyResult XmlApplyEngine::ApplyAllDiffs(
    const std::filesystem::path& leftFile,
    const std::filesystem::path& rightFile,
    const std::vector<KeyDiffEntry>& missingInRight,
    const std::vector<KeyDiffEntry>& extraInRight)
{
    ApplyResult result;

    tinyxml2::XMLDocument leftDoc, rightDoc;
    if (!LoadDoc(leftFile, leftDoc)) {
        result.errorMessage = "Failed to load left file";
        return result;
    }
    if (!LoadDoc(rightFile, rightDoc)) {
        result.errorMessage = "Failed to load right file";
        return result;
    }

    auto* leftData  = leftDoc.FirstChildElement("data");
    auto* rightData = rightDoc.FirstChildElement("data");
    if (!leftData || !rightData) {
        result.errorMessage = "Missing <data> root element";
        return result;
    }

    // Remove extra vals first
    for (const auto& entry : extraInRight) {
        auto* group = FindGroup(rightData, entry.groupId);
        if (!group) continue;
        auto* spec = FindSpec(group, entry.specId);
        if (!spec) continue;
        auto* val = FindVal(spec, entry.valId);
        if (val) spec->DeleteChild(val);
    }

    // Add missing vals
    for (const auto& entry : missingInRight) {
        auto* leftGroup = FindGroup(leftData, entry.groupId);
        if (!leftGroup) continue;
        auto* leftSpec = FindSpec(leftGroup, entry.specId);
        if (!leftSpec) continue;
        auto* leftVal = FindVal(leftSpec, entry.valId);
        if (!leftVal) continue;

        auto* rightGroup = FindGroup(rightData, entry.groupId);
        if (!rightGroup) {
            rightData->InsertEndChild(leftGroup->DeepClone(&rightDoc));
            continue;
        }
        auto* rightSpec = FindSpec(rightGroup, entry.specId);
        if (!rightSpec) {
            rightGroup->InsertEndChild(leftSpec->DeepClone(&rightDoc));
            continue;
        }
        rightSpec->InsertEndChild(leftVal->DeepClone(&rightDoc));
    }

    if (!SaveDocument(rightFile, rightDoc)) {
        result.errorMessage = "Failed to save right file";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace ModelCompare
