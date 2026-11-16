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
    // R230: with Unload or profile confirmation open, paired synthetic
    // keys never request GUI close, switch tabs or change settings.
    REQUIRE(menu.open_unload_confirmation_preview());
    REQUIRE(navigation.color_modal_open()); // ABI-preserved modal predicate
    REQUIRE(routing.begin(113,true,true));
    for(const unsigned code:{15u,209u,28u,1u}) {
        const auto press=deliver(code,true,true,true,true);
        REQUIRE(press.delivery==MenuKeyDelivery::MenuExclusive &&
                !press.action_applied && !press.request_close);
        const auto up=deliver(code,false,true,true,true);
        REQUIRE(up.delivery==MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.tab()==MenuTab::Combat &&
            menu.unload_confirmation_preview());
    REQUIRE(routing.end());
    REQUIRE(menu.close_unload_confirmation_preview());
    REQUIRE(navigation.select_tab(MenuTab::Profiles));
    REQUIRE(menu.begin_profile_action_preview(
        0u,MenuProfileAction::Load));
    REQUIRE(navigation.color_modal_open());
    REQUIRE(routing.begin(114,true,true));
    for(const unsigned code:{15u,209u,28u,1u}) {
        const auto press=deliver(code,true,true,true,true);
        REQUIRE(press.delivery==MenuKeyDelivery::MenuExclusive &&
                !press.action_applied && !press.request_close);
        REQUIRE(deliver(code,false,true,true,true).delivery==
                MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.tab()==MenuTab::Profiles &&
            menu.profile_action_preview());
    REQUIRE(routing.end());
    REQUIRE(menu.cancel_profile_action_preview());
    REQUIRE(navigation.apply(MenuNavAction::NextTab));
    REQUIRE(menu.tab()==MenuTab::Combat && game_events==3);
    // R231: profile name and bind capture are exclusive input modes,
    // even without a dropdown or confirmation popup.
    REQUIRE(navigation.select_tab(MenuTab::Profiles));
    REQUIRE(menu.focus_profile_name_preview());
    REQUIRE(menu.modal_input_owned() && navigation.color_modal_open());
    REQUIRE(routing.begin(115,true,true));
    for(const unsigned code:{15u,28u,1u}) {
        const auto press=deliver(code,true,true,true,true);
        REQUIRE(press.delivery==MenuKeyDelivery::MenuExclusive &&
                !press.action_applied && !press.request_close);
        REQUIRE(deliver(code,false,true,true,true).delivery==
                MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.profile_name_focused() &&
            menu.tab()==MenuTab::Profiles);
    REQUIRE(routing.end());
    REQUIRE(menu.blur_profile_name_preview());
    REQUIRE(menu.begin_profile_bind_capture(0u));
    REQUIRE(menu.modal_input_owned() && navigation.color_modal_open());
    REQUIRE(routing.begin(116,true,true));
    for(const unsigned code:{15u,28u,1u}) {
        const auto press=deliver(code,true,true,true,true);
        REQUIRE(press.delivery==MenuKeyDelivery::MenuExclusive &&
                !press.action_applied && !press.request_close);
        REQUIRE(deliver(code,false,true,true,true).delivery==
                MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.profile_bind_capture_index()==0u &&
            menu.tab()==MenuTab::Profiles && game_events==3);
    REQUIRE(routing.end());
    REQUIRE(menu.cancel_profile_bind_capture());
    REQUIRE(navigation.apply(MenuNavAction::NextTab));
    REQUIRE(menu.tab()==MenuTab::Combat);
    // R234: module binding and recovered text editing are exclusive
    // keyboard modes, just like the Profiles card and color popup.
    REQUIRE(menu.begin_bind_capture(0u));
    REQUIRE(navigation.color_modal_open());
    REQUIRE(routing.begin(117,true,true));
    for(const unsigned code:{15u,28u,1u}) {
        const auto down=deliver(code,true,true,true,true);
        REQUIRE(down.delivery==MenuKeyDelivery::MenuExclusive &&
                !down.action_applied && !down.request_close);
        REQUIRE(deliver(code,false,true,true,true).delivery==
                MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.tab()==MenuTab::Combat &&
            menu.bind_capture_index()==0u);
    REQUIRE(routing.end());
    REQUIRE(menu.cancel_bind_capture());
    REQUIRE(navigation.select_tab(MenuTab::Block));
    REQUIRE(navigation.focus_visible_row(42u));
    REQUIRE(menu.toggle_expanded(42u));
    const auto& widgets=recovered_widget_catalog();
    std::size_t sourceText=widgets.size();
    for(std::size_t i=0u;i<widgets.size();++i) {
        if(widgets[i].kind==RecoveredWidgetKind::Text &&
           std::string_view(widgets[i].module)=="Auto Tool" &&
           std::string_view(widgets[i].auxiliary)=="Search item") {
            sourceText=i;
            break;
        }
    }
    REQUIRE(sourceText<widgets.size());
    REQUIRE(menu.focus_recovered_text_preview(42u,sourceText));
    REQUIRE(navigation.color_modal_open());
    REQUIRE(routing.begin(118,true,true));
    for(const unsigned code:{15u,28u,1u}) {
        const auto down=deliver(code,true,true,true,true);
        REQUIRE(down.delivery==MenuKeyDelivery::MenuExclusive &&
                !down.action_applied && !down.request_close);
        REQUIRE(deliver(code,false,true,true,true).delivery==
                MenuKeyDelivery::MenuExclusive);
    }
    REQUIRE(menu.tab()==MenuTab::Block &&
            menu.focused_text_widget()==sourceText);
    REQUIRE(routing.end());
    REQUIRE(menu.blur_recovered_text_preview());
    REQUIRE(navigation.select_tab(MenuTab::Combat));
    std::cout << "R234_MODULE_BIND_AND_RECOVERED_TEXT_KEYBOARD_ISOLATION_PASS\\n";
    std::cout << "R231_PROFILE_EDIT_BIND_KEYBOARD_ISOLATION_PASS\\n";
    std::cout << "R230_UNLOAD_AND_PROFILE_KEYBOARD_MODAL_QUARANTINE_PASS\\n";
    std::cout << "R108_EXCLUSIVE_MODAL_KEYBOARD_ROUTER_SYNTHETIC_GAME_NONDELIVERY_PASS\\n"
              << "R108_OWNER_EPOCH_RELEASE_FOCUS_LOSS_AND_CLOSE_DRAIN_PASS\\n"
              << "R108_NO_GAME_JVM_HOOK_OR_PHYSICAL_INPUT_CERTIFICATION\\n";
    return 0;
}
