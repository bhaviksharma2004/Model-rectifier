#pragma once

#include "DiffTypes.h"
#include <filesystem>

namespace ModelCompare {

class IComparisonStrategy {
public:
    virtual ~IComparisonStrategy() = default;

    virtual FileDiffResult CompareFiles(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile
    ) = 0;

    virtual std::string GetDescription() const = 0;
};

} 
