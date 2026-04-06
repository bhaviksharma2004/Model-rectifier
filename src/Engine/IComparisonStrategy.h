// =============================================================================
// IComparisonStrategy.h
// Abstract interface for XML file comparison strategies (Strategy Pattern).
//
// Design:
//   This interface decouples the "what to compare" from the "how to find files".
//   CompareEngine handles directory walking and file matching, then delegates
//   the actual comparison to the active IComparisonStrategy.
//
// Extensibility:
//   To add a new comparison mode (e.g., value comparison, attribute diff):
//   1. Create a new class that inherits from IComparisonStrategy
//   2. Implement CompareFiles() with your custom logic
//   3. Pass it to CompareEngine via its constructor or SetStrategy()
//
// Current implementations:
//   - StructuralIdComparator: Compares group_ID/spec_ID/val_id hierarchy
// =============================================================================
#pragma once

#include "DiffTypes.h"
#include <filesystem>

namespace ModelCompare {

class IComparisonStrategy {
public:
    virtual ~IComparisonStrategy() = default;

    // Compare two XML files and return the diff result.
    // Both files are guaranteed to exist when this is called.
    virtual FileDiffResult CompareFiles(
        const std::filesystem::path& leftFile,
        const std::filesystem::path& rightFile
    ) = 0;

    // Human-readable description of what this strategy compares.
    // Used for UI display and logging.
    virtual std::string GetDescription() const = 0;
};

} // namespace ModelCompare
