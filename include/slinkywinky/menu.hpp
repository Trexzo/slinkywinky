#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
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
    bool toggle_preview(std::size_t catalog_index) noexcept;
    bool toggle_expanded(std::size_t catalog_index) noexcept;
    bool change_settings_page(std::size_t catalog_index,int direction) noexcept;
    std::size_t settings_offset(std::size_t catalog_index) const noexcept;
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
};

} // namespace slinkywinky
