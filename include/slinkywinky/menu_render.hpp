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
// R138: only the six source Text fields can have an entered value.
// Store edited values sparsely in render snapshots; a dense 385-slot array
// of std::optional<std::string> caused a Windows WGL test stack overflow.
struct MenuRenderTextValue {
    std::size_t widget_index=0;
    std::string value;
};
inline bool operator==(const MenuRenderTextValue& a,
                       const MenuRenderTextValue& b) noexcept {
    return a.widget_index==b.widget_index && a.value==b.value;
}
inline bool operator!=(const MenuRenderTextValue& a,
                       const MenuRenderTextValue& b) noexcept {
    return !(a==b);
}
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
    std::array<std::optional<unsigned>,385> recovered_choice_overrides{};
    // R131: local typed-choice selections are never gameplay settings.
    std::array<std::optional<std::uint32_t>,385> recovered_multi_overrides{};
    // R134: each bit represents one selected option, never game state.
    std::array<std::optional<std::uint32_t>,385> recovered_color_overrides{};
    // R136: independent RGB swatch values for native GL11 preview.
    std::vector<MenuRenderTextValue> recovered_text_overrides; // max six
    std::optional<std::size_t> focused_text_widget{};
    // R137: local editable UTF-8 text, never a gameplay action.
    std::array<std::optional<float>,385> recovered_slider_overrides{};
    // R132: inert source-indexed numeric slider display values.
    std::array<std::optional<RecoveredRangePreview>,385>
        recovered_range_overrides{}; // R133: independent dual-handle preview
    std::size_t piercing_selected=0;
    std::vector<MenuItem> visible;
    std::vector<MenuRenderParameter> parameters; // expanded visible rows only
    std::vector<MenuProfileCard> profile_cards; // inert, bounded render snapshot
    bool is_placeholder=false; // Profiles is informational; Unload not admitted.
};

// Guard against reintroducing a large dense string array in the
// immutable frame. The WGL fixture keeps several frame snapshots live
// in the same Windows 1 MB test thread stack.
static_assert(sizeof(MenuRenderFrame)<32768u,
    "MenuRenderFrame too large; use sparse text/metadata snapshots");

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
