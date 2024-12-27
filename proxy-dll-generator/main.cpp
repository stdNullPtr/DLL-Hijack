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
    HMODULE hModule = LoadLibrary(dllPath.c_str());
    if (!hModule)
    {
        std::cerr << "Failed to load DLL: " << dllPath << std::endl;
        return {};
    }

    std::vector<FunctionInfo> functions;

    // Enumerate exported functions
    auto baseAddress = reinterpret_cast<char*>(hModule);
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(baseAddress);
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(baseAddress + dosHeader->e_lfanew);

    auto exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
    {
        std::cerr << "No export directory found in DLL." << std::endl;
        FreeLibrary(hModule);
        return {};
    }

    auto exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(baseAddress + exportDirRVA);
    auto names = reinterpret_cast<DWORD*>(baseAddress + exportDir->AddressOfNames);
    auto ordinals = reinterpret_cast<WORD*>(baseAddress + exportDir->AddressOfNameOrdinals);
    auto functionsRVA = reinterpret_cast<DWORD*>(baseAddress + exportDir->AddressOfFunctions);

    for (size_t i = 0; i < exportDir->NumberOfNames; ++i)
    {
        std::string functionName(reinterpret_cast<char*>(baseAddress + names[i]));
        FARPROC address = reinterpret_cast<FARPROC>(baseAddress + functionsRVA[ordinals[i]]);
        functions.push_back({ functionName, address });
    }

    FreeLibrary(hModule);
    return functions;
}

void GeneratePragmaExports(const std::vector<FunctionInfo>& functions, const std::string& outputPath)
{
    std::ofstream outputFile(outputPath);
    if (!outputFile)
    {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        return;
    }

    outputFile << "#define WIN32_LEAN_AND_MEAN\n";
    outputFile << "#include <windows.h>\n\n";

    for (const auto& func : functions)
    {
        outputFile << "#pragma comment(linker, \"/export:" << func.name << "=My" << func.name << "\")\n";
    }

    outputFile.close();
    std::cout << "Generated #pragma exports in: " << outputPath << std::endl;
}

int main()
{
    std::string dllPath = R"(C:\Users\anton\Desktop\hax\DLL hijack\version_orig.dll)";

    std::string outputPath = "exports.h";

    std::vector<FunctionInfo> functions = ParseDLL(dllPath);
    if (functions.empty())
    {
        std::cerr << "No functions found in DLL." << std::endl;
        return 1;
    }

    GeneratePragmaExports(functions, outputPath);

    return 0;
}
