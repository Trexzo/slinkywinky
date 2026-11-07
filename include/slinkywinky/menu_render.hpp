#pragma once
#include "slinkywinky/menu.hpp"
#include <cstdint>
#include <optional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace slinkywinky {
class MenuProfileBridge;
class MenuNavigation;

struct MenuRenderParameter {
    std::size_t catalog_index=0;
    std::string caption; // immutable, first-party ASCII formatted primary setting
    float value=0.0f;
    float minimum=0.0f;
    float maximum=0.0f;
    bool numeric=false; // true only for existing first-party typed control
};

// R114: strictly presentation-only profile names and descriptions. These are
// immutable snapshot values, NOT file paths or authorization to save/load.
// The real session supplies only the native Default card. Other cards may
// be used by the isolated screenshot fixture, never inferred from disk.
struct MenuProfileCard {
    std::string name;
    std::string description;
    std::string bind; // Empty or short printable display-only badge.
    bool is_default=false;
};
// R63: a first-party, renderer-neutral admission boundary.
// This never calls OpenGL, Minecraft, JNI, a loader, or gameplay input.
struct MenuRenderFrame {
    std::uint64_t epoch=0;
    std::uint64_t sequence=0;
    unsigned viewport_width=0;
    unsigned viewport_height=0;
    MenuTab tab=MenuTab::Combat;
    MenuAppearance appearance{};
    unsigned aim_preview_mode=0;
    unsigned target_preview_mode=0;
    PiercingPreview piercing_preview{}; // local GUI sandbox, no game
    std::optional<MenuDropdownPreview> dropdown_preview{}; // R127 snapshot
    bool unload_confirmation_preview=false; // R128 modal, no runtime unload
    std::array<std::optional<bool>,385> recovered_boolean_overrides{};
    // R130: changes only inert typed UI preview values.
    std::size_t piercing_selected=0;
    std::vector<MenuItem> visible;
    std::vector<MenuRenderParameter> parameters; // expanded visible rows only
    std::vector<MenuProfileCard> profile_cards; // inert, bounded render snapshot
    bool is_placeholder=false; // Profiles is informational; Unload not admitted.
};

class MenuRenderSession {
public:
    // Must be invoked on the actual graphics-context owner; epoch is unique
    // to the lifetime of that context, not a Minecraft game session.
    bool attach(std::uint64_t epoch,unsigned width,unsigned height) noexcept;
    bool resize(unsigned width,unsigned height) noexcept;
    std::optional<MenuRenderFrame> next(const MenuState& menu,
                                        const MenuProfileBridge* profile=nullptr);
    std::optional<MenuRenderFrame> next(const MenuState& menu,
                                        const MenuProfileBridge* profile,
                                        const MenuNavigation* navigation);
    // Cleanup is intentionally owner-thread only; a failed close leaves this
    // context locked until the host can perform the correct teardown.
    bool detach() noexcept;
    bool attached() const noexcept;
    std::uint64_t sequence() const noexcept;
private:
    static bool valid_viewport(unsigned width,unsigned height) noexcept;
    bool owner() const noexcept;
    mutable std::mutex mutex_;
    std::thread::id thread_{};
    std::uint64_t last_epoch_=0;
    bool active_=false;
    std::uint64_t epoch_=0;
    std::uint64_t sequence_=0;
    unsigned width_=0, height_=0;
};
} // namespace slinkywinky
