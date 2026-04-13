#pragma once

#include "DiffTypes.h"
#include <filesystem>
#include <string>

namespace tinyxml2 { class XMLDocument; class XMLElement; class XMLNode; }

namespace ModelCompare {

class XmlApplyEngine {
public:
    struct ApplyResult {
        bool success = false;
        std::string errorMessage;
    };

    static ApplyResult AddMissing(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile,
        const KeyDiffEntry& entry
    );

    static ApplyResult RemoveExtra(
        const std::filesystem::path& rightFile,
        const KeyDiffEntry& entry
    );

    static ApplyResult ApplyAllDiffs(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile,
        const std::vector<KeyDiffEntry>& missingInRight,
        const std::vector<KeyDiffEntry>& extraInRight
    );

    static bool SaveDocument(
        const std::filesystem::path& filePath,
        tinyxml2::XMLDocument& doc
    );
};

} 
