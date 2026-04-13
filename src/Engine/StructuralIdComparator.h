#pragma once

#include "IComparisonStrategy.h"

namespace ModelCompare {

class StructuralIdComparator : public IComparisonStrategy {
public:
    FileDiffResult CompareFiles(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile
    ) override;

    std::string GetDescription() const override {
        return "Structural ID Comparison (group_ID / spec_ID / val_id)";
    }
};

}
