// R267: actual PR2 R211 owner + PR1 R254 independent OFFLINE owner
// published as a single six-module authoritative selection feed.
// Three real native event decision probes; three OFFLINE-only policies.
// No game actions, interception, input generation or JNI hooks.
#include "slinkywinky/combined_module_owner_adapter.hpp"
#include "slinkywinky/module_runtime_roster.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    std::string("R267 line ")+std::to_string(__LINE__)+": " #x);}while(false)

static SprintObservedTick observation(std::uint64_t gen,std::uint64_t time,bool ready) {
    SprintObservedTick t{};
    t.observation.generation=gen;t.observation.time_ms=time;
    t.observation.flags=ready?3u:0u;
    t.update.player_present=ready;
    t.movement.player_present=ready;
    if(ready) {
        t.update.player_sprinting=true;
        t.movement.speed=0.1f;
        t.movement.forward=-0.2f;
        t.movement.strafe=0.1f;
    }
    return t;
}
static const ModuleArrayListState& find_state(
        const ModuleArrayListSnapshot& value,const char* name) {
    for(const auto& item:value.modules)
        if(item.canonical_name==name)return item;
    throw std::runtime_error("R267 source row missing");
}
struct Focus {
    MenuTab tab;
    unsigned row;
    std::size_t index;
    const char* name;
    bool enable;
};
static void click_and_confirm(
        SprintObservationPreview& native,RecoveredSourcePolicyOwner& policy,
        CombinedModuleOwnerAdapter& combined,ModuleArrayListFeed& feed,
        std::uint64_t seq,bool world_ready,const Focus focus) {
    std::mutex mutex;
    std::condition_variable cv;
    bool pending=false,complete=false;
    GuiModuleControlHandoff* channel=nullptr;
    std::exception_ptr gui_error,owner_error;
    std::thread gui([&]{
        try{
            MenuState menu;
            MenuProfileBridge profile(menu);
            MenuNavigation nav(menu,profile);
            CHECK(nav.select_tab(focus.tab));
            for(unsigned i=0;i<focus.row;++i)
                CHECK(nav.apply(MenuNavAction::NextRow));
            CHECK(nav.visible_focus() && *nav.visible_focus()==focus.index);
            CHECK(std::string(official_module_catalog()[focus.index].name)==focus.name);
            GuiModuleControlHandoff handoff;
            CHECK(handoff.request_focused_toggle(menu,nav,feed,seq));
            const auto view=handoff.view();
            CHECK(view.request && view.request->canonical_name==focus.name);
            CHECK(view.request->enable==focus.enable);
            CHECK(view.phase==GuiModuleControlPhase::Pending);
            // Preview and actual source state are deliberately unrelated.
            CHECK(menu.toggle_preview(focus.index));
            {
                std::lock_guard<std::mutex> lock(mutex);
                channel=&handoff;
                pending=true;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock,[&]{return complete;});
            }
            CHECK(handoff.view().phase==GuiModuleControlPhase::Acknowledged);
            CHECK(handoff.view().last_confirmed_active==
                  (focus.enable&&world_ready));
        }catch(...){
            gui_error=std::current_exception();
            {
                std::lock_guard<std::mutex> lock(mutex);
                pending=true;
            }
            cv.notify_all();
        }
    });
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,[&]{return pending;});
    }
    if(!gui_error){
        try {
            CHECK(channel);
            CHECK(combined.apply_request_on_owner(
                native,policy,*channel,feed,seq+1u,world_ready));
            const auto authoritative=feed.latest();
            CHECK(authoritative && authoritative->source_sequence==seq+1u);
            CHECK(find_state(*authoritative,focus.name).requested_enabled==focus.enable);
            CHECK(find_state(*authoritative,focus.name).active_in_world==
                (focus.enable&&world_ready));
            const auto roster=make_module_runtime_roster(*authoritative);
            CHECK(roster && roster->source_reported==6u);
            CHECK(roster->native_owner_reports==3u);
            CHECK(roster->offline_policy_reports==3u);
        }catch(...){owner_error=std::current_exception();}
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        complete=true;
    }
    cv.notify_all();
    gui.join();
    if(gui_error)std::rethrow_exception(gui_error);
    if(owner_error)std::rethrow_exception(owner_error);
}
int main() {
    SprintObservationPreview native;
    RecoveredSourcePolicyOwner policy;
    CombinedModuleOwnerAdapter combined;
    ModuleArrayListFeed feed;
    CHECK(native.observe(observation(30u,1000u,true)).admitted);
    CHECK(policy.transition(30u,true));
    CHECK(combined.publish_on_owner(native,policy,feed,100u,true));
    auto original=feed.latest();
    CHECK(original && original->modules.size()==6u);
    CHECK(find_state(*original,"Sprint").requested_enabled);
    CHECK(find_state(*original,"Sprint").active_in_world);
    CHECK(!find_state(*original,"Safe Walk").requested_enabled);
    auto roster=make_module_runtime_roster(feed);
    CHECK(roster && roster->source_reported==6u &&
          roster->native_registered==66u);
    CHECK(roster->native_owner_reports==3u && roster->offline_policy_reports==3u);
    std::cout<<"R267_SIX_SOURCE_ROW_IDENTITY_AND_PROVENANCE_PASS\n";

    std::uint64_t seq=100u;
    // A real source-focused GUI gesture is sent from a DIFFERENT thread
    // for every owner request. All six decisions require owner ACKs.
    const std::vector<Focus> controls{
        {MenuTab::Move,8u,25u,"Sprint",false},
        {MenuTab::Move,5u,22u,"No Slow",true},
        {MenuTab::Move,4u,21u,"No Jump Delay",true},
        {MenuTab::Utility,11u,60u,"No Hit Delay",true},
        {MenuTab::Move,7u,24u,"Safe Walk",true},
        {MenuTab::Block,5u,47u,"Fast Place",true},
    };
    for(const auto& control:controls) {
        click_and_confirm(native,policy,combined,feed,seq,true,control);
        ++seq;
    }
    CHECK(seq==106u);
    CHECK(!native.control_state("Sprint")->requested_enabled);
    CHECK(native.control_state("No Slow")->requested_enabled);
    CHECK(native.control_state("No Jump Delay")->requested_enabled);
    CHECK(policy.selected(RecoveredSourceModule::NoHitDelay));
    CHECK(policy.selected(RecoveredSourceModule::SafeWalk));
    CHECK(policy.selected(RecoveredSourceModule::FastPlace));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.size()==5u);
    std::cout<<"R267_SIX_GUI_OWNER_TOGGLES_NATIVE_PLUS_OFFLINE_ACK_PASS\n";

    bool foreign=false;
    std::thread outsider([&] {
        foreign=combined.publish_on_owner(native,policy,feed,seq+1u,true);
    });
    outsider.join();
    CHECK(!foreign && !combined.retired() && !feed.retired());
    CHECK(native.observe(observation(31u,1001u,false)).admitted);
    CHECK(policy.transition(31u,false));
    CHECK(combined.publish_on_owner(native,policy,feed,107u,false));
    roster=make_module_runtime_roster(feed);
    CHECK(roster && !roster->world_ready && roster->world_generation==31u);
    CHECK(roster->source_reported==6u);
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    CHECK(find_state(*feed.latest(),"Safe Walk").requested_enabled);
    CHECK(!find_state(*feed.latest(),"Safe Walk").active_in_world);
    CHECK(find_state(*feed.latest(),"No Slow").requested_enabled);
    CHECK(!find_state(*feed.latest(),"No Slow").active_in_world);
    std::cout<<"R267_WORLD_LOSS_RETAINS_SELECTIONS_NO_FALSE_ACTIVE_ROWS_PASS\n";

    // Inter-owner generation drift must quarantine the entire merged feed.
    CHECK(policy.transition(32u,false)); // PR2 still generation 31
    CHECK(!combined.publish_on_owner(native,policy,feed,108u,false));
    CHECK(combined.retired() && feed.retired() && !feed.latest());
    CHECK(!make_module_runtime_roster(feed));
    CHECK(native.shutdown() && policy.retire());
    std::cout<<"R267_MISMATCHED_PROVIDERS_FAIL_CLOSED_AND_RETIRE_PASS\n";
}
