# SingleExeXamlIsland
XAML Islands allows you use modern UWP XAML UI in Win32 apps. However, the `.xaml` sources will be compiled and bundled in a `resource.pri` file. You have to ship it with your executable.

Also according to the [document from Microsoft](https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/host-custom-control-with-xaml-islands-cpp), XAML part of your app is complied as a DLL, and it has a dependency of `Microsoft.Toolkit.Win32.UI.SDK`, which is another DLL. So finally you have to ship at least three other files with your exe file.

This project is based on this [document](https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/host-custom-control-with-xaml-islands-cpp).

# Details
Since Microsoft doesn't provide an API to load `resource.pri` from memory. It's done by hooking some system function. So I may not working in future version of Windows.

According to debugging, the `resource.pri` is loaded by `MrmCoreR.dll`. It first call `GetFileAttributesExW` to get information of `resource.pri`.\
Then call `GetDriveTypeW` to get drive type of resource file.\
Finally it call `CreateFileW` to open the file. If the drive type is fixed, it use `MapViewOfFile` to map the file to memory and read that memory. If the drive type is not fixed, it use `ReadFile` to read the file directly.

This program hook `CreateFileW`, and use `CreatePipe` to create an anonymous pipe, write content of `resource.pri` into the pipe, then return the read handle of the pipe.\
Hook `GetDriveTypeW` and return `DRIVE_REMOVABLE` to force `MrmCoreR.dll` to use `ReadFile`.\
Hook `GetFileAttributesExW` to return correct information of resource file.

# WinUI 2
Windows 11 brings a new UI. However it's required to add WinUI resources to your `App.xaml` file, like this:
```
<Application.Resources>
    <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
</Application.Resources>
```
And by doing so, you have to ship `Microsoft.UI.Xaml.dll` with your exe file. You can't use the one installed in system, until Windows 11.\
Windows 11 added [dynamic dependency API](https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/framework-packages/use-the-dynamic-dependency-api), which allow unpackaged apps to reference and to use framework packages such as WinUI 2.\
This part of code is in [winui2 branch](https://github.com/ysc3839/SingleExeXamlIsland/tree/winui2). You can compare it with main branch to see differents.

Because of a bug of current Windows 11, the dynamic dependency API functions is not present in `kernel32.dll`, they are in `kernelbase.dll` or `kernel.appcore.dll` (This is forwards to `kernelbase.dll`). However the import library provided by Windows 11 SDK declares they are in `kernel32.dll`, which makes the program unable to start.\
You have to load those functions manually using `GetProcAddress`, or link with `OneCoreUAP_apiset.Lib`, or create an import library that links to `kernelbase.dll`.

# WinUI 2 on Windows 10
With `Microsoft Dynamic Dependency` code from [WindowsAppSDK](https://github.com/microsoft/WindowsAppSDK/tree/main/dev/DynamicDependency/API), now it's able to use WinUI 2 on Windows 10.\
This part of code is in [winui2-win10 branch](https://github.com/ysc3839/SingleExeXamlIsland/tree/winui2-win10).
