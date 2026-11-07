#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
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
    bool toggle_preview(std::size_t catalog_index) noexcept;
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
    std::array<bool,67> enabled_{};
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
    std::array<std::optional<float>,385> recovered_slider_overrides_{};
    std::array<std::optional<RecoveredRangePreview>,385>
        recovered_range_overrides_{};
};

} // namespace slinkywinky
