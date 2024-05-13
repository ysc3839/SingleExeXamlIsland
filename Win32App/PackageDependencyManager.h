// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "PackageDependency.h"

namespace MddCore
{
/// @note Unless otherwise stated, all public static methods are thread safe and acquire the global lock if necessary.
/// @note Unless otherwise stated, all private static methods assume the global lock managed by their caller and held for their duration.
class PackageDependencyManager
{
public:
    PackageDependencyManager() = delete;
    ~PackageDependencyManager() = delete;

public:
    static void CreatePackageDependency(
        PSID user,
        _In_ PCWSTR packageFamilyName,
        PACKAGE_VERSION minVersion,
        MddPackageDependencyProcessorArchitectures packageDependencyProcessorArchitectures,
        MddPackageDependencyLifetimeKind lifetimeKind,
        PCWSTR lifetimeArtifact,
        MddCreatePackageDependencyOptions options,
        _Outptr_result_maybenull_ PWSTR* packageDependencyId);

    static void DeletePackageDependency(
        _In_ PCWSTR packageDependencyId);

public:
    /// @warning Unlocked data access. Caller's responsible for thread safety.
    static const PackageDependency* GetPackageDependency(
        _In_ PCWSTR packageDependencyId);

private:
    static void Verify(
        const MddCore::PackageDependency& packageDependency);
};
}
