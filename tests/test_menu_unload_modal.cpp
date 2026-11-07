#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_unload_modal_geometry.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R128 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        MenuRenderSession session;
        CHECK(session.attach(128,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto point=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=session.next(menu,&profile,&nav);
        CHECK(frame && !frame->unload_confirmation_preview);
        const auto tab=point(menu_nav_x+6.5f*menu_tab_width(
                              panel->logical_width()),menu_nav_y+21.0f);
        const auto target=hit_test_menu(*frame,mode,tab.first,tab.second);
        CHECK(target && target->kind==MenuHitKind::UnloadTab);
        CHECK(target->tab==MenuTab::Unload);
        MenuMouseGesture click;
        CHECK(click.press(*frame,mode,tab.first,tab.second,100,true));
        const auto admitted=click.release(*frame,mode,
                                          tab.first,tab.second,120,true);
        CHECK(admitted && admitted->kind==MenuHitKind::UnloadTab);
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(menu.unload_confirmation_preview());
        CHECK(!menu.open_unload_confirmation_preview());
        CHECK(menu.tab()==MenuTab::Combat); // modal is not a tab
        frame=session.next(menu,&profile,&nav);
        CHECK(frame && frame->unload_confirmation_preview);
        const auto modal=menu_unload_modal_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        CHECK(modal && modal->width<=600.0f);
        // No tab, module or background hit can pass through the modal.
        CHECK(!hit_test_menu(*frame,mode,tab.first,tab.second));
        const auto outside=point(10.0f,62.0f);
        CHECK(!hit_test_menu(*frame,mode,outside.first,outside.second));
        CHECK(!nav.apply(MenuNavAction::NextTab));
        CHECK(!nav.select_tab(MenuTab::Move));
        CHECK(!nav.focus_visible_row(0u));
        CHECK(!menu.select_preview_dropdown_option(0u));
        const auto x=point(modal->close_x+10.0f,modal->close_y+10.0f);
        auto xHit=hit_test_menu(*frame,mode,x.first,x.second);
        CHECK(xHit && xHit->kind==MenuHitKind::ModalClose);
        const auto okay=point(modal->continue_x+
                             modal->continue_width*0.5f,
                             modal->continue_y+10.0f);
        auto continueHit=hit_test_menu(*frame,mode,
                                      okay.first,okay.second);
        CHECK(continueHit && continueHit->kind==MenuHitKind::ModalContinue);
        // Drag between X and Continue cannot commit either action.
        CHECK(click.press(*frame,mode,x.first,x.second,1000,true));
        CHECK(!click.release(*frame,mode,
                             okay.first,okay.second,1020,true));
        CHECK(menu.unload_confirmation_preview());
        // A confirmed button click closes only this display model.
        CHECK(click.press(*frame,mode,okay.first,okay.second,2000,true));
        const auto confirm=click.release(
            *frame,mode,okay.first,okay.second,2020,true);
        CHECK(confirm && confirm->kind==MenuHitKind::ModalContinue);
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(!menu.unload_confirmation_preview());
        CHECK(menu.tab()==MenuTab::Combat);
        CHECK(!menu.close_unload_confirmation_preview());
        CHECK(nav.apply(MenuNavAction::NextTab));
        CHECK(menu.tab()==MenuTab::Move);
        CHECK(menu.open_unload_confirmation_preview());
        // A changed frame revokes a pending modal press.
        auto open=session.next(menu,&profile,&nav);
        const auto modal2=menu_unload_modal_geometry(
            *open,panel->logical_width(),panel->logical_height());
        CHECK(modal2);
        const auto close2=point(modal2->close_x+10.0f,
                                modal2->close_y+10.0f);
        CHECK(click.press(*open,mode,close2.first,close2.second,
                          3000,true));
        CHECK(menu.select(MenuTab::Visual));
        auto stale=session.next(menu,&profile,&nav);
        CHECK(!click.release(*stale,mode,
              close2.first,close2.second,3020,true));
        CHECK(!menu.unload_confirmation_preview());
        CHECK(session.detach());
    }
    return 0;
}
