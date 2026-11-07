#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R139 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& source=recovered_widget_catalog();
    std::size_t buttons=0;
    for(const auto& item:source)
        if(item.kind==RecoveredWidgetKind::Button) {
            CHECK(std::string_view(item.module)=="Friends");
            const std::string_view label(item.label);
            CHECK(label=="Add nearby players as friends" ||
                  label=="Remove all friends");
            ++buttons;
        }
    CHECK(buttons==2u);
    CHECK(std::string_view(official_module_catalog()[57].name)=="Friends");
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Utility));
        CHECK(profile.scroll_by_visible(8,3u));
        CHECK(menu.page().front().catalog_index==57u);
        CHECK(nav.focus_visible_row(57u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession render;
        CHECK(render.attach(139,900,630));
        const auto frame=render.next(menu,&profile,&nav);
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(frame && panel);
        CHECK(frame->visible.front().expanded);
        std::array<const RecoveredWidget*,5> items{};
        CHECK(recovered_widget_visible_page(
            "Friends",0,0,0,items)==4u);
        const auto rect=menu_module_card_rect(*frame,panel->logical_width(),0);
        CHECK(rect.y+rect.height<
              panel->logical_height()-menu_panel_padding);
        for(std::size_t j=0;j<items.size() && items[j];++j) {
            if(items[j]->kind!=RecoveredWidgetKind::Button)continue;
            CHECK(j==2u || j==3u);
            const float y=rect.y+menu_row_body_height(*frame)+
                43.0f+27.0f*static_cast<float>(j)+2.0f;
            const float x=rect.x+rect.width*0.5f;
            const int px=static_cast<int>(
                panel->left+std::lround(x*panel->scale));
            const int py=static_cast<int>(
                panel->top+std::lround((y+10.0f)*panel->scale));
            // Source HTML contains no click callbacks on either button.
            // Both are visibly styled in GL11 but have no input admission;
            // in particular the dangerous remove-all command is inert.
            CHECK(!hit_test_menu(*frame,mode,px,py));
        }
        CHECK(!menu.preview_enabled(57u));
        CHECK(!profile.enabled(57u));
        CHECK(!menu.focused_text_widget());
        CHECK(!menu.dropdown_preview());
        CHECK(render.detach());
    }
    return 0;
}
