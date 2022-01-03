// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.

#pragma once

template <typename T, typename T1, typename T2>
constexpr T RVA2VA(T1 base, T2 rva)
{
	return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T>
constexpr T DataDirectoryFromModuleBase(void *moduleBase, size_t entryID)
{
	auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
	auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	auto dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

PIMAGE_THUNK_DATA FindAddressByName(void *moduleBase, PIMAGE_IMPORT_DESCRIPTOR iat, std::string_view funcName)
{
	auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, iat->OriginalFirstThunk);
	auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, iat->FirstThunk);
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;

		const auto import = RVA2VA<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
		if (funcName != import->Name)
			continue;

		return impAddr;
	}
	return nullptr;
}

PIMAGE_IMPORT_DESCRIPTOR FindIatInModule(void *moduleBase, std::string_view dllName)
{
	for (auto imports = DataDirectoryFromModuleBase<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT); imports->Name; ++imports)
	{
		if (dllName == RVA2VA<LPCSTR>(moduleBase, imports->Name))
			return imports;
	}
	return nullptr;
}

std::wstring _priPath;
std::wstring _exePath;
void* _priData;
DWORD _priSize;

BOOL WINAPI MyGetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
	if (fInfoLevelId == GetFileExInfoStandard && _priPath == lpFileName)
	{
		if (!GetFileAttributesExW(_exePath.c_str(), fInfoLevelId, lpFileInformation))
			return FALSE;

		auto info = reinterpret_cast<LPWIN32_FILE_ATTRIBUTE_DATA>(lpFileInformation);
		info->nFileSizeHigh = 0;
		info->nFileSizeLow = _priSize;
		return TRUE;
	}
	return GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}

UINT WINAPI MyGetDriveTypeW(LPCWSTR lpRootPathName)
{
	return DRIVE_REMOVABLE;
}

HANDLE WINAPI MyCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if (_priPath == lpFileName)
	{
		HANDLE hReadPipe, hWritePipe;
		if (!CreatePipe(&hReadPipe, &hWritePipe, nullptr, _priSize))
			return INVALID_HANDLE_VALUE;

		DWORD written;
		if (!WriteFile(hWritePipe, _priData, _priSize, &written, nullptr) || written != _priSize)
		{
			CloseHandle(hWritePipe);
			CloseHandle(hReadPipe);
			return INVALID_HANDLE_VALUE;
		}

		CloseHandle(hWritePipe);
		return hReadPipe;
	}
	return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

bool HookMrmCore(HINSTANCE hInstance, std::wstring_view priPath, std::wstring_view exePath)
{
	auto hRes = FindResourceW(hInstance, MAKEINTRESOURCEW(1), L"PRI");
	if (!hRes)
		return false;

	auto hResData = LoadResource(hInstance, hRes);
	if (!hResData)
		return false;

	auto size = SizeofResource(hInstance, hRes);
	if (!size)
		return false;

	_priData = hResData;
	_priSize = size;

	HMODULE hMrmCore = LoadLibraryExW(L"mrmcorer.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hMrmCore)
		return false;

	auto iat = FindIatInModule(hMrmCore, "api-ms-win-core-file-l1-1-0.dll");
	if (!iat)
		return false;

	auto addrGetFileAttributesExW = FindAddressByName(hMrmCore, iat, "GetFileAttributesExW");
	auto addrGetDriveTypeW = FindAddressByName(hMrmCore, iat, "GetDriveTypeW");
	auto addrCreateFileW = FindAddressByName(hMrmCore, iat, "CreateFileW");
	if (!addrGetFileAttributesExW || !addrGetDriveTypeW || !addrCreateFileW)
		return false;

	bool success = false;
	DWORD oldProtect1;
	if (VirtualProtect(addrGetFileAttributesExW, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect1))
	{
		DWORD oldProtect2;
		if (VirtualProtect(addrGetDriveTypeW, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect2))
		{
			DWORD oldProtect3;
			if (VirtualProtect(addrCreateFileW, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect3))
			{
				addrGetFileAttributesExW->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<decltype(GetFileAttributesExW)*>(MyGetFileAttributesExW));
				addrGetDriveTypeW->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<decltype(GetDriveTypeW)*>(MyGetDriveTypeW));
				addrCreateFileW->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<decltype(CreateFileW)*>(MyCreateFileW));
				_priPath = priPath;
				_exePath = exePath;
				success = true;

				VirtualProtect(addrCreateFileW, sizeof(IMAGE_THUNK_DATA), oldProtect3, &oldProtect3);
			}
			VirtualProtect(addrGetDriveTypeW, sizeof(IMAGE_THUNK_DATA), oldProtect2, &oldProtect2);
		}
		VirtualProtect(addrGetFileAttributesExW, sizeof(IMAGE_THUNK_DATA), oldProtect1, &oldProtect1);
	}
	return success;
}
