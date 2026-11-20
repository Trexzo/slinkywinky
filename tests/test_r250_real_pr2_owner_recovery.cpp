// R250: explicit post-quarantine owner re-admission against the ACTUAL
// pinned PR2 SprintObservationPreview; old objects never reopen.
// Offline native policy evidence only, NOT live JVM attach, OS identity
// or physical input ownership, gameplay write or original Slinky parity.
#include "slinkywinky/native_module_recovery_fence.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(expr) do {if(!(expr))throw std::runtime_error( \
    std::string("R250 failure at line ")+std::to_string(__LINE__)+": " #expr);}while(false)

static SprintObservedTick frame(std::uint64_t gen,std::uint64_t stamp){
    SprintObservedTick t;
    t.observation.generation=gen;
    t.observation.time_ms=stamp;
    t.observation.flags=3u;
    t.observation.xyz={0,0,0,0,0,0};
    t.observation.angles={0,0};
    t.update.player_present=true;
    t.update.player_sprinting=true;
    t.movement.player_present=true;
    t.movement.forward=-0.2f;
    t.movement.strafe=0.1f;
    t.movement.speed=0.1f;
    t.movement.using_item=true;
    t.jump_delay=JumpDelayInput{true,10};
    t.no_slow=NoSlowInput{true,true,261,0.1f,-0.2f,{}};
    return t;
}
// Deliberately make an actual PR2 No Slow native mutation succeed and
// throw AFTERWARDS, reproducing an uncertain acknowledgement; the
// native state remains genuinely changed.
struct PostCommitFailure {
    SprintObservationPreview& real;
    auto control_states() const {return real.control_states();}
    std::uint64_t generation() const noexcept {return real.generation();}
    bool request_module(ModuleControlRequest req){
        CHECK(real.request_module(req));
        throw std::runtime_error("R250 simulated post-native-commit failure");
    }
};
static void select_no_slow(MenuState& menu,MenuNavigation& nav){
    CHECK(nav.select_tab(MenuTab::Move));
    for(int i=0;i<5;++i)CHECK(nav.apply(MenuNavAction::NextRow));
    CHECK(nav.visible_focus() && *nav.visible_focus()==22u);
}
static void main_recovery(){
    // Old owner and GUI use this thread for the abbreviated failure setup.
    // R247-R249 separately prove cross-thread source GUI handoff.
    SprintObservationPreview old_native;
    NativeModuleSnapshotAdapter old_adapter;
    ModuleArrayListFeed old_feed;
    GuiModuleControlHandoff old_gui;
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    select_no_slow(menu,nav);
    CHECK(old_native.observe(frame(70u,1000u)).admitted);
    CHECK(old_adapter.publish_on_owner(old_native,old_feed,40u,true));
    CHECK(old_adapter.last_confirmed_world_generation()==70u);
    CHECK(old_adapter.last_confirmed_source_sequence()==40u);
    CHECK(old_gui.request_focused_toggle(menu,nav,old_feed,40u));
    PostCommitFailure failure{old_native};
    CHECK(!old_adapter.apply_request_on_owner(
        failure,old_gui,old_feed,41u,true));
    CHECK(old_adapter.retired() && old_feed.retired() &&
          !old_feed.latest() &&
          old_gui.view().phase==GuiModuleControlPhase::Retired);
    CHECK(old_native.control_state("No Slow")->active_in_world);
    // Native side effects were NOT rolled back by the UI quarantine.
    CHECK(old_adapter.last_confirmed_world_generation()==70u);
    CHECK(old_adapter.last_confirmed_source_sequence()==40u);

    // Deny a new instance while the ambiguous old native manager still
    // owns a live session, even with a larger generation.
    SprintObservationPreview premature_native;
    NativeModuleSnapshotAdapter premature_adapter;
    ModuleArrayListFeed premature_feed;
    CHECK(premature_native.observe(frame(71u,1001u)).admitted);
    CHECK(!NativeModuleRecoveryFence::admit_after_quiescence(
        old_native,old_adapter,old_feed,old_gui,premature_native,
        premature_adapter,premature_feed,41u,true));
    CHECK(!premature_feed.latest() && !premature_adapter.retired());
    CHECK(premature_native.shutdown());

    // A real old-owner shutdown must complete before any new source
    // can be considered. Read-only status checks happen after stop.
    CHECK(old_native.shutdown() && old_native.stopped());
    CHECK(!old_adapter.publish_on_owner(old_native,old_feed,42u,true));
    CHECK(!old_gui.request_focused_toggle(menu,nav,old_feed,40u));

    // A distinct native thread owns the replacement manager and its
    // brand-new adapter. The GUI handoff lives on this original thread.
    ModuleArrayListFeed fresh_feed;
    GuiModuleControlHandoff fresh_gui;
    std::mutex mu;
    std::condition_variable cv;
    int phase=0;
    std::exception_ptr owner_error;
    bool admitted=false,processed=false,old_replay_denied=false;
    std::thread fresh_owner([&]{
        try {
            SprintObservationPreview native;
            NativeModuleSnapshotAdapter adapter;
            CHECK(native.observe(frame(70u,2000u)).admitted);
            // A new object is insufficient; exact old world-generation
            // replay must not reset the proven per-session floor.
            CHECK(!NativeModuleRecoveryFence::admit_after_quiescence(
                old_native,old_adapter,old_feed,old_gui,native,
                adapter,fresh_feed,41u,true));
            CHECK(!fresh_feed.latest() && !adapter.retired());
            // Another manager with the SAME generation still cannot
            // publish, regardless of its own valid control_states().
            CHECK(native.observe(frame(71u,2001u)).admitted);
            CHECK(!NativeModuleRecoveryFence::admit_after_quiescence(
                old_native,old_adapter,old_feed,old_gui,native,
                adapter,fresh_feed,40u,true)); // replay sequence
            CHECK(!NativeModuleRecoveryFence::admit_after_quiescence(
                old_native,old_adapter,old_feed,old_gui,native,
                adapter,fresh_feed,41u,false)); // not in world
            // The old quarantined feed can never be the "fresh" one.
            CHECK(!NativeModuleRecoveryFence::admit_after_quiescence(
                old_native,old_adapter,old_feed,old_gui,native,
                adapter,old_feed,41u,true));
            CHECK(!adapter.retired());
            admitted=NativeModuleRecoveryFence::admit_after_quiescence(
                old_native,old_adapter,old_feed,old_gui,native,
                adapter,fresh_feed,41u,true);
            CHECK(admitted);
            const auto first=fresh_feed.latest();
            CHECK(first && first->source_sequence==41u &&
                  first->world_generation==71u && first->world_ready &&
                  first->modules.size()==3u);
            CHECK(!first->modules[1].requested_enabled &&
                  !first->modules[1].active_in_world);
            CHECK(first->modules[2].requested_enabled &&
                  first->modules[2].active_in_world);
            {
                std::lock_guard<std::mutex> g(mu);
                phase=1;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> g(mu);
                cv.wait(g,[&]{return phase>=2;});
            }
            processed=adapter.apply_request_on_owner(
                native,fresh_gui,fresh_feed,42u,true);
            CHECK(processed &&
                  fresh_gui.view().phase==GuiModuleControlPhase::Acknowledged &&
                  native.control_state("No Slow")->active_in_world);
            CHECK(fresh_feed.latest()->modules[1].active_in_world);
            const auto actual=native.observe(frame(71u,2002u));
            CHECK(actual.admitted && actual.no_slow &&
                  actual.no_slow->replace_input);
            old_replay_denied=!old_adapter.publish_on_owner(
                native,old_feed,43u,true);
            CHECK(old_replay_denied && old_feed.retired());
            CHECK(native.shutdown());
            CHECK(adapter.retire_owner(fresh_feed));
            {
                std::lock_guard<std::mutex> g(mu);
                phase=3;
            }
            cv.notify_all();
        } catch(...) {
            owner_error=std::current_exception();
            {
                std::lock_guard<std::mutex> g(mu);
                phase=99;
            }
            cv.notify_all();
        }
    });
    {
        std::unique_lock<std::mutex> g(mu);
        cv.wait(g,[&]{return phase>=1;});
    }
    if(phase==1) {
        CHECK(fresh_gui.request_focused_toggle(menu,nav,fresh_feed,41u));
        CHECK(fresh_gui.view().request &&
              fresh_gui.view().request->canonical_name=="No Slow" &&
              fresh_gui.view().request->enable);
        CHECK(!old_gui.request_focused_toggle(menu,nav,fresh_feed,41u));
        {
            std::lock_guard<std::mutex> g(mu);
            phase=2;
        }
        cv.notify_all();
        {
            std::unique_lock<std::mutex> g(mu);
            cv.wait(g,[&]{return phase>=3;});
        }
    }
    fresh_owner.join();
    if(owner_error)std::rethrow_exception(owner_error);
    CHECK(admitted && processed && old_replay_denied && phase==3);
    CHECK(fresh_feed.retired() && !fresh_feed.latest());
    CHECK(old_feed.retired() && !old_feed.latest());
    std::cout<<"R250_REAL_PR2_QUARANTINE_REQUIRES_OLD_NATIVE_SHUTDOWN_PASS\n";
    std::cout<<"R250_REAL_PR2_NEW_OWNER_AND_STRICT_GENERATION_SEQUENCE_PASS\n";
    std::cout<<"R250_FRESH_GUI_REQUEST_ACK_ACTUAL_NATIVE_POLICY_PASS\n";
}
int main(){
    main_recovery();
    std::cout<<"R250_OFFLINE_ONLY_NO_JVM_ATTACH_INPUT_OR_GAMEPLAY_WRITES\n";
}
