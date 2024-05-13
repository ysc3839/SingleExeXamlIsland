﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include <appmodel.h>

#include "MsixDynamicDependency.h"

#include <wil/resource.h>
#include "wil_msixdynamicdependency.h"
#include "appmodel_msixdynamicdependency.h"

#include "PackageInfo.h"
#include "WinRTPackage.h"

namespace MddCore
{
class PackageGraphNode
{
public:
    PackageGraphNode() = default;

    ~PackageGraphNode()
    {
        Reset();
    }

    PackageGraphNode(const PackageGraphNode& other) = delete;

    PackageGraphNode(PackageGraphNode&& other) :
        m_packageInfoReference(std::move(other.m_packageInfoReference)),
        m_packageInfo(std::move(other.m_packageInfo)),
        m_rank(std::move(other.m_rank)),
        m_context(std::move(other.m_context)),
        m_addDllDirectoryCookies(std::move(other.m_addDllDirectoryCookies)),
        m_id(std::move(other.m_id))
    {
    }

    PackageGraphNode(
        _In_ PCWSTR packageFullName,
        INT32 rank,
        _In_ PCWSTR packageDependencyId);

    PackageGraphNode& operator=(PackageGraphNode& other) = delete;

    PackageGraphNode& operator=(PackageGraphNode&& other)
    {
        if (this != &other)
        {
            Reset();
            m_packageInfoReference = std::move(other.m_packageInfoReference);
            m_packageInfo = std::move(other.m_packageInfo);
            m_rank = std::move(other.m_rank);
            m_context = std::move(other.m_context);
            m_addDllDirectoryCookies = std::move(other.m_addDllDirectoryCookies);
            m_id = std::move(other.m_id);
        }
        return *this;
    }

    void Reset()
    {
        m_addDllDirectoryCookies.clear();
        m_context = nullptr;
        m_rank = MDD_PACKAGE_DEPENDENCY_RANK_DEFAULT;
        m_packageInfo.Reset();
        m_packageInfoReference.reset();
        m_id.clear();
    }

    bool IsDynamic() const
    {
        return !IsStatic();
    }

    bool IsStatic() const
    {
        return !m_packageInfoReference.get();
    }

    UINT32 CountMatchingPackages(
        const UINT32 flags,
        const PackageInfoType packageInfoType) const;

    UINT32 CountMatchingPackages(
        const UINT32 flags,
        const PackagePathType packagePathType) const;

    UINT32 GetMatchingPackages(
        const UINT32 flags,
        const PackageInfoType packageInfoType,
        wil::unique_cotaskmem_ptr<BYTE[]>& buffer) const;

    UINT32 GetMatchingPackages(
        const UINT32 flags,
        const PackagePathType packagePathType,
        wil::unique_cotaskmem_ptr<BYTE[]>& buffer) const;

    const MddCore::PackageInfo& PackageInfo() const
    {
        return m_packageInfo;
    }

    int32_t Rank() const
    {
        return m_rank;
    }

    MDD_PACKAGEDEPENDENCY_CONTEXT Context()
    {
        return m_context;
    }

    const std::wstring Id() const
    {
        return m_id;
    }

    void GenerateContext();

    void AddDllDirectories();

    void RemoveDllDirectories();

public:
    std::shared_ptr<MddCore::WinRTPackage> CreateWinRTPackage() const;

private:
    static volatile MDD_PACKAGEDEPENDENCY_CONTEXT s_lastContext;

private:
    mutable wil::unique_package_info_reference m_packageInfoReference;
    MddCore::PackageInfo m_packageInfo;
    INT32 m_rank{ MDD_PACKAGE_DEPENDENCY_RANK_DEFAULT };
    MDD_PACKAGEDEPENDENCY_CONTEXT m_context{};
    std::vector<wil::unique_dll_directory_cookie> m_addDllDirectoryCookies;
    std::wstring m_id;
};
}
