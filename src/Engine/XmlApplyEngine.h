// =============================================================================
// XmlApplyEngine.h
// Engine for surgically modifying Right model XML files to match Left structure.
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <string>

namespace tinyxml2 { class XMLDocument; }

namespace ModelCompare {

class XmlApplyEngine {
public:
    struct ApplyResult {
        bool success = false;
        std::string errorMessage;
    };

    // Add a missing val node from Left into Right XML
    static ApplyResult AddMissingVal(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile,
        const KeyDiffEntry& entry
    );

    // Remove an extra val node from Right XML
    static ApplyResult RemoveExtraVal(
        const std::filesystem::path& rightFile,
        const KeyDiffEntry& entry
    );

    // Apply all diffs for a file
    static ApplyResult ApplyAllDiffs(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile,
        const std::vector<KeyDiffEntry>& missingInRight,
        const std::vector<KeyDiffEntry>& extraInRight
    );

    // Save XMLDocument to a wide-path file (handles Unicode filenames)
    static bool SaveDocument(
        const std::filesystem::path& filePath,
        tinyxml2::XMLDocument& doc
    );
};

} // namespace ModelCompare
