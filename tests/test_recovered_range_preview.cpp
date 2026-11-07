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
#include <string>
#include <string_view>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R133 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& all=recovered_widget_catalog();
    std::size_t rangeCount=0,rangeIndex=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        if(all[i].kind==RecoveredWidgetKind::Range) {
            ++rangeCount;
            CHECK(all[i].min_value<all[i].max_value &&
                  all[i].step>0.0f &&
                  all[i].example_value>=all[i].min_value &&
                  all[i].example_other<=all[i].max_value &&
                  all[i].example_value<=all[i].example_other);
        }
        if(std::string_view(all[i].module)=="Aim Assist" &&
           std::string_view(all[i].label)=="Range" &&
           all[i].kind==RecoveredWidgetKind::Range)
            rangeIndex=i;
    }
    CHECK(rangeCount==4u && rangeIndex<all.size());
    const auto& source=all[rangeIndex];
    CHECK(source.min_value==1.0f && source.max_value==8.0f);
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(133,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        std::optional<MenuRenderFrame> frame;
        bool onPage=false;
        for(int page=0;page<8 && !onPage;++page) {
            frame=frames.next(menu,&profile,&nav);
            CHECK(frame && frame->visible[0].catalog_index==0u);
            std::array<const RecoveredWidget*,5> widgets{};
            CHECK(recovered_widget_visible_page(
                "Aim Assist",frame->visible[0].settings_offset,
                static_cast<int>(frame->aim_preview_mode),
                static_cast<int>(frame->target_preview_mode),
                widgets)>0);
            for(const auto* widget:widgets)
                if(widget==&all[rangeIndex])onPage=true;
            if(!onPage)CHECK(menu.change_settings_page(0u,1));
        }
        CHECK(onPage && frame && frame->visible[0].expanded);
        const auto track=menu_numeric_track_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            0u,rangeIndex);
        CHECK(track && track->width>40.0f);
        const float span=source.max_value-source.min_value;
        const auto lower=xy(track->x+track->width*
            ((source.example_value-source.min_value)/span),
            track->y+track->height*0.5f);
        const auto upper=xy(track->x+track->width*
            ((source.example_other-source.min_value)/span),
            track->y+track->height*0.5f);
        const auto right=xy(track->x+track->width-2.0f,
                            track->y+track->height*0.5f);
        const auto left=xy(track->x+2.0f,
                           track->y+track->height*0.5f);
        const auto lowHit=hit_test_menu(
            *frame,mode,lower.first,lower.second);
        const auto highHit=hit_test_menu(
            *frame,mode,upper.first,upper.second);
        CHECK(lowHit && highHit &&
              lowHit->kind==MenuHitKind::RecoveredRangePreview &&
              highHit->kind==MenuHitKind::RecoveredRangePreview &&
              lowHit->control_index==rangeIndex &&
              highHit->control_index==rangeIndex &&
              lowHit->sub_index==0u && highHit->sub_index==1u);
        CHECK(!menu.set_recovered_range_preview(
            0u,rangeIndex,2u,4.0f));
        CHECK(!menu.set_recovered_range_preview(
            1u,rangeIndex,0u,4.0f));
        CHECK(!menu.set_recovered_range_preview(
            0u,all.size(),0u,4.0f));
        CHECK(!menu.set_recovered_range_preview(
            0u,rangeIndex,0u,std::numeric_limits<float>::quiet_NaN()));
        CHECK(!menu.set_recovered_range_preview(
            0u,rangeIndex,1u,source.max_value+1.0f));
        MenuMouseGesture gesture;
        CHECK(gesture.press(*frame,mode,lower.first,lower.second,100,true));
        // Mouse movement crosses to the region closer to the upper knob;
        // the lower knob must *remain owned* until release.
        CHECK(gesture.move(*frame,mode,right.first,right.second,true));
        const auto released=gesture.release(
            *frame,mode,right.first,right.second,220,true);
        CHECK(released && released->sub_index==0u &&
              released->control_index==rangeIndex);
        const float logicalRelease=static_cast<float>(
            right.first-static_cast<int>(panel->left))/panel->scale;
        const auto requested=menu_numeric_value_at_x(
            source,*track,logicalRelease);
        CHECK(requested);
        CHECK(menu.set_recovered_range_preview(
            0u,rangeIndex,released->sub_index,*requested));
        const auto first=menu.recovered_range_override(rangeIndex);
        CHECK(first && std::fabs(first->lower-source.example_other)<0.0001f &&
              std::fabs(first->upper-source.example_other)<0.0001f);
        // Reopen the range for a separate high-handle test; direct model
        // edits are still scoped to this exact page/widget and source step.
        CHECK(menu.set_recovered_range_preview(
            0u,rangeIndex,1u,source.max_value));
        const auto widened=menu.recovered_range_override(rangeIndex);
        CHECK(widened && widened->upper==source.max_value &&
              widened->lower<=widened->upper);
        const auto secondFrame=frames.next(menu,&profile,&nav);
        CHECK(secondFrame && secondFrame->recovered_range_overrides[rangeIndex]);
        const auto newestUpper=xy(track->x+track->width-2.0f,
            track->y+track->height*0.5f);
        CHECK(gesture.press(*secondFrame,mode,
              newestUpper.first,newestUpper.second,2000,true));
        CHECK(gesture.move(*secondFrame,mode,
              left.first,left.second,true));
        const auto upperDrag=gesture.release(
            *secondFrame,mode,left.first,left.second,2120,true);
        CHECK(upperDrag &&
              upperDrag->kind==MenuHitKind::RecoveredRangePreview &&
              upperDrag->sub_index==1u);
        const auto lowRequested=menu_numeric_value_at_x(
            source,*track,static_cast<float>(
                left.first-static_cast<int>(panel->left))/panel->scale);
        CHECK(lowRequested && menu.set_recovered_range_preview(
            0u,rangeIndex,upperDrag->sub_index,*lowRequested));
        const auto second=menu.recovered_range_override(rangeIndex);
        CHECK(second && second->upper==second->lower &&
              second->lower>=source.min_value);
        CHECK(!menu.preview_enabled(0u) && !profile.enabled(0u));
        // Model writes invalidate an in-flight range drag; no stale commit.
        const auto thirdFrame=frames.next(menu,&profile,&nav);
        CHECK(gesture.press(*thirdFrame,mode,left.first,left.second,
              3000,true));
        CHECK(menu.set_recovered_range_preview(
            0u,rangeIndex,1u,source.max_value));
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(!gesture.release(*stale,mode,
              right.first,right.second,3020,true));
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.set_recovered_range_preview(
            0u,rangeIndex,0u,source.min_value));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(menu.select(MenuTab::Move));
        CHECK(!menu.set_recovered_range_preview(
            0u,rangeIndex,1u,source.max_value));
        CHECK(frames.detach());
    }
    return 0;
}
