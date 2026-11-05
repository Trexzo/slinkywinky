#include "slinkywinky/menu_profile.hpp"
#include <algorithm>
#include <utility>

namespace slinkywinky {
namespace {
// Public module-catalog index, not an original Slinky implementation ID.
// These seven flags map 1:1 to existing native C++ settings fields.
bool* setting(ClientSettings& settings, std::size_t index) noexcept {
    switch(index) {
        case 0: return &settings.combat.aim_enabled;
        case 1: return &settings.combat.auto_block_enabled;
        case 2: return &settings.combat.auto_clicker_enabled;
        case 4: return &settings.backtrack_enabled;
        case 6: return &settings.combat.hit_select_enabled;
        case 12: return &settings.combat.reach_enabled;
        case 15: return &settings.velocity.enabled;
        default: return nullptr;
    }
}
const bool* setting(const ClientSettings& settings, std::size_t index) noexcept {
    switch(index) {
        case 0: return &settings.combat.aim_enabled;
        case 1: return &settings.combat.auto_block_enabled;
        case 2: return &settings.combat.auto_clicker_enabled;
        case 4: return &settings.backtrack_enabled;
        case 6: return &settings.combat.hit_select_enabled;
        case 12: return &settings.combat.reach_enabled;
        case 15: return &settings.velocity.enabled;
        default: return nullptr;
    }
}
} // namespace

bool module_has_profile_setting(std::size_t index) noexcept {
    ClientSettings placeholder;
    return setting(placeholder,index)!=nullptr;
}

MenuProfileBridge::MenuProfileBridge(MenuState& menu) noexcept : menu_(menu) {
    synchronize_visible();
}
bool* MenuProfileBridge::flag(std::size_t index) noexcept {
    return setting(settings_,index);
}
const bool* MenuProfileBridge::flag(std::size_t index) const noexcept {
    return setting(settings_,index);
}
void MenuProfileBridge::synchronize_visible() noexcept {
    // All page writes are preview flags. In particular nothing executes an
    // attack, changes client inputs or mutates any injected Minecraft state.
    for(const auto& item:menu_.page()) {
        const bool* stored=flag(item.catalog_index);
        const bool intended=stored!=nullptr && *stored;
        if(menu_.preview_enabled(item.catalog_index)!=intended)
            menu_.toggle_preview(item.catalog_index);
    }
}
bool MenuProfileBridge::select(MenuTab tab) noexcept {
    if(!menu_.select(tab))return false;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::scroll_by(int delta) noexcept {
    if(!menu_.scroll_by(delta))return false;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::enabled(std::size_t index) const noexcept {
    const bool* stored=flag(index);
    return stored!=nullptr && *stored;
}
bool MenuProfileBridge::toggle(std::size_t index) noexcept {
    bool* stored=flag(index);
    if(stored==nullptr || official_module_catalog()[index].tab!=menu_.tab())
        return false;
    // Off-screen indices must not be changed by stale hit-testing coordinates.
    const auto page=menu_.page();
    if(std::none_of(page.begin(),page.end(),
                    [index](const MenuItem& row){return row.catalog_index==index;}))
        return false;
    const bool next=!*stored;
    if(menu_.preview_enabled(index)!=next && !menu_.toggle_preview(index))
        return false;
    *stored=next;
    return true;
}
bool MenuProfileBridge::adopt(const ClientSettings& settings) {
    // Roundtrip through the existing strict parser; no partial import.
    const auto checked=parse_settings(serialize_settings(settings));
    if(!checked.valid)return false;
    settings_=checked.settings;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::load(const std::filesystem::path& path) {
    const auto loaded=load_settings(path);
    return loaded.valid && adopt(loaded.settings);
}
bool MenuProfileBridge::save(const std::filesystem::path& path,
                             std::string* error) const {
    return save_settings(path,settings_,error);
}
} // namespace slinkywinky
