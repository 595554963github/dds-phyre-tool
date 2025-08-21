#include <iostream>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <cstring> 
#include <filesystem>

#include "PhyreException.h"
#include "PhyreContainer.h"
#include "version.h"

namespace fs = std::filesystem;

bool IsPhyreFile(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    char header[16];
    file.read(header, 16);
    return file.gcount() >= 16 &&
        std::memcmp(header, "RYHP", 4) == 0 &&
        std::memcmp(header + 12, "11XD", 4) == 0;
}

void printBanner() {
    std::wcout << L"DDS Phyre tool v" VERSION_FULL L" by ffgriever\n\n";
}

bool ConvertPhyreToDDS(const std::wstring& inputFile) {
    try {
        fs::path inputPath(inputFile);
        fs::path outputPath = inputPath.parent_path() / (inputPath.stem().wstring() + L".dds");

        phyre::PhyreContainer phyreFile(inputFile);
        phyreFile.ConvertPhyre2DDS(inputFile, outputPath.wstring());
        std::wcout << L"转换成功: " << outputPath.wstring() << L"\n";
        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"转换失败: " << e.what() << L"\n";
        return false;
    }
    catch (...) {
        std::wcerr << L"转换失败: 未知错误\n";
        return false;
    }
}

void printUsage() {
    std::wcerr << L"用法: dds-phyre-tool.exe <输入文件>\n";
    std::wcerr << L"示例: dds-phyre-tool.exe texture.phyre\n或者把文件拖到exe上即可解包\n";
    std::wcerr << L"输出文件将自动保存为同名的dds\n";
}

int wmain(int argc, wchar_t* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U16TEXT);
    (void)_setmode(_fileno(stderr), _O_U16TEXT);
    std::locale::global(std::locale(""));

    printBanner();

    if (argc != 2) {
        std::wcerr << L"错误: 参数数量不正确\n";
        printUsage();
        return EXIT_FAILURE;
    }

    std::wstring inputFile = argv[1];
    if (!inputFile.empty() && inputFile.front() == L'"' && inputFile.back() == L'"') {
        inputFile = inputFile.substr(1, inputFile.size() - 2);
    }

    DWORD inputAttrib = GetFileAttributesW(inputFile.c_str());
    if (inputAttrib == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"错误: 输入文件不存在 - " << inputFile << L"\n";
        return EXIT_FAILURE;
    }

    if (inputAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        std::wcerr << L"错误: 不支持目录处理，请指定单个文件\n";
        printUsage();
        return EXIT_FAILURE;
    }

    if (!IsPhyreFile(inputFile)) {
        std::wcerr << L"错误:不是有效的Phyre文件-" << inputFile << L"\n";
        return EXIT_FAILURE;
    }

    bool success = ConvertPhyreToDDS(inputFile);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}