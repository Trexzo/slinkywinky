#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <atomic>
#define CHECK(x) do {if(!(x))throw std::runtime_error("R97 hit test: " #x);}while(false)

using namespace slinkywinky;
namespace {
int pixel_x(const MenuPanelGeometry& p,float logical) {
    return static_cast<int>(p.left)+static_cast<int>(std::lround(p.scale*logical));
}
int pixel_y(const MenuPanelGeometry& p,float logical) {
    return static_cast<int>(p.top)+static_cast<int>(std::lround(p.scale*logical));
}
MenuHit exact_hit(const MenuRenderFrame& f,MenuPanelMode mode,
                  const MenuPanelGeometry& p,float x,float y,
                  MenuHitKind expected) {
    const auto hit=hit_test_menu(f,mode,pixel_x(p,x),pixel_y(p,y));
    CHECK(hit && hit->kind==expected);
    return *hit;
}
void panel_roundtrip(MenuPanelMode mode,unsigned width,unsigned height,
                     unsigned expected_left,unsigned expected_top,
                     unsigned expected_width,unsigned expected_height,
                     float expected_scale) {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuRenderSession session;
    CHECK(session.attach(21,width,height));
    const auto frame=session.next(menu,&profile);
    CHECK(frame.has_value());
    const auto p=menu_panel_geometry(width,height,false,mode);
    CHECK(p && p->left==expected_left && p->top==expected_top &&
          p->width==expected_width && p->height==expected_height &&
          std::fabs(p->scale-expected_scale)<0.0001f);
    const float w=p->logical_width();
    const float cell=menu_tab_width(w);
    const auto tab=exact_hit(*frame,mode,*p,menu_nav_x+4.5f*cell,35.f,MenuHitKind::Tab);
    CHECK(tab.tab==MenuTab::Utility);
    CHECK(!hit_test_menu(*frame,mode,
                         pixel_x(*p,menu_nav_x+6.5f*cell),pixel_y(*p,35.f)));
    CHECK(!hit_test_menu(*frame,mode,static_cast<int>(p->left)-1,
                         pixel_y(*p,175.f)));
    CHECK(!hit_test_menu(*frame,mode,
                         static_cast<int>(p->left+p->width),
                         pixel_y(*p,175.f)));
    CHECK(!hit_test_menu(*frame,mode,pixel_x(*p,50.f),
                         static_cast<int>(p->top)-1));
    CHECK(!hit_test_menu(*frame,mode,pixel_x(*p,50.f),
                         static_cast<int>(p->top+p->height)));
    const auto expand=exact_hit(*frame,mode,*p,(frame->appearance.compact?w-108.f:39.f),
                                menu_first_row_y+(frame->appearance.compact?20.f:43.f),
                                MenuHitKind::Expand);
    CHECK(expand.catalog_index==0 && expand.tab==MenuTab::Combat);
    const auto toggle=exact_hit(*frame,mode,*p,w-54.f,menu_first_row_y+20.f,
                                MenuHitKind::Toggle);
    CHECK(toggle.catalog_index==0);
    const auto body=exact_hit(*frame,mode,*p,60.f,menu_first_row_y+20.f,
                              MenuHitKind::RowBody);
    CHECK(body.catalog_index==0);
    // Unsupported catalog index 3: even on painted toggle/expand widgets
    // hit testing yields only inert focus and can never activate the module.
    const auto unsupported=exact_hit(*frame,mode,*p,w-54.f,menu_row_top(*frame,3)+20.f,
                                      MenuHitKind::RowBody);
    CHECK(unsupported.catalog_index==3);
    CHECK(!hit_test_menu(*frame,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+menu_row_body_height(*frame)+3.f)));
    CHECK(!hit_test_menu(*frame,mode,pixel_x(*p,60.f),pixel_y(*p,menu_row_top(*frame,7)+
                           menu_row_total_height(*frame,7)+20.f)));
    // Mismatched/forged render frame metadata is never actionable.
    auto forged=*frame;
    forged.visible[0].catalog_index=64;
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;
    forged.visible[0].title="FORGED";
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;forged.visible[0].settings_offset=5;
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,w-54.f),
                         pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;forged.visible[0].expanded=true;
    forged.visible[0].settings_offset=99999;
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,w-54.f),
                         pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;forged.sequence=0;
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;
    forged.profile_cards.push_back({"Default","Invalid non-profile card.","",true});
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,w-54.f),
                         pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;forged.visible[1]=forged.visible[0];
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
    forged=*frame;std::swap(forged.visible[0],forged.visible[1]);
    CHECK(!hit_test_menu(forged,mode,pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
    CHECK(session.detach());
}
}
int main() {
    try {
    CHECK(!menu_panel_geometry(319,630,false,MenuPanelMode::StandalonePreview));
    CHECK(!menu_panel_geometry(676,239,false,MenuPanelMode::CompactOverlay));
    CHECK(!menu_panel_geometry(8193,630,false,MenuPanelMode::BoundedOverlay));
    CHECK(!menu_panel_geometry(676,8193,false,MenuPanelMode::BoundedOverlay));
    CHECK(!menu_panel_geometry(676,630,false,static_cast<MenuPanelMode>(99)));
    panel_roundtrip(MenuPanelMode::StandalonePreview,676,630,0,0,676,630,1.0f);
    panel_roundtrip(MenuPanelMode::BoundedOverlay,676,630,12,12,652,606,1.0f);
    panel_roundtrip(MenuPanelMode::CompactOverlay,676,630,98,105,480,420,0.70f);
    panel_roundtrip(MenuPanelMode::CompactOverlay,1024,768,272,174,480,420,0.70f);
    // On a narrow viewport the compact row #4 is partially clipped.
    // Even its visible sliver must never become an actionable hit.
    {
        MenuState menu;
        MenuRenderSession session;
        CHECK(session.attach(5,320,240));
        const auto frame=session.next(menu);
        const auto p=menu_panel_geometry(320,240,false,MenuPanelMode::CompactOverlay);
        CHECK(frame && p && p->left==12 && p->top==12 &&
              p->width==296 && p->height==216);
        CHECK(hit_test_menu(*frame,MenuPanelMode::CompactOverlay,
                            pixel_x(*p,60.f),pixel_y(*p,menu_row_top(*frame,2)+20.f)));
        CHECK(!hit_test_menu(*frame,MenuPanelMode::CompactOverlay,
                             pixel_x(*p,60.f),pixel_y(*p,menu_row_top(*frame,3)+4.f)));
        CHECK(session.detach());
    }
    // New scroll pages hit their real catalog indices rather than stale
    // absolute row IDs; Profiles has no module rows.
    {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuRenderSession session;
        CHECK(session.attach(42,676,630));
        CHECK(profile.scroll_by(9));
        const auto frame=session.next(menu,&profile);
        const auto p=menu_panel_geometry(676,630,false,MenuPanelMode::CompactOverlay);
        CHECK(frame && p);
        const auto hit=hit_test_menu(*frame,MenuPanelMode::CompactOverlay,
                                    pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f));
        CHECK(hit && hit->kind==MenuHitKind::RowBody && hit->catalog_index==9);
        CHECK(profile.select(MenuTab::Profiles));
        const auto profiles=session.next(menu,&profile);
        CHECK(profiles && profiles->is_placeholder);
        CHECK(!hit_test_menu(*profiles,MenuPanelMode::CompactOverlay,
                             pixel_x(*p,60.f),pixel_y(*p,menu_first_row_y+20.f)));
        CHECK(session.detach());
    }
    // Consumer never reaches another thread's UI/model.
    {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        std::atomic<bool> rejected{false};
        std::thread foreign([&]{
            rejected=!nav.focus_visible_row(0) &&
                     !nav.apply(MenuNavAction::ToggleFocusedPreview);
        });
        foreign.join();
        CHECK(rejected && !profile.enabled(0));
    }
    return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
