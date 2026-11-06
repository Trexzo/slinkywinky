#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include "slinkywinky/windows_stop_gate.hpp"
// R94: compiled PID-scoped event names must exactly match the target JNI DLL;
// never substitute a Java Attach for these owner-process control events.
#include <cstdint>
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
                             option == L"--hook-probe=600" ||
                             option == L"--hook-probe=continuous";
    const bool stop_option = option == L"--stop-observation";
    const bool render_preflight = option == L"--render-preflight";
    const bool render_callback = option == L"--render-callback=8";
    const bool render_context = option == L"--render-context=8";
    const bool render_overlay = option == L"--render-overlay=8";
    const bool render_multiframe = option == L"--render-overlay=72";
    const bool render_retained = option == L"--render-retained=72";
    const bool render_visibility = option == L"--render-visibility=72";
    const bool manual_visibility = option == L"--render-visibility=manual";
    const bool toggle_visibility = option == L"--toggle-visibility";
    const bool stop_visibility = option == L"--stop-visibility";
    if ((!list && !attach) || (attach && !numeric(argv[2])) ||
        (argc == 5 && option != L"--tick-probe" &&
         option != L"--instrument-probe" && !hook_option && !stop_option && !render_preflight && !render_callback && !render_context && !render_overlay && !render_multiframe && !render_retained && !render_visibility && !manual_visibility &&
         !toggle_visibility && !stop_visibility)) {
        std::wcerr << L"Usage: slinkywinky_loader.exe list JDK8_HOME\n"
                   << L"   or: slinkywinky_loader.exe attach PID JDK8_HOME [--hook-probe[=120|600|continuous]|--stop-observation|--render-preflight|--render-callback=8|--render-context=8|--render-overlay=8|--render-overlay=72|--render-retained=72|--render-visibility=72|--render-visibility=manual|--toggle-visibility|--stop-visibility]\n";
        return 2;
    }
    // R94 named controls do NOT use the Java Attach API (safepoint). They
    // work only while the owner-PID JNI renderer has created the events.
    if(toggle_visibility || stop_visibility) {
        std::uint64_t target=0;
        try {target=std::stoull(std::wstring(argv[2]));}
        catch(...) {return 2;}
        if(!target || target>0xffffffffULL)return 2;
        const auto eventName=toggle_visibility ?
            slinkywinky::visibility_toggle_event_name(static_cast<std::uint32_t>(target)):
            slinkywinky::visibility_stop_event_name(static_cast<std::uint32_t>(target));
        HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName.c_str());
        if(!event) {
            std::wcerr << L"ATTACH_RESULT=FAIL no active R94 visibility gate for PID\n";
            return 2;
        }
        const BOOL sent=SetEvent(event);
        CloseHandle(event);
        if(!sent)return 2;
        std::wcout << L"ATTACH_RESULT=SUCCESS pid=" << target <<
                      (toggle_visibility?L" mode=toggle-visibility\n":
                                         L" mode=stop-visibility\n");
        return 0;
    }
    // Never enter the Java Attach API for an active continuous session. It may
    // require a JVM safepoint while our tick instrumentation is executing.
    // This separate event has one power only: request cooperative shutdown.
    if(stop_option) {
        std::uint64_t target=0;
        try { target=std::stoull(std::wstring(argv[2])); }
        catch(...) {return 2;}
        if(target==0 || target>0xffffffffULL)return 2;
        const auto eventName=slinkywinky::continuous_stop_event_name(
            static_cast<std::uint32_t>(target));
        HANDLE signal=OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName.c_str());
        if(!signal) {
            std::wcerr << L"ATTACH_RESULT=FAIL no active continuous stop gate for PID\n";
            return 2;
        }
        const BOOL requested=SetEvent(signal);
        CloseHandle(signal);
        if(!requested)return 2;
        std::wcout << L"ATTACH_RESULT=SUCCESS pid=" << target
                   << L" mode=stop-observation\n";
        return 0;
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
        (argc == 5 && (option == L"--instrument-probe" || hook_option || stop_option || render_preflight || render_callback || render_context || render_overlay || render_multiframe || render_retained || render_visibility || manual_visibility) &&
         !regular_file_exists(instrument)) ||
        (argc == 5 && (hook_option || render_context || render_overlay || render_multiframe || render_retained || render_visibility) &&
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
