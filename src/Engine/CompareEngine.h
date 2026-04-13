














#pragma once

#include "DiffTypes.h"
#include "IComparisonStrategy.h"

#include <filesystem>
#include <memory>
#include <functional>

namespace ModelCompare {

class CompareEngine {
public:
    using ProgressCallback = std::function<
        void(size_t current, size_t total, const std::wstring& currentFile)
    >;

    explicit CompareEngine(std::unique_ptr<IComparisonStrategy> strategy);

    ModelDiffReport CompareModels(
        const std::filesystem::path& leftModelRoot,
        const std::filesystem::path& rightModelRoot,
        ProgressCallback progressCb = nullptr
    );

    void SetStrategy(std::unique_ptr<IComparisonStrategy> strategy);

private:
    std::unique_ptr<IComparisonStrategy> m_strategy;

    static std::vector<std::filesystem::path>
    EnumerateXmlFiles(const std::filesystem::path& root);

    static std::filesystem::path
    MakeRelative(const std::filesystem::path& file,
                 const std::filesystem::path& root);
};

} 