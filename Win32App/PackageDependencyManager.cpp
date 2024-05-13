// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "PackageDependencyManager.h"

#include "PackageGraph.h"

static std::recursive_mutex g_lock;
std::vector<MddCore::PackageDependency> g_packageDependencies;

void MddCore::PackageDependencyManager::CreatePackageDependency(
    PSID user,
    _In_ PCWSTR packageFamilyName,
    PACKAGE_VERSION minVersion,
    MddPackageDependencyProcessorArchitectures packageDependencyProcessorArchitectures,
    MddPackageDependencyLifetimeKind lifetimeKind,
    PCWSTR lifetimeArtifact,
    MddCreatePackageDependencyOptions options,
    _Outptr_result_maybenull_ PWSTR* packageDependencyId)
{
    *packageDependencyId = nullptr;

    MddCore::PackageDependency packageDependency(user, packageFamilyName, minVersion, packageDependencyProcessorArchitectures, lifetimeKind, lifetimeArtifact, options);
    Verify(packageDependency);
    packageDependency.GenerateId();

    auto lock{ std::unique_lock<std::recursive_mutex>(g_lock) };

    g_packageDependencies.push_back(packageDependency);

    auto id{ wil::make_process_heap_string(packageDependency.Id().c_str()) };
    *packageDependencyId = id.release();
}

void MddCore::PackageDependencyManager::DeletePackageDependency(
    _In_ PCWSTR packageDependencyId)
{
    if (!packageDependencyId)
    {
        return;
    }

    auto lock{ std::unique_lock<std::recursive_mutex>(g_lock) };

    for (size_t index=0; index < g_packageDependencies.size(); ++index)
    {
        const auto& packageDependency{ g_packageDependencies[index] };
        if (CompareStringOrdinal(packageDependency.Id().c_str(), -1, packageDependencyId, -1, TRUE) == CSTR_EQUAL)
        {
            g_packageDependencies.erase(g_packageDependencies.begin() + index);
            break;
        }
    }
}

const MddCore::PackageDependency* MddCore::PackageDependencyManager::GetPackageDependency(
    _In_ PCWSTR packageDependencyId)
{
    // Check the in-memory list
    for (size_t index=0; index < g_packageDependencies.size(); ++index)
    {
        const auto& packageDependency{ g_packageDependencies[index] };
        if (CompareStringOrdinal(packageDependency.Id().c_str(), -1, packageDependencyId, -1, TRUE) == CSTR_EQUAL)
        {
            // Gotcha!
            return &packageDependency;
        }
    }

    // Not found
    return nullptr;
}

void MddCore::PackageDependencyManager::Verify(
    const MddCore::PackageDependency& packageDependency)
{
    // Verify the package dependency's dependency resolution (if necessary)
    if (WI_IsFlagClear(packageDependency.Options(), MddCreatePackageDependencyOptions::DoNotVerifyDependencyResolution))
    {
        MddAddPackageDependencyOptions options{};
        wil::unique_process_heap_string packageFullName;
        THROW_IF_FAILED(MddCore::PackageGraph::ResolvePackageDependency(packageDependency, options, packageFullName));
    }
}
