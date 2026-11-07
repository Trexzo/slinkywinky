#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R135 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& widgets=recovered_widget_catalog();
    std::size_t teams=widgets.size();
    for(std::size_t i=0;i<widgets.size();++i)
        if(std::string_view(widgets[i].module)=="Teams" &&
           widgets[i].kind==RecoveredWidgetKind::MultiSelect)
            teams=i;
    CHECK(teams<widgets.size() &&
          menu_source_multi_count(teams)==16u);
    MenuState legacy;
    CHECK(legacy.select(MenuTab::Utility));
    CHECK(legacy.scroll_by(999));
    CHECK(legacy.scroll()==10u && legacy.page().size()==8u);
    CHECK(!legacy.scroll_by(1));
    CHECK(!legacy.scroll_by_visible(1,0u));
    CHECK(!legacy.scroll_by_visible(1,9u));
    CHECK(legacy.scroll_by_visible(999,3u));
    CHECK(legacy.scroll()==15u);
    CHECK(legacy.page().size()==3u);
    CHECK(legacy.page()[1].catalog_index==65u);
    CHECK(!legacy.scroll_by_visible(1,3u));
    CHECK(legacy.scroll_by_visible(-999,3u));
    CHECK(legacy.scroll()==0u);

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        const auto panel=menu_panel_geometry(900u,630u,false,mode);
        CHECK(panel);
        const bool oneColumn=menu_module_columns(
            panel->logical_width())==1u;
        CHECK(oneColumn==(mode!=MenuPanelMode::StandalonePreview));
        const std::size_t limit=oneColumn?3u:8u;
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation navigation(menu,profile);
        CHECK(navigation.select_tab(MenuTab::Utility));
        CHECK(profile.scroll_by_visible(999,limit));
        CHECK(menu.scroll()==(oneColumn?15u:10u));
        const auto rows=menu.page();
        CHECK(rows.size()==limit);
        const auto it=std::find_if(rows.begin(),rows.end(),
            [](const MenuItem& card){return card.catalog_index==65u;});
        CHECK(it!=rows.end());
        CHECK(navigation.focus_visible_row(65u));
        CHECK(navigation.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession session;
        CHECK(session.attach(135,900u,630u));
        const auto frame=session.next(menu,&profile,&navigation);
        CHECK(frame);
        const auto position=static_cast<std::size_t>(it-rows.begin());
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),position);
        CHECK(card.y+card.height<
              panel->logical_height()-menu_panel_padding);
        CHECK(menu.open_recovered_multi_preview(65u,teams));
        const auto opened=session.next(menu,&profile,&navigation);
        const auto popup=menu_dropdown_popup_geometry(
            *opened,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->total_options==16u &&
              popup->option_count==6u);
        CHECK(menu.scroll_preview_popup(1));
        CHECK(menu.dropdown_preview()->option_offset==1u);
        // A panel scroll cannot silently retain a stale popup.
        CHECK(profile.scroll_by_visible(-1,limit));
        CHECK(!menu.dropdown_preview());
        CHECK(menu.open_recovered_multi_preview(65u,teams));
        CHECK(menu.close_preview_dropdown());
        CHECK(session.detach());
    }
    return 0;
}
