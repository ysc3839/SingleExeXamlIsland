﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#if !defined(MDDWINRT_H)
#define MDDWINRT_H

#include <activationregistration.h>

namespace MddCore::WinRT
{
    enum class ThreadingModel
    {
        Unknown,
        Both,
        STA,
        MTA
    };

    static_assert(static_cast<int>(MddCore::WinRT::ThreadingModel::Both) == static_cast<int>(ABI::Windows::Foundation::ThreadingType::ThreadingType_BOTH) + 1);
    static_assert(static_cast<int>(MddCore::WinRT::ThreadingModel::STA) == static_cast<int>(ABI::Windows::Foundation::ThreadingType::ThreadingType_STA) + 1);
    static_assert(static_cast<int>(MddCore::WinRT::ThreadingModel::MTA) == static_cast<int>(ABI::Windows::Foundation::ThreadingType::ThreadingType_MTA) + 1);

    HRESULT ToThreadingType(
        MddCore::WinRT::ThreadingModel threadingModel,
        ABI::Windows::Foundation::ThreadingType& threadingType,
        HRESULT errorIfUnknown = REGDB_E_CLASSNOTREG) noexcept;

    HRESULT GetThreadingModel(
        HSTRING className,
        ABI::Windows::Foundation::ThreadingType& threading_model) noexcept;

    HRESULT GetActivationFactory(
        HSTRING className,
        REFIID iid,
        void** factory) noexcept;
}

#endif // MDDWINRT_H
