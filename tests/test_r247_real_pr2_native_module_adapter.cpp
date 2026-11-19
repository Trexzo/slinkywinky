// R247: compile and execute the ACTUAL PR #2 native module manager
// headers against PR #1's GUI/ArrayList adapter, without merging sources.
// PR #2 checkout is verified at commit 7f457376058ad42e0a0e0323ed874d7e5b2ae03a.
// No Minecraft JVM, JNI bridge, keyboard/mouse action, packet or game writes.
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

#define CHECK(x) do {if(!(x))throw std::runtime_error("R247 " #x);} while(false)

static SprintObservedTick in_world(std::uint64_t generation,
                                  std::uint64_t stamp) {
    SprintObservedTick frame;
    frame.observation.generation=generation;
    frame.observation.time_ms=stamp;
    frame.observation.flags=3u;
    frame.observation.xyz={0,0,0,0,0,0};
    frame.observation.angles={0,0};
    frame.update.player_present=true;
    frame.update.player_sprinting=true;
    frame.movement.player_present=true;
    frame.movement.forward=-0.2f;
    frame.movement.strafe=0.1f;
    frame.movement.speed=0.1f;
    frame.movement.using_item=true;
    frame.jump_delay=JumpDelayInput{true,10};
    frame.no_slow=NoSlowInput{true,true,261,0.1f,-0.2f,{}};
    return frame;
}
static SprintObservedTick absent(std::uint64_t gen,std::uint64_t stamp) {
    SprintObservedTick frame;
    frame.observation.generation=gen;
    frame.observation.time_ms=stamp;
    frame.observation.flags=0u;
    frame.update.player_present=false;
    frame.movement.player_present=false;
    return frame;
}
static bool has_active(const ModuleArrayListFeed& feed,
                       const std::string& name) {
    auto snapshot=feed.latest();
    if(!snapshot)return false;
    auto display=make_module_arraylist_display(*snapshot);
    return display && std::find(display->active_names.begin(),
        display->active_names.end(),name)!=display->active_names.end();
}

int main() {
    // The native owner below is THE source class, not a substitute
    // with matching method names or manually forged native states.
    SprintObservationPreview native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(native.configure_no_slow({0.8f,0.4f,0.6f}));
    const auto first=native.observe(in_world(30u,1000u));
    CHECK(first.admitted && first.world_ready &&
          first.generation==30u && !first.no_slow);
    CHECK(adapter.publish_on_owner(native,feed,10u,true));
    const auto starting=feed.latest();
    CHECK(starting && starting->modules.size()==3u);
    CHECK(starting->modules[0].canonical_name=="No Jump Delay");
    CHECK(starting->modules[1].canonical_name=="No Slow");
    CHECK(starting->modules[2].canonical_name=="Sprint");
    CHECK(!starting->modules[1].requested_enabled &&
          !starting->modules[1].active_in_world);
    CHECK(starting->modules[2].requested_enabled &&
          starting->modules[2].active_in_world);
    CHECK(!has_active(feed,"No Slow") && has_active(feed,"Sprint"));

    // GUI thread creates the real PR #1 handoff and submits three
    // separate source-focused toggles. Native engine stays on this
    // owning main thread; no caller transfers game pointers.
    std::mutex mutex;
    std::condition_variable cv;
    int requested=0,acknowledged=0;
    GuiModuleControlHandoff* handoff=nullptr;
    std::exception_ptr gui_error;
    bool preview_still_inert=false;
    std::thread gui([&]{
        try {
            MenuState menu;
            MenuProfileBridge profile(menu);
            MenuNavigation navigation(menu,profile);
            CHECK(navigation.select_tab(MenuTab::Move));
            for(int step=0;step<5;++step)
                CHECK(navigation.apply(MenuNavAction::NextRow));
            CHECK(navigation.visible_focus() &&
                  *navigation.visible_focus()==22u);
            GuiModuleControlHandoff channel;
            {
                std::lock_guard<std::mutex> lock(mutex);
                handoff=&channel;
            }
            cv.notify_all();
            for(int request=1;request<=3;++request) {
                if(request>1) {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock,[&]{return acknowledged>=request-1;});
                }
                CHECK(channel.request_focused_toggle(
                    menu,navigation,feed,static_cast<std::uint64_t>(9+request)));
                CHECK(channel.view().request &&
                      channel.view().request->canonical_name=="No Slow");
                if(request==1) {
                    CHECK(menu.toggle_preview(22u));
                    CHECK(menu.preview_enabled(22u));
                }
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    requested=request;
                }
                cv.notify_all();
            }
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock,[&]{return acknowledged>=3;});
            }
            preview_still_inert=menu.preview_enabled(22u);
        } catch(...) {
            gui_error=std::current_exception();
            {
                std::lock_guard<std::mutex> lock(mutex);
                requested=99;
            }
            cv.notify_all();
        }
    });
    for(int step=1;step<=3;++step) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock,[&]{return requested>=step;});
        }
        if(requested==99)break;
        CHECK(handoff!=nullptr);
        const bool expected_enabled=step!=2;
        auto pending=handoff->view();
        CHECK(pending.request && pending.request->id==
              static_cast<std::uint64_t>(step) &&
              pending.request->enable==expected_enabled);
        CHECK(adapter.apply_request_on_owner(native,*handoff,feed,
                                            static_cast<std::uint64_t>(10+step),
                                            true));
        auto result=native.control_state("No Slow");
        CHECK(result && result->requested_enabled==expected_enabled &&
              result->active_in_world==expected_enabled);
        CHECK(has_active(feed,"No Slow")==expected_enabled &&
              has_active(feed,"Sprint"));
        CHECK(handoff->view().phase==GuiModuleControlPhase::Acknowledged);
        auto evaluated=native.observe(
            in_world(30u,static_cast<std::uint64_t>(1000+step)));
        CHECK(evaluated.admitted && evaluated.world_ready);
        CHECK(evaluated.no_slow.has_value()==expected_enabled);
        if(expected_enabled) {
            CHECK(evaluated.no_slow->replace_input);
            CHECK(evaluated.no_slow->forward < -0.79f &&
                  evaluated.no_slow->forward > -0.81f);
            CHECK(evaluated.no_slow->strafe > 0.39f &&
                  evaluated.no_slow->strafe < 0.41f);
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            acknowledged=step;
        }
        cv.notify_all();
    }
    gui.join();
    if(gui_error)std::rethrow_exception(gui_error);
    CHECK(acknowledged==3 && preview_still_inert);
    CHECK(feed.latest() && feed.latest()->source_sequence==13u);

    // Real native ledger transitions and callback retirement after world
    // exit. The GUI still requests No Slow, but it cannot display active.
    const auto lost=native.observe(absent(31u,1004u));
    CHECK(lost.admitted && !lost.world_ready && lost.generation==31u);
    CHECK(adapter.publish_on_owner(native,feed,14u,false));
    auto worldless=feed.latest();
    CHECK(worldless && !worldless->world_ready &&
          worldless->world_generation==31u);
    CHECK(worldless->modules[1].requested_enabled &&
          !worldless->modules[1].active_in_world);
    CHECK(!has_active(feed,"No Slow") && !has_active(feed,"Sprint"));
    // Native owner re-entry creates new event handlers, not stale
    // HUD feedback from menu.preview_enabled.
    const auto returned=native.observe(in_world(32u,1005u));
    CHECK(returned.admitted && returned.world_ready &&
          returned.session_transition && returned.no_slow &&
          returned.no_slow->replace_input);
    CHECK(adapter.publish_on_owner(native,feed,15u,true));
    CHECK(has_active(feed,"No Slow") && has_active(feed,"Sprint"));

    bool foreign=false;
    std::thread stranger([&] {
        foreign=!adapter.publish_on_owner(native,feed,16u,true) &&
                !native.control_states().has_value() &&
                !native.request_module({"No Slow",false});
    });
    stranger.join();
    CHECK(foreign && !adapter.retired() && !feed.retired());
    CHECK(native.shutdown());
    CHECK(adapter.retire_owner(feed));
    CHECK(adapter.retired() && feed.retired() && !feed.latest());
    std::cout<<"R247_ACTUAL_PR2_NATIVE_NO_SLOW_GUI_ENABLE_DISABLE_REENABLE_PASS\n";
    std::cout<<"R247_ACTUAL_PR2_NATIVE_POLICY_DECISION_AND_WORLD_LOSS_RECOVERY_PASS\n";
    std::cout<<"R247_PR1_PR2_CROSS_SOURCE_COMPILE_AND_OWNER_ACK_PASS\n";
    std::cout<<"R247_NO_MINECRAFT_JVM_JNI_INPUT_OR_ACTION_WRITE_PASS\n";
}
