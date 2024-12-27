#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <filesystem>

#pragma comment(linker, "/export:GetFileVersionInfoA=MyGetFileVersionInfoA")/*
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=MyGetFileVersionInfoByHandle")
#pragma comment(linker, "/export:GetFileVersionInfoExA=MyGetFileVersionInfoExA")
#pragma comment(linker, "/export:GetFileVersionInfoExW=MyGetFileVersionInfoExW")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=MyGetFileVersionInfoSizeA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExA=MyGetFileVersionInfoSizeExA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExW=MyGetFileVersionInfoSizeExW")*/
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=MyGetFileVersionInfoSizeW")
#pragma comment(linker, "/export:GetFileVersionInfoW=MyGetFileVersionInfoW")/*
#pragma comment(linker, "/export:VerFindFileA=MyVerFindFileA")
#pragma comment(linker, "/export:VerFindFileW=MyVerFindFileW")
#pragma comment(linker, "/export:VerInstallFileA=MyVerInstallFileA")
#pragma comment(linker, "/export:VerInstallFileW=MyVerInstallFileW")
#pragma comment(linker, "/export:VerLanguageNameA=MyVerLanguageNameA")
#pragma comment(linker, "/export:VerLanguageNameW=MyVerLanguageNameW")
#pragma comment(linker, "/export:VerQueryValueA=MyVerQueryValueA")*/
#pragma comment(linker, "/export:VerQueryValueW=MyVerQueryValueW")

#define TARGET_DLL R"(C:\Windows\System32\version.dll)"

typedef BOOL (APIENTRY*PFN_GetFileVersionInfoA)(LPCSTR, DWORD, DWORD, LPVOID);
typedef DWORD (APIENTRY*PFN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
typedef BOOL (APIENTRY*PFN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef BOOL (APIENTRY*PFN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

FARPROC GetFuncAddress(const std::filesystem::path& dllPath, const std::string& funcName)
{
    std::cout << "DLL path: " << dllPath << " Func name: " << funcName << '\n';

    if (dllPath.empty())
    {
        std::cerr << "DLL path is empty." << '\n';
        return nullptr;
    }

    if (!exists(dllPath))
    {
        std::cerr << "DLL file doesn't exist: " << dllPath << '\n';
        return nullptr;
    }

    HMODULE hModule = LoadLibraryEx(dllPath.string().c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hModule)
    {
        std::cerr << "Failed to load DLL: " << dllPath << '\n';
        return nullptr;
    }

    FARPROC funcAddress = GetProcAddress(hModule, funcName.c_str());
    if (!funcAddress)
    {
        std::cerr << "Failed to get function address: " << funcName << '\n';
        return nullptr;
    }

    return funcAddress;
}

extern "C" {
__declspec(dllexport) BOOL MyGetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    static auto pRealFunc = reinterpret_cast<PFN_GetFileVersionInfoA>(GetFuncAddress(TARGET_DLL, "GetFileVersionInfoA"));
    return pRealFunc(lptstrFilename, dwHandle, dwLen, lpData);
}

__declspec(dllexport) DWORD MyGetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle)
{
    static auto pRealFunc = reinterpret_cast<PFN_GetFileVersionInfoSizeW>(GetFuncAddress(TARGET_DLL, "GetFileVersionInfoSizeW"));
    return pRealFunc(lptstrFilename, lpdwHandle);
}

__declspec(dllexport) BOOL MyGetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    static auto pRealFunc = reinterpret_cast<PFN_GetFileVersionInfoW>(GetFuncAddress(TARGET_DLL, "GetFileVersionInfoW"));
    return pRealFunc(lptstrFilename, dwHandle, dwLen, lpData);
}

__declspec(dllexport) BOOL MyVerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
    static auto pRealFunc = reinterpret_cast<PFN_VerQueryValueW>(GetFuncAddress(TARGET_DLL, "VerQueryValueW"));
    return pRealFunc(pBlock, lpSubBlock, lplpBuffer, puLen);
}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        if (!GetConsoleWindow())
        {
            AllocConsole();

            (void)freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            (void)freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            (void)freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
        }
    }
    return TRUE;
}
