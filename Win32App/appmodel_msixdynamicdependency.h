// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include <appmodel.h>

#define PACKAGE_PROPERTY_STATIC             0x00080000
#define PACKAGE_FILTER_STATIC               PACKAGE_PROPERTY_STATIC
#define PACKAGE_PROPERTY_DYNAMIC            0x00100000
#define PACKAGE_FILTER_DYNAMIC              PACKAGE_PROPERTY_DYNAMIC

typedef enum PackageInfoType
{
    PackageInfoType_PackageInfoInstallPath = 0,
    PackageInfoType_PackageInfoMutablePath = 1,
    PackageInfoType_PackageInfoEffectivePath = 2,
    PackageInfoType_PackageInfoMachineExternalPath = 3,
    PackageInfoType_PackageInfoUserExternalPath = 4,
    PackageInfoType_PackageInfoEffectiveExternalPath = 5,
    // Reserve 6-15
    PackageInfoType_PackageInfoGeneration = 16,
    PackageInfoType_PackageInfoAliases = 17,
} PackageInfoType;

inline PackagePathType ToPackagePathType(const PackageInfoType packageInfoType)
{
    return static_cast<PackagePathType>(packageInfoType);
}

extern "C" HRESULT WINAPI GetCurrentPackageInfo3(
    _In_ UINT32 flags,
    _In_ PackageInfoType packageInfoType,
    _Inout_ UINT32* bufferLength,
    _Out_writes_bytes_opt_(*bufferLength) void* buffer,
    _Out_opt_ UINT32* count);
