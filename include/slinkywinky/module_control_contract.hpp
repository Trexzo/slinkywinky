#pragma once
// R210: narrow, first-party GUI/runtime integration contract.
// No PR #1 UI code is modified. A menu toggle is only a *request*;
// actual runtime and rendered ArrayList state must come from the
// validated owner-thread snapshot, never MenuState.preview_enabled.
#include <cstdint>
#include <string_view>

namespace slinkywinky {

inline constexpr std::uint32_t kModuleControlContractVersion = 1;

struct ModuleControlRequest {
    std::string_view canonical_name{};
    bool enable = false;
};

struct ModuleControlState {
    std::string_view canonical_name{};
    bool requested_enabled = false;
    bool active_in_world = false;
    bool settings_override = false;
    std::uint64_t world_generation = 0;
};

// Future PR #1 adapter must publish a copy of ModuleControlState from
// the admitted owner thread. No game/JVM pointer crosses this boundary.
// "requested" means the UI selection is remembered; "active" means
// event handlers are admitted in the current in-game session.
} // namespace slinkywinky
