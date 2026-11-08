#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace slinkywinky {

// R115 source-verified 67 module descriptors from the user-provided candidate
// decrypted archive. Listing a module does not implement game behavior.
enum class MenuTab : std::uint8_t {
    Combat, Move, Visual, Block, Utility, Profiles, Unload, Count
};

struct MenuModule {
    const char* name;
    MenuTab tab;
};

const std::array<MenuModule,67>& official_module_catalog() noexcept;
const char* menu_tab_name(MenuTab tab) noexcept;

struct MenuAppearance {
    // Independent of gameplay profiles by design. Defaults are first-party
    // design choices; only default orange and compact/wide names come from docs.
    bool compact=false;
    bool wide=false;
    bool allow_input_while_open=false;
    std::uint32_t accent_rgb=0xF35D12u;
    float scale=1.0f;
};

// GUI appearance is stored independently from ClientSettings/combat profiles.
std::string serialize_menu_appearance(const MenuAppearance& appearance);
bool parse_menu_appearance(const std::string& text, MenuAppearance& appearance) noexcept;
bool load_menu_appearance(const std::filesystem::path& path,MenuAppearance& result) noexcept;
bool save_menu_appearance(const std::filesystem::path& path,
                          const MenuAppearance& appearance) noexcept;

// R124: eight source-backed *display controls* for Piercing, all
// disconnected from ClientSettings, original Minecraft input and gameplay.
struct PiercingPreview {
    unsigned sort=0;  // 0 Hurt time, 1 Health
    std::array<bool,7> enabled{{false,false,false,false,false,false,true}};
};
bool operator==(const PiercingPreview& a,const PiercingPreview& b) noexcept;
std::string serialize_piercing_preview(const PiercingPreview& prefs);
bool parse_piercing_preview(const std::string& data,PiercingPreview& out) noexcept;
bool load_piercing_preview(const std::filesystem::path& file,
                          PiercingPreview& out) noexcept;
bool save_piercing_preview(const std::filesystem::path& file,
                          const PiercingPreview& prefs) noexcept;

// R127: a preview-only popup description; never a game/injector input route.
enum class MenuDropdownKind : std::uint8_t {
    AimMode, TargetMode, PiercingSort, RecoveredChoice, RecoveredMulti
};
struct MenuDropdownPreview {
    MenuDropdownKind kind=MenuDropdownKind::AimMode;
    std::size_t catalog_index=0;
    std::size_t control_index=0; // visible recovered callsite index
    std::size_t page_offset=0;   // immutable at time of opening
    std::size_t widget_index=0; // R131: source-global recovered choice
    unsigned option_offset=0; // R134: bounded option viewport
};
inline bool operator==(const MenuDropdownPreview& a,
                       const MenuDropdownPreview& b) noexcept {
    return a.kind==b.kind && a.catalog_index==b.catalog_index &&
           a.control_index==b.control_index &&
           a.page_offset==b.page_offset &&
           a.widget_index==b.widget_index &&
           a.option_offset==b.option_offset;
}
inline bool operator!=(const MenuDropdownPreview& a,
                       const MenuDropdownPreview& b) noexcept {
    return !(a==b);
}

// R133: two bounded source-fixture values, *only* for GUI previews.
struct RecoveredRangePreview {
    float lower=0.0f;
    float upper=0.0f;
};
inline bool operator==(const RecoveredRangePreview& a,
                       const RecoveredRangePreview& b) noexcept {
    return a.lower==b.lower && a.upper==b.upper;
}
inline bool operator!=(const RecoveredRangePreview& a,
                       const RecoveredRangePreview& b) noexcept {
    return !(a==b);
}

// R140: recovered binding chips are an inert, per-module GUI preview.
// codes are source-normalized Win32 VKs (or 0x103/0x104 for mouse 4/5).
// R142–R143: local reconstructed Profiles cards and modal state.
// No settings file, launcher, Minecraft or native profile mutation.
struct MenuLocalProfile {
    std::string name;
    std::string description;
    std::string bind;
};
enum class MenuProfileAction : std::uint8_t {
    Load, Save, OpenFolder, Delete
};
// R145: hover-only tooltip for the four Profiles icons, including
// protected Default actions. No action or filesystem permission implied.
// R147: renderer-neutral pointer hover for a *visible* numeric widget.
// This is a presentation tooltip only; the binding never changes gameplay.
// R148: transient native-preview drag. Its value is displayed while
// captured, but never written to stored GUI overrides or Minecraft state.
struct MenuNumericDragPreview {
    std::size_t catalog_index=0;
    std::size_t widget_index=0;
    unsigned handle=0;
    float lower=0.0f; // slider value or range lower endpoint
    float upper=0.0f; // only used by ranges
};
inline bool operator==(const MenuNumericDragPreview& a,
                       const MenuNumericDragPreview& b) noexcept {
    return a.catalog_index==b.catalog_index &&
           a.widget_index==b.widget_index && a.handle==b.handle &&
           a.lower==b.lower && a.upper==b.upper;
}
inline bool operator!=(const MenuNumericDragPreview& a,
                       const MenuNumericDragPreview& b) noexcept {
    return !(a==b);
}
struct MenuNumericHover {
    std::size_t catalog_index=0;
    std::size_t widget_index=0;
};
inline bool operator==(const MenuNumericHover& a,
                       const MenuNumericHover& b) noexcept {
    return a.catalog_index==b.catalog_index &&
           a.widget_index==b.widget_index;
}
inline bool operator!=(const MenuNumericHover& a,
                       const MenuNumericHover& b) noexcept {
    return !(a==b);
}

struct MenuProfileIconHover {
    std::size_t profile_index=0;
    unsigned action=0;
};
inline bool operator==(const MenuProfileIconHover& a,
                       const MenuProfileIconHover& b) noexcept {
    return a.profile_index==b.profile_index && a.action==b.action;
}
inline bool operator!=(const MenuProfileIconHover& a,
                       const MenuProfileIconHover& b) noexcept {
    return !(a==b);
}
struct MenuProfileActionPreview {
    std::size_t profile_index=0u;
    MenuProfileAction action=MenuProfileAction::Load;
};
inline bool operator==(const MenuProfileActionPreview& a,
                       const MenuProfileActionPreview& b) noexcept {
    return a.profile_index==b.profile_index && a.action==b.action;
}
inline bool operator!=(const MenuProfileActionPreview& a,
                       const MenuProfileActionPreview& b) noexcept {
    return !(a==b);
}

struct MenuBindPreview {
    std::uint16_t code=0u;
    bool hold=false;
};
inline bool operator==(const MenuBindPreview& a,
                       const MenuBindPreview& b) noexcept {
    return a.code==b.code && a.hold==b.hold;
}
inline bool operator!=(const MenuBindPreview& a,
                       const MenuBindPreview& b) noexcept {
    return !(a==b);
}

struct MenuItem {
    std::size_t catalog_index=0;
    std::string title;
    bool preview_enabled=false;
    bool expanded=false;
    std::size_t settings_offset=0; // read-only recovered control viewport start
};

// Deterministic, input-independent model for a future native game renderer.
// The preview_enabled bits are UI demo flags, NEVER gameplay module state.
class MenuState {
public:
    MenuTab tab() const noexcept {return selected_;}
    std::size_t scroll() const noexcept {return scroll_;}
    const MenuAppearance& appearance() const noexcept {return appearance_;}
    bool select(MenuTab tab) noexcept;
    bool scroll_by(int delta) noexcept;
    // R135: the bounded one-column preview can admit a smaller visible
    // window; this never changes enabled flags or gameplay state.
    bool scroll_by_visible(int delta,std::size_t visible_limit) noexcept;
    // Source-shaped Profiles tab, strictly a local GUI sandbox.
    const std::vector<MenuLocalProfile>& local_profiles() const noexcept {
        return local_profiles_;
    }
    const std::string& profile_name_draft() const noexcept {
        return profile_name_draft_;
    }
    bool profile_name_focused() const noexcept {
        return profile_name_focused_;
    }
    bool profile_name_taken() const noexcept {
        return profile_name_taken_;
    }
    bool focus_profile_name_preview() noexcept;
    bool blur_profile_name_preview() noexcept;
    bool append_profile_name_preview(std::string_view codepoint);
    bool erase_profile_name_preview() noexcept;
    bool create_profile_preview();
    // R144: profile row binding is a local display value, never an
    // active keyboard shortcut or persisted gameplay profile selection.
    bool begin_profile_bind_capture(std::size_t profile_index) noexcept;
    bool commit_profile_bind_capture(std::uint16_t code);
    bool cancel_profile_bind_capture() noexcept;
    std::optional<std::size_t> profile_bind_capture_index() const noexcept {
        return profile_bind_capture_index_;
    }
    std::optional<MenuProfileActionPreview> profile_action_preview()
        const noexcept { return profile_action_preview_; }
    bool set_profile_icon_hover(
        std::optional<MenuProfileIconHover> hover) noexcept;
    std::optional<MenuProfileIconHover> profile_icon_hover()
        const noexcept { return profile_icon_hover_; }
    bool begin_profile_action_preview(std::size_t profile_index,
                                      MenuProfileAction action) noexcept;
    bool cancel_profile_action_preview() noexcept;
    bool confirm_profile_action_preview() noexcept;

    bool toggle_preview(std::size_t catalog_index) noexcept;
    // R140: no global hooks, Java keybindings or client settings writes.
    bool begin_bind_capture(std::size_t catalog_index) noexcept;
    bool commit_bind_capture(std::uint16_t code) noexcept;
    bool cancel_bind_capture() noexcept;
    bool toggle_bind_hold(std::size_t catalog_index) noexcept;
    const std::array<MenuBindPreview,67>& bind_previews() const noexcept {
        return bind_previews_;
    }
    std::optional<std::size_t> bind_capture_index() const noexcept {
        return bind_capture_index_;
    }
    // R148: source-typed temporary drag, committed only through existing
    // R132/R133 guarded setters after a verified mouse-up.
    bool begin_numeric_drag_preview(std::size_t catalog_index,
                                    std::size_t widget_index,
                                    unsigned captured_handle) noexcept;
    bool move_numeric_drag_preview(float requested_value) noexcept;
    bool cancel_numeric_drag_preview() noexcept;
    std::optional<MenuNumericDragPreview> numeric_drag_preview()
        const noexcept {return numeric_drag_preview_;}
    bool set_numeric_hover(
        std::optional<MenuNumericHover> hover) noexcept;
    std::optional<MenuNumericHover> numeric_hover() const noexcept {
        return numeric_hover_;
    }
    bool set_bind_hover_row(
        std::optional<std::size_t> catalog_index) noexcept;
    std::optional<std::size_t> bind_hover_row() const noexcept {
        return bind_hover_row_;
    }
    bool toggle_expanded(std::size_t catalog_index) noexcept;
    bool change_settings_page(std::size_t catalog_index,int direction) noexcept;
    std::size_t settings_offset(std::size_t catalog_index) const noexcept;
    // Preview-only enums; no Minecraft input, gameplay or profile writes.
    unsigned aim_preview_mode() const noexcept {return aim_preview_mode_;}
    unsigned target_preview_mode() const noexcept {return target_preview_mode_;}
    bool set_aim_preview_mode(unsigned mode) noexcept;
    bool set_target_preview_mode(unsigned mode) noexcept;
    const PiercingPreview& piercing_preview() const noexcept {
        return piercing_preview_;
    }
    std::size_t piercing_selected() const noexcept {return piercing_selected_;}
    bool move_piercing_selection(int direction) noexcept;
    bool activate_piercing_selection() noexcept;
    // R125 click admission: exactly the clicked visible callsite. Never
    // changes page, gameplay flags or any other recovered module.
    bool select_piercing_control(std::size_t control) noexcept;
    const std::optional<MenuDropdownPreview>& dropdown_preview() const noexcept {
        return dropdown_preview_;
    }
    bool open_preview_dropdown(MenuDropdownKind kind,
                               std::size_t control_index) noexcept;
    bool open_recovered_choice_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept;
    bool open_recovered_multi_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept;
    bool scroll_preview_popup(int direction) noexcept;
    bool select_preview_dropdown_option(unsigned option) noexcept;
    bool close_preview_dropdown() noexcept;
    // R128: recovered seven-tab Unload confirmation is display-only.
    bool unload_confirmation_preview() const noexcept {
        return unload_confirmation_preview_;
    }
    bool open_unload_confirmation_preview() noexcept;
    bool close_unload_confirmation_preview() noexcept;
    // R130: edit only the source-indexed, currently visible preview Boolean.
    // Never toggles the gameplay module nor changes ClientSettings.
    bool toggle_recovered_boolean_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept;
    // Explicit-value owner-thread source widget setter; admission matches
    // toggle_recovered_boolean_preview and cannot bypass page guards.
    bool set_recovered_boolean_preview(
        std::size_t catalog_index,std::size_t widget_index,
        bool desired) noexcept;
    std::optional<bool> recovered_boolean_override(
        std::size_t widget_index) const noexcept;
    const std::array<std::optional<bool>,385>&
    recovered_boolean_overrides() const noexcept {
        return recovered_boolean_overrides_;
    }
    const std::array<std::optional<unsigned>,385>&
    recovered_choice_overrides() const noexcept {
        return recovered_choice_overrides_;
    }
    const std::array<std::optional<std::uint32_t>,385>&
    recovered_multi_overrides() const noexcept {
        return recovered_multi_overrides_;
    }
    // R136: independent 24-bit swatch edits, not MenuAppearance or game.
    bool set_recovered_color_preview(
        std::size_t catalog_index,std::size_t widget_index,
        std::uint32_t rgb) noexcept;
    std::optional<std::uint32_t> recovered_color_override(
        std::size_t widget_index) const noexcept;
    const std::array<std::optional<std::uint32_t>,385>&
    recovered_color_overrides() const noexcept {
        return recovered_color_overrides_;
    }
    // R137: inline text editing in first-party GUI preview only.
    // No Friends list or item registry mutations.
    bool focus_recovered_text_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept;
    bool blur_recovered_text_preview() noexcept;
    bool append_recovered_text_preview(std::string_view unicode_scalar);
    bool erase_recovered_text_preview() noexcept;
    std::optional<std::size_t> focused_text_widget() const noexcept {
        return focused_text_widget_;
    }
    const std::array<std::optional<std::string>,385>&
    recovered_text_overrides() const noexcept {
        return recovered_text_overrides_;
    }
    // R132: source-indexed, step-quantized preview sliders only.
    // Not copied into gameplay settings.
    bool set_recovered_slider_preview(std::size_t catalog_index,
                                      std::size_t widget_index,
                                      float requested_value) noexcept;
    std::optional<float> recovered_slider_override(
        std::size_t widget_index) const noexcept;
    const std::array<std::optional<float>,385>&
    recovered_slider_overrides() const noexcept {
        return recovered_slider_overrides_;
    }
    // R133: nearest-handle source-range preview, never ClientSettings.
    bool set_recovered_range_preview(std::size_t catalog_index,
                                     std::size_t widget_index,
                                     unsigned handle,
                                     float requested_value) noexcept;
    std::optional<RecoveredRangePreview> recovered_range_override(
        std::size_t widget_index) const noexcept;
    const std::array<std::optional<RecoveredRangePreview>,385>&
    recovered_range_overrides() const noexcept {
        return recovered_range_overrides_;
    }
    // Only for explicitly preview-only persistence; never changes gameplay.
    bool adopt_piercing_preview(const PiercingPreview& prefs) noexcept;
    bool preview_enabled(std::size_t catalog_index) const noexcept;
    bool expanded(std::size_t catalog_index) const noexcept;
    void set_compact(bool value) noexcept {appearance_.compact=value;}
    void set_wide(bool value) noexcept {appearance_.wide=value;}
    bool set_scale(float value) noexcept;
    bool set_accent(std::uint32_t rgb) noexcept;
    void set_appearance(const MenuAppearance& appearance) noexcept {
        appearance_=appearance;
    }
    void set_allow_input(bool value) noexcept {appearance_.allow_input_while_open=value;}
    std::size_t tab_count(MenuTab tab) const noexcept;
    std::vector<MenuItem> page(std::size_t maximum=8) const;
private:
    MenuTab selected_=MenuTab::Combat;
    std::size_t scroll_=0;
    MenuAppearance appearance_{};
    std::vector<MenuLocalProfile> local_profiles_{{
        "Default","Contains the default configuration.",""
    }};
    std::string profile_name_draft_{};
    bool profile_name_focused_=false;
    bool profile_name_taken_=false;
    std::optional<MenuProfileActionPreview> profile_action_preview_{};
    std::optional<MenuProfileIconHover> profile_icon_hover_{};
    std::optional<std::size_t> profile_bind_capture_index_{};
    std::array<bool,67> enabled_{};
    // Default examples mirror the supplied HTML transcription's st.binds,
    // not an authenticated native Slinky configuration.
    std::array<MenuBindPreview,67> bind_previews_=[] {
        std::array<MenuBindPreview,67> bindings{};
        bindings[0].code=0x52u;  // Aim Assist R
        bindings[2].code=0x103u; // Auto Clicker MOUSE4
        bindings[15].code=0x56u; // Velocity V
        bindings[32].code=0xA1u; // GUI RSHIFT
        bindings[53].code=0x43u; // Blink C
        bindings[53].hold=true;
        return bindings;
    }();
    std::optional<std::size_t> bind_capture_index_{};
    std::optional<std::size_t> bind_hover_row_{};
    std::optional<MenuNumericHover> numeric_hover_{};
    std::optional<MenuNumericDragPreview> numeric_drag_preview_{};
    std::array<bool,67> expanded_{};
    std::array<std::size_t,67> settings_offsets_{};
    unsigned aim_preview_mode_=0; // Regular, Linear, Lock-on, Silent
    unsigned target_preview_mode_=0; // Single, Switch
    PiercingPreview piercing_preview_{};
    std::size_t piercing_selected_=0; // 0 dropdown + 7 bools
    std::optional<MenuDropdownPreview> dropdown_preview_{};
    bool unload_confirmation_preview_=false;
    // Each optional maps to one immutable 385-entry recovered fixture record.
    // nullopt renders the unverified reference-example state; no game writes.
    std::array<std::optional<bool>,385> recovered_boolean_overrides_{};
    std::array<std::optional<unsigned>,385> recovered_choice_overrides_{};
    std::array<std::optional<std::uint32_t>,385> recovered_multi_overrides_{};
    std::array<std::optional<std::uint32_t>,385> recovered_color_overrides_{};
    std::array<std::optional<std::string>,385> recovered_text_overrides_{};
    std::optional<std::size_t> focused_text_widget_{};
    std::array<std::optional<float>,385> recovered_slider_overrides_{};
    std::array<std::optional<RecoveredRangePreview>,385>
        recovered_range_overrides_{};
};

} // namespace slinkywinky
