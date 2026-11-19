#pragma once
#include "slinkywinky/module_arraylist.hpp"
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace slinkywinky {
class MenuState;
class MenuNavigation;

// R210: copy-only first-party GUI -> admitted module-owner request transport.
// A request is NEVER an activation acknowledgement or a gameplay action.
// R245: only the three source-backed PR #2 R211 native controls
// (No Jump Delay, No Slow, Sprint) are admitted. The R244 66-entry
// native identity fence applies to all requested names. Others are previews.
struct GuiModuleControlRequest {
    std::uint64_t id=0;
    std::uint64_t observed_source_sequence=0;
    std::uint64_t world_generation=0;
    std::string canonical_name;
    bool enable=false;
};
enum class GuiModuleControlPhase : unsigned char {
    Idle, Pending, Dispatched, Acknowledged, Denied, Retired
};
struct GuiModuleControlView {
    GuiModuleControlPhase phase=GuiModuleControlPhase::Idle;
    std::optional<GuiModuleControlRequest> request;
    std::uint64_t last_confirmed_source_sequence=0;
    bool last_confirmed_active=false;
};

class GuiModuleControlHandoff final {
public:
    GuiModuleControlHandoff();
    GuiModuleControlHandoff(const GuiModuleControlHandoff&)=delete;
    GuiModuleControlHandoff& operator=(const GuiModuleControlHandoff&)=delete;

    // Called only AFTER a separately authenticated GUI gesture, from the
    // painted menu's actual render owner. This API does NOT capture input.
    bool request_focused_toggle(const MenuState& menu,
        const MenuNavigation& navigation,const ModuleArrayListFeed& source,
        std::uint64_t expected_source_sequence=0);
    // The caller must be the externally admitted game/module owner.
    // Its thread identity is pinned on first successful dequeue.
    std::optional<GuiModuleControlRequest> take_on_module_owner();
    bool acknowledge_on_module_owner(std::uint64_t id,
        const ModuleArrayListSnapshot& acknowledged,
        ModuleArrayListFeed& published_view);
    bool deny_on_module_owner(std::uint64_t id);
    GuiModuleControlView view() const;
    void retire();
private:
    bool module_owner_locked() const noexcept;
    void retire_locked() noexcept;
    mutable std::mutex mutex_;
    const std::thread::id gui_owner_;
    std::thread::id module_owner_{};
    GuiModuleControlView view_{};
    std::uint64_t next_request_id_=1;
};
} // namespace slinkywinky
