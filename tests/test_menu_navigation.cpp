#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include <atomic>
#include <cmath>
#include <stdexcept>
#include <thread>
#define CHECK(x) do {if(!(x))throw std::runtime_error("R96 menu navigation: " #x);}while(false)

using namespace slinkywinky;
int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation input(menu,profile);
    CHECK(input.owns(menu,profile));
    CHECK(input.visible_focus() && *input.visible_focus()==0);
    CHECK(!input.apply(MenuNavAction::PreviousRow)); // No wrap.
    CHECK(!input.select_tab(MenuTab::Unload));
    CHECK(!input.select_tab(MenuTab::Count));
    CHECK(!input.focus_visible_row(64));
    CHECK(input.apply(MenuNavAction::NextRow));
    CHECK(*input.visible_focus()==1);
    CHECK(input.focus_visible_row(0));
    CHECK(*input.visible_focus()==0);
    CHECK(input.focus_visible_row(1));
    const auto initial=profile.primary(1);
    CHECK(initial.has_value());
    CHECK(!input.apply(MenuNavAction::IncreaseSetting)); // Not expanded.
    CHECK(input.apply(MenuNavAction::ExpandFocused));
    CHECK(menu.expanded(1));
    CHECK(input.apply(MenuNavAction::IncreaseSetting));
    CHECK(std::fabs(profile.primary(1)->value -
                    (initial->value+initial->step))<0.001f);
    CHECK(input.apply(MenuNavAction::ToggleFocusedPreview));
    CHECK(profile.enabled(1) && menu.preview_enabled(1));
    // A second toggle is only local profile state; no JVM is available.
    CHECK(input.apply(MenuNavAction::ToggleFocusedPreview));
    CHECK(!profile.enabled(1));
    CHECK(input.apply(MenuNavAction::NextRow));
    CHECK(*input.visible_focus()==2);
    CHECK(input.apply(MenuNavAction::NextRow));
    CHECK(*input.visible_focus()==3);
    CHECK(!input.apply(MenuNavAction::ToggleFocusedPreview));
    CHECK(!input.apply(MenuNavAction::ExpandFocused));
    CHECK(!input.apply(MenuNavAction::IncreaseSetting));
    CHECK(!menu.preview_enabled(3) && !menu.expanded(3));

    // All 17 combat rows are navigable; the eight-row viewport auto-scrolls.
    for(std::size_t i=4;i<=16;i++) {
        CHECK(input.apply(MenuNavAction::NextRow));
        CHECK(input.visible_focus() && *input.visible_focus()==i);
    }
    CHECK(menu.scroll()==9);
    CHECK(!input.apply(MenuNavAction::NextRow)); // Never past final row.
    CHECK(!input.apply(MenuNavAction::ToggleFocusedPreview)); // Weapons unsupported.
    CHECK(input.apply(MenuNavAction::PreviousRow));
    CHECK(*input.visible_focus()==15);
    for(std::size_t i=14;i<=14;i--) {
        CHECK(input.apply(MenuNavAction::PreviousRow));
        CHECK(*input.visible_focus()==i);
        if(i==0)break;
    }
    CHECK(menu.scroll()==0);
    CHECK(!input.apply(MenuNavAction::PreviousRow));

    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Move && menu.scroll()==0);
    CHECK(input.visible_focus() && *input.visible_focus()==17);
    CHECK(!input.apply(MenuNavAction::ToggleFocusedPreview));
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Visual && *input.visible_focus()==27);
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Block && *input.visible_focus()==42);
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Utility && *input.visible_focus()==49);
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Profiles && !input.visible_focus());
    CHECK(!input.apply(MenuNavAction::ToggleFocusedPreview));
    CHECK(!input.apply(MenuNavAction::NextRow));
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Combat && *input.visible_focus()==0);
    CHECK(input.apply(MenuNavAction::PreviousTab));
    CHECK(menu.tab()==MenuTab::Profiles && !input.visible_focus());
    CHECK(input.apply(MenuNavAction::PreviousTab));
    CHECK(menu.tab()==MenuTab::Utility && *input.visible_focus()==49);

    // R96 immutable render snapshots can express current focus without
    // granting gameplay activation or touching native Minecraft memory.
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Profiles);
    CHECK(input.apply(MenuNavAction::NextTab));
    CHECK(menu.tab()==MenuTab::Combat);
    MenuRenderSession frameSource;
    CHECK(frameSource.attach(50,676,630));
    const auto frame0=frameSource.next(menu,&profile,&input);
    const auto focus0=input.visible_focus();
    CHECK(frame0 && focus0==0);
    CHECK(input.apply(MenuNavAction::NextRow));
    const auto frame1=frameSource.next(menu,&profile,&input);
    const auto focus1=input.visible_focus();
    CHECK(frame1 && focus1==1);
    CHECK(focus0==0); // Value snapshot predates cursor move.
    const auto unbound=frameSource.next(menu,&profile);
    CHECK(unbound); // Original MenuRenderFrame ABI has no focus field.

    MenuState foreignMenu;
    MenuProfileBridge foreignProfile(foreignMenu);
    MenuNavigation foreignInput(foreignMenu,foreignProfile);
    const auto sequence=frameSource.sequence();
    CHECK(!frameSource.next(menu,&profile,&foreignInput));
    CHECK(!frameSource.next(menu,nullptr,&input));
    CHECK(!frameSource.next(menu,&foreignProfile,&input));
    CHECK(frameSource.sequence()==sequence);
    CHECK(!input.owns(menu,foreignProfile));

    std::atomic<bool> refused{false};
    std::thread notOwner([&]{
        const auto before=menu.scroll();
        refused=!input.owner_thread() &&
            !input.apply(MenuNavAction::NextRow) &&
            !input.visible_focus() && menu.scroll()==before;
    });
    notOwner.join();
    CHECK(refused);
    CHECK(input.visible_focus() && *input.visible_focus()==1);
    CHECK(frameSource.detach());
    // No game/OS keyboard input is ever used by this test.
    return 0;
}
