// =============================================================================
// StructuralIdComparator.h
// Comparison strategy that checks structural ID hierarchy only.
//
// Compares group_ID/spec_ID/val_id composite keys between two XML files.
// Does NOT compare values, names, or other data attributes.
//
// Business rule: "ID should be same for every model. If group_ID same on
// both side but value have diff, we don't count it as diff."
// =============================================================================
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
