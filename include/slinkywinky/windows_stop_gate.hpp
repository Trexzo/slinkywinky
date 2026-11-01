#pragma once
#ifdef _WIN32
#include <cstdint>
#include <string>

namespace slinkywinky {
// Single-purpose, local-session, PID-scoped event. This is independent of
// read-only HUD IPC and can ONLY request owner-thread observation shutdown.
inline std::wstring continuous_stop_event_name(std::uint32_t pid) {
    return std::wstring(L"Local\\SlinkyWinky-ObserveStop-v1-") +
           std::to_wstring(pid);
}
} // namespace slinkywinky
#endif
