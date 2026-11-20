// R257 pinned PR2 Reach event-8 execution bridge parity against
// independently integrated PR1 event-8 owner containing R256 policies.
#include "slinkywinky/recovered_event8_query_owner.hpp"
#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky;
namespace pr2=slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x))throw std::runtime_error( \
    "R257 pinned parity line "+std::to_string(__LINE__)+": " #x);}while(false)
int main(){
    pr2::ModuleExecutionBridge native;
    const auto reach=native.add({"Reach",0,{{8u,7}},false});
    CHECK(native.bind_reach(reach));
    CHECK(native.request(reach,true));
    CHECK(native.apply(true,true));
    RecoveredEvent8Owner ours;
    CHECK(ours.transition(42u,true));
    CHECK(ours.select(RecoveredEvent8Module::Reach,true));
    CHECK(ours.select(RecoveredEvent8Module::Hitboxes,true));
    CHECK(ours.select(RecoveredEvent8Module::GhostHand,true));
    RecoveredEvent8Input sample;
    sample.reach.sprinting=true;
    sample.reach_settings.distance=4.25f;
    sample.reach_settings.activation_ticks=3.0f;
    sample.expansion=0.75f;
    sample.ghost_settings.allow_fists=true;
    sample.ghost_settings.ignore_non_players=true;
    int hit_proposals=0,world_generations=0;
    for(int tick=0;tick<140;++tick){
        if(tick==70){
            CHECK(ours.transition(42,false));
            CHECK(ours.transition(43,true));
            native.reset_reach_world_state();
            ++world_generations;
        }
        sample.reach.sprinting=(tick%9!=0);
        sample.reach.last_attacker_time=tick<35?0:
            tick<75?33:tick<100?44:77;
        pr2::ExecutionFrame frame;
        frame.event=pr2::kReachQueryEvent;
        frame.world_present=true;
        frame.player_present=true;
        pr2::ReachQueryPayload payload;
        payload.input=sample.reach;
        payload.settings=sample.reach_settings;
        frame.reach=payload;
        CHECK(native.dispatch(frame));
        const auto result=ours.query(8,sample);
        CHECK(result && result->reach_callback_ran);
        CHECK(frame.reach && frame.reach->executions==1);
        CHECK(frame.reach->proposed_distance==result->proposed_reach);
        CHECK(native.reach_cooldown_ticks()==ours.reach_cooldown_ticks());
        CHECK(result->hitboxes && result->hitboxes->proposed_active &&
              result->hitboxes->expansion_per_side==0.75);
        CHECK(result->ghost_hand && result->ghost_hand->reach_query_marker &&
              result->ghost_hand->ignore_entity);
        if(result->proposed_reach)++hit_proposals;
    }
    CHECK(hit_proposals>0 && world_generations==1);
    CHECK(!ours.query(27,sample));
    CHECK(ours.retire());
    native.stop();
    std::cout<<"R257_REAL_PINNED_PR2_REACH_CALLBACK_140_FRAMES_PARITY_PASS\n";
    std::cout<<"R257_R256_HITBOXES_AND_GHOSTHAND_SAME_EVENT8_INPUT_PASS\n";
    std::cout<<"R257_VALUE_ONLY_NO_JAVA8_OR_MINECRAFT_WRITE_PASS\n";
}
