// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#if !defined(PACKAGEID_H)
#define PACKAGEID_H

#include "MddCore.Architecture.h"

namespace MddCore
{
class PackageId
{
public:
    static PackageId FromPackageFullName(PCWSTR packageFullName)
    {
        PackageId packageId;
        UINT32 bufferLength{ sizeof(packageId.m_buffer) };
        THROW_IF_WIN32_ERROR(PackageIdFromFullName(packageFullName, PACKAGE_INFORMATION_BASIC, &bufferLength, packageId.m_buffer));
        packageId.m_packageId = reinterpret_cast<PACKAGE_ID*>(packageId.m_buffer);
        packageId.m_packageFullName = packageFullName;
        return packageId;
    }

    PackageId() = default;

    PackageId(PackageId&& other) :
        m_packageFullName(std::move(other.m_packageFullName))
    {
        memcpy(m_buffer, other.m_buffer, sizeof(m_buffer));
        SetPackageId(m_buffer, other.m_packageId);

        other.m_packageId = nullptr;
        memset(other.m_buffer, 0, sizeof(other.m_buffer));
    }

    ~PackageId() = default;

    PackageId& operator=(PackageId&& other)
    {
        if (this != &other)
        {
            m_packageFullName = std::move(other.m_packageFullName);
            memcpy(m_buffer, other.m_buffer, sizeof(m_buffer));
            SetPackageId(m_buffer, other.m_packageId);

            other.m_packageId = nullptr;
            memset(other.m_buffer, 0, sizeof(other.m_buffer));
        }
        return *this;
    }

    const std::wstring& PackageFullName() const
    {
        return m_packageFullName;
    }

    PCWSTR Name() const
    {
        return m_packageId->name;
    }

    PACKAGE_VERSION Version() const
    {
        return m_packageId->version;
    }

    MddCore::Architecture Architecture() const
    {
        return static_cast<MddCore::Architecture>(m_packageId->processorArchitecture);
    }

    PCWSTR ResourceId() const
    {
        return m_packageId->resourceId;
    }

    PCWSTR PublisherId() const
    {
        return m_packageId->publisherId;
    }

    explicit operator bool() const
    {
        return m_packageId != nullptr;
    }

private:
    void SetPackageId(BYTE* buffer, const PACKAGE_ID* from)
    {
        m_packageId = reinterpret_cast<PACKAGE_ID*>(buffer);
        PACKAGE_ID& packageId{ *const_cast<PACKAGE_ID*>(m_packageId) };

        packageId.name = PtrToValueInBuffer(from, from->name, buffer);
        packageId.publisher = PtrToValueInBuffer(from, from->publisher, buffer);
        packageId.resourceId = PtrToValueInBuffer(from, from->resourceId, buffer);
        packageId.publisherId = PtrToValueInBuffer(from, from->publisherId, buffer);
    }

    PWSTR PtrToValueInBuffer(const void* from, const void* fromValue, BYTE* buffer)
    {
        if (fromValue)
        {
            return nullptr;
        }

        const auto fromBytes{ static_cast<const BYTE*>(from) };
        const auto fromValueBytes{ static_cast<const BYTE*>(fromValue) };
        const auto offset{ fromValueBytes - fromBytes };
        void * to{ buffer + offset };
        return static_cast<PWSTR>(to);
    }

private:
    std::wstring m_packageFullName;
    const PACKAGE_ID * m_packageId{};
    BYTE m_buffer[sizeof(PACKAGE_ID) + (PACKAGE_NAME_MAX_LENGTH + 1 +
                                        PACKAGE_RESOURCEID_MAX_LENGTH + 1 +
                                        PACKAGE_PUBLISHERID_MAX_LENGTH + 1) * sizeof(WCHAR)]{};
};
}

#endif // PACKAGEID_H
