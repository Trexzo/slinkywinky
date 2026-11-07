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
    // R118: wheel paging is admitted only over a real, fully contained
    // expanded settings pane with more than five recovered controls. This
    // test uses only immutable snapshots; it cannot synthesize Windows
    // messages or touch original Minecraft input.
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuRenderSession session;
        const unsigned viewportW=1024,viewportH=768;
        CHECK(session.attach(118,viewportW,viewportH));
        CHECK(menu.toggle_expanded(0)); // Aim Assist: 24 callsites (some conditional)
        const auto frame=session.next(menu,&profile);
        const auto panel=menu_panel_geometry(
            viewportW,viewportH,false,mode);
        CHECK(frame && panel && frame->visible[0].expanded);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),0);
        const float settingY=card.y+menu_row_body_height(*frame)+16.0f;
        const int insideX=pixel_x(*panel,card.x+25.0f);
        const int insideY=pixel_y(*panel,settingY);
        const auto target=settings_page_wheel_target(
            *frame,mode,insideX,insideY);
        CHECK(target && *target==0);
        CHECK(!settings_page_wheel_target(*frame,mode,insideX,
            pixel_y(*panel,card.y+20.0f)));
        CHECK(!settings_page_wheel_target(*frame,mode,
            static_cast<int>(panel->left)-1,insideY));
        CHECK(!settings_page_wheel_target(*frame,mode,
            pixel_x(*panel,card.x+card.width+6.0f),insideY));
        CHECK(!settings_page_wheel_target(*frame,mode,insideX,
            pixel_y(*panel,card.y+card.height+5.0f)));
        auto altered=*frame;
        altered.visible[0].settings_offset=7; // forged unaligned page
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        altered=*frame;
        altered.aim_preview_mode=7; // forged invalid selection state
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        CHECK(!hit_test_menu(altered,mode,insideX,insideY));
        altered=*frame;
        altered.target_preview_mode=2;
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        altered=*frame;
        altered.piercing_preview.sort=2; // invalid sandbox state
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        CHECK(!hit_test_menu(altered,mode,insideX,insideY));
        altered=*frame;
        altered.piercing_selected=8; // invalid sandbox selection
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        altered=*frame;
        altered.visible[0].title="Forged page";
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        altered=*frame;
        altered.visible[1]=altered.visible[0]; // duplicate catalog item
        CHECK(!settings_page_wheel_target(altered,mode,insideX,insideY));
        CHECK(menu.toggle_expanded(0));
        auto collapsed=session.next(menu,&profile);
        CHECK(collapsed);
        CHECK(!settings_page_wheel_target(*collapsed,mode,insideX,insideY));
        CHECK(menu.toggle_expanded(3)); // two controls: no paging
        auto shortCard=session.next(menu,&profile);
        CHECK(shortCard);
        const auto item=menu_module_card_rect(
            *shortCard,panel->logical_width(),3);
        CHECK(!settings_page_wheel_target(*shortCard,mode,
            pixel_x(*panel,item.x+20.0f),
            pixel_y(*panel,item.y+menu_row_body_height(*shortCard)+12.0f)));
        CHECK(menu.select(MenuTab::Profiles));
        auto profiles=session.next(menu,&profile);
        CHECK(profiles);
        CHECK(!settings_page_wheel_target(*profiles,mode,insideX,insideY));
        CHECK(session.detach());
    }
    // R121: an Auto Clicker settings page crosses Inventory -> Conditions.
    // Its second section heading adds 20 logical pixels to the native card.
    // Renderer, wheel paging and pointer refusal must use that same height.
    {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuRenderSession session;
        CHECK(session.attach(121,900,630));
        CHECK(menu.toggle_expanded(2));  // Auto Clicker in sorted native catalog
        const auto first=session.next(menu,&profile);
        CHECK(first && first->visible[2].settings_offset==0);
        CHECK(menu_expanded_height(*first,2)==209.0f);
        CHECK(menu.change_settings_page(2,1));
        const auto second=session.next(menu,&profile);
        CHECK(second && second->visible[2].settings_offset==5);
        CHECK(menu_expanded_height(*second,2)==202.0f);
        const auto card=menu_module_card_rect(*second,900.0f,2);
        CHECK(card.height==menu_row_body_height(*second)+202.0f);
        CHECK(card.y+card.height<630.0f);
        const auto panel=menu_panel_geometry(
            900,630,false,MenuPanelMode::StandalonePreview);
        CHECK(panel);
        const int middleX=pixel_x(*panel,card.x+28.0f);
        const auto lower=settings_page_wheel_target(*second,
            MenuPanelMode::StandalonePreview,middleX,
            pixel_y(*panel,card.y+card.height-10.0f));
        CHECK(lower && *lower==2);
        // The section heading and its content are read-only.
        CHECK(!hit_test_menu(*second,MenuPanelMode::StandalonePreview,
            middleX,pixel_y(*panel,card.y+card.height-48.0f)));
        CHECK(!settings_page_wheel_target(*second,
            MenuPanelMode::StandalonePreview,middleX,
            pixel_y(*panel,card.y+card.height+2.0f)));
        CHECK(menu.change_settings_page(2,-1));
        CHECK(menu_expanded_height(*session.next(menu,&profile),2)==209.0f);
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
