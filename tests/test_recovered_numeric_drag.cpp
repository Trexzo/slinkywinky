#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_numeric_tooltip.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
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
    "R148 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    try {
    const auto& all=recovered_widget_catalog();
    std::size_t cps=all.size(),targetRange=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        if(std::string_view(all[i].module)=="Auto Clicker" &&
           std::string_view(all[i].label)=="Target CPS" &&
           all[i].kind==RecoveredWidgetKind::Slider)cps=i;
        if(std::string_view(all[i].module)=="Aim Assist" &&
           std::string_view(all[i].label)=="Range" &&
           all[i].kind==RecoveredWidgetKind::Range)targetRange=i;
    }
    CHECK(cps<all.size() && targetRange<all.size());
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState model;
        MenuProfileBridge profile(model);
        MenuNavigation nav(model,profile);
        CHECK(nav.focus_visible_row(2u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession session;
        CHECK(session.attach(148u,900u,630u));
        const auto panel=menu_panel_geometry(900u,630u,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=session.next(model,&profile,&nav);
        CHECK(frame);
        const auto track=menu_numeric_track_geometry(
            *frame,panel->logical_width(),panel->logical_height(),2u,cps);
        CHECK(track && track->width>30.0f);
        const auto start=xy(track->x+track->width*0.35f,
                            track->y+track->height*0.5f);
        const auto end=xy(track->x+track->width*0.8f,
                          track->y+track->height*0.5f);
        const auto down=hit_test_menu(
            *frame,mode,start.first,start.second);
        CHECK(down && down->kind==MenuHitKind::RecoveredSliderPreview &&
              down->control_index==cps);
        MenuMouseGesture gate;
        CHECK(gate.press(*frame,mode,start.first,start.second,
                         1000u,true));
        CHECK(!model.begin_numeric_drag_preview(0u,cps,0u));
        CHECK(!model.begin_numeric_drag_preview(2u,cps,1u));
        CHECK(!model.begin_numeric_drag_preview(2u,all.size(),0u));
        CHECK(model.begin_numeric_drag_preview(2u,cps,0u));
        CHECK(!model.begin_numeric_drag_preview(2u,cps,0u));
        const auto sample=menu_slider_value_at_x(
            all[cps],*track,track->x+track->width*0.8f);
        CHECK(sample);
        CHECK(!model.move_numeric_drag_preview(
            std::numeric_limits<float>::quiet_NaN()));
        CHECK(!model.move_numeric_drag_preview(
            all[cps].max_value+1.0f));
        CHECK(model.move_numeric_drag_preview(*sample));
        CHECK(!model.recovered_slider_override(cps));
        auto during=session.next(model,&profile,&nav);
        CHECK(during && during->numeric_drag_preview);
        CHECK(during->numeric_drag_preview->widget_index==cps);
        CHECK(during->numeric_drag_preview->handle==0u);
        CHECK(during->recovered_slider_overrides[cps] &&
              *during->recovered_slider_overrides[cps]==
                  profile.settings().combat.clicks_per_second);
        const auto tip=menu_numeric_tooltip_geometry(
            *during,panel->logical_width(),panel->logical_height());
        CHECK(tip && tip->label.find("CPS")!=std::string::npos);
        CHECK(gate.move(*during,mode,end.first,end.second,true));
        auto committed=gate.release(
            *during,mode,end.first,end.second,1110u,true);
        CHECK(committed && committed->control_index==cps &&
              committed->kind==MenuHitKind::RecoveredSliderPreview);
        CHECK(model.cancel_numeric_drag_preview());
        CHECK(!model.cancel_numeric_drag_preview());
        CHECK(!model.recovered_slider_override(cps)); // no early write
        const float physicalX=(static_cast<float>(end.first)-
            static_cast<float>(panel->left))/panel->scale;
        const auto releaseValue=menu_slider_value_at_x(
            all[cps],*track,physicalX);
        CHECK(releaseValue);
        CHECK(profile.set_recovered_slider(
            committed->catalog_index,committed->control_index,
            *releaseValue));
        const auto stored=model.recovered_slider_override(cps);
        CHECK(stored && std::fabs(*stored-*sample)<0.5f);
        // A canceled drag must not rewrite the accepted source value.
        frame=session.next(model,&profile,&nav);
        CHECK(frame && gate.press(*frame,mode,start.first,start.second,
                                   3000u,true));
        CHECK(model.begin_numeric_drag_preview(2u,cps,0u));
        CHECK(model.move_numeric_drag_preview(all[cps].min_value));
        CHECK(model.cancel_numeric_drag_preview());
        CHECK(gate.cancel());
        CHECK(model.recovered_slider_override(cps)==stored);
        CHECK(!model.numeric_drag_preview());
        // Page changes revoke drag admission and the source click owner.
        frame=session.next(model,&profile,&nav);
        CHECK(frame && gate.press(*frame,mode,start.first,start.second,
                                   5000u,true));
        CHECK(model.begin_numeric_drag_preview(2u,cps,0u));
        CHECK(model.change_settings_page(2u,1));
        CHECK(!model.numeric_drag_preview());
        auto paged=session.next(model,&profile,&nav);
        CHECK(paged && !gate.release(*paged,mode,
                   start.first,start.second,5050u,true));
        CHECK(model.recovered_slider_override(cps)==stored);
        // R148: captured numeric pointer may leave its thin track yet
        // remain inside the owned preview panel; it clamps to min and
        // accepts a deliberate 3-second drag. Ordinary buttons do not.
        CHECK(model.change_settings_page(2u,-1));
        frame=session.next(model,&profile,&nav);
        CHECK(frame && gate.press(*frame,mode,
             start.first,start.second,8000u,true));
        CHECK(model.begin_numeric_drag_preview(2u,cps,0u));
        const auto offTrack=xy(track->x-18.0f,
                               track->y+track->height*0.5f);
        auto captureFrame=session.next(model,&profile,&nav);
        CHECK(captureFrame && gate.move(*captureFrame,mode,
             offTrack.first,offTrack.second,true));
        const auto outsideValue=menu_slider_value_at_x(
             all[cps],*track,track->x-18.0f);
        CHECK(outsideValue && *outsideValue==all[cps].min_value);
        CHECK(model.move_numeric_drag_preview(*outsideValue));
        captureFrame=session.next(model,&profile,&nav);
        const auto capturedTooltip=menu_numeric_tooltip_geometry(
             *captureFrame,panel->logical_width(),panel->logical_height());
        CHECK(capturedTooltip);
        const auto releaseOutside=gate.release(*captureFrame,mode,
            offTrack.first,offTrack.second,11000u,true);
        CHECK(releaseOutside &&
              releaseOutside->kind==MenuHitKind::RecoveredSliderPreview &&
              releaseOutside->control_index==cps);
        CHECK(model.cancel_numeric_drag_preview());
        CHECK(model.recovered_slider_override(cps)==stored);
        CHECK(profile.set_recovered_slider(
            2u,cps,*outsideValue));
        CHECK(model.recovered_slider_override(cps)==all[cps].min_value);
        // A numeric release in the inert gap between tabs and cards
        // must retain its press owner even when hit_test_menu is null.
        frame=session.next(model,&profile,&nav);
        CHECK(frame && gate.press(*frame,mode,
             start.first,start.second,14000u,true));
        CHECK(model.begin_numeric_drag_preview(2u,cps,0u));
        const auto blank=xy(panel->logical_width()-20.0f,55.0f);
        auto noTarget=session.next(model,&profile,&nav);
        CHECK(noTarget &&
              !hit_test_menu(*noTarget,mode,blank.first,blank.second));
        CHECK(gate.move(*noTarget,mode,
                        blank.first,blank.second,true));
        const auto blankRelease=gate.release(*noTarget,mode,
            blank.first,blank.second,15100u,true);
        CHECK(blankRelease &&
              blankRelease->kind==MenuHitKind::RecoveredSliderPreview &&
              blankRelease->control_index==cps);
        CHECK(model.cancel_numeric_drag_preview());
        CHECK(model.recovered_slider_override(cps)==all[cps].min_value);
        CHECK(!profile.enabled(2u) && !model.preview_enabled(2u));
        CHECK(session.detach());

        MenuState ranged;
        MenuProfileBridge ranges(ranged);
        MenuNavigation rangeNav(ranged,ranges);
        CHECK(rangeNav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession rangeSession;
        CHECK(rangeSession.attach(149u,900u,630u));
        std::optional<MenuRenderFrame> rangeFrame;
        bool found=false;
        for(unsigned page=0;page<8u&&!found;++page) {
            rangeFrame=rangeSession.next(ranged,&ranges,&rangeNav);
            CHECK(rangeFrame);
            std::array<const RecoveredWidget*,5> visible{};
            recovered_widget_visible_page(
                "Aim Assist",rangeFrame->visible[0].settings_offset,
                static_cast<int>(rangeFrame->aim_preview_mode),
                static_cast<int>(rangeFrame->target_preview_mode),visible);
            for(const auto* widget:visible)
                if(widget==&all[targetRange])found=true;
            if(!found)CHECK(ranged.change_settings_page(0u,1));
        }
        CHECK(found && rangeFrame);
        const auto rangeTrack=menu_numeric_track_geometry(
            *rangeFrame,panel->logical_width(),panel->logical_height(),
            0u,targetRange);
        CHECK(rangeTrack);
        const auto& source=all[targetRange];
        const float ratio=(source.example_value-source.min_value)/
                           (source.max_value-source.min_value);
        const auto low=xy(rangeTrack->x+rangeTrack->width*ratio,
                          rangeTrack->y+rangeTrack->height*0.5f);
        const auto nearEnd=xy(rangeTrack->x+rangeTrack->width-2.0f,
                              rangeTrack->y+rangeTrack->height*0.5f);
        const auto hit=hit_test_menu(
            *rangeFrame,mode,low.first,low.second);
        CHECK(hit && hit->kind==MenuHitKind::RecoveredRangePreview &&
              hit->sub_index==0u);
        MenuMouseGesture rangeGate;
        CHECK(rangeGate.press(*rangeFrame,mode,low.first,low.second,
                              100u,true));
        CHECK(ranged.begin_numeric_drag_preview(
            0u,targetRange,hit->sub_index));
        CHECK(ranged.move_numeric_drag_preview(source.max_value));
        const auto drag=ranged.numeric_drag_preview();
        CHECK(drag && drag->handle==0u);
        CHECK(drag->lower==source.example_other &&
              drag->upper==source.example_other);
        CHECK(!ranged.recovered_range_override(targetRange));
        const auto rangeDuring=rangeSession.next(ranged,&ranges,&rangeNav);
        CHECK(rangeDuring && rangeDuring->numeric_drag_preview);
        const auto rangeTip=menu_numeric_tooltip_geometry(
            *rangeDuring,panel->logical_width(),panel->logical_height());
        CHECK(rangeTip && rangeTip->label=="4.5 - 4.5 blocks");
        CHECK(rangeGate.move(*rangeDuring,mode,
                             nearEnd.first,nearEnd.second,true));
        const auto rangeRelease=rangeGate.release(
            *rangeDuring,mode,nearEnd.first,nearEnd.second,230u,true);
        CHECK(rangeRelease && rangeRelease->sub_index==0u &&
              rangeRelease->control_index==targetRange);
        CHECK(ranged.cancel_numeric_drag_preview());
        CHECK(!ranged.recovered_range_override(targetRange));
        const float logical=(static_cast<float>(nearEnd.first)-
                             static_cast<float>(panel->left))/panel->scale;
        const auto value=menu_numeric_value_at_x(
            source,*rangeTrack,logical);
        CHECK(value && ranged.set_recovered_range_preview(
            0u,targetRange,rangeRelease->sub_index,*value));
        const auto storedRange=ranged.recovered_range_override(targetRange);
        CHECK(storedRange &&
              storedRange->lower==storedRange->upper);
        CHECK(!ranges.enabled(0u) && !ranged.preview_enabled(0u));
        CHECK(rangeSession.detach());
    }
    return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}
