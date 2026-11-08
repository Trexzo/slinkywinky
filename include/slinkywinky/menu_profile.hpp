#pragma once
#include "slinkywinky/menu.hpp"
#include "slinkywinky/settings.hpp"
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace slinkywinky {

// First-party configuration binding for modules already modeled by the C++
// engine. This is NOT a gameplay/injection bridge and does not send input.
bool module_has_profile_setting(std::size_t catalog_index) noexcept;

// One bounded, first-party numeric tuning control for each implemented
// module. Values are local profile data, not live gameplay commands.
struct MenuPrimaryParameter {
    const char* name;
    float value;
    float minimum;
    float maximum;
    float step;
    bool integral;
};

// Owner-thread menu controller: only selected, visible, implemented settings
// may change. Unsupported catalog entries remain off and cannot be enabled.
// The resulting ClientSettings are saved for later, separately admitted
// runtime integration; this controller does not activate any game module.
class MenuProfileBridge {
public:
    explicit MenuProfileBridge(MenuState& menu);
    bool select(MenuTab tab);
    bool scroll_by(int delta);
    bool scroll_by_visible(int delta,std::size_t visible_limit);
    bool toggle(std::size_t catalog_index);
    bool enabled(std::size_t catalog_index) const noexcept;
    // R155: exactly five source-matched slider controls now persist to
    // existing native ClientSettings. Unmapped controls remain GUI-only.
    // These settings do NOT execute in Minecraft until runtime admission.
    bool set_recovered_slider(std::size_t catalog_index,
                              std::size_t widget_index,float value);
    std::optional<float> recovered_slider_setting(
                              std::size_t widget_index) const noexcept;
    std::optional<MenuPrimaryParameter> primary(std::size_t catalog_index) const noexcept;
    // Only the currently visible, expanded module may be adjusted by one step.
    bool adjust_primary(std::size_t catalog_index, int direction);
    bool adopt(const ClientSettings& settings);
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path, std::string* error=nullptr) const;
    const ClientSettings& settings() const noexcept { return settings_; }
    bool owns(const MenuState& menu) const noexcept {return &menu_==&menu;}
private:
    bool* flag(std::size_t catalog_index) noexcept;
    const bool* flag(std::size_t catalog_index) const noexcept;
    void synchronize_visible();
    MenuState& menu_;
    ClientSettings settings_{};
};
} // namespace slinkywinky
