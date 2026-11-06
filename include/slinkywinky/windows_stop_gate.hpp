#pragma once
#ifdef _WIN32
#include <cstdint>
#include <string>

namespace slinkywinky {
// Single-purpose, local-session, PID-scoped event. This is independent of
// read-only HUD IPC and can ONLY request owner-thread observation shutdown.
// R94: distinct manual-menu controls, never the observation-stop event.
// Only a live native visibility session creates these PID-scoped events.
inline std::wstring visibility_toggle_event_name(std::uint32_t pid) {
    return std::wstring(L"Local\\\\SlinkyWinky-VisibilityToggle-R94-v1-") +
           std::to_wstring(pid);
}
inline std::wstring visibility_stop_event_name(std::uint32_t pid) {
    return std::wstring(L"Local\\\\SlinkyWinky-VisibilityStop-R94-v1-") +
           std::to_wstring(pid);
}
inline std::wstring continuous_stop_event_name(std::uint32_t pid) {
    return std::wstring(L"Local\\SlinkyWinky-ObserveStop-v1-") +
           std::to_wstring(pid);
}
} // namespace slinkywinky
#endif
