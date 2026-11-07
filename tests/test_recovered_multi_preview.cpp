#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do { if(!(x)) throw std::runtime_error( \
    "R134 line " + std::to_string(__LINE__) + ": " #x); } while(false)
using namespace slinkywinky;

static std::size_t find_multi(std::string_view module) {
    const auto& all=recovered_widget_catalog();
    for(std::size_t i=0;i<all.size();++i)
        if(all[i].kind==RecoveredWidgetKind::MultiSelect &&
           std::string_view(all[i].module)==module)
            return i;
    return all.size();
}

int main() {
    const auto& all=recovered_widget_catalog();
    unsigned multiRecords=0;
    for(std::size_t i=0;i<all.size();++i) {
        if(all[i].kind!=RecoveredWidgetKind::MultiSelect)continue;
        ++multiRecords;
        const unsigned count=menu_source_multi_count(i);
        CHECK(count>=4u && count<=16u);
        CHECK(!menu_source_multi_label(i,count).size());
    }
    CHECK(multiRecords==7u);
    const auto small=find_multi("Auto Tool");
    const auto large=find_multi("Teams");
    CHECK(small<all.size() && large<all.size());
    CHECK(menu_source_multi_count(small)==4u);
    CHECK(menu_source_multi_count(large)==16u);
    CHECK(menu_source_multi_label(small,1u)=="Sword");
    CHECK(menu_source_multi_label(large,15u)=="White");
    CHECK(menu_source_multi_count(all.size())==0u);

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Block));
        CHECK(nav.focus_visible_row(42u)); // Auto Tool
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(134,900,630));
        CHECK(!menu.open_recovered_multi_preview(42u,all.size()));
        CHECK(!menu.open_recovered_multi_preview(41u,small));
        CHECK(menu.open_recovered_multi_preview(42u,small));
        CHECK(!menu.open_recovered_multi_preview(42u,small));
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->dropdown_preview &&
              frame->dropdown_preview->kind==MenuDropdownKind::RecoveredMulti);
        auto popup=menu_dropdown_popup_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->option_count==4u &&
              popup->total_options==4u && popup->selected_mask==0u);
        CHECK(!menu.scroll_preview_popup(1));
        CHECK(!menu.select_preview_dropdown_option(4u));
        CHECK(!hit_test_menu(*frame,mode,xy(15,15).first,
                                         xy(15,15).second));
        const auto second=xy(popup->x+popup->width*0.5f,
             popup->y+popup->row_height*1.5f);
        const auto hit=hit_test_menu(*frame,mode,
                                     second.first,second.second);
        CHECK(hit && hit->kind==MenuHitKind::DropdownOption &&
              hit->control_index==1u);
        MenuMouseGesture gesture;
        CHECK(!gesture.press(*frame,mode,
             second.first,second.second,100,false));
        CHECK(gesture.press(*frame,mode,
             second.first,second.second,200,true));
        const auto chosen=gesture.release(*frame,mode,
             second.first,second.second,220,true);
        CHECK(chosen && chosen->control_index==1u);
        CHECK(menu.select_preview_dropdown_option(
            static_cast<unsigned>(chosen->control_index)));
        CHECK(menu.dropdown_preview()); // multiselect stays open
        CHECK(menu.recovered_multi_overrides()[small].value_or(0u)==2u);
        auto changed=frames.next(menu,&profile,&nav);
        CHECK(changed && changed->dropdown_preview &&
              changed->recovered_multi_overrides[small].value_or(0u)==2u);
        const auto selectedRect=menu_dropdown_popup_geometry(
            *changed,panel->logical_width(),panel->logical_height());
        CHECK(selectedRect && selectedRect->selected_mask==2u);
        // A second option adds a bit without affecting the first option.
        CHECK(menu.select_preview_dropdown_option(3u));
        CHECK(menu.recovered_multi_overrides()[small].value_or(0u)==10u);
        CHECK(!profile.enabled(42u) && !menu.preview_enabled(42u));
        CHECK(menu.close_preview_dropdown());
        CHECK(menu.open_recovered_multi_preview(42u,small));
        CHECK(menu.select_preview_dropdown_option(1u));
        CHECK(menu.recovered_multi_overrides()[small].value_or(0u)==8u);
        CHECK(menu.close_preview_dropdown());
        CHECK(!menu.select_preview_dropdown_option(0u));

        // The 900x630 compact/bounded one-column panel clips the
        // seventh visible Utility card. Test the full sixteen-option
        // Teams popup in the original two-column 900px standalone panel;
        // all three modes were tested above on first-row Auto Tool.
        if(mode==MenuPanelMode::StandalonePreview) {
            // Teams has sixteen independently selectable colors. Only six
            // rows appear at a time, but wheel exposes the full option set.
            CHECK(nav.select_tab(MenuTab::Utility));
            CHECK(profile.scroll_by(10)); // 18 utility modules -> last 8
            CHECK(nav.focus_visible_row(65u)); // Teams
            CHECK(nav.apply(MenuNavAction::ExpandFocused));
            CHECK(!menu.open_recovered_multi_preview(42u,large));
            CHECK(menu.open_recovered_multi_preview(65u,large));
            frame=frames.next(menu,&profile,&nav);
            CHECK(frame && frame->dropdown_preview &&
                  frame->dropdown_preview->widget_index==large);
            popup=menu_dropdown_popup_geometry(
                *frame,panel->logical_width(),panel->logical_height());
            CHECK(popup && popup->total_options==16u &&
                  popup->option_count==6u && popup->first_option==0u);
            CHECK(!menu.select_preview_dropdown_option(15u));
            for(unsigned i=0;i<10u;++i)CHECK(menu.scroll_preview_popup(1));
            CHECK(!menu.scroll_preview_popup(1));
            CHECK(menu.dropdown_preview()->option_offset==10u);
            changed=frames.next(menu,&profile,&nav);
            popup=menu_dropdown_popup_geometry(
                *changed,panel->logical_width(),panel->logical_height());
            CHECK(popup && popup->first_option==10u &&
                  popup->option_count==6u);
            const auto white=xy(popup->x+popup->width*0.5f,
                popup->y+popup->row_height*5.5f);
            const auto whiteHit=hit_test_menu(*changed,mode,
                                              white.first,white.second);
            CHECK(whiteHit && whiteHit->kind==MenuHitKind::DropdownOption &&
                  whiteHit->control_index==15u);
            CHECK(gesture.press(*changed,mode,white.first,white.second,
                                2000,true));
            const auto committed=gesture.release(
                *changed,mode,white.first,white.second,2020,true);
            CHECK(committed && committed->control_index==15u);
            CHECK(menu.select_preview_dropdown_option(15u));
            CHECK(menu.dropdown_preview());
            CHECK(menu.recovered_multi_overrides()[large].value_or(0u)==
                  (1u<<15u));
            CHECK(menu.scroll_preview_popup(-1));
            CHECK(menu.dropdown_preview()->option_offset==9u);
            CHECK(!menu.select_preview_dropdown_option(0u));
            // A popup scroll invalidates the same held click's frame.
            auto held=frames.next(menu,&profile,&nav);
            const auto heldRect=menu_dropdown_popup_geometry(
                *held,panel->logical_width(),panel->logical_height());
            CHECK(heldRect);
            const auto heldPos=xy(heldRect->x+heldRect->width*0.5f,
                heldRect->y+heldRect->row_height*2.5f);
            CHECK(gesture.press(*held,mode,
                                heldPos.first,heldPos.second,3000,true));
            CHECK(menu.scroll_preview_popup(-1));
            const auto stale=frames.next(menu,&profile,&nav);
            CHECK(!gesture.release(*stale,mode,
                  heldPos.first,heldPos.second,3020,true));
            CHECK(menu.recovered_multi_overrides()[large].value_or(0u)==
                  (1u<<15u));
            CHECK(menu.close_preview_dropdown());
            CHECK(menu.open_unload_confirmation_preview());
            CHECK(!menu.open_recovered_multi_preview(65u,large));
            CHECK(menu.close_unload_confirmation_preview());
        }
        CHECK(frames.detach());
    }
    // R131 regression: a ten-option *single-choice* dropdown also uses
    // the bounded popup viewport. The last option must stay reachable.
    {
        std::size_t slot1=all.size();
        for(std::size_t i=0;i<all.size();++i)
            if(std::string_view(all[i].module)=="Inv Manager" &&
               std::string_view(all[i].label)=="Slot 1" &&
               all[i].kind==RecoveredWidgetKind::Dropdown)
                slot1=i;
        CHECK(slot1<all.size() && menu_source_choice_count(slot1)==10u);
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Utility));
        CHECK(profile.scroll_by(9));
        CHECK(nav.focus_visible_row(58u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        CHECK(menu.change_settings_page(58u,1));
        MenuRenderSession frames;
        CHECK(frames.attach(1349,900,630));
        CHECK(menu.open_recovered_choice_preview(58u,slot1));
        const auto frame=frames.next(menu,&profile,&nav);
        const auto panel=menu_panel_geometry(
            900,630,false,MenuPanelMode::StandalonePreview);
        CHECK(frame && panel);
        auto popup=menu_dropdown_popup_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->total_options==10u &&
              popup->option_count==6u && popup->first_option==0u);
        CHECK(!menu.select_preview_dropdown_option(9u));
        for(unsigned i=0;i<4u;++i)CHECK(menu.scroll_preview_popup(1));
        CHECK(!menu.scroll_preview_popup(1));
        const auto shifted=frames.next(menu,&profile,&nav);
        popup=menu_dropdown_popup_geometry(
            *shifted,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->first_option==4u);
        const auto coord=std::pair<int,int>{
            static_cast<int>(std::lround(popup->x+popup->width*0.5f)),
            static_cast<int>(std::lround(
                popup->y+popup->row_height*5.5f))
        };
        const auto hit=hit_test_menu(
            *shifted,MenuPanelMode::StandalonePreview,
            coord.first,coord.second);
        CHECK(hit && hit->kind==MenuHitKind::DropdownOption &&
              hit->control_index==9u);
        CHECK(menu.select_preview_dropdown_option(9u));
        CHECK(!menu.dropdown_preview());
        CHECK(menu.recovered_choice_overrides()[slot1].value_or(99u)==9u);
        CHECK(!profile.enabled(58u));
        CHECK(frames.detach());
    }
    return 0;
}
