#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_expander_geometry.hpp"
#include <atomic>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <thread>
#define CHECK(x) do {if(!(x))throw std::runtime_error("R98 mouse gate: " #x);}while(false)

using namespace slinkywinky;
namespace {
struct Point {int x,y;};
Point physical(const MenuPanelGeometry& panel,float logical_x,float logical_y) {
    return {static_cast<int>(panel.left+
                static_cast<unsigned>(std::lround(panel.scale*logical_x))),
            static_cast<int>(panel.top+
                static_cast<unsigned>(std::lround(panel.scale*logical_y)))};
}
void all_modes(MenuPanelMode mode) {
    MenuState model;
    MenuProfileBridge profile(model);
    MenuNavigation nav(model,profile);
    MenuRenderSession frames;
    CHECK(frames.attach(100,676,630));
    auto frame=frames.next(model,&profile,&nav);
    const auto panel=menu_panel_geometry(676,630,false,mode);
    CHECK(frame && panel);
    const float w=panel->logical_width();
    const Point toggle=physical(*panel,w-54.f,menu_first_row_y+21.f);
    const auto expRect=menu_expander_geometry(*frame,w,0);
    CHECK(expRect);
    const Point expand=physical(*panel,
        expRect->plus_x+10.0f,expRect->y+10.0f);
    const Point body=physical(*panel,60.f,menu_first_row_y+20.f);
    const Point nextrow=physical(*panel,60.f,menu_row_top(*frame,1)+20.f);
    const Point outside={static_cast<int>(panel->left)-1,toggle.y};
    MenuMouseGesture gate;
    CHECK(gate.owner_thread() && !gate.pending());
    CHECK(!gate.set_double_click_window(0));
    CHECK(!gate.set_double_click_window(5001));
    CHECK(gate.set_double_click_window(500));
    CHECK(!gate.press(*frame,mode,toggle.x,toggle.y,90,false));
    CHECK(!gate.pending());
    CHECK(!gate.press(*frame,mode,outside.x,outside.y,91,true));
    CHECK(!gate.release(*frame,mode,toggle.x,toggle.y,100,true));
    CHECK(gate.press(*frame,mode,toggle.x,toggle.y,110,true));
    CHECK(gate.pending());
    CHECK(!gate.press(*frame,mode,toggle.x,toggle.y,111,true)); // repeated down cancels
    CHECK(!gate.pending());
    CHECK(!gate.release(*frame,mode,toggle.x,toggle.y,120,true));

    CHECK(gate.press(*frame,mode,toggle.x,toggle.y,200,true));
    auto fresh=frames.next(model,&profile,&nav);
    CHECK(fresh && gate.move(*fresh,mode,toggle.x,toggle.y,true));
    const auto result=gate.release(*fresh,mode,toggle.x,toggle.y,230,true);
    CHECK(result && result->kind==MenuHitKind::Toggle && result->catalog_index==0);
    CHECK(!gate.pending());
    CHECK(!gate.release(*fresh,mode,toggle.x,toggle.y,231,true));
    CHECK(nav.focus_visible_row(result->catalog_index));
    CHECK(nav.apply(MenuNavAction::ToggleFocusedPreview)); // local only
    CHECK(profile.enabled(0));
    fresh=frames.next(model,&profile,&nav);
    CHECK(fresh);
    CHECK(gate.press(*fresh,mode,toggle.x,toggle.y,350,true));
    CHECK(!gate.release(*fresh,mode,toggle.x,toggle.y,360,true)); // double click denied
    CHECK(profile.enabled(0)); // no second toggle
    // The exact 500ms Windows double-click threshold must also reject.
    CHECK(gate.press(*fresh,mode,toggle.x,toggle.y,700,true));
    CHECK(!gate.release(*fresh,mode,toggle.x,toggle.y,730,true));
    CHECK(gate.press(*fresh,mode,toggle.x,toggle.y,800,true));
    const auto later=gate.release(*fresh,mode,toggle.x,toggle.y,820,true);
    CHECK(later && later->kind==MenuHitKind::Toggle); // after cooldown

    CHECK(gate.press(*fresh,mode,expand.x,expand.y,850,true));
    CHECK(!gate.move(*fresh,mode,body.x,body.y,true));
    CHECK(!gate.move(*fresh,mode,expand.x,expand.y,true)); // can't revive drag
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,780,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,800,true));
    CHECK(!gate.release(*fresh,mode,toggle.x,toggle.y,820,true)); // different target
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,900,true));
    CHECK(!gate.move(*fresh,mode,expand.x,expand.y,false)); // focus lost
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,950,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,1000,true));
    CHECK(gate.cancel()); // deactivation, capture loss, resize, mode change
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,1030,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,1090,true));
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,2700,true)); // over 1500ms
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,3000,true));
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,2999,true)); // time rollback
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,3200,true));
    CHECK(!gate.release(*fresh,mode,outside.x,outside.y,3230,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,3300,true));
    CHECK(!gate.release(*fresh,mode,expand.x,expand.y,3340,false));

    CHECK(gate.press(*fresh,mode,body.x,body.y,3500,true));
    CHECK(nav.focus_visible_row(1)); // model focus changes, visible frame unchanged
    CHECK(gate.release(*fresh,mode,body.x,body.y,3520,true));
    CHECK(gate.press(*fresh,mode,nextrow.x,nextrow.y,3900,true));
    auto otherTab=*fresh;otherTab.tab=MenuTab::Move;
    CHECK(!gate.release(otherTab,mode,nextrow.x,nextrow.y,3910,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,4200,true));
    auto reordered=*fresh;
    std::swap(reordered.visible[0],reordered.visible[1]);
    CHECK(!gate.release(reordered,mode,expand.x,expand.y,4230,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,4400,true));
    auto differentViewport=*fresh;
    differentViewport.viewport_width=844;
    CHECK(!gate.release(differentViewport,mode,expand.x,expand.y,4430,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,4600,true));
    auto differentAppearance=*fresh;
    differentAppearance.appearance.wide=true;
    CHECK(!gate.release(differentAppearance,mode,expand.x,expand.y,4630,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,4800,true));
    CHECK(!gate.release(*fresh,mode==MenuPanelMode::StandalonePreview?
                              MenuPanelMode::BoundedOverlay:
                              MenuPanelMode::StandalonePreview,
                        expand.x,expand.y,4830,true));
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,5000,true));
    auto older=*fresh;older.sequence=1;
    CHECK(!gate.release(older,mode,expand.x,expand.y,5030,true));

    // New frame from the same menu must stay accepted if data is unchanged.
    CHECK(gate.press(*fresh,mode,expand.x,expand.y,5200,true));
    const auto current=frames.next(model,&profile,&nav);
    CHECK(current && current->sequence>fresh->sequence);
    const auto expanded=gate.release(*current,mode,expand.x,expand.y,5220,true);
    CHECK(expanded && expanded->kind==MenuHitKind::Expand);
    CHECK(nav.focus_visible_row(expanded->catalog_index));
    CHECK(nav.apply(MenuNavAction::ExpandFocused)); // inert settings UI
    CHECK(model.expanded(0));
    CHECK(gate.press(*current,mode,body.x,body.y,5400,true));
    const auto changed=frames.next(model,&profile,&nav);
    CHECK(changed && !gate.release(*changed,mode,body.x,body.y,5420,true));
    CHECK(!gate.pending());

    // Unsupported rows: only inert RowBody; no settings actions may leak.
    const Point unsupported=physical(*panel,w-54.f,
        menu_row_top(*changed,3)+20.f);
    const auto unsupportedCard=menu_module_card_rect(*changed,w,3);
    const bool fullyVisible=unsupportedCard.y+unsupportedCard.height<=
        panel->logical_height()-menu_panel_padding;
    if(fullyVisible) {
        CHECK(gate.press(*changed,mode,unsupported.x,unsupported.y,5600,true));
        const auto noAction=gate.release(
            *changed,mode,unsupported.x,unsupported.y,5620,true);
        CHECK(noAction && noAction->kind==MenuHitKind::RowBody &&
              noAction->catalog_index==3);
        CHECK(nav.focus_visible_row(noAction->catalog_index));
        CHECK(!nav.apply(MenuNavAction::ToggleFocusedPreview));
    } else {
        // R152 places this row 1px past the compact panel's safe boundary.
        // A partially clipped card must not accept even a body click.
        CHECK(!hit_test_menu(*changed,mode,unsupported.x,unsupported.y));
        CHECK(!gate.press(
            *changed,mode,unsupported.x,unsupported.y,5600,true));
    }
    CHECK(!model.preview_enabled(3));

    // Wrong-thread calls cannot mutate or complete a click.
    CHECK(gate.press(*changed,mode,body.x,body.y,6000,true));
    std::atomic<bool> refused{false};
    std::thread other([&] {
        refused=!gate.cancel() && !gate.set_double_click_window(250) &&
                !gate.press(*changed,mode,body.x,body.y,6010,true) &&
                !gate.release(*changed,mode,body.x,body.y,6020,true) &&
                !gate.move(*changed,mode,body.x,body.y,true);
    });
    other.join();
    CHECK(refused && gate.pending());
    CHECK(gate.cancel());
    CHECK(!gate.pending());
    CHECK(gate.press(*changed,mode,body.x,body.y,6200,true));
    CHECK(gate.set_double_click_window(900)); // OS setting changed mid-press
    CHECK(!gate.pending());
    CHECK(!gate.release(*changed,mode,body.x,body.y,6240,true));
    CHECK(frames.detach());
}
}
int main() {
    try {
    all_modes(MenuPanelMode::StandalonePreview);
    all_modes(MenuPanelMode::BoundedOverlay);
    all_modes(MenuPanelMode::CompactOverlay);
    // R114: a changed read-only profile list invalidates held navigation
    // clicks. Clicking the four visual action icons is always refused.
    MenuState profileMenu;
    CHECK(profileMenu.select(MenuTab::Profiles));
    MenuRenderSession profileFrames;
    CHECK(profileFrames.attach(701,676,630));
    const auto profiles=profileFrames.next(profileMenu);
    CHECK(profiles && profiles->profile_cards.size()==1);
    MenuMouseGesture gate;
    CHECK(!gate.press(*profiles,MenuPanelMode::StandalonePreview,
                      303,88,90,true));
    CHECK(gate.press(*profiles,MenuPanelMode::StandalonePreview,
                     45,35,100,true));
    auto changed=*profiles;
    changed.sequence++;
    changed.profile_cards[0].description="Changed before release.";
    CHECK(!gate.release(changed,MenuPanelMode::StandalonePreview,
                        45,35,120,true));
    CHECK(!gate.pending());
    CHECK(profileFrames.detach());
    // R117: an in-flight pointer gesture is cancelled if a read-only
    // settings page changes, even when the physical target is still a
    // navigation tab and other frame fields stay unchanged.
    MenuState pageMenu;
    MenuProfileBridge pageProfile(pageMenu);
    MenuNavigation pageNav(pageMenu,pageProfile);
    CHECK(pageNav.apply(MenuNavAction::ExpandFocused));
    MenuRenderSession pageFrames;
    CHECK(pageFrames.attach(702,676,630));
    const auto beforePage=pageFrames.next(pageMenu,&pageProfile,&pageNav);
    CHECK(beforePage && beforePage->visible[0].settings_offset==0);
    MenuMouseGesture pageGate;
    CHECK(pageGate.press(*beforePage,MenuPanelMode::StandalonePreview,
                         40,30,200,true));
    CHECK(pageNav.apply(MenuNavAction::NextSettingsPage));
    const auto afterPage=pageFrames.next(pageMenu,&pageProfile,&pageNav);
    CHECK(afterPage && afterPage->visible[0].settings_offset==5);
    CHECK(!pageGate.release(*afterPage,MenuPanelMode::StandalonePreview,
                            40,30,215,true));
    CHECK(!pageGate.pending());
    CHECK(pageFrames.detach());
    // R123: a mode switch with the same module, card and page offset
    // still invalidates an old held click. Prevent stale GUI actions.
    MenuState modeMenu;
    MenuProfileBridge modeProfile(modeMenu);
    MenuNavigation modeNav(modeMenu,modeProfile);
    CHECK(modeNav.apply(MenuNavAction::ExpandFocused));
    MenuRenderSession modeFrames;
    CHECK(modeFrames.attach(703,900,630));
    const auto modeBefore=modeFrames.next(
        modeMenu,&modeProfile,&modeNav);
    CHECK(modeBefore && modeBefore->visible[0].settings_offset==0);
    MenuMouseGesture modeGate;
    CHECK(modeGate.press(*modeBefore,
        MenuPanelMode::StandalonePreview,40,30,300,true));
    CHECK(modeNav.apply(MenuNavAction::NextAimPreviewMode));
    const auto modeAfter=modeFrames.next(
        modeMenu,&modeProfile,&modeNav);
    CHECK(modeAfter && modeAfter->aim_preview_mode==1);
    CHECK(!modeGate.release(*modeAfter,
        MenuPanelMode::StandalonePreview,40,30,315,true));
    CHECK(!modeGate.pending());
    CHECK(modeGate.press(*modeAfter,
        MenuPanelMode::StandalonePreview,40,30,400,true));
    CHECK(modeNav.apply(MenuNavAction::NextTargetPreviewMode));
    const auto targetAfter=modeFrames.next(
        modeMenu,&modeProfile,&modeNav);
    CHECK(targetAfter && targetAfter->target_preview_mode==1);
    CHECK(!modeGate.release(*targetAfter,
        MenuPanelMode::StandalonePreview,40,30,420,true));
    CHECK(modeFrames.detach());
    return 0;
    } catch(const std::exception& e) {
        std::fprintf(stderr,"%s\n",e.what());
        return 1;
    }
}
