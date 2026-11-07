#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_slider_aliases.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R146 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& all=recovered_widget_catalog();
    std::optional<std::size_t> horizSilent,horizRegular;
    std::optional<std::size_t> vertSilent,vertRegular;
    std::size_t aliasCount=0;
    for(std::size_t i=0;i<all.size();++i) {
        if(!menu_is_aim_speed_branch(i))continue;
        ++aliasCount;
        const auto peer=menu_aim_speed_alias_peer(i);
        CHECK(peer && *peer!=i && *peer<all.size());
        CHECK(menu_aim_speed_alias_peer(*peer)==i);
        const auto& w=all[i];
        const bool silent=
            w.visibility==RecoveredWidgetVisibility::AimModeSilent;
        if(std::string_view(w.label)=="Horizontal speed") {
            if(silent)horizSilent=i;
            else horizRegular=i;
        } else if(std::string_view(w.label)=="Vertical speed") {
            if(silent)vertSilent=i;
            else vertRegular=i;
        } else CHECK(false);
    }
    CHECK(aliasCount==4u);
    CHECK(horizSilent && horizRegular && vertSilent && vertRegular);
    CHECK(menu_aim_speed_alias_peer(*horizSilent)==*horizRegular);
    CHECK(menu_aim_speed_alias_peer(*vertSilent)==*vertRegular);
    CHECK(menu_aim_speed_alias_peer(*horizSilent)!=*vertRegular);
    CHECK(!menu_aim_speed_alias_peer(all.size()));
    CHECK(!menu_is_aim_speed_branch(all.size()));
    std::size_t other=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        if(std::string_view(all[i].module)=="Auto Clicker" &&
           all[i].kind==RecoveredWidgetKind::Slider) {
            other=i;
            break;
        }
    }
    CHECK(other<all.size() && !menu_aim_speed_alias_peer(other));

    MenuState menu;
    MenuProfileBridge bridge(menu);
    MenuNavigation nav(menu,bridge);
    CHECK(nav.select_tab(MenuTab::Combat));
    CHECK(nav.focus_visible_row(0u));
    CHECK(nav.apply(MenuNavAction::ExpandFocused));
    MenuRenderSession render;
    CHECK(render.attach(146u,900u,630u));
    auto apply_visible=[&](std::size_t index,float value) {
        for(unsigned page=0;page<8u;++page) {
            if(menu.set_recovered_slider_preview(0u,index,value))
                return true;
            if(!menu.change_settings_page(0u,1))break;
        }
        return false;
    };

    CHECK(menu.set_aim_preview_mode(3u)); // Silent
    CHECK(apply_visible(*horizSilent,6.5f));
    CHECK(apply_visible(*vertSilent,3.0f));
    CHECK(menu.recovered_slider_override(*horizSilent)==6.5f);
    CHECK(menu.recovered_slider_override(*horizRegular)==6.5f);
    CHECK(menu.recovered_slider_override(*vertSilent)==3.0f);
    CHECK(menu.recovered_slider_override(*vertRegular)==3.0f);
    CHECK(!menu.recovered_slider_override(other));
    auto silentFrame=render.next(menu,&bridge,&nav);
    CHECK(silentFrame &&
          silentFrame->recovered_slider_overrides[*horizSilent]==6.5f &&
          silentFrame->recovered_slider_overrides[*horizRegular]==6.5f);

    CHECK(menu.set_aim_preview_mode(0u)); // Regular
    CHECK(apply_visible(*horizRegular,8.0f));
    CHECK(apply_visible(*vertRegular,4.5f));
    CHECK(menu.recovered_slider_override(*horizSilent)==8.0f);
    CHECK(menu.recovered_slider_override(*horizRegular)==8.0f);
    CHECK(menu.recovered_slider_override(*vertSilent)==4.5f);
    CHECK(menu.recovered_slider_override(*vertRegular)==4.5f);
    CHECK(!menu.recovered_slider_override(other));
    auto regularFrame=render.next(menu,&bridge,&nav);
    CHECK(regularFrame &&
          regularFrame->recovered_slider_overrides[*horizSilent]==8.0f &&
          regularFrame->recovered_slider_overrides[*vertRegular]==4.5f);

    CHECK(menu.set_aim_preview_mode(3u)); // Return to Silent
    CHECK(menu.recovered_slider_override(*horizSilent)==8.0f);
    CHECK(menu.recovered_slider_override(*vertSilent)==4.5f);
    CHECK(!menu.preview_enabled(0u));
    CHECK(!bridge.enabled(0u));
    CHECK(render.detach());
    return 0;
}
