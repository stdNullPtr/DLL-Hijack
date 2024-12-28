#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct FunctionInfo
{
    std::string name;
    FARPROC address;
};

std::vector<FunctionInfo> ParseDLL(const std::string& dllPath)
{
    const HMODULE hModule{LoadLibrary(dllPath.c_str())};
    if (!hModule)
    {
        std::cerr << "Failed to load DLL: " << dllPath << '\n';
        return {};
    }

    std::vector<FunctionInfo> functions;

    const auto baseAddress{reinterpret_cast<char*>(hModule)};
    const auto dosHeader{reinterpret_cast<IMAGE_DOS_HEADER*>(baseAddress)};
    const auto ntHeaders{reinterpret_cast<IMAGE_NT_HEADERS*>(baseAddress + dosHeader->e_lfanew)};

    auto exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
    {
        std::cerr << "No export directory found in DLL." << '\n';
        FreeLibrary(hModule);
        return {};
    }

    const auto exportDir{reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(baseAddress + exportDirRVA)};
    const auto names{reinterpret_cast<DWORD*>(baseAddress + exportDir->AddressOfNames)};
    const auto ordinals{reinterpret_cast<WORD*>(baseAddress + exportDir->AddressOfNameOrdinals)};
    const auto functionsRVA{reinterpret_cast<DWORD*>(baseAddress + exportDir->AddressOfFunctions)};

    for (size_t i{0}; i < exportDir->NumberOfNames; ++i)
    {
        const std::string functionName(baseAddress + names[i]);
        const FARPROC address{reinterpret_cast<FARPROC>(baseAddress + functionsRVA[ordinals[i]])};
        functions.push_back({functionName, address});
    }

    FreeLibrary(hModule);
    return functions;
}

void GeneratePragmaExports(const std::vector<FunctionInfo>& functions, const std::string& outputPath)
{
    std::ofstream outputFile(outputPath);
    if (!outputFile)
    {
        std::cerr << "Failed to open output file: " << outputPath << '\n';
        return;
    }

    outputFile << "#define WIN32_LEAN_AND_MEAN\n";
    outputFile << "#include <windows.h>\n\n";

    for (const auto& func : functions)
    {
        outputFile << "#pragma comment(linker, \"/export:" << func.name << "=My" << func.name << "\")\n";
    }

    outputFile.close();
    std::cout << "Generated #pragma exports in: " << outputPath << '\n';
}

int main()
{
    const std::string dllPath{R"(C:\Windows\System32\version.dll)"};
    const std::string outputPath{"exports.h"};
    const std::vector functions{ParseDLL(dllPath)};

    if (functions.empty())
    {
        std::cerr << "No functions found in DLL." << '\n';
        return 1;
    }

    GeneratePragmaExports(functions, outputPath);

    return 0;
}
