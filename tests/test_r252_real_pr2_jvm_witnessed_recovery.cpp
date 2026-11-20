// R252 — source-pinned ACTUAL PR2 native owner recovery with a
// test-exclusive R251-style JVM identity witness, not a production
// OS/JVM attestation. No real Minecraft JVM or gameplay input.
#include "slinkywinky/jvm_witnessed_native_recovery.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    std::string("R252 line ")+std::to_string(__LINE__)+": " #x);}while(false)

static SprintObservedTick frame(std::uint64_t generation,
                                std::uint64_t timestamp) {
    SprintObservedTick tick;
    tick.observation.generation=generation;
    tick.observation.time_ms=timestamp;
    tick.observation.flags=3u;
    tick.observation.xyz={0,0,0,0,0,0};
    tick.observation.angles={0,0};
    tick.update.player_present=true;
    tick.update.player_sprinting=true;
    tick.movement.player_present=true;
    tick.movement.forward=-0.2f;
    tick.movement.strafe=0.1f;
    tick.movement.speed=0.1f;
    tick.movement.using_item=true;
    tick.jump_delay=JumpDelayInput{true,10};
    tick.no_slow=NoSlowInput{true,true,261,0.1f,-0.2f,{}};
    return tick;
}
static JvmLeaseIdentity read(const std::string& line){
    auto p=parse_r252_fixture_receipt(line);
    CHECK(p);
    return *p;
}
class FakeExclusiveWitness {
public:
    JvmLeaseIdentity old_claim,next_claim;
    bool old_quiesced=true,new_active=true,revoke_after_call=false;
    bool throw_after_call=false,skip_callback=false;
    unsigned calls=0;
    template<typename Fn>
    bool with_exclusive_lease(const JvmLeaseIdentity& old_lease,
                              const JvmLeaseIdentity& new_lease,Fn body) {
        ++calls;
        if(!(old_lease==old_claim) || !(new_lease==next_claim) ||
           !old_quiesced || !new_active)
            return false;
        if(skip_callback)return true;  // MUST NOT imply success
        const bool actual=body();
        if(throw_after_call)
            throw std::runtime_error("R252 witness failure after native publish");
        return actual && !revoke_after_call;
    }
};

struct OldSession {
    SprintObservationPreview native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    GuiModuleControlHandoff gui;
    OldSession(){
        CHECK(native.observe(frame(70u,1000u)).admitted);
        CHECK(adapter.publish_on_owner(native,feed,40u,true));
        CHECK(native.request_module({"No Slow",true}));
        CHECK(native.control_state("No Slow")->active_in_world);
        gui.retire();
        CHECK(adapter.retire_owner(feed));
        CHECK(native.shutdown() && native.stopped());
    }
};
struct FreshSession {
    SprintObservationPreview native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    FreshSession(){
        CHECK(native.observe(frame(71u,2000u)).admitted);
        CHECK(!native.control_state("No Slow")->requested_enabled);
    }
};
static const JvmLeaseIdentity kOld=read(
    "R252_LEASE_RECEIPT 1724 1799999000000 2 0123456789abcdef0123456789abcdef");
static const JvmLeaseIdentity kNew=read(
    "R252_LEASE_RECEIPT 1724 1799999000000 3 fedcba9876543210fedcba9876543210");
static void check_malformed(){
    const std::string good=
        "R252_LEASE_RECEIPT 1724 1799999000000 2 0123456789abcdef0123456789abcdef";
    CHECK(parse_r252_fixture_receipt(good)==kOld);
    const std::string invalid[]={
        "",good+" junk",
        "R252_LEASE_RECEIPT 1724 1799999000000 2 ",
        "R252_LEASE_RECEIPT 1724 1799999000000 2 00000000000000000000000000000000",
        "R252_LEASE_RECEIPT 1724 1799999000000 2 ABCDEF0123456789abcdef0123456789",
        "R252_LEASE_RECEIPT -1 1799999000000 2 0123456789abcdef0123456789abcdef",
        "R252_LEASE_RECEIPT 1724 0 2 0123456789abcdef0123456789abcdef",
        "R252_LEASE_RECEIPT 1724 1799999000000 18446744073709551616 0123456789abcdef0123456789abcdef",
        "R252_LEASE_RECEIPT 1724  1799999000000 2 0123456789abcdef0123456789abcdef"
    };
    for(const auto& line:invalid)
        CHECK(!parse_r252_fixture_receipt(line));
    std::cout<<"R252_STRICT_R251_JAVA_RECEIPT_PARSER_NEGATIVES_PASS\n";
}

static void check_negatives(){
    OldSession old;
    FreshSession fresh;
    FakeExclusiveWitness witness{kOld,kNew};
    JvmWitnessedNativeRecovery guard;
    auto attempt=[&](const JvmLeaseIdentity& oldId,
                     const JvmLeaseIdentity& newId,bool ready=true)->bool {
        return guard.admit_on_owner(witness,oldId,newId,
            old.native,old.adapter,old.feed,old.gui,
            fresh.native,fresh.adapter,fresh.feed,41u,ready);
    };
    JvmLeaseIdentity tampered=kNew;
    tampered.pid++;
    CHECK(!attempt(kOld,tampered));
    tampered=kNew;tampered.jvm_start_ms++;
    CHECK(!attempt(kOld,tampered));
    tampered=kNew;tampered.lease_epoch=2;
    CHECK(!attempt(kOld,tampered));
    tampered=kNew;tampered.challenge_hex="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    CHECK(!attempt(kOld,tampered));
    tampered=kNew;tampered.challenge_hex=kOld.challenge_hex;
    CHECK(!attempt(kOld,tampered));
    CHECK(!attempt(kOld,kOld));  // exact replay
    CHECK(!attempt(kOld,kNew,false)); // world-not-ready
    CHECK(!guard.used() && !guard.poisoned() && !fresh.feed.latest());
    witness.new_active=false;
    CHECK(!attempt(kOld,kNew));
    witness.new_active=true;
    witness.old_quiesced=false;
    CHECK(!attempt(kOld,kNew));
    witness.old_quiesced=true;
    witness.skip_callback=true;
    CHECK(!attempt(kOld,kNew)); // a witness MUST invoke callback
    witness.skip_callback=false;

    bool foreign=false;
    std::thread other([&]{foreign=attempt(kOld,kNew);});
    other.join();
    CHECK(!foreign && !fresh.feed.latest());

    CHECK(attempt(kOld,kNew));
    CHECK(guard.used() && !guard.poisoned());
    CHECK(fresh.feed.latest() &&
          fresh.feed.latest()->world_generation==71u &&
          fresh.feed.latest()->source_sequence==41u);
    CHECK(!attempt(kOld,kNew)); // cannot replay bridge after publication

    // Source-valid GUI No Slow command on the owner after R252 admission.
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    CHECK(nav.select_tab(MenuTab::Move));
    for(int i=0;i<5;++i)CHECK(nav.apply(MenuNavAction::NextRow));
    CHECK(nav.visible_focus() && *nav.visible_focus()==22u);
    GuiModuleControlHandoff gui;
    CHECK(gui.request_focused_toggle(menu,nav,fresh.feed,41u));
    CHECK(fresh.adapter.apply_request_on_owner(
        fresh.native,gui,fresh.feed,42u,true));
    CHECK(fresh.native.control_state("No Slow")->active_in_world);
    CHECK(gui.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(fresh.native.observe(frame(71u,2001u)).no_slow);
    CHECK(fresh.native.shutdown());
    CHECK(fresh.adapter.retire_owner(fresh.feed));
    std::cout<<"R252_WITNESSED_REAL_PR2_NATIVE_GUI_RECOVERY_PASS\n";
}

static void check_ambiguity(){
    for(int failure=0;failure<2;++failure){
        OldSession old;
        FreshSession fresh;
        FakeExclusiveWitness witness{kOld,kNew};
        witness.revoke_after_call=failure==0;
        witness.throw_after_call=failure==1;
        JvmWitnessedNativeRecovery guard;
        CHECK(!guard.admit_on_owner(witness,kOld,kNew,
            old.native,old.adapter,old.feed,old.gui,
            fresh.native,fresh.adapter,fresh.feed,41u,true));
        CHECK(guard.poisoned() && !guard.used());
        CHECK(fresh.adapter.retired() && fresh.feed.retired() &&
              !fresh.feed.latest());
        CHECK(!guard.admit_on_owner(witness,kOld,kNew,
            old.native,old.adapter,old.feed,old.gui,
            fresh.native,fresh.adapter,fresh.feed,42u,true));
        CHECK(fresh.native.shutdown());
    }
    std::cout<<"R252_REVOKED_OR_THROWN_JVM_WITNESS_QUARANTINES_NATIVE_FEED_PASS\n";
}
static void check_new_incarnation(){
    OldSession old;
    FreshSession fresh;
    const auto freshJvm=read(
        "R252_LEASE_RECEIPT 9472 1800000000000 1 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    FakeExclusiveWitness witness{kOld,freshJvm};
    JvmWitnessedNativeRecovery bridge;
    CHECK(bridge.admit_on_owner(witness,kOld,freshJvm,
        old.native,old.adapter,old.feed,old.gui,
        fresh.native,fresh.adapter,fresh.feed,41u,true));
    CHECK(bridge.used() && fresh.feed.latest());
    CHECK(fresh.native.shutdown());
    CHECK(fresh.adapter.retire_owner(fresh.feed));
    std::cout<<"R252_DIFFERENT_JVM_INCAR_RESTARTS_R251_EPOCH_AT_ONE_PASS\n";
}
int main(){
    check_malformed();
    check_negatives();
    check_ambiguity();
    check_new_incarnation();
    std::cout<<"R252_OFFLINE_WITNESS_CONTRACT_NOT_OS_AUTHENTICATION_OR_LIVE_GAME\n";
}
