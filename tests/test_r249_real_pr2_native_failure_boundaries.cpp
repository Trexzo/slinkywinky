// R249 — PR1 native adapter failure boundaries using the ACTUAL pinned
// PR2 SprintObservationPreview and thin, test-only fault interceptors.
// No game, Java, injected client, packets or gameplay writes.
#include "slinkywinky/native_module_snapshot_adapter.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
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
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    std::string("R249 line ")+std::to_string(__LINE__)+": " #x);}while(false)

static SprintObservedTick sample(std::uint64_t gen,std::uint64_t time,
                                bool player=true) {
    SprintObservedTick t;
    t.observation.generation=gen;
    t.observation.time_ms=time;
    t.observation.flags=player?3u:0u;
    t.observation.xyz={0,0,0,0,0,0};
    t.observation.angles={0,0};
    t.update.player_present=player;
    t.movement.player_present=player;
    t.update.player_sprinting=player;
    t.movement.forward=-0.2f;
    t.movement.strafe=0.1f;
    t.movement.speed=0.1f;
    t.movement.using_item=player;
    if(player) {
        t.jump_delay=JumpDelayInput{true,10};
        t.no_slow=NoSlowInput{true,true,261,0.1f,-0.2f,{}};
    }
    return t;
}
// Interceptors leave the native policy implementation untouched.
// Only the boundary return/throw/ack behavior is injected.
enum class Fault {
    None, StableDenial, DenialAfterMutation, ThrowBeforeMutation,
    ThrowAfterMutation, RetireAckAfterMutation, StateReadThrows
};
struct NativeBoundary {
    SprintObservationPreview& real;
    GuiModuleControlHandoff* receiver=nullptr;
    Fault fault=Fault::None;
    std::uint64_t generation() const noexcept {return real.generation();}
    auto control_states() const {
        if(fault==Fault::StateReadThrows)
            throw std::runtime_error("R249 synthetic owner read exception");
        return real.control_states();
    }
    bool request_module(ModuleControlRequest request) {
        if(fault==Fault::StableDenial)return false;
        if(fault==Fault::ThrowBeforeMutation)
            throw std::runtime_error("R249 synthetic pre-call exception");
        const bool accepted=real.request_module(request);
        if(!accepted)return false;
        if(fault==Fault::DenialAfterMutation)return false;
        if(fault==Fault::ThrowAfterMutation)
            throw std::runtime_error("R249 synthetic post-call exception");
        if(fault==Fault::RetireAckAfterMutation) {
            CHECK(receiver!=nullptr);
            receiver->retire(); // receiver vanished after real native state write
        }
        return true;
    }
};

// Source-side GUI request always originates on its own owner thread.
// Native owner and its PR2 manager stay on the calling thread.
struct AwaitingRequest {
    std::mutex mutex;
    std::condition_variable cv;
    bool pending=false;
    bool done=false;
    GuiModuleControlHandoff* receiver=nullptr;
    std::exception_ptr gui_error;
    GuiModuleControlPhase final_phase=GuiModuleControlPhase::Idle;
    std::thread gui;

    void start(const ModuleArrayListFeed& feed,std::uint64_t sequence) {
        gui=std::thread([this,&feed,sequence]{
            try {
                MenuState menu;
                MenuProfileBridge profile(menu);
                MenuNavigation navigation(menu,profile);
                CHECK(navigation.select_tab(MenuTab::Move));
                for(int i=0;i<5;++i)CHECK(
                    navigation.apply(MenuNavAction::NextRow));
                CHECK(navigation.visible_focus() &&
                      *navigation.visible_focus()==22u);
                GuiModuleControlHandoff handoff;
                CHECK(handoff.request_focused_toggle(
                    menu,navigation,feed,sequence));
                CHECK(handoff.view().request &&
                      handoff.view().request->canonical_name=="No Slow" &&
                      handoff.view().request->enable);
                CHECK(menu.toggle_preview(22u));
                {
                    std::lock_guard<std::mutex> guard(mutex);
                    receiver=&handoff;
                    pending=true;
                }
                cv.notify_all();
                {
                    std::unique_lock<std::mutex> guard(mutex);
                    cv.wait(guard,[this]{return done;});
                }
                final_phase=handoff.view().phase;
                CHECK(menu.preview_enabled(22u)); // GUI preview stays inert
            } catch(...) {
                gui_error=std::current_exception();
                {
                    std::lock_guard<std::mutex> guard(mutex);
                    pending=true;
                }
                cv.notify_all();
            }
        });
        std::unique_lock<std::mutex> guard(mutex);
        cv.wait(guard,[this]{return pending;});
    }
    void finish() {
        {
            std::lock_guard<std::mutex> guard(mutex);
            done=true;
        }
        cv.notify_all();
        gui.join();
        if(gui_error)std::rethrow_exception(gui_error);
    }
};
static void run_failure(Fault fault,bool drift_world=false,
                        bool shutdown_before=false) {
    SprintObservationPreview real;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(real.observe(sample(7u,1000u)).admitted);
    CHECK(adapter.publish_on_owner(real,feed,20u,true));
    CHECK(feed.latest() && feed.latest()->modules.size()==3u);
    CHECK(feed.latest()->modules[1].canonical_name=="No Slow");
    CHECK(!feed.latest()->modules[1].active_in_world);
    AwaitingRequest gui;
    gui.start(feed,20u);
    CHECK(gui.receiver!=nullptr);
    NativeBoundary boundary{real,gui.receiver,fault};
    if(drift_world) {
        const auto world_loss=real.observe(sample(8u,1001u,false));
        CHECK(world_loss.admitted && !world_loss.world_ready);
        // This real manager now has a different generation, so the
        // pending command must never reach its request_module.
        boundary.fault=Fault::ThrowBeforeMutation;
    }
    if(shutdown_before) {
        CHECK(real.shutdown());
        boundary.fault=Fault::ThrowBeforeMutation;
    }
    const bool accepted=adapter.apply_request_on_owner(
        boundary,*gui.receiver,feed,21u,true);
    CHECK(!accepted);
    gui.finish();
    if(fault==Fault::StableDenial && !drift_world && !shutdown_before){
        CHECK(!adapter.retired() && !feed.retired());
        CHECK(gui.final_phase==GuiModuleControlPhase::Denied);
        CHECK(feed.latest() && feed.latest()->source_sequence==20u);
        CHECK(!real.control_state("No Slow")->requested_enabled);
        CHECK(!real.control_state("No Slow")->active_in_world);
        CHECK(adapter.retire_owner(feed));
    } else {
        CHECK(adapter.retired() && feed.retired() && !feed.latest());
        CHECK(gui.final_phase==GuiModuleControlPhase::Retired);
        if(!shutdown_before && !drift_world &&
           (fault==Fault::DenialAfterMutation ||
            fault==Fault::ThrowAfterMutation ||
            fault==Fault::RetireAckAfterMutation)) {
            // Actual PR2 state HAS changed, yet zero active HUD rows
            // or optimistic GUI acknowledgements can be published.
            CHECK(real.control_state("No Slow")->requested_enabled);
            CHECK(real.control_state("No Slow")->active_in_world);
        }
    }
    if(!shutdown_before)CHECK(real.shutdown());
}
int main(){
    run_failure(Fault::StableDenial);
    run_failure(Fault::DenialAfterMutation);
    run_failure(Fault::ThrowBeforeMutation);
    run_failure(Fault::ThrowAfterMutation);
    run_failure(Fault::RetireAckAfterMutation);
    run_failure(Fault::StateReadThrows);
    run_failure(Fault::None,true,false);  // real-world generation changed
    run_failure(Fault::None,false,true);  // real native shutdown/read refused
    std::cout<<"R249_REAL_NATIVE_DENIAL_VS_AMBIGUOUS_MUTATION_FENCE_PASS\n";
    std::cout<<"R249_REAL_NATIVE_EXCEPTION_INTERRUPTED_ACK_AND_STALE_WORLD_PASS\n";

    // Even a read-only native state publication may throw. It must
    // retire the previous visible source state, not propagate a throw
    // through the GUI's rendering/render-owner boundary.
    SprintObservationPreview real;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(real.observe(sample(50u,5000u)).admitted);
    CHECK(adapter.publish_on_owner(real,feed,100u,true));
    CHECK(feed.latest() && !feed.retired());
    NativeBoundary broken{real,nullptr,Fault::StateReadThrows};
    CHECK(!adapter.publish_on_owner(broken,feed,101u,true));
    CHECK(adapter.retired() && feed.retired() && !feed.latest());
    CHECK(real.shutdown());
    std::cout<<"R249_REAL_NATIVE_SNAPSHOT_READ_EXCEPTION_QUARANTINE_PASS\n";
    std::cout<<"R249_NO_GAMEPLAY_CLIENT_ATTACH_OR_NATIVE_ACTION_ROLLBACK_CLAIM\n";
}
