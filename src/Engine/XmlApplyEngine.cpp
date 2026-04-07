// =============================================================================
// XmlApplyEngine.cpp
// Surgically modifies Right model XML to match Left model's ID structure.
//
// Level-aware operations with sorted insertion:
//   - Group: Deep-clone/remove entire <group> subtree
//   - Spec:  Deep-clone/remove entire <spec> subtree within matching group
//   - Val:   Deep-clone/remove single <val> node within matching spec
//
// Insertion order:
//   New elements are placed at the correct ascending ID position among
//   siblings, comparing numeric IDs to maintain sorted order.
// =============================================================================
#include "pch.h"
#include "XmlApplyEngine.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>
#include <sstream>
#include <cstdlib>

namespace ModelCompare {

// ---------------------------------------------------------------------------
// Helper: Load XML document from a wide-path file
// ---------------------------------------------------------------------------
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
// Helper: Parse a numeric ID from a string for sorted comparison.
//         Falls back to 0 if not a valid integer.
// ---------------------------------------------------------------------------
static int ParseNumericId(const char* idStr) {
    if (!idStr || !*idStr) return 0;
    return std::atoi(idStr);
}

// ---------------------------------------------------------------------------
// Helper: Find group element by group_ID
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Helper: Find spec element by spec_ID within a group
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Helper: Find val element by val_id within a spec
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// InsertSorted: Insert `newNode` as a child of `parent`, maintaining
//               ascending numeric ID order among siblings of `childTag`.
//
// Strategy: Find the last sibling whose ID <= newId, insert after it.
//           If no such sibling exists, the new node goes first.
// ---------------------------------------------------------------------------
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
            insertAfter = sibling;  // keep scanning for the last one <= newId
        } else {
            break;  // sibling ID is greater, stop
        }
    }

    if (insertAfter) {
        parent->InsertAfterChild(insertAfter, newNode);
    } else {
        // New node has the smallest ID — insert at the very beginning.
        // InsertFirstChild puts it before all existing children.
        auto* firstChild = parent->FirstChild();
        if (firstChild) {
            // TinyXML2 has no InsertBeforeChild, so we detach all children,
            // insert new node first, then re-attach. But a simpler approach:
            // just use InsertFirstChild which inserts as the first child node.
            parent->InsertFirstChild(newNode);
        } else {
            parent->InsertEndChild(newNode);
        }
    }
}

// ---------------------------------------------------------------------------
// AddMissing: Copy a group, spec, or val from Left file into Right file.
//             Insertion position is sorted by ascending ID.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// RemoveExtra: Delete a group, spec, or val from Right file.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// ApplyAllDiffs: Apply all missing + extra diffs in a single load/save cycle.
//
// Order: removals first (to avoid referencing stale nodes), then additions
//        with sorted insertion.
// ---------------------------------------------------------------------------
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

    // --- Phase 1: Remove extras ---
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

    // --- Phase 2: Add missing (sorted insertion) ---
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

} // namespace ModelCompare
