// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include <pch.h>

#include <roapi.h>
#include <wrl.h>
#include <ctxtcall.h>
#include <activation.h>
#include <VersionHelpers.h>

#include "urfw.h"

#include <detours.h>

#include "MddWinRT.h"

static decltype(RoActivateInstance)* TrueRoActivateInstance = RoActivateInstance;
static decltype(RoGetActivationFactory)* TrueRoGetActivationFactory = RoGetActivationFactory;

enum class ActivationLocation
{
    CurrentApartment,
    CrossApartmentMTA
};

VOID CALLBACK EnsureMTAInitializedCallBack
(
    PTP_CALLBACK_INSTANCE /*instance*/,
    PVOID                 /*parameter*/,
    PTP_WORK              /*work*/
)
{
    Microsoft::WRL::ComPtr<IComThreadingInfo> spThreadingInfo;
    CoGetObjectContext(IID_PPV_ARGS(&spThreadingInfo));
}

/*
In the context callback call to the MTA apartment, there is a bug that prevents COM
from automatically initializing MTA remoting. It only allows NTA to be intialized
outside of the NTA and blocks all others. The workaround for this is to spin up another
thread that is not been CoInitialize. COM treats this thread as a implicit MTA and
when we call CoGetObjectContext on it we implicitily initialized the MTA.
*/
HRESULT EnsureMTAInitialized()
{
    TP_CALLBACK_ENVIRON callBackEnviron;
    InitializeThreadpoolEnvironment(&callBackEnviron);
    PTP_POOL pool = CreateThreadpool(nullptr);
    if (pool == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SetThreadpoolThreadMaximum(pool, 1);
    if (!SetThreadpoolThreadMinimum(pool, 1))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    PTP_CLEANUP_GROUP cleanupgroup = CreateThreadpoolCleanupGroup();
    if (cleanupgroup == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SetThreadpoolCallbackPool(&callBackEnviron, pool);
    SetThreadpoolCallbackCleanupGroup(&callBackEnviron,
        cleanupgroup,
        nullptr);
    PTP_WORK ensureMTAInitializedWork = CreateThreadpoolWork(
        &EnsureMTAInitializedCallBack,
        nullptr,
        &callBackEnviron);
    if (ensureMTAInitializedWork == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SubmitThreadpoolWork(ensureMTAInitializedWork);
    CloseThreadpoolCleanupGroupMembers(cleanupgroup, FALSE, nullptr);
    return S_OK;
}

HRESULT GetActivationLocation(HSTRING activatableClassId, ActivationLocation &activationLocation)
{
    // We don't override inbox (Windows.*) runtimeclasses
    UINT32 activatableClassIdAsStringLength{};
    auto activatableClassIdAsString{ WindowsGetStringRawBuffer(activatableClassId, &activatableClassIdAsStringLength) };
    {
        // Does the activatableClassId start with "Windows."?
        auto windowsNamespacePrefix{ L"Windows." };
        const int windowsNamespacePrefixLength{ 8 };
        if (activatableClassIdAsStringLength >= windowsNamespacePrefixLength)
        {
            if (CompareStringOrdinal(activatableClassIdAsString, windowsNamespacePrefixLength, windowsNamespacePrefix, windowsNamespacePrefixLength, FALSE) == CSTR_EQUAL)
            {
                return REGDB_E_CLASSNOTREG;
            }
        }
    }

    APTTYPE aptType{};
    APTTYPEQUALIFIER aptQualifier{};
    RETURN_IF_FAILED(CoGetApartmentType(&aptType, &aptQualifier));

    ABI::Windows::Foundation::ThreadingType threading_model;
    const HRESULT hr{ MddCore::WinRT::GetThreadingModel(activatableClassId, threading_model) };
    if (FAILED(hr))
    {
        if (hr == REGDB_E_CLASSNOTREG)  // Not found
        {
            return REGDB_E_CLASSNOTREG;
        }
        RETURN_HR_MSG(hr, "URFW: ActivatableClassId=%ls", activatableClassIdAsString);
    }
    switch (threading_model)
    {
    case ABI::Windows::Foundation::ThreadingType_BOTH:
        activationLocation = ActivationLocation::CurrentApartment;
        break;
    case ABI::Windows::Foundation::ThreadingType_STA:
        if (aptType == APTTYPE_MTA)
        {
            return RO_E_UNSUPPORTED_FROM_MTA;
        }
        else
        {
            activationLocation = ActivationLocation::CurrentApartment;
        }
        break;
    case ABI::Windows::Foundation::ThreadingType_MTA:
        if (aptType == APTTYPE_MTA)
        {
            activationLocation = ActivationLocation::CurrentApartment;
        }
        else
        {
            activationLocation = ActivationLocation::CrossApartmentMTA;
        }
        break;
    }
    return S_OK;
}


HRESULT WINAPI RoActivateInstanceDetour(HSTRING activatableClassId, IInspectable** instance)
{
    ActivationLocation location;
    HRESULT hr = GetActivationLocation(activatableClassId, location);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        return TrueRoActivateInstance(activatableClassId, instance);
    }
    RETURN_IF_FAILED(hr);

    // Activate in current apartment
    if (location == ActivationLocation::CurrentApartment)
    {
        Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
        RETURN_IF_FAILED(MddCore::WinRT::GetActivationFactory(activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
        return pFactory->ActivateInstance(instance);
    }

    // Cross apartment MTA activation
    struct CrossApartmentMTAActData {
        HSTRING activatableClassId;
        IStream *stream;
    };

    CrossApartmentMTAActData cbdata{ activatableClassId };
    CO_MTA_USAGE_COOKIE mtaUsageCookie;
    RETURN_IF_FAILED(CoIncrementMTAUsage(&mtaUsageCookie));
    RETURN_IF_FAILED(EnsureMTAInitialized());
    Microsoft::WRL::ComPtr<IContextCallback> defaultContext;
    ComCallData data;
    data.pUserDefined = &cbdata;
    RETURN_IF_FAILED(CoGetDefaultContext(APTTYPE_MTA, IID_PPV_ARGS(&defaultContext)));
    RETURN_IF_FAILED(defaultContext->ContextCallback(
        [](_In_ ComCallData* pComCallData) -> HRESULT
        {
            CrossApartmentMTAActData* data = reinterpret_cast<CrossApartmentMTAActData*>(pComCallData->pUserDefined);
            Microsoft::WRL::ComPtr<IInspectable> instance;
            Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
            RETURN_IF_FAILED(MddCore::WinRT::GetActivationFactory(data->activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
            RETURN_IF_FAILED(pFactory->ActivateInstance(&instance));
            RETURN_IF_FAILED(CoMarshalInterThreadInterfaceInStream(IID_IInspectable, instance.Get(), &data->stream));
            return S_OK;
        },
        &data, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr)); // 5 is meaningless.
    RETURN_IF_FAILED(CoGetInterfaceAndReleaseStream(cbdata.stream, IID_IInspectable, (LPVOID*)instance));
    return S_OK;
}

HRESULT WINAPI RoGetActivationFactoryDetour(HSTRING activatableClassId, REFIID iid, void** factory)
{
    ActivationLocation location;
    HRESULT hr = GetActivationLocation(activatableClassId, location);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        return TrueRoGetActivationFactory(activatableClassId, iid, factory);
    }
    RETURN_IF_FAILED(hr);

    // Activate in current apartment
    if (location == ActivationLocation::CurrentApartment)
    {
        RETURN_IF_FAILED(MddCore::WinRT::GetActivationFactory(activatableClassId, iid, factory));
        return S_OK;
    }
    // Cross apartment MTA activation
    struct CrossApartmentMTAActData {
        HSTRING activatableClassId;
        IStream* stream;
    };
    CrossApartmentMTAActData cbdata{ activatableClassId };
    CO_MTA_USAGE_COOKIE mtaUsageCookie;
    RETURN_IF_FAILED(CoIncrementMTAUsage(&mtaUsageCookie));
    RETURN_IF_FAILED(EnsureMTAInitialized());
    Microsoft::WRL::ComPtr<IContextCallback> defaultContext;
    ComCallData data;
    data.pUserDefined = &cbdata;
    RETURN_IF_FAILED(CoGetDefaultContext(APTTYPE_MTA, IID_PPV_ARGS(&defaultContext)));
    defaultContext->ContextCallback(
        [](_In_ ComCallData* pComCallData) -> HRESULT
        {
            CrossApartmentMTAActData* data = reinterpret_cast<CrossApartmentMTAActData*>(pComCallData->pUserDefined);
            Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
            RETURN_IF_FAILED(MddCore::WinRT::GetActivationFactory(data->activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
            RETURN_IF_FAILED(CoMarshalInterThreadInterfaceInStream(IID_IActivationFactory, pFactory.Get(), &data->stream));
            return S_OK;
        },
        &data, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr); // 5 is meaningless.
    RETURN_IF_FAILED(CoGetInterfaceAndReleaseStream(cbdata.stream, IID_IActivationFactory, factory));
    return S_OK;
}

HRESULT UrfwInitialize() noexcept
{
    DetourAttach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourAttach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);

    return S_OK;
}

void UrfwShutdown() noexcept
{
    DetourDetach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourDetach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);
}
