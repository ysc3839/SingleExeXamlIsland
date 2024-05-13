// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MsixDynamicDependency.h"
#include "MddCore.Architecture.h"

namespace MddCore
{
class PackageDependency
{
public:
    PackageDependency() = default;

    PackageDependency(
        const PSID user,
        const std::wstring& packageFamilyName,
        const PACKAGE_VERSION minVersion,
        const MddPackageDependencyProcessorArchitectures packageDependencyProcessorArchitectures,
        const MddPackageDependencyLifetimeKind lifetimeKind,
        PCWSTR lifetimeArtifact,
        const MddCreatePackageDependencyOptions options) :
        m_packageFamilyName(packageFamilyName),
        m_minVersion(minVersion),
        m_packageDependencyProcessorArchitectures(packageDependencyProcessorArchitectures),
        m_lifetimeKind(lifetimeKind),
        m_lifetimeArtifact(lifetimeArtifact ? std::wstring(lifetimeArtifact) : std::wstring()),
        m_options(options)
    {
        User(user);
    }

    PackageDependency(
        const PSID user,
        const std::wstring& packageFamilyName,
        const PACKAGE_VERSION minVersion,
        const MddPackageDependencyProcessorArchitectures packageDependencyProcessorArchitectures,
        const MddPackageDependencyLifetimeKind lifetimeKind,
        const std::wstring& lifetimeArtifact,
        const MddCreatePackageDependencyOptions options) :
        m_packageFamilyName(packageFamilyName),
        m_minVersion(minVersion),
        m_packageDependencyProcessorArchitectures(packageDependencyProcessorArchitectures),
        m_lifetimeKind(lifetimeKind),
        m_lifetimeArtifact(lifetimeArtifact),
        m_options(options)
    {
        User(user);
    }

    ~PackageDependency() = default;

    PackageDependency(const PackageDependency& other) :
        m_packageFamilyName(other.m_packageFamilyName),
        m_minVersion(other.m_minVersion),
        m_packageDependencyProcessorArchitectures(other.m_packageDependencyProcessorArchitectures),
        m_lifetimeKind(other.m_lifetimeKind),
        m_lifetimeArtifact(other.m_lifetimeArtifact),
        m_options(other.m_options),
        m_packageDependencyId(other.m_packageDependencyId),
        m_packageFullName(other.m_packageFullName)
    {
        User(other.m_user.get());
    }

    PackageDependency(PackageDependency&& other) :
        m_user(std::move(other.m_user)),
        m_packageFamilyName(std::move(other.m_packageFamilyName)),
        m_minVersion(std::move(other.m_minVersion)),
        m_packageDependencyProcessorArchitectures(std::move(other.m_packageDependencyProcessorArchitectures)),
        m_lifetimeKind(std::move(other.m_lifetimeKind)),
        m_lifetimeArtifact(std::move(other.m_lifetimeArtifact)),
        m_options(std::move(other.m_options)),
        m_packageDependencyId(std::move(other.m_packageDependencyId)),
        m_packageFullName(std::move(other.m_packageFullName))
    {
    }

    PackageDependency& operator=(const PackageDependency& other)
    {
        if (this != &other)
        {
            Reset();
            User(other.m_user.get());
            m_packageFamilyName = other.m_packageFamilyName;
            m_minVersion = other.m_minVersion;
            m_packageDependencyProcessorArchitectures = other.m_packageDependencyProcessorArchitectures;
            m_lifetimeKind = other.m_lifetimeKind;
            m_lifetimeArtifact = other.m_lifetimeArtifact;
            m_options = other.m_options;
            m_packageDependencyId = other.m_packageDependencyId;
            m_packageFullName = other.m_packageFullName;
        }
        return *this;
    }

    PackageDependency& operator=(PackageDependency&& other)
    {
        if (this != &other)
        {
            Reset();
            m_user = std::move(other.m_user);
            m_packageFamilyName = std::move(other.m_packageFamilyName);
            m_minVersion = std::move(other.m_minVersion);
            m_packageDependencyProcessorArchitectures = std::move(other.m_packageDependencyProcessorArchitectures);
            m_lifetimeKind = std::move(other.m_lifetimeKind);
            m_lifetimeArtifact = std::move(other.m_lifetimeArtifact);
            m_options = std::move(other.m_options);
            m_packageDependencyId = std::move(other.m_packageDependencyId);
            m_packageFullName = std::move(other.m_packageFullName);
        }
        return *this;
    }

    void Reset()
    {
        m_user.reset();
        m_packageFamilyName.clear();
        m_minVersion = PACKAGE_VERSION{};
        m_packageDependencyProcessorArchitectures = MddPackageDependencyProcessorArchitectures::None;
        m_lifetimeKind = MddPackageDependencyLifetimeKind::Process;
        m_lifetimeArtifact.clear();
        m_options = MddCreatePackageDependencyOptions::None;
        m_packageDependencyId.clear();
        m_packageFullName.clear();
    }

    void GenerateId();

    void User(PSID user)
    {
        if (user)
        {
            const auto sidLength{ GetLengthSid(user) };
            wil::unique_any_psid sid{ static_cast<PSID>(::LocalAlloc(LMEM_FIXED, sidLength)) };
            THROW_IF_NULL_ALLOC(sid);
            THROW_IF_WIN32_BOOL_FALSE(CopySid(sidLength, sid.get(), user));
            m_user = std::move(sid);
        }
        else
        {
            m_user.reset();
        }
    }

    PSID User() const
    {
        return m_user.get();
    }

    PSID User()
    {
        return m_user.get();
    }

    const std::wstring& PackageFamilyName() const
    {
        return m_packageFamilyName;
    }

    void PackageFamilyName(const std::wstring& packageFamilyName)
    {
        m_packageFamilyName = packageFamilyName;
    }

    const PACKAGE_VERSION& MinVersion() const
    {
        return m_minVersion;
    }

    void MinVersion(const PACKAGE_VERSION& minVersion)
    {
        m_minVersion = minVersion;
    }

    MddPackageDependencyProcessorArchitectures Architectures() const
    {
        return m_packageDependencyProcessorArchitectures;
    }

    void Architectures(const MddPackageDependencyProcessorArchitectures packageDependencyProcessorArchitectures)
    {
        m_packageDependencyProcessorArchitectures = packageDependencyProcessorArchitectures;
    }

    MddPackageDependencyLifetimeKind LifetimeKind() const
    {
        return m_lifetimeKind;
    }

    void LifetimeKind(const MddPackageDependencyLifetimeKind lifetimeKind)
    {
        m_lifetimeKind = lifetimeKind;
    }

    const std::wstring& LifetimeArtifact() const
    {
        return m_lifetimeArtifact;
    }

    void LifetimeArtifact(const std::wstring& lifetimeArtifact)
    {
        m_lifetimeArtifact = lifetimeArtifact;
    }

    MddCreatePackageDependencyOptions Options() const
    {
        return m_options;
    }

    void Options(const MddCreatePackageDependencyOptions options)
    {
        m_options = options;
    }

    const std::wstring& Id() const
    {
        return m_packageDependencyId;
    }

    void Id(const std::wstring& id)
    {
        m_packageDependencyId = id;
    }

    const std::wstring& PackageFullName() const
    {
        return m_packageFullName;
    }

    void PackageFullName(const std::wstring& packageFullName)
    {
        m_packageFullName = packageFullName;
    }

    void ResolvedPackageFullName(const std::wstring& packageFullName) const
    {
        m_packageFullName = packageFullName;
    }

    std::vector<std::wstring> FindPackagesByFamily() const;

    bool IsArchitectureInArchitectures(const MddCore::Architecture architecture) const
    {
        const auto architectureAsArchitectures{ ToArchitectures(architecture) };
        return WI_IsAnyFlagSet(m_packageDependencyProcessorArchitectures, architectureAsArchitectures);
    }

    static MddPackageDependencyProcessorArchitectures ToArchitectures(const MddCore::Architecture architecture)
    {
        switch (architecture)
        {
        case MddCore::Architecture::X86: return MddPackageDependencyProcessorArchitectures::X86;
        case MddCore::Architecture::X64: return MddPackageDependencyProcessorArchitectures::X64;
        case MddCore::Architecture::Arm: return MddPackageDependencyProcessorArchitectures::Arm;
        case MddCore::Architecture::Arm64: return MddPackageDependencyProcessorArchitectures::Arm64;
        case MddCore::Architecture::X86OnArm64: return MddPackageDependencyProcessorArchitectures::X86OnArm64;
        case MddCore::Architecture::Neutral: return MddPackageDependencyProcessorArchitectures::Neutral;
        case MddCore::Architecture::Unknown: THROW_HR_MSG(E_UNEXPECTED, "Unsupported architecture 0x%X", architecture);
        default: THROW_HR_MSG(E_UNEXPECTED, "Unknown architecture 0x%X", architecture);
        }
    }

    bool operator!() const
    {
        return m_packageDependencyId.empty();
    }

private:
    wil::unique_any_psid m_user;
    std::wstring m_packageFamilyName;
    PACKAGE_VERSION m_minVersion{};
    MddPackageDependencyProcessorArchitectures m_packageDependencyProcessorArchitectures = MddPackageDependencyProcessorArchitectures::None;
    MddPackageDependencyLifetimeKind m_lifetimeKind = MddPackageDependencyLifetimeKind::Process;
    std::wstring m_lifetimeArtifact;
    MddCreatePackageDependencyOptions m_options = MddCreatePackageDependencyOptions::None;
    std::wstring m_packageDependencyId;
    mutable std::wstring m_packageFullName;
};
}
