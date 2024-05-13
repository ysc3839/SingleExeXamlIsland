// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "pch.h"

#include "PackageGraphNode.h"
#include "MddDetourPackageGraph.h"

volatile MDD_PACKAGEDEPENDENCY_CONTEXT MddCore::PackageGraphNode::s_lastContext{};

MddCore::PackageGraphNode::PackageGraphNode(
    _In_ PCWSTR packageFullName,
    INT32 rank,
    _In_ PCWSTR packageDependencyId) :
    m_rank(rank),
    m_id(packageDependencyId)
{
    THROW_IF_WIN32_ERROR(OpenPackageInfoByFullName(packageFullName, 0, &m_packageInfoReference));
    m_packageInfo = std::move(MddCore::PackageInfo::FromPackageInfoReference(m_packageInfoReference.get()));
}

UINT32 MddCore::PackageGraphNode::CountMatchingPackages(
    const UINT32 flags,
    const PackageInfoType packageInfoType) const
{
    const auto packageInfoFlags{ flags & ~(PACKAGE_FILTER_STATIC | PACKAGE_FILTER_DYNAMIC) };
    return CountMatchingPackages(packageInfoFlags, ToPackagePathType(packageInfoType));
}

UINT32 MddCore::PackageGraphNode::CountMatchingPackages(
    const UINT32 flags,
    const PackagePathType packagePathType) const
{
    UINT32 bufferLength{};
    UINT32 count{};
    const LONG rc{ MddGetPackageInfo1Or2(m_packageInfoReference.get(), flags, packagePathType, &bufferLength, nullptr, &count) };
    if ((rc != ERROR_SUCCESS) && (rc != ERROR_INSUFFICIENT_BUFFER))
    {
        THROW_WIN32(rc);
    }
    return count;
}

UINT32 MddCore::PackageGraphNode::GetMatchingPackages(
    const UINT32 flags,
    const PackageInfoType packageInfoType,
    wil::unique_cotaskmem_ptr<BYTE[]>& buffer) const
{
    const auto packageInfoFlags{ flags & ~(PACKAGE_FILTER_STATIC | PACKAGE_FILTER_DYNAMIC) };
    return GetMatchingPackages(packageInfoFlags, ToPackagePathType(packageInfoType), buffer);
}

UINT32 MddCore::PackageGraphNode::GetMatchingPackages(
    const UINT32 flags,
    const PackagePathType packagePathType,
    wil::unique_cotaskmem_ptr<BYTE[]>& buffer) const
{
    UINT32 bufferLength{};
    const LONG rc{ MddGetPackageInfo1Or2(m_packageInfoReference.get(), flags, packagePathType, &bufferLength, nullptr, nullptr) };
    if (rc == ERROR_SUCCESS)
    {
        // Success with no buffer can only mean count==0
        return 0;
    }
    else if (rc != ERROR_INSUFFICIENT_BUFFER)
    {
        THROW_WIN32(rc);
    }
    buffer = std::move(wil::make_unique_cotaskmem<BYTE[]>(bufferLength));
    UINT32 count{};
    THROW_IF_WIN32_ERROR(MddGetPackageInfo1Or2(m_packageInfoReference.get(), flags, packagePathType, &bufferLength, buffer.get(), &count));
    return count;
}

void MddCore::PackageGraphNode::GenerateContext()
{
#if defined(_WIN64)
    m_context = reinterpret_cast<MDD_PACKAGEDEPENDENCY_CONTEXT>(InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&s_lastContext)));
#else
    m_context =  reinterpret_cast<MDD_PACKAGEDEPENDENCY_CONTEXT>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&s_lastContext)));
#endif
}

void MddCore::PackageGraphNode::AddDllDirectories()
{
    for (size_t index=0; index < m_packageInfo.Count(); ++index)
    {
        const auto& package{ m_packageInfo.Package(index) };

        wil::unique_dll_directory_cookie cookie(AddDllDirectory(package.path));
        THROW_LAST_ERROR_IF_NULL(cookie);

        m_addDllDirectoryCookies.push_back(std::move(cookie));
    }
}

void MddCore::PackageGraphNode::RemoveDllDirectories()
{
    m_addDllDirectoryCookies.clear();
}

std::shared_ptr<MddCore::WinRTPackage> MddCore::PackageGraphNode::CreateWinRTPackage() const
{
    const auto& package{ m_packageInfo.Package(0) };

    return std::make_shared<MddCore::WinRTPackage>(m_context, package.path);
}
