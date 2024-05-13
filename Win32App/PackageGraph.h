// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#if !defined(PACKAGEGRAPH_H)
#define PACKAGEGRAPH_H

#include "MsixDynamicDependency.h"

#include "PackageId.h"
#include "PackageGraphNode.h"
#include "PackageDependency.h"

namespace MddCore
{
class PackageGraph
{
public:
    PackageGraph() = default;

    ~PackageGraph() = default;

    HRESULT Add(
        _In_ PCWSTR packageDependencyId,
        INT32 rank,
        MddAddPackageDependencyOptions options,
        MDD_PACKAGEDEPENDENCY_CONTEXT& packageDependencyContext,
        _Outptr_opt_result_maybenull_ PWSTR* packageFullName);

private:
    HRESULT Add(
        _In_ PCWSTR packageFullName,
        INT32 rank,
        MddAddPackageDependencyOptions options,
        _In_ PCWSTR packageDependencyId,
        MDD_PACKAGEDEPENDENCY_CONTEXT& context);

public:
    HRESULT GetResolvedPackageDependency(
        PCWSTR packageDependencyId,
        wil::unique_process_heap_string& packageFullName) noexcept;

    static HRESULT GetResolvedPackageDependency(
        const MddCore::PackageDependency& packageDependency,
        wil::unique_process_heap_string& packageFullName);

public:
    HRESULT GetResolvedPackageDependency2(
        PCWSTR packageDependencyId,
        wil::unique_process_heap_string& packageFullName) noexcept;

    static HRESULT GetResolvedPackageDependency2(
        const MddCore::PackageDependency& packageDependency,
        wil::unique_process_heap_string& packageFullName);

public:
    HRESULT ResolvePackageDependency(
        PCWSTR packageDependencyId,
        MddAddPackageDependencyOptions options,
        wil::unique_process_heap_string& packageFullName) noexcept;

    static HRESULT ResolvePackageDependency(
        const MddCore::PackageDependency& packageDependency,
        MddAddPackageDependencyOptions options,
        wil::unique_process_heap_string& packageFullName);

public:
    HRESULT Remove(
        MDD_PACKAGEDEPENDENCY_CONTEXT context);

public:
    HRESULT GetPackageDependencyForContext(
        _In_ MDD_PACKAGEDEPENDENCY_CONTEXT context,
        wil::unique_process_heap_string& packageDependencyId);

private:
    static bool IsPackageABetterFitPerArchitecture(
        const MddCore::PackageId& bestFit,
        const MddCore::PackageId& candidate);

    void AddToDllSearchOrder(PackageGraphNode& package);

    void RemoveFromDllSearchOrder(PackageGraphNode& package);

    inline static MddCore::Architecture GetCurrentArchitecture()
    {
#if defined(_M_ARM)
        return Architecture::Arm;
#elif defined(_M_ARM64)
        return Architecture::Arm64;
#elif defined(_M_IX86)
        return Architecture::X86;
#elif defined(_M_X64)
        return Architecture::X64;
#else
#   error "Unknown processor architecture"
#endif
    }

public:
    const std::vector<MddCore::PackageGraphNode>& PackageGraphNodes() const
    {
        return m_packageGraphNodes;
    }

private:
    std::vector<MddCore::PackageGraphNode> m_packageGraphNodes;
    std::wstring m_pathListLastAddedToPath;
};
}

#endif // PACKAGEGRAPH_H
