#include "slinkywinky/menu_profile.hpp"
#include <algorithm>
#include <cmath>
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
    return index==0 || index==1 || index==2 || index==4 ||
           index==6 || index==12 || index==15;
}

MenuProfileBridge::MenuProfileBridge(MenuState& menu) : menu_(menu) {
    synchronize_visible();
}
bool* MenuProfileBridge::flag(std::size_t index) noexcept {
    return setting(settings_,index);
}
const bool* MenuProfileBridge::flag(std::size_t index) const noexcept {
    return setting(settings_,index);
}
void MenuProfileBridge::synchronize_visible() {
    // All page writes are preview flags. In particular nothing executes an
    // attack, changes client inputs or mutates any injected Minecraft state.
    for(const auto& item:menu_.page()) {
        const bool* stored=flag(item.catalog_index);
        const bool intended=stored!=nullptr && *stored;
        if(menu_.preview_enabled(item.catalog_index)!=intended)
            menu_.toggle_preview(item.catalog_index);
    }
}
bool MenuProfileBridge::select(MenuTab tab) {
    if(!menu_.select(tab))return false;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::scroll_by(int delta) {
    if(!menu_.scroll_by(delta))return false;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::scroll_by_visible(
        int delta,std::size_t visible_limit) {
    if(!menu_.scroll_by_visible(delta,visible_limit))return false;
    synchronize_visible();
    return true;
}
bool MenuProfileBridge::enabled(std::size_t index) const noexcept {
    const bool* stored=flag(index);
    return stored!=nullptr && *stored;
}
bool MenuProfileBridge::toggle(std::size_t index) {
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
std::optional<MenuPrimaryParameter> MenuProfileBridge::primary(
    std::size_t index) const noexcept {
    // Keep one intentionally limited, documented parameter per supported
    // module. This must remain a native profile editor, not a game action.
    const auto& c=settings_.combat;
    switch(index) {
        case 0: return MenuPrimaryParameter{"Range",c.aim_range,0.f,6.f,0.25f,false};
        case 1: return MenuPrimaryParameter{"Range",c.block_range,0.f,6.f,0.25f,false};
        case 2: return MenuPrimaryParameter{"Target CPS",c.clicks_per_second,1.f,30.f,0.5f,false};
        case 4: return MenuPrimaryParameter{"Maximum delay",static_cast<float>(settings_.backtrack_ms),0.f,1000.f,25.f,true};
        case 6: return MenuPrimaryParameter{"Min hurt",static_cast<float>(c.hit_select_min_hurt),0.f,10.f,1.f,true};
        case 12: return MenuPrimaryParameter{"Distance",c.reach.distance,0.f,6.f,0.1f,false};
        case 15: return MenuPrimaryParameter{"Velocity H",settings_.velocity.horizontal,0.f,1.f,0.05f,false};
        default: return std::nullopt;
    }
}
bool MenuProfileBridge::adjust_primary(std::size_t index,int direction) {
    if(direction!=1 && direction!=-1)return false;
    const auto parameter=primary(index);
    if(!parameter || menu_.tab()!=official_module_catalog()[index].tab ||
       !menu_.expanded(index))return false;
    const auto page=menu_.page();
    if(std::none_of(page.begin(),page.end(),
                    [index](const MenuItem& row){return row.catalog_index==index;}))
        return false;
    // Round to hundredths for stable UI steps (1/2, 1/4, 1/10, 1/20)
    // rather than accumulating floating-point drift in saved profiles.
    const float next=std::round((parameter->value+
        static_cast<float>(direction)*parameter->step)*100.f)/100.f;
    if(next<parameter->minimum || next>parameter->maximum ||
       std::fabs(next-parameter->value)<0.0001f)return false;
    ClientSettings updated=settings_;
    switch(index) {
        case 0: updated.combat.aim_range=next;break;
        case 1: updated.combat.block_range=next;break;
        case 2: updated.combat.clicks_per_second=next;break;
        case 4: updated.backtrack_ms=static_cast<unsigned>(next);break;
        case 6: updated.combat.hit_select_min_hurt=static_cast<int>(next);break;
        case 12: updated.combat.reach.distance=next;break;
        case 15: updated.velocity.horizontal=next;break;
        default:return false;
    }
    // Transactional strict schema validation, no partial UI or profile edit.
    return adopt(updated);
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
