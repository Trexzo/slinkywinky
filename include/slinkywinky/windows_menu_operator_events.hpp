#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <string>

namespace slinkywinky {

// R100: no Java Attach or OS keyboard/mouse injection to send a UI command.
// This scheme is scoped to BOTH the target PID and its kernel process-birth
// token. The active original render session alone creates these events.
inline bool menu_operator_process_birth(std::uint32_t pid,
                                        std::uint64_t* birth) noexcept {
    if(!pid || !birth)return false;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return false;
    FILETIME created{},exited{},kernel{},user{};
    const BOOL okay=GetProcessTimes(process,&created,&exited,&kernel,&user);
    CloseHandle(process);
    if(!okay)return false;
    const std::uint64_t value=
        (static_cast<std::uint64_t>(created.dwHighDateTime)<<32)|
         static_cast<std::uint64_t>(created.dwLowDateTime);
    if(!value)return false;
    *birth=value;
    return true;
}

// Slot 0..7 is a first-party native MenuNavAction; 8 requests stop.
// Slot 9=ack-accepted, 10=ack-refused, 11=single-sender mutex.
// No slot ever maps to a Minecraft gameplay action.
inline const wchar_t* menu_operator_slot_name(int slot) noexcept {
    switch(slot) {
        case 0:return L"next-tab";
        case 1:return L"previous-tab";
        case 2:return L"next-row";
        case 3:return L"previous-row";
        case 4:return L"expand";
        case 5:return L"toggle-preview";
        case 6:return L"increase";
        case 7:return L"decrease";
        case 8:return L"stop";
        case 9:return L"ack-success";
        case 10:return L"ack-refused";
        case 11:return L"producer-mutex";
        default:return nullptr;
    }
}
inline std::wstring menu_operator_event_name(std::uint32_t pid,
        std::uint64_t birth,int slot) {
    const wchar_t* control=menu_operator_slot_name(slot);
    if(!pid || !birth || !control)return {};
    return std::wstring(L"Local\\SlinkyWinky-MenuR100-v1-")+
           std::to_wstring(pid)+L"-"+std::to_wstring(birth)+
           L"-"+control;
}
} // namespace slinkywinky
#endif
