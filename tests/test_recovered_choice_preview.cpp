#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R131 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& all=recovered_widget_catalog();
    unsigned dropCount=0;
    for(const auto& widget:all)
        if(widget.kind==RecoveredWidgetKind::Dropdown)++dropCount;
    CHECK(dropCount==32u);
    CHECK(menu_source_choice_count(all.size())==0u);
    std::size_t pattern=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        const auto& row=all[i];
        if(std::string_view(row.module)=="Auto Clicker" &&
           std::string_view(row.label)=="Click pattern")
            pattern=i;
    }
    CHECK(pattern<all.size());
    CHECK(menu_source_choice_count(pattern)==2u);
    CHECK(menu_source_choice_label(pattern,0u)=="Jitter");
    CHECK(menu_source_choice_label(pattern,1u)=="Butterfly");
    CHECK(menu_source_choice_label(pattern,2u).empty());

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.focus_visible_row(2u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(131,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[2].catalog_index==2u &&
              frame->visible[2].expanded);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),2u);
        CHECK(card.y+card.height<
              panel->logical_height()-menu_panel_padding);
        const float controlX=card.x+card.width*0.54f;
        const float controlW=std::max(16.0f,card.width*0.46f-10.0f);
        const auto target=xy(controlX+controlW*0.5f,card.y+
            menu_row_body_height(*frame)+43.0f+2.0f+10.0f);
        const auto hit=hit_test_menu(
            *frame,mode,target.first,target.second);
        CHECK(hit && hit->kind==MenuHitKind::RecoveredChoiceControl &&
              hit->catalog_index==2u && hit->control_index==pattern);
        MenuMouseGesture gesture;
        CHECK(gesture.press(*frame,mode,target.first,target.second,
                            100,true));
        const auto opened=gesture.release(
            *frame,mode,target.first,target.second,120,true);
        CHECK(opened && opened->control_index==pattern);
        CHECK(menu.open_recovered_choice_preview(
            opened->catalog_index,opened->control_index));
        CHECK(menu.dropdown_preview() &&
              menu.dropdown_preview()->widget_index==pattern &&
              menu.dropdown_preview()->kind==
                  MenuDropdownKind::RecoveredChoice);
        const auto snap=frames.next(menu,&profile,&nav);
        const auto popup=menu_dropdown_popup_geometry(
            *snap,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->option_count==2u &&
              popup->selected==0u);
        CHECK(!menu.select_preview_dropdown_option(2u));
        CHECK(menu.dropdown_preview());
        const auto p=xy(popup->x+popup->width*0.5f,
                        popup->y+popup->row_height*1.5f);
        const auto popupHit=hit_test_menu(
            *snap,mode,p.first,p.second);
        CHECK(popupHit && popupHit->kind==MenuHitKind::DropdownOption &&
              popupHit->control_index==1u);
        // A popup cannot leak a click to the underlying card.
        const auto blocked=hit_test_menu(
            *snap,mode,target.first,target.second);
        CHECK(blocked && blocked->kind==MenuHitKind::DropdownOption &&
              blocked->control_index==0u);
        CHECK(gesture.press(*snap,mode,p.first,p.second,1000,true));
        const auto chosen=gesture.release(
            *snap,mode,p.first,p.second,1020,true);
        CHECK(chosen && chosen->kind==MenuHitKind::DropdownOption);
        CHECK(menu.select_preview_dropdown_option(
            static_cast<unsigned>(chosen->control_index)));
        CHECK(!menu.dropdown_preview());
        const auto updated=frames.next(menu,&profile,&nav);
        CHECK(updated && updated->recovered_choice_overrides[pattern] &&
              *updated->recovered_choice_overrides[pattern]==1u);
        CHECK(!menu.preview_enabled(2u) && !profile.enabled(2u));
        // One UI preview choice does not affect another module's settings.
        CHECK(menu.open_recovered_choice_preview(2u,pattern));
        const auto pressFrame=frames.next(menu,&profile,&nav);
        const auto openAgain=menu_dropdown_popup_geometry(
            *pressFrame,panel->logical_width(),panel->logical_height());
        CHECK(openAgain && openAgain->selected==1u);
        const auto firstChoice=xy(openAgain->x+openAgain->width*0.5f,
            openAgain->y+openAgain->row_height*0.5f);
        CHECK(gesture.press(*pressFrame,mode,
              firstChoice.first,firstChoice.second,2000,true));
        CHECK(menu.close_preview_dropdown());
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(!gesture.release(*stale,mode,
              firstChoice.first,firstChoice.second,2020,true));
        CHECK(stale->recovered_choice_overrides[pattern] &&
              *stale->recovered_choice_overrides[pattern]==1u);
        // The original Aim mode is still owned by its specialized path.
        CHECK(!menu.open_recovered_choice_preview(0u,pattern));
        CHECK(frames.detach());
    }
    return 0;
}
