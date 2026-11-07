#include "slinkywinky/menu_keyboard_router.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <atomic>
#include <stdexcept>
#include <thread>
#include <iostream>
#define REQUIRE(expr) do { if(!(expr)) throw std::runtime_error(\
    "R108 modal model assertion: " #expr); } while(false)

using namespace slinkywinky;

int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation navigation(menu,profile);
    MenuKeyboardRouter routing(navigation);
    unsigned game_events=0,gui_events=0,quarantined=0;
    const auto deliver=[&](unsigned key,bool down,bool focused,bool source,bool shown) {
        const auto event=routing.event(key,down,focused,source,shown);
        if(event.delivery==MenuKeyDelivery::GameUnchanged)++game_events;
        if(event.delivery==MenuKeyDelivery::MenuExclusive)++gui_events;
        if(event.delivery==MenuKeyDelivery::Quarantined)++quarantined;
        return event;
    };

    // Without a source-attested exclusive *Minecraft* event dispatcher,
    // a visible overlay must never imply permission to consume gameplay.
    REQUIRE(deliver(17,true,true,true,false).delivery==
            MenuKeyDelivery::GameUnchanged);
    REQUIRE(game_events==1);
    REQUIRE(deliver(17,true,true,true,true).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(!routing.begin(108,false,true));
    REQUIRE(!routing.begin(108,true,false));
    REQUIRE(!routing.begin(0,true,true));
    REQUIRE(deliver(200,true,true,true,true).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(routing.begin(108,true,true));
    REQUIRE(routing.active() && routing.epoch()==108);
    REQUIRE(!routing.begin(108,true,true));
    REQUIRE(!routing.begin(109,true,true));

    bool foreignAccepted=false;
    std::thread foreign([&] {
        const auto x=routing.event(15,true,true,true,true);
        foreignAccepted=x.delivery!=MenuKeyDelivery::Quarantined ||
            routing.active() || routing.epoch()!=0;
    });
    foreign.join();
    REQUIRE(!foreignAccepted);
    REQUIRE(menu.tab()==MenuTab::Combat);

    auto x=deliver(15,true,true,true,true);
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            x.action_applied && !x.request_close);
    REQUIRE(menu.tab()==MenuTab::Move);
    x=deliver(15,true,true,true,true); // auto-repeat must not switch tab
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive && !x.action_applied);
    REQUIRE(menu.tab()==MenuTab::Move);
    REQUIRE(deliver(15,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    x=deliver(203,true,true,true,true); // previous tab
    REQUIRE(x.action_applied && menu.tab()==MenuTab::Combat);
    REQUIRE(deliver(203,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);

    // R118: actual LWJGL2 KEY_PRIOR=201 and KEY_NEXT=209 (validated
    // against the installed original 2.9.4 Java Keyboard constants).
    // The events are synthetic and only delivered after explicit modal
    // admission; no original Minecraft event handler is modified here.
    x=deliver(209,true,true,true,true);
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            !x.action_applied && menu.settings_offset(0)==0);
    REQUIRE(deliver(209,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    x=deliver(28,true,true,true,true);
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive && x.action_applied);
    REQUIRE(deliver(28,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(menu.expanded(0));
    x=deliver(209,true,true,true,true);
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            x.action_applied && menu.settings_offset(0)==5);
    x=deliver(209,true,true,true,true); // key repeat never advances twice
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            !x.action_applied && menu.settings_offset(0)==5);
    REQUIRE(deliver(209,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    x=deliver(201,true,true,true,true);
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            x.action_applied && menu.settings_offset(0)==0);
    REQUIRE(deliver(201,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    x=deliver(201,true,true,true,true); // first-page boundary
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive &&
            !x.action_applied && menu.settings_offset(0)==0);
    REQUIRE(deliver(201,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    x=deliver(28,true,true,true,true);
    REQUIRE(x.action_applied && !menu.expanded(0));
    REQUIRE(deliver(28,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(menu.settings_offset(0)==0 && game_events==1);

    // Unmapped gameplay keys and their releases are menu-owned. The mock
    // game's count cannot rise while the modal session is active.
    REQUIRE(deliver(17,true,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(deliver(17,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(deliver(200,true,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(deliver(200,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(game_events==1);
    REQUIRE(menu.tab()==MenuTab::Combat);
    x=deliver(1,true,true,true,true);  // Escape closes, no game escape.
    REQUIRE(x.delivery==MenuKeyDelivery::MenuExclusive && x.request_close);
    REQUIRE(routing.end());
    REQUIRE(!routing.active());
    REQUIRE(!routing.begin(109,true,true)); // old key still held.
    REQUIRE(deliver(1,false,true,true,false).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(deliver(17,true,true,true,false).delivery==
            MenuKeyDelivery::GameUnchanged);
    REQUIRE(game_events==2);

    // Another session requires strictly newer epoch + independently
    // verified all-input release and an exclusive event-owner certificate.
    REQUIRE(!routing.begin(108,true,true));
    REQUIRE(routing.begin(109,true,true));
    REQUIRE(deliver(208,true,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(deliver(208,false,true,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
    REQUIRE(deliver(57,true,false,true,true).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(routing.revoked());
    REQUIRE(deliver(57,false,true,true,true).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(!routing.end());
    REQUIRE(!routing.release_after_abort(false,true,true));
    REQUIRE(!routing.release_after_abort(true,false,true));
    REQUIRE(!routing.release_after_abort(true,true,false));
    REQUIRE(routing.revoked());
    REQUIRE(routing.release_after_abort(true,true,true));
    REQUIRE(!routing.revoked());
    REQUIRE(deliver(17,true,true,true,false).delivery==
            MenuKeyDelivery::GameUnchanged);
    REQUIRE(game_events==3);
    REQUIRE(!routing.begin(110,false,true));
    REQUIRE(!routing.begin(110,true,false));
    REQUIRE(routing.begin(110,true,true));
    REQUIRE(deliver(15,true,true,false,true).delivery==
            MenuKeyDelivery::Quarantined); // source owner changed.
    REQUIRE(routing.revoked());
    REQUIRE(routing.begin(111,true,true));
    REQUIRE(deliver(256,true,true,true,true).delivery==
            MenuKeyDelivery::Quarantined); // unknown code.
    REQUIRE(routing.revoked());
    REQUIRE(routing.begin(112,true,true));
    REQUIRE(deliver(210,true,true,true,true).request_close);
    REQUIRE(routing.end());
    REQUIRE(deliver(210,false,true,true,false).delivery==
            MenuKeyDelivery::Quarantined);
    REQUIRE(game_events==3 && gui_events>=9 && quarantined>=6);
    REQUIRE(!routing.active() && !routing.revoked());
    std::cout << "R108_EXCLUSIVE_MODAL_KEYBOARD_ROUTER_SYNTHETIC_GAME_NONDELIVERY_PASS\\n"
              << "R108_OWNER_EPOCH_RELEASE_FOCUS_LOSS_AND_CLOSE_DRAIN_PASS\\n"
              << "R108_NO_GAME_JVM_HOOK_OR_PHYSICAL_INPUT_CERTIFICATION\\n";
    return 0;
}
