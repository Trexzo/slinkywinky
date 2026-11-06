#define WIN32_LEAN_AND_MEAN
#include "slinkywinky/windows_menu_focus_fence.hpp"
#include "slinkywinky/windows_visibility_hotkey.hpp"
#include <windows.h>
#include <stdexcept>
#include <thread>
#include <iostream>
#define REQUIRE(ok) do { if(!(ok))throw std::runtime_error("R104 focus fence: " #ok); } while(false)
using namespace slinkywinky;

int main() {
    MenuFocusFence gate;
    REQUIRE(gate.epoch()==0);
    REQUIRE(!gate.can_consume_game_input());
    REQUIRE(!gate.foreground_candidate());
    REQUIRE(!gate.begin(0,true));
    REQUIRE(!gate.begin(1,false));
    REQUIRE(gate.begin(1,true));
    REQUIRE(gate.observe(GameWindowFocus::ValidBackground)==FocusFenceState::Background);
    REQUIRE(gate.observe(GameWindowFocus::ValidBackground)==FocusFenceState::Background);
    REQUIRE(gate.background_samples()==2);
    REQUIRE(!gate.foreground_candidate());

    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Dormant);
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Dormant);
    REQUIRE(!gate.foreground_candidate());
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::ForegroundCandidate);
    REQUIRE(gate.foreground_candidate());
    REQUIRE(!gate.can_consume_game_input()); // No modal route actually exists.
    REQUIRE(gate.foreground_samples()==3);

    // A wrong-thread query must fail without corrupting the owner session.
    bool crossThreadDenied=false;
    std::thread other([&]() {
        crossThreadDenied=gate.observe(GameWindowFocus::ValidForeground)==
                          FocusFenceState::Refused;
        crossThreadDenied=crossThreadDenied && !gate.foreground_candidate() &&
                         !gate.can_consume_game_input();
        crossThreadDenied=crossThreadDenied &&
                         gate.state()==FocusFenceState::Refused &&
                         gate.epoch()==0 && gate.foreground_samples()==0 &&
                         gate.background_samples()==0 && gate.denied_samples()==0;
        crossThreadDenied=crossThreadDenied && !gate.begin(2,true);
    });
    other.join();
    REQUIRE(crossThreadDenied);
    REQUIRE(gate.foreground_candidate());
    REQUIRE(gate.epoch()==1);

    REQUIRE(gate.observe(GameWindowFocus::ValidBackground)==FocusFenceState::FocusLost);
    REQUIRE(!gate.foreground_candidate());
    REQUIRE(!gate.can_consume_game_input());
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::FocusLost);
    REQUIRE(!gate.begin(1,true));
    REQUIRE(!gate.begin(2,false)); // Cannot rearm with a possibly held key.
    REQUIRE(gate.begin(2,true));
    for(int n=0;n<3;n++)gate.observe(GameWindowFocus::ValidForeground);
    REQUIRE(gate.foreground_candidate());
    REQUIRE(gate.observe(GameWindowFocus::Refused)==FocusFenceState::Refused);
    REQUIRE(gate.denied_samples()==1);
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Refused);
    REQUIRE(!gate.can_consume_game_input());

    REQUIRE(!gate.begin(2,true));
    REQUIRE(gate.begin(3,true));
    REQUIRE(gate.state()==FocusFenceState::Dormant);
    REQUIRE(gate.foreground_samples()==0);
    REQUIRE(gate.background_samples()==0);
    REQUIRE(gate.denied_samples()==0);
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Dormant);
    REQUIRE(gate.observe(GameWindowFocus::ValidBackground)==FocusFenceState::Background);
    REQUIRE(!gate.foreground_candidate());
    // Two foreground samples followed by background never suffice for admission.
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Dormant);
    REQUIRE(gate.observe(GameWindowFocus::ValidForeground)==FocusFenceState::Dormant);
    REQUIRE(gate.observe(GameWindowFocus::ValidBackground)==FocusFenceState::Background);
    REQUIRE(!gate.can_consume_game_input());
    // R106: a visible menu edge is not admitted just because a caller
    // knows our PID or because one foreground observation succeeded.
    ScopedVisibilityHotkey hotkey(106);
    REQUIRE(!hotkey.can_consume_game_input());
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidBackground,false,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,false));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!hotkey.candidate());
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(hotkey.candidate());
    REQUIRE(hotkey.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(hotkey.sample(GameWindowFocus::ValidForeground,true,true));
    bool foreignHotkeyAllowed=false;
    std::thread wrongHotkeyThread([&]() {
        foreignHotkeyAllowed=hotkey.sample(
            GameWindowFocus::ValidForeground,true,true) || hotkey.candidate();
    });
    wrongHotkeyThread.join();
    REQUIRE(!foreignHotkeyAllowed && hotkey.candidate());
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidBackground,false,true));
    REQUIRE(!hotkey.candidate());
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!hotkey.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!hotkey.candidate());
    // A new local admission epoch can proceed, but only after all input is up.
    ScopedVisibilityHotkey rearmed(107);
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(rearmed.candidate());
    REQUIRE(rearmed.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::Refused,false,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,false,true));
    REQUIRE(!rearmed.sample(GameWindowFocus::ValidForeground,true,true));
    REQUIRE(!rearmed.can_consume_game_input());
    std::cout<<"R106_STRICT_WGL_FOCUS_THREE_SAMPLES_INSERT_RELEASE_AND_NO_PASSTHROUGH_CLAIM_PASS\n";
    std::cout<<"R104_FOREGROUND_STABILITY_LOSS_LATCH_RELEASE_REARM_NO_GAME_INPUT_PASS\n";
}
