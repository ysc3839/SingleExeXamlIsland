// Win32App.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "Win32App.h"
#include "MrmHook.h"

#include "urfw.h"
#include "MddDetourPackageGraph.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
winrt::Xaml::App hostApp{ nullptr };
winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _desktopWindowXamlSource{ nullptr };
winrt::Xaml::MainPage _myUserControl{ nullptr };

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                AdjustLayout(HWND);
HRESULT             AddPackageDepends(LPCWSTR);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DetourTransactionBegin();
	MddDetourPackageGraphInitialize();
	UrfwInitialize();
	DetourTransactionCommit();

	// TODO: Place code here.
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	AddPackageDepends(L"Microsoft.UI.Xaml.2.7_8wekyb3d8bbwe");
	AddPackageDepends(L"Microsoft.VCLibs.140.00_8wekyb3d8bbwe");

	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(hInstance, exePath, MAX_PATH);

	wchar_t priPath[MAX_PATH];
	wcscpy_s(priPath, exePath);
	auto pos = wcsrchr(priPath, L'\\');
	pos[1] = 0;
	wcscat_s(priPath, L"resources.pri");

	HookMrmCore(hInstance, priPath, exePath);

	hostApp = winrt::Xaml::App{};
	_desktopWindowXamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource{};

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WIN32APP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32APP));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32APP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32APP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	// Begin XAML Islands walkthrough code.
	if (_desktopWindowXamlSource != nullptr)
	{
		auto interop = _desktopWindowXamlSource.as<IDesktopWindowXamlSourceNative>();
		check_hresult(interop->AttachToWindow(hWnd));
		HWND hWndXamlIsland = nullptr;
		interop->get_WindowHandle(&hWndXamlIsland);
		RECT windowRect;
		::GetClientRect(hWnd, &windowRect);
		::SetWindowPos(hWndXamlIsland, NULL, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_SHOWWINDOW);
		_myUserControl = winrt::Xaml::MainPage();
		_desktopWindowXamlSource.Content(_myUserControl);
	}
	// End XAML Islands walkthrough code.

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		if (_desktopWindowXamlSource != nullptr)
		{
			_desktopWindowXamlSource.Close();
			_desktopWindowXamlSource = nullptr;
		}
		break;
	case WM_SIZE:
		AdjustLayout(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void AdjustLayout(HWND hWnd)
{
	if (_desktopWindowXamlSource != nullptr)
	{
		auto interop = _desktopWindowXamlSource.as<IDesktopWindowXamlSourceNative>();
		HWND xamlHostHwnd = NULL;
		check_hresult(interop->get_WindowHandle(&xamlHostHwnd));
		RECT windowRect;
		::GetClientRect(hWnd, &windowRect);
		::SetWindowPos(xamlHostHwnd, NULL, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_SHOWWINDOW);
	}
}

HRESULT AddPackageDepends(LPCWSTR packageFamilyName)
{
	PWSTR packageDependencyId;
	HRESULT hr = MddTryCreatePackageDependency(nullptr, packageFamilyName, {}, MddPackageDependencyProcessorArchitectures::None, MddPackageDependencyLifetimeKind::Process, nullptr, MddCreatePackageDependencyOptions::None, &packageDependencyId);
	if (SUCCEEDED(hr))
	{
		MDD_PACKAGEDEPENDENCY_CONTEXT context;
		PWSTR packageFullName;
		hr = MddAddPackageDependency(packageDependencyId, 0, MddAddPackageDependencyOptions::None, &context, &packageFullName);
		if (SUCCEEDED(hr))
		{
			HeapFree(GetProcessHeap(), 0, packageFullName);
		}
		HeapFree(GetProcessHeap(), 0, packageDependencyId);
	}
	return hr;
}
