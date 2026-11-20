// R248 — all three PR2 R211 native modules exercised through PR1's
// actual source-owned GUI request/acknowledgement and ArrayList path.
// Isolated first-party offline C++ integration; NO Minecraft, JNI, OS input
// interception, actual player writes, packets, or game-account actions.
#include "slinkywinky/native_module_snapshot_adapter.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(expr) do { if(!(expr)) throw std::runtime_error( \
    std::string("R248 failure line ")+std::to_string(__LINE__)+": " #expr); } while(false)

static SprintObservedTick world_tick(std::uint64_t gen,std::uint64_t millis) {
    SprintObservedTick t;
    t.observation.generation=gen;
    t.observation.time_ms=millis;
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
static SprintObservedTick away_tick(std::uint64_t gen,std::uint64_t millis) {
    SprintObservedTick t;
    t.observation.generation=gen;
    t.observation.time_ms=millis;
    t.observation.flags=0u;
    t.update.player_present=false;
    t.movement.player_present=false;
    return t;
}
static bool visible(const ModuleArrayListFeed& feed,const std::string& name) {
    const auto copy=feed.latest();
    if(!copy)return false;
    const auto display=make_module_arraylist_display(*copy);
    if(!display)return false;
    return std::find(display->active_names.begin(),display->active_names.end(),
                     name)!=display->active_names.end();
}
struct Focus {
    const char* name;
    unsigned row;
    std::size_t catalog;
};
static constexpr Focus kJump{"No Jump Delay",4u,21u};
static constexpr Focus kSlow{"No Slow",5u,22u};
static constexpr Focus kSprint{"Sprint",8u,25u};

// GUI and native owner use DIFFERENT threads. The only shared object is the
// value-only PR1 request handoff. No native manager call runs on GUI thread.
static void click_and_confirm(SprintObservationPreview& native,
                              NativeModuleSnapshotAdapter& adapter,
                              ModuleArrayListFeed& feed,
                              Focus selection,std::uint64_t before_seq,
                              bool world_ready,bool expected_enabled) {
    std::mutex lock;
    std::condition_variable cv;
    bool pending=false,done=false;
    GuiModuleControlHandoff* channel=nullptr;
    std::exception_ptr gui_error,owner_error;
    std::thread gui([&]{
        try {
            MenuState menu;
            MenuProfileBridge profile(menu);
            MenuNavigation navigation(menu,profile);
            CHECK(navigation.select_tab(MenuTab::Move));
            for(unsigned i=0;i<selection.row;++i)
                CHECK(navigation.apply(MenuNavAction::NextRow));
            CHECK(navigation.visible_focus() &&
                  *navigation.visible_focus()==selection.catalog);
            GuiModuleControlHandoff handoff;
            CHECK(handoff.request_focused_toggle(
                menu,navigation,feed,before_seq));
            const auto request=handoff.view().request;
            CHECK(request && request->canonical_name==selection.name &&
                  request->enable==expected_enabled &&
                  request->observed_source_sequence==before_seq);
            // The local preview can intentionally disagree with the
            // native decision; it must never publish an active HUD row.
            CHECK(menu.toggle_preview(selection.catalog));
            {
                std::lock_guard<std::mutex> guard(lock);
                channel=&handoff;
                pending=true;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> guard(lock);
                cv.wait(guard,[&]{return done;});
            }
            CHECK(handoff.view().phase==GuiModuleControlPhase::Acknowledged);
            CHECK(handoff.view().last_confirmed_active==
                  (expected_enabled && world_ready));
            CHECK(menu.preview_enabled(selection.catalog));
        } catch(...) {
            gui_error=std::current_exception();
            {
                std::lock_guard<std::mutex> guard(lock);
                pending=true;
            }
            cv.notify_all();
        }
    });
    {
        std::unique_lock<std::mutex> guard(lock);
        cv.wait(guard,[&]{return pending;});
    }
    if(!gui_error) {
        try {
            CHECK(channel);
            CHECK(adapter.apply_request_on_owner(
                native,*channel,feed,before_seq+1,world_ready));
            const auto result=native.control_state(selection.name);
            CHECK(result && result->requested_enabled==expected_enabled);
            CHECK(result->active_in_world==(expected_enabled&&world_ready));
            CHECK(feed.latest() &&
                  feed.latest()->source_sequence==before_seq+1);
            CHECK(visible(feed,selection.name)==(expected_enabled&&world_ready));
        } catch(...) {owner_error=std::current_exception();}
    }
    {
        std::lock_guard<std::mutex> guard(lock);
        done=true;
    }
    cv.notify_all();
    gui.join();
    if(gui_error)std::rethrow_exception(gui_error);
    if(owner_error)std::rethrow_exception(owner_error);
}
static void verify_decisions(SprintObservationPreview& native,
                             ModuleArrayListFeed& feed,
                             std::uint64_t gen,std::uint64_t timestamp,
                             bool sprint,bool jump,bool slow) {
    const auto observed=native.observe(world_tick(gen,timestamp));
    CHECK(observed.admitted && observed.world_ready &&
          observed.generation==gen);
    CHECK(observed.update.proposed_key_pressed==sprint);
    CHECK(observed.proposed_jump_ticks.has_value()==jump);
    if(jump) CHECK(*observed.proposed_jump_ticks==0);
    CHECK(observed.no_slow.has_value()==slow);
    if(slow) {
        CHECK(observed.no_slow->replace_input);
        CHECK(observed.no_slow->forward < -0.79f &&
              observed.no_slow->forward > -0.81f);
        CHECK(observed.no_slow->strafe > 0.39f &&
              observed.no_slow->strafe < 0.41f);
    }
    CHECK(visible(feed,"Sprint")==sprint);
    CHECK(visible(feed,"No Jump Delay")==jump);
    CHECK(visible(feed,"No Slow")==slow);
}
int main(){
    // This is the ACTUAL pinned PR2 manager, instantiated from the real
    // reference_compat header by the separate R247-R248 CI checkout.
    SprintObservationPreview native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(native.configure_no_slow({0.8f,0.4f,0.6f}));
    CHECK(native.observe(world_tick(90u,1000u)).admitted);
    CHECK(adapter.publish_on_owner(native,feed,100u,true));
    verify_decisions(native,feed,90u,1001u,true,false,false);

    click_and_confirm(native,adapter,feed,kSprint,100u,true,false);
    verify_decisions(native,feed,90u,1002u,false,false,false);
    click_and_confirm(native,adapter,feed,kJump,101u,true,true);
    verify_decisions(native,feed,90u,1003u,false,true,false);
    click_and_confirm(native,adapter,feed,kSlow,102u,true,true);
    verify_decisions(native,feed,90u,1004u,false,true,true);
    click_and_confirm(native,adapter,feed,kSprint,103u,true,true);
    verify_decisions(native,feed,90u,1005u,true,true,true);
    std::cout<<"R248_ACTUAL_PR2_THREE_MODULE_GUI_ACK_AND_NATIVE_DECISIONS_PASS\n";

    // Original native manager retires active callbacks at world loss,
    // while preserving all three requested selections.
    const auto lost=native.observe(away_tick(91u,1006u));
    CHECK(lost.admitted && !lost.world_ready && lost.generation==91u);
    CHECK(adapter.publish_on_owner(native,feed,105u,false));
    const auto worldless=feed.latest();
    CHECK(worldless && !worldless->world_ready &&
          worldless->world_generation==91u &&
          worldless->modules.size()==3u);
    for(const auto& item:worldless->modules)
        CHECK(item.requested_enabled && !item.active_in_world);
    CHECK(!visible(feed,"Sprint") &&
          !visible(feed,"No Jump Delay") &&
          !visible(feed,"No Slow"));

    // A genuine source-owned GUI command can change requested state
    // while the world is absent, without falsely activating callbacks.
    click_and_confirm(native,adapter,feed,kSlow,105u,false,false);
    CHECK(!native.control_state("No Slow")->active_in_world);
    CHECK(native.control_state("Sprint")->requested_enabled);
    CHECK(native.control_state("No Jump Delay")->requested_enabled);
    const auto entered=native.observe(world_tick(92u,1007u));
    CHECK(entered.admitted && entered.world_ready &&
          entered.session_transition &&
          entered.proposed_jump_ticks &&
          !entered.no_slow);
    CHECK(adapter.publish_on_owner(native,feed,107u,true));
    verify_decisions(native,feed,92u,1008u,true,true,false);

    click_and_confirm(native,adapter,feed,kSlow,107u,true,true);
    verify_decisions(native,feed,92u,1009u,true,true,true);
    click_and_confirm(native,adapter,feed,kJump,108u,true,false);
    verify_decisions(native,feed,92u,1010u,true,false,true);
    std::cout<<"R248_ACTUAL_PR2_WORLD_LOSS_WORLDLESS_TOGGLE_REENTRY_PASS\n";

    // Source snapshots never give a foreign GUI thread permission to
    // drive the PR2 native manager or publish authoritative HUD state.
    bool outsider_denied=false;
    std::thread outsider([&]{
        outsider_denied=!adapter.publish_on_owner(native,feed,110u,true) &&
            !native.control_states().has_value() &&
            !native.request_module({"Sprint",false});
    });
    outsider.join();
    CHECK(outsider_denied && !adapter.retired() && !feed.retired());
    CHECK(native.shutdown());
    CHECK(adapter.retire_owner(feed));
    CHECK(adapter.retired() && feed.retired() && !feed.latest());
    std::cout<<"R248_ACTUAL_PR2_UNLOAD_AND_FOREIGN_OWNER_REFUSAL_PASS\n";
    std::cout<<"R248_NO_MINECRAFT_INPUT_PACKET_OR_GAMEPLAY_WRITES\n";
}
