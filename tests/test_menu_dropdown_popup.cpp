#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R127 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    CHECK(menu_dropdown_option_count(MenuDropdownKind::AimMode)==4u);
    CHECK(menu_dropdown_option_count(MenuDropdownKind::TargetMode)==2u);
    CHECK(menu_dropdown_option_count(MenuDropdownKind::PiercingSort)==2u);
    CHECK(menu_dropdown_option_label(MenuDropdownKind::AimMode,2)=="Lock-on");
    CHECK(menu_dropdown_option_label(MenuDropdownKind::TargetMode,2).empty());
    CHECK(menu_dropdown_option_label(MenuDropdownKind::PiercingSort,1)=="Health");

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge bridge(menu);
        MenuNavigation nav(menu,bridge);
        CHECK(!menu.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(127,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto toClient=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        CHECK(menu.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        CHECK(!menu.open_preview_dropdown(MenuDropdownKind::PiercingSort,0u));
        auto snap=frames.next(menu,&bridge,&nav);
        CHECK(snap && snap->dropdown_preview &&
              snap->dropdown_preview->kind==MenuDropdownKind::AimMode);
        auto popup=menu_dropdown_popup_geometry(
            *snap,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->option_count==4u && popup->selected==0u);
        // All other modules and even the navigation bar are inert
        // while a dropdown owns pointer input.
        CHECK(!hit_test_menu(*snap,mode,
              toClient(25,20).first,toClient(25,20).second));
        const auto choice=toClient(popup->x+popup->width*0.5f,
                 popup->y+popup->row_height*2.0f+14.0f);
        const auto hit=hit_test_menu(*snap,mode,choice.first,choice.second);
        CHECK(hit && hit->kind==MenuHitKind::DropdownOption &&
              hit->catalog_index==0u && hit->control_index==2u);
        const auto off=toClient(popup->x-3.0f,popup->y+14.0f);
        CHECK(!hit_test_menu(*snap,mode,off.first,off.second));
        MenuMouseGesture gate;
        CHECK(!gate.press(*snap,mode,choice.first,choice.second,1,false));
        CHECK(gate.press(*snap,mode,choice.first,choice.second,100,true));
        const auto committed=gate.release(
             *snap,mode,choice.first,choice.second,120,true);
        CHECK(committed && committed->control_index==2u);
        CHECK(menu.select_preview_dropdown_option(
             static_cast<unsigned>(committed->control_index)));
        CHECK(!menu.dropdown_preview() &&
              menu.aim_preview_mode()==2u && menu.target_preview_mode()==0u);
        auto after=frames.next(menu,&bridge,&nav);
        CHECK(after && !after->dropdown_preview);
        // A held mouse press cannot survive the popup's disappearance.
        CHECK(menu.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        snap=frames.next(menu,&bridge,&nav);
        popup=menu_dropdown_popup_geometry(
            *snap,panel->logical_width(),panel->logical_height());
        CHECK(popup);
        const auto current=toClient(popup->x+popup->width*0.5f,
                                    popup->y+popup->row_height*1.5f);
        CHECK(gate.press(*snap,mode,current.first,current.second,1000,true));
        CHECK(menu.close_preview_dropdown());
        after=frames.next(menu,&bridge,&nav);
        CHECK(!gate.release(*after,mode,
               current.first,current.second,1020,true));
        CHECK(menu.aim_preview_mode()==2u);
        CHECK(!menu.close_preview_dropdown());

        // Target mode callsite moves with Aim's conditional branches.
        bool found=false;
        for(int page=0;page<6 && !found;++page) {
            const auto frame=frames.next(menu,&bridge,&nav);
            CHECK(frame);
            std::array<const RecoveredWidget*,5> pageItems{};
            CHECK(recovered_widget_visible_page(
                "Aim Assist",frame->visible[0].settings_offset,
                static_cast<int>(frame->aim_preview_mode),
                static_cast<int>(frame->target_preview_mode),
                pageItems)>0);
            for(std::size_t i=0;i<pageItems.size() && pageItems[i];++i) {
                if(std::string_view(pageItems[i]->group)!="Target" ||
                   std::string_view(pageItems[i]->label)!="Mode")
                    continue;
                const auto index=frame->visible[0].settings_offset+i;
                CHECK(menu.open_preview_dropdown(
                    MenuDropdownKind::TargetMode,index));
                const auto open=frames.next(menu,&bridge,&nav);
                const auto rect=menu_dropdown_popup_geometry(
                    *open,panel->logical_width(),panel->logical_height());
                CHECK(rect && rect->selected==0u);
                CHECK(!menu.select_preview_dropdown_option(2u));
                CHECK(menu.dropdown_preview());
                const auto option=toClient(rect->x+rect->width*0.5f,
                          rect->y+rect->row_height*1.5f);
                const auto targetHit=hit_test_menu(*open,mode,
                                                    option.first,option.second);
                CHECK(targetHit &&
                      targetHit->kind==MenuHitKind::DropdownOption &&
                      targetHit->control_index==1u);
                CHECK(menu.select_preview_dropdown_option(1u));
                CHECK(menu.target_preview_mode()==1u &&
                      menu.aim_preview_mode()==2u);
                found=true;
                break;
            }
            if(!found)CHECK(menu.change_settings_page(0,1));
        }
        CHECK(found);
        // No stale open popup survives a changed module tab.
        CHECK(menu.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        CHECK(menu.select(MenuTab::Move));
        CHECK(!menu.dropdown_preview());
        CHECK(!menu.select_preview_dropdown_option(0u));
        CHECK(frames.detach());

        // Piercing is a distinct source-backed 2-choice popup, never a
        // gameplay module. The seven boolean sandbox flags stay unchanged.
        MenuState piercing;
        MenuProfileBridge bp(piercing);
        MenuNavigation np(piercing,bp);
        CHECK(bp.scroll_by(9));
        CHECK(np.focus_visible_row(11));
        CHECK(np.apply(MenuNavAction::ExpandFocused));
        CHECK(piercing.open_preview_dropdown(
            MenuDropdownKind::PiercingSort,0u));
        const auto oldFlags=piercing.piercing_preview().enabled;
        MenuRenderSession fps;
        CHECK(fps.attach(128,900,630));
        const auto snapshot=fps.next(piercing,&bp,&np);
        const auto rect=menu_dropdown_popup_geometry(
            *snapshot,panel->logical_width(),panel->logical_height());
        CHECK(rect && rect->option_count==2u);
        const auto point=toClient(rect->x+rect->width*0.5f,
                                 rect->y+rect->row_height*1.5f);
        const auto selected=hit_test_menu(*snapshot,mode,point.first,point.second);
        CHECK(selected && selected->kind==MenuHitKind::DropdownOption &&
              selected->catalog_index==11u && selected->control_index==1u);
        CHECK(piercing.select_preview_dropdown_option(1u));
        CHECK(piercing.piercing_preview().sort==1u &&
              piercing.piercing_preview().enabled==oldFlags);
        CHECK(fps.detach());
    }
    return 0;
}
