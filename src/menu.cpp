#include "slinkywinky/menu.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace slinkywinky {
namespace {
// R115: exact category/name descriptors from the supplied candidate decrypted
// binary archive. This does not authenticate an official Slinky build.
#include "r115_recovered_catalog.inc"

bool is_module_tab(MenuTab tab) noexcept {
    return tab <= MenuTab::Utility;
}
} // namespace

const std::array<MenuModule,67>& official_module_catalog() noexcept {return catalog;}

const char* menu_tab_name(MenuTab tab) noexcept {
    switch(tab){
        case MenuTab::Combat:return "Combat";
        case MenuTab::Move:return "Move";
        case MenuTab::Visual:return "Visual";
        case MenuTab::Block:return "Block";
        case MenuTab::Utility:return "Utility";
        case MenuTab::Profiles:return "Profiles";
        case MenuTab::Unload:return "Unload";
        default:return "Unknown";
    }
}
std::size_t MenuState::tab_count(MenuTab tab) const noexcept {
    if(!is_module_tab(tab))return 0;
    return static_cast<std::size_t>(std::count_if(catalog.begin(),catalog.end(),
        [tab](const MenuModule& item){return item.tab==tab;}));
}
bool MenuState::select(MenuTab tab) noexcept {
    if(tab==MenuTab::Count || tab==MenuTab::Unload)return false;
    selected_=tab;
    scroll_=0;
    return true;
}
bool MenuState::scroll_by(int delta) noexcept {
    const auto total=tab_count(selected_);
    if(total==0)return false;
    // Renderer has eight visible rows. Keep scrolling independent of module
    // enable flags, with no overflow from large wheel deltas.
    const auto maxScroll=total>8?total-8:0;
    const std::int64_t proposed=static_cast<std::int64_t>(scroll_)+delta;
    const std::int64_t capped=std::max<std::int64_t>(0,
        std::min<std::int64_t>(static_cast<std::int64_t>(maxScroll),proposed));
    const auto next=static_cast<std::size_t>(capped);
    if(next==scroll_)return false;
    scroll_=next;
    return true;
}
bool MenuState::toggle_preview(std::size_t index) noexcept {
    if(index>=catalog.size() || catalog[index].tab!=selected_)return false;
    enabled_[index]=!enabled_[index];
    return true;
}
bool MenuState::toggle_expanded(std::size_t index) noexcept {
    if(index>=catalog.size() || catalog[index].tab!=selected_)return false;
    expanded_[index]=!expanded_[index];
    if(!expanded_[index])settings_offsets_[index]=0;
    return true;
}
std::size_t MenuState::settings_offset(std::size_t index) const noexcept {
    return index<settings_offsets_.size()?settings_offsets_[index]:0;
}
bool MenuState::change_settings_page(std::size_t index,int direction) noexcept {
    if((direction!=1&&direction!=-1) || index>=catalog.size() ||
       catalog[index].tab!=selected_ || !expanded_[index])return false;
    const std::size_t count=recovered_widget_count(catalog[index].name);
    if(count==0)return false;
    const std::size_t previous=settings_offsets_[index];
    // No arithmetic on untrusted or potentially overflowing offsets.
    if(direction<0) {
        if(previous<5)return false;
        settings_offsets_[index]-=5;
    } else {
        if(previous>=count || count-previous<=5)return false;
        settings_offsets_[index]+=5;
    }
    return true;
}
bool MenuState::preview_enabled(std::size_t index) const noexcept {
    return index<enabled_.size() && enabled_[index];
}
bool MenuState::expanded(std::size_t index) const noexcept {
    return index<expanded_.size() && expanded_[index];
}
bool MenuState::set_scale(float value) noexcept {
    if(!std::isfinite(value)||value<0.65f||value>1.8f)return false;
    appearance_.scale=value;
    return true;
}
bool MenuState::set_accent(std::uint32_t rgb) noexcept {
    if(rgb>0xffffffu)return false;
    appearance_.accent_rgb=rgb;
    return true;
}
std::vector<MenuItem> MenuState::page(std::size_t maximum) const {
    std::vector<MenuItem> rows;
    if(!is_module_tab(selected_))return rows;
    const auto cap=std::min<std::size_t>(8,maximum);
    rows.reserve(cap);
    std::size_t skipped=0;
    for(std::size_t i=0;i<catalog.size();++i) {
        if(catalog[i].tab!=selected_)continue;
        if(skipped++<scroll_)continue;
        if(rows.size()==cap)break;
        rows.push_back({i,catalog[i].name,enabled_[i],
                        expanded_[i],settings_offsets_[i]});
    }
    return rows;
}
} // namespace slinkywinky
