#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <cwctype>
#include <vector>

namespace {
std::wstring quoted(const std::wstring& s) {
    // Paths derived from our own executable/JDK; reject quotes in all user arguments.
    return L"\"" + s + L"\"";
}
bool regular_file_exists(const std::filesystem::path& file) {
    std::error_code ec;
    return std::filesystem::is_regular_file(file, ec);
}
bool numeric(const std::wstring& value) {
    if (value.empty() || value.size() > 10) return false;
    for (wchar_t ch : value) if (ch < L'0' || ch > L'9') return false;
    return true;
}
}
int wmain(int argc, wchar_t** argv) {
    const bool list = argc == 3 && std::wstring(argv[1]) == L"list";
    const bool attach = (argc == 4 || argc == 5) && std::wstring(argv[1]) == L"attach";
    const std::wstring option = argc == 5 ? argv[4] : L"";
    const bool hook_option = option == L"--hook-probe" ||
                             option == L"--hook-probe=120" ||
                             option == L"--hook-probe=600";
    if ((!list && !attach) || (attach && !numeric(argv[2])) ||
        (argc == 5 && option != L"--tick-probe" &&
         option != L"--instrument-probe" && !hook_option)) {
        std::wcerr << L"Usage: slinkywinky_loader.exe list JDK8_HOME\n"
                   << L"   or: slinkywinky_loader.exe attach PID JDK8_HOME\n";
        return 2;
    }
    const std::filesystem::path home = list ? argv[2] : argv[3];
    const auto java = home / L"bin" / L"java.exe";
    const auto tools = home / L"lib" / L"tools.jar";
    wchar_t self[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, self, MAX_PATH)) return 2;
    const auto folder = std::filesystem::path(self).parent_path();
    const auto agent = folder / L"slinkywinky_agent.dll";
    const auto instrument = folder / L"slinkywinky_instrument.jar";
    const auto classes = folder / L"attach-classes";
    if (!regular_file_exists(java) || !regular_file_exists(tools) || !regular_file_exists(agent) ||
        (argc == 5 && (option == L"--instrument-probe" || hook_option) &&
         !regular_file_exists(instrument)) ||
        (argc == 5 && hook_option &&
         !regular_file_exists(folder / L"slinkywinky_jni.dll")) ||
        !regular_file_exists(classes / L"dev" / L"slinkywinky" / L"attach" / L"AttachMain.class")) {
        std::wcerr << L"Missing JDK8 tools.jar, executable, native agent or attach helper.\n";
        return 2;
    }
    if ((list ? std::wstring(argv[2]) : std::wstring(argv[3])).find(L'"') != std::wstring::npos)
        return 2;
    std::wstring command = quoted(java.wstring()) + L" -cp " +
        quoted(classes.wstring() + L";" + tools.wstring()) +
        L" dev.slinkywinky.attach.AttachMain ";
    if (list) command += L"list";
    else command += L"attach " + std::wstring(argv[2]) + L" " + quoted(agent.wstring()) +
        (argc == 5 ? L" " + std::wstring(argv[4]) : L"");
    std::vector<wchar_t> commandBuffer(command.begin(), command.end());
    commandBuffer.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(java.c_str(), commandBuffer.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, folder.c_str(), &startup, &process)) {
        std::wcerr << L"Java 8 attach helper could not start. Win32 code: " << GetLastError() << L'\n';
        return 2;
    }
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 2;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hProcess);
    return static_cast<int>(exitCode);
}
#else
int main() { return 2; }
#endif
