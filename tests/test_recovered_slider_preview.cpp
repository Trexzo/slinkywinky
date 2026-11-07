#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R132 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    try {
    const auto& all=recovered_widget_catalog();
    std::size_t sliderCount=0,rangeCount=0;
    std::size_t target=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        const auto& widget=all[i];
        if(widget.kind==RecoveredWidgetKind::Slider) {
            ++sliderCount;
            CHECK(std::isfinite(widget.min_value) &&
                  std::isfinite(widget.max_value) &&
                  std::isfinite(widget.step) &&
                  widget.max_value>widget.min_value &&
                  widget.step>0.0f);
        }
        if(widget.kind==RecoveredWidgetKind::Range)++rangeCount;
        if(std::string_view(widget.module)=="Auto Clicker" &&
           std::string_view(widget.label)=="Target CPS" &&
           widget.kind==RecoveredWidgetKind::Slider)
            target=i;
    }
    CHECK(sliderCount==115u && rangeCount==4u && target<all.size());
    const auto& source=all[target];
    CHECK(source.min_value==1.0f && source.max_value==20.0f);
    CHECK(std::fabs(source.step-0.1f)<0.00001f);
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation navigation(menu,profile);
        CHECK(navigation.focus_visible_row(2u));
        CHECK(navigation.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession session;
        CHECK(session.attach(132,900u,630u));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto point=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=session.next(menu,&profile,&navigation);
        CHECK(frame && frame->visible[2].catalog_index==2u &&
              frame->visible[2].expanded);
        const auto track=menu_numeric_track_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            2u,target);
        CHECK(track && track->width>30.0f && track->height==20.0f);
        const auto start=point(track->x+2.0f,
                               track->y+track->height*0.5f);
        const auto halfway=point(track->x+track->width*0.5f,
                                 track->y+track->height*0.5f);
        const auto end=point(track->x+track->width-2.0f,
                             track->y+track->height*0.5f);
        const auto firstHit=hit_test_menu(
            *frame,mode,start.first,start.second);
        CHECK(firstHit &&
              firstHit->kind==MenuHitKind::RecoveredSliderPreview &&
              firstHit->catalog_index==2u &&
              firstHit->control_index==target);
        const auto label=point(track->x-50.0f,
                               track->y+track->height*0.5f);
        CHECK(!hit_test_menu(*frame,mode,label.first,label.second));
        // Invalid, wrong-module and non-numeric callsite edits are denied.
        CHECK(!menu.set_recovered_slider_preview(2u,all.size(),5.0f));
        CHECK(!menu.set_recovered_slider_preview(0u,target,5.0f));
        CHECK(!menu.set_recovered_slider_preview(
            2u,target,std::numeric_limits<float>::quiet_NaN()));
        CHECK(!menu.set_recovered_slider_preview(
            2u,target,std::numeric_limits<float>::infinity()));
        CHECK(!menu.set_recovered_slider_preview(2u,target,0.0f));
        CHECK(!menu.set_recovered_slider_preview(2u,target,25.0f));
        CHECK(!menu.recovered_slider_override(target));
        MenuMouseGesture gesture;
        CHECK(!gesture.press(*frame,mode,start.first,start.second,20,false));
        CHECK(gesture.press(*frame,mode,start.first,start.second,100,true));
        CHECK(gesture.move(*frame,mode,halfway.first,halfway.second,true));
        CHECK(gesture.move(*frame,mode,end.first,end.second,true));
        const auto release=gesture.release(
            *frame,mode,end.first,end.second,220,true);
        CHECK(release && release->kind==MenuHitKind::RecoveredSliderPreview &&
              release->control_index==target);
        const float logicalReleaseX=static_cast<float>(
            end.first-static_cast<int>(panel->left))/panel->scale;
        const auto selected=menu_slider_value_at_x(
            source,*track,logicalReleaseX);
        CHECK(selected && *selected>=source.min_value &&
              *selected<=source.max_value);
        CHECK(menu.set_recovered_slider_preview(2u,target,*selected));
        const auto updated=session.next(menu,&profile,&navigation);
        CHECK(updated && updated->recovered_slider_overrides[target]);
        const auto current=*updated->recovered_slider_overrides[target];
        CHECK(current>=source.min_value && current<=source.max_value);
        CHECK(std::fabs((current-source.min_value)/source.step-
              std::round((current-source.min_value)/source.step))<0.01f);
        CHECK(!menu.preview_enabled(2u) && !profile.enabled(2u));
        // Input ownership revokes a held slider after an independent edit.
        CHECK(gesture.press(*updated,mode,start.first,start.second,
                            1000,true));
        CHECK(menu.set_recovered_slider_preview(2u,target,12.34f));
        CHECK(menu.recovered_slider_override(target) &&
              std::fabs(*menu.recovered_slider_override(target)-12.3f)<0.0001f);
        const auto changed=session.next(menu,&profile,&navigation);
        CHECK(changed && !gesture.release(*changed,mode,
                         end.first,end.second,1020,true));
        // Page change removes the old widget's edit admission.
        CHECK(menu.change_settings_page(2u,1));
        CHECK(!menu.set_recovered_slider_preview(2u,target,10.0f));
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.set_recovered_slider_preview(2u,target,10.0f));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(session.detach());
    }
    // All pixel math is independent from the original native gameplay state.
    const MenuNumericTrack synthetic{10.0f,20.0f,200.0f,18.0f};
    const auto low=menu_slider_value_at_x(source,synthetic,-100.0f);
    const auto high=menu_slider_value_at_x(source,synthetic,400.0f);
    CHECK(low && *low==source.min_value);
    CHECK(high && *high==source.max_value);
    CHECK(!menu_slider_value_at_x(
        source,synthetic,std::numeric_limits<float>::infinity()));
    return 0;
    } catch(const std::exception& e) {
        std::fprintf(stderr,"%s\n",e.what());
        return 1;
    }
}
