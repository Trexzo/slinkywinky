#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R137 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    CHECK(menu_one_printable_utf8("A"));
    CHECK(menu_one_printable_utf8("!"));
    CHECK(menu_one_printable_utf8("\xC3\xA4")); // ä
    CHECK(menu_one_printable_utf8("\xF0\x9F\x98\x80")); // emoji
    CHECK(!menu_one_printable_utf8(""));
    CHECK(!menu_one_printable_utf8("\x00"));
    CHECK(!menu_one_printable_utf8("\x7F"));
    CHECK(!menu_one_printable_utf8("\xC0\xAF"));
    CHECK(!menu_one_printable_utf8("\xED\xA0\x80"));
    CHECK(!menu_one_printable_utf8("\xF4\x90\x80\x80"));
    CHECK(!menu_one_printable_utf8("ab"));
    const auto& widgets=recovered_widget_catalog();
    std::size_t fields=0,searchItem=widgets.size(),foreign=widgets.size();
    for(std::size_t i=0;i<widgets.size();++i) {
        if(widgets[i].kind!=RecoveredWidgetKind::Text)continue;
        ++fields;
        if(std::string_view(widgets[i].module)=="Auto Tool")
            searchItem=i;
        if(std::string_view(widgets[i].module)=="Friends")
            foreign=i;
    }
    CHECK(fields==6u && searchItem<widgets.size() &&
          foreign<widgets.size());
    CHECK(std::string_view(widgets[searchItem].auxiliary)=="Search item");
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Block));
        CHECK(nav.focus_visible_row(42u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(137,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[0].catalog_index==42u &&
              frame->visible[0].expanded);
        const auto field=menu_text_field_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            42u,searchItem);
        CHECK(field && field->height==22.0f && field->width>30.0f);
        const auto inside=xy(field->x+field->width*0.5f,field->y+11.0f);
        const auto label=xy(field->x-60.0f,field->y+11.0f);
        const auto hit=hit_test_menu(
            *frame,mode,inside.first,inside.second);
        CHECK(hit && hit->kind==MenuHitKind::RecoveredTextField &&
              hit->control_index==searchItem &&
              hit->catalog_index==42u);
        CHECK(!hit_test_menu(*frame,mode,label.first,label.second));
        CHECK(!menu.focus_recovered_text_preview(42u,foreign));
        CHECK(!menu.focus_recovered_text_preview(41u,searchItem));
        MenuMouseGesture gesture;
        CHECK(gesture.press(*frame,mode,
            inside.first,inside.second,100,true));
        const auto clicked=gesture.release(*frame,mode,
            inside.first,inside.second,120,true);
        CHECK(clicked && clicked->kind==MenuHitKind::RecoveredTextField);
        CHECK(menu.focus_recovered_text_preview(
            clicked->catalog_index,clicked->control_index));
        CHECK(menu.focused_text_widget()==searchItem);
        CHECK(menu.append_recovered_text_preview("a"));
        CHECK(menu.append_recovered_text_preview("B"));
        CHECK(menu.append_recovered_text_preview("\xC3\xA4"));
        CHECK(menu.append_recovered_text_preview("\xF0\x9F\x98\x80"));
        CHECK(!menu.append_recovered_text_preview("\n"));
        CHECK(!menu.append_recovered_text_preview("\xC0\xAF"));
        CHECK(menu.recovered_text_overrides()[searchItem] &&
              *menu.recovered_text_overrides()[searchItem]==
                  "aB\xC3\xA4\xF0\x9F\x98\x80");
        CHECK(menu.erase_recovered_text_preview());
        CHECK(menu.recovered_text_overrides()[searchItem] &&
              *menu.recovered_text_overrides()[searchItem]=="aB\xC3\xA4");
        CHECK(menu.erase_recovered_text_preview());
        CHECK(*menu.recovered_text_overrides()[searchItem]=="aB");
        const auto changed=frames.next(menu,&profile,&nav);
        CHECK(changed && changed->focused_text_widget==searchItem &&
              changed->recovered_text_overrides.size()==1u &&
              changed->recovered_text_overrides[0].widget_index==searchItem &&
              changed->recovered_text_overrides[0].value=="aB");
        CHECK(gesture.press(*changed,mode,
            inside.first,inside.second,1000,true));
        CHECK(menu.append_recovered_text_preview("x"));
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(!gesture.release(*stale,mode,
            inside.first,inside.second,1020,true));
        CHECK(!menu.preview_enabled(42u) && !profile.enabled(42u));
        CHECK(menu.blur_recovered_text_preview());
        CHECK(!menu.append_recovered_text_preview("x"));
        CHECK(menu.focus_recovered_text_preview(42u,searchItem));
        // The source fixture for Auto Tool spans six visible callsites;
        // moving away from page one must drop keyboard focus.
        CHECK(menu.change_settings_page(42u,1));
        CHECK(!menu.focused_text_widget());
        CHECK(!menu.focus_recovered_text_preview(42u,searchItem));
        CHECK(!menu.erase_recovered_text_preview());
        CHECK(menu.change_settings_page(42u,-1));
        CHECK(menu.focus_recovered_text_preview(42u,searchItem));
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.focused_text_widget());
        CHECK(!menu.focus_recovered_text_preview(42u,searchItem));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(menu.focus_recovered_text_preview(42u,searchItem));
        // Bound retained text to 128 UTF-8 bytes and never execute
        // a Friends or inventory operation as a side effect of editing.
        for(int k=0;k<125;++k)CHECK(menu.append_recovered_text_preview("z"));
        CHECK(!menu.append_recovered_text_preview("z"));
        CHECK(menu.recovered_text_overrides()[searchItem]->size()==128u);
        CHECK(menu.appearance().accent_rgb==0xF35D12u);
        CHECK(!profile.enabled(42u));
        CHECK(frames.detach());
    }
    return 0;
}
