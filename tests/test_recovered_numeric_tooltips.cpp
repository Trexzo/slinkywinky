#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_numeric_formats.hpp"
#include "slinkywinky/menu_numeric_tooltip.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do { if(!(x))throw std::runtime_error( \
    "R147 line "+std::to_string(__LINE__)+": " #x); } while(false)
using namespace slinkywinky;

int main() {
    try {
    CHECK(menu_numeric_formats.size()==115u);
    const auto a=menu_format_numeric_values("%.1f CPS",12.5f);
    CHECK(a && *a=="12.5 CPS");
    const auto b=menu_format_numeric_values("%.0f%%",80.0f);
    CHECK(b && *b=="80%");
    const auto c=menu_format_numeric_values("%0.2f blocks",3.125f);
    CHECK(c && *c=="3.12 blocks");
    const auto d=menu_format_numeric_values(
        "%0.1f - %0.1f blocks",3.0f,4.5f);
    CHECK(d && *d=="3.0 - 4.5 blocks");
    const auto e=menu_format_numeric_values("%.3fx",1.234f);
    CHECK(e && *e=="1.234x");
    CHECK(!menu_format_numeric_values("%.1f",5.0f,6.0f));
    CHECK(!menu_format_numeric_values("%.1f - %.1f",5.0f));
    CHECK(!menu_format_numeric_values("%n",5.0f));
    CHECK(!menu_format_numeric_values("%f",5.0f));
    CHECK(!menu_format_numeric_values("%5.1f",5.0f));
    CHECK(!menu_format_numeric_values("%.9f",5.0f));
    CHECK(!menu_format_numeric_values("%.1f",
        std::numeric_limits<float>::infinity()));
    CHECK(!menu_format_numeric_values("",5.0f));

    const auto& all=recovered_widget_catalog();
    std::size_t numeric=0u,covered=0u,uncovered=0u;
    std::size_t cpsIndex=all.size(),foreign=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        const auto& w=all[i];
        if(w.kind!=RecoveredWidgetKind::Slider &&
           w.kind!=RecoveredWidgetKind::Range) {
            CHECK(menu_numeric_format_for_widget(i).empty());
            continue;
        }
        ++numeric;
        const auto fmt=menu_numeric_format_for_widget(i);
        if(fmt.empty()) {
            ++uncovered;
            CHECK(std::string_view(w.module)=="Aim Assist");
            CHECK(std::string_view(w.group)=="Multi-point");
            CHECK(std::string_view(w.label)=="Horizontal" ||
                  std::string_view(w.label)=="Vertical");
        } else {
            ++covered;
            const auto caption=menu_format_numeric_values(
                fmt,w.example_value,
                w.kind==RecoveredWidgetKind::Range?
                   std::optional<float>(w.example_other):std::nullopt);
            CHECK(caption && !caption->empty());
        }
        if(std::string_view(w.module)=="Auto Clicker" &&
           std::string_view(w.label)=="Target CPS")
            cpsIndex=i;
        if(std::string_view(w.module)=="Reach" &&
           std::string_view(w.label)=="Distance")
            foreign=i;
    }
    CHECK(numeric==119u && covered==117u && uncovered==2u);
    CHECK(cpsIndex<all.size() && foreign<all.size());
    CHECK(menu_numeric_format_for_widget(cpsIndex)=="%0.1f CPS");
    CHECK(menu_numeric_format_for_widget(foreign)=="%0.2f blocks");
    CHECK(menu_numeric_format_for_widget(all.size()).empty());

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Combat));
        CHECK(nav.focus_visible_row(2u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(147u,900u,630u));
        const auto panel=menu_panel_geometry(900u,630u,false,mode);
        CHECK(panel);
        const auto pixel=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+
                    std::lround(x*panel->scale)),
                static_cast<int>(panel->top+
                    std::lround(y*panel->scale))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame);
        const auto track=menu_numeric_track_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            2u,cpsIndex);
        CHECK(track && track->width>20.0f);
        CHECK(!menu.numeric_hover());
        CHECK(!menu.set_numeric_hover(MenuNumericHover{2u,foreign}));
        CHECK(!menu.set_numeric_hover(MenuNumericHover{0u,cpsIndex}));
        CHECK(!menu.set_numeric_hover(MenuNumericHover{2u,all.size()}));
        CHECK(menu.set_numeric_hover(MenuNumericHover{2u,cpsIndex}));
        CHECK(!menu.set_numeric_hover(MenuNumericHover{2u,cpsIndex}));
        frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->numeric_hover &&
              frame->numeric_hover->widget_index==cpsIndex);
        const auto tip=menu_numeric_tooltip_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        CHECK(tip && tip->label=="12.0 CPS");
        CHECK(tip->height==20.0f && tip->width>35.0f);
        CHECK(tip->x>=8.0f && tip->x+tip->width<=
              panel->logical_width()-8.0f);
        CHECK(tip->y>=8.0f && tip->y<track->y);
        const auto xy=pixel(track->x+track->width*0.5f,
                            track->y+track->height*0.5f);
        const auto hoverHit=hit_test_menu(
            *frame,mode,xy.first,xy.second);
        CHECK(hoverHit &&
              hoverHit->kind==MenuHitKind::RecoveredSliderPreview &&
              hoverHit->control_index==cpsIndex);
        CHECK(profile.set_recovered_slider(2u,cpsIndex,14.0f));
        const auto changed=frames.next(menu,&profile,&nav);
        const auto changedTip=menu_numeric_tooltip_geometry(
            *changed,panel->logical_width(),panel->logical_height());
        CHECK(changedTip && changedTip->label=="14.0 CPS");
        CHECK(!menu.preview_enabled(2u) && !profile.enabled(2u));
        MenuMouseGesture gesture;
        CHECK(gesture.press(*changed,mode,xy.first,xy.second,
                            1000u,true));
        CHECK(menu.set_numeric_hover(std::nullopt));
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(stale && !stale->numeric_hover);
        CHECK(!gesture.release(*stale,mode,xy.first,xy.second,
                               1020u,true));
        CHECK(!menu_numeric_tooltip_geometry(
            *stale,panel->logical_width(),panel->logical_height()));
        CHECK(menu.set_numeric_hover(MenuNumericHover{2u,cpsIndex}));
        CHECK(nav.select_tab(MenuTab::Move));
        CHECK(!menu.numeric_hover());
        CHECK(!menu.set_numeric_hover(MenuNumericHover{2u,cpsIndex}));
        CHECK(frames.detach());
    }
    return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}
