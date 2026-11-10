#include "slinkywinky/menu_input_handoff_ledger.hpp"
#include <stdexcept>
#include <iostream>
#include <thread>

using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    "R190 input handoff model: " #x);}while(false)

namespace {
InputHandoffEvent key(unsigned code,bool down) {
    return {InputHandoffDevice::Keyboard,code,down};
}
InputHandoffEvent mouse(unsigned code,bool down) {
    return {InputHandoffDevice::Mouse,code,down};
}
void full_fixture_cycle() {
    MenuInputHandoffLedger h;
    CHECK(h.phase()==InputHandoffPhase::Idle);
    CHECK(h.tracked_released() && h.epoch()==0);
    CHECK(!h.may_deliver_live_minecraft_event());
    CHECK(!h.may_suppress_vanilla_event());
    CHECK(h.simulate(key(17,true),true,true,false,false)==
          InputHandoffRoute::GameUnchanged);
    CHECK(h.simulate(mouse(0,true),true,true,true,false)==
          InputHandoffRoute::Quarantined);

    // Request must fail closed until source, focus, visibility AND all
    // real controls released are independently claimed by test fixture.
    CHECK(!h.request(0,true,true,true,true));
    CHECK(!h.request(190,false,true,true,true));
    CHECK(!h.request(190,true,false,true,true));
    CHECK(!h.request(190,true,true,false,true));
    CHECK(!h.request(190,true,true,true,false));
    CHECK(h.request(190,true,true,true,true));
    CHECK(h.phase()==InputHandoffPhase::Candidate && h.epoch()==190);
    CHECK(h.simulate(key(17,true),true,true,true,true)==
          InputHandoffRoute::Quarantined);
    CHECK(!h.request(191,true,true,true,true));
    CHECK(!h.confirm_fixture(191,true,true,true));
    CHECK(h.phase()==InputHandoffPhase::Candidate);
    CHECK(h.confirm_fixture(190,true,true,true));
    CHECK(h.phase()==InputHandoffPhase::MenuFixture);

    // One synthetic keyboard press and five synthetic mouse buttons stay
    // fixture-owned, even across all simultaneous button chords.
    CHECK(h.simulate(key(200,true),true,true,true,true)==
          InputHandoffRoute::MenuFixtureOnly);
    for(unsigned i=0;i<5;i++)
        CHECK(h.simulate(mouse(i,true),true,true,true,true)==
              InputHandoffRoute::MenuFixtureOnly);
    CHECK(!h.tracked_released());
    CHECK(h.tracked_down(InputHandoffDevice::Keyboard,200));
    CHECK(h.tracked_down(InputHandoffDevice::Mouse,0));
    CHECK(h.tracked_down(InputHandoffDevice::Mouse,4));
    CHECK(!h.tracked_down(InputHandoffDevice::Mouse,5));
    CHECK(!h.tracked_down(InputHandoffDevice::Keyboard,256));
    CHECK(!h.tracked_down(static_cast<InputHandoffDevice>(255),0));
    CHECK(!h.close(false));
    CHECK(h.close(true));
    CHECK(h.phase()==InputHandoffPhase::Draining);
    CHECK(!h.restore(true,false,true));
    CHECK(!h.restore(true,true,false));
    CHECK(!h.restore(false,true,true));

    // Every menu-held release is quarantined after close, never passed
    // through to the original Minecraft input dispatcher.
    CHECK(h.simulate(key(200,false),false,false,false,false)==
          InputHandoffRoute::Quarantined);
    for(unsigned i=0;i<5;i++)
        CHECK(h.simulate(mouse(i,false),false,false,false,false)==
              InputHandoffRoute::Quarantined);
    CHECK(h.tracked_released());
    CHECK(!h.tracked_down(InputHandoffDevice::Mouse,0));
    CHECK(!h.tracked_down(InputHandoffDevice::Keyboard,200));
    CHECK(h.phase()==InputHandoffPhase::Draining);
    CHECK(h.restore(true,true,true));
    CHECK(h.phase()==InputHandoffPhase::Idle && h.epoch()==190);
    CHECK(!h.request(190,true,true,true,true));
    CHECK(h.simulate(key(200,false),true,true,false,false)==
          InputHandoffRoute::GameUnchanged);

    // No second owner may read or mutate the constructing thread model.
    bool foreign_accepted=false;
    std::thread other([&] {
        foreign_accepted=h.request(191,true,true,true,true) ||
            h.confirm_fixture(190,true,true,true) || h.abort() ||
            h.restore(true,true,true) ||
            h.simulate(mouse(0,true),true,true,true,true)!=
                InputHandoffRoute::Quarantined ||
            h.epoch()!=0 || h.phase()!=InputHandoffPhase::Revoked ||
            h.tracked_released();
    });
    other.join();
    CHECK(!foreign_accepted && h.phase()==InputHandoffPhase::Idle);
    CHECK(!h.may_deliver_live_minecraft_event());

    // Missing separate dispatcher attestations terminally revoke.
    CHECK(h.request(191,true,true,true,true));
    CHECK(!h.confirm_fixture(191,false,true,true));
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(h.simulate(key(17,true),true,true,true,true)==
          InputHandoffRoute::Quarantined);
    CHECK(!h.request(192,true,true,true,true));
    CHECK(!h.restore(true,true,false));
    CHECK(h.restore(true,true,true));
    CHECK(h.request(192,true,true,true,true));
    CHECK(!h.confirm_fixture(192,true,false,true));
    CHECK(h.restore(true,true,true));
    CHECK(h.request(193,true,true,true,true));
    CHECK(!h.confirm_fixture(193,true,true,false));
    CHECK(h.restore(true,true,true));

    // A focus loss after synthetic mouse press quarantines the edge;
    // never infer restoration solely from overlay being hidden.
    CHECK(h.request(194,true,true,true,true));
    CHECK(h.confirm_fixture(194,true,true,true));
    CHECK(h.simulate(mouse(0,true),true,true,true,true)==
          InputHandoffRoute::MenuFixtureOnly);
    CHECK(!h.tracked_released());
    CHECK(h.simulate(mouse(0,false),true,false,true,true)==
          InputHandoffRoute::Quarantined);
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(!h.restore(true,false,true));
    CHECK(!h.restore(true,true,false));
    CHECK(h.restore(true,true,true));
    CHECK(h.tracked_released());
    CHECK(!h.request(194,true,true,true,true));

    // Unknown key, out-of-range mouse code, unknown device and source
    // dispatch loss are all terminal; each needs independently attested
    // cleanup and a strictly increasing session epoch.
    CHECK(h.request(195,true,true,true,true));
    CHECK(h.confirm_fixture(195,true,true,true));
    CHECK(h.simulate(key(256,true),true,true,true,true)==
          InputHandoffRoute::Quarantined);
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(h.restore(true,true,true));
    CHECK(h.request(196,true,true,true,true));
    CHECK(h.confirm_fixture(196,true,true,true));
    CHECK(h.simulate(mouse(5,true),true,true,true,true)==
          InputHandoffRoute::Quarantined);
    CHECK(h.restore(true,true,true));
    CHECK(h.request(197,true,true,true,true));
    CHECK(h.confirm_fixture(197,true,true,true));
    CHECK(h.simulate({static_cast<InputHandoffDevice>(255),1,true},
                     true,true,true,true)==InputHandoffRoute::Quarantined);
    CHECK(h.restore(true,true,true));
    CHECK(h.request(198,true,true,true,true));
    CHECK(h.confirm_fixture(198,true,true,true));
    CHECK(h.simulate(key(17,true),true,true,true,false)==
          InputHandoffRoute::Quarantined);
    CHECK(h.restore(true,true,true));
    CHECK(h.request(199,true,true,true,true));
    CHECK(h.abort());
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(h.restore(true,true,true));

    // A new press during release draining is not silently passed to game.
    CHECK(h.request(200,true,true,true,true));
    CHECK(h.confirm_fixture(200,true,true,true));
    CHECK(h.close(true));
    CHECK(h.simulate(mouse(1,true),true,true,false,true)==
          InputHandoffRoute::Quarantined);
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(h.restore(true,true,true));
    CHECK(h.phase()==InputHandoffPhase::Idle);
    // Even an unknown release in the drain phase is not a license to
    // forward it to vanilla; require independently attested cleanup.
    CHECK(h.request(201,true,true,true,true));
    CHECK(h.confirm_fixture(201,true,true,true));
    CHECK(h.close(true));
    CHECK(h.simulate(key(256,false),true,true,false,true)==
          InputHandoffRoute::Quarantined);
    CHECK(h.phase()==InputHandoffPhase::Revoked);
    CHECK(!h.request(202,true,true,true,true));
    CHECK(h.restore(true,true,true));
    CHECK(h.phase()==InputHandoffPhase::Idle);
    CHECK(!h.may_deliver_live_minecraft_event());
    CHECK(!h.may_suppress_vanilla_event());
}
}
int main() {
    try {
        full_fixture_cycle();
        std::cout<<"R190_INPUT_HANDOFF_PAIRED_MOUSE_KEY_DRAIN_PASS\n";
        std::cout<<"R190_FOCUS_LOSS_REVOKES_AND_REQUIRES_HOST_RESTORE_PASS\n";
        std::cout<<"R190_NO_ORIGINAL_GAME_HOOK_OR_LIVE_EVENT_DELIVERY\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
