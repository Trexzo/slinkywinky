// R254 source comparison uses REAL pinned PR2 policy source in a
// separate source tree. The PR1 policies are independently implemented.
#include "slinkywinky/recovered_source_module_policies.hpp"
#include "slinkywinky/no_hit_delay_controller.hpp"
#include "slinkywinky/safe_walk_controller.hpp"
#include "slinkywinky/fast_place_controller.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky;
namespace ref=slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R254 parity line "+std::to_string(__LINE__)+": " #x);}while(false)
static bool eq(double a,double b){
    return a==b || std::abs(a-b)<1e-5;
}
static void hit_parity(){
    ref::NoHitDelayController original;
    std::uint64_t cases=0;
    for(int click=0;click<2;++click)
    for(int ready=0;ready<2;++ready)
    for(int counter : {-1,0,1,10})
    for(unsigned mode : {0u,1u,2u,99u})
    for(int target=0;target<2;++target)
    for(int ordinal : {0,1,2})
    for(int item=0;item<2;++item){
        RecoveredNoHitInput input;
        input.attack_click=click;input.jvm_ready=ready;
        input.blocked_ticks=counter;input.mode=mode;
        input.target_present=target;input.target_kind=ordinal;
        input.using_item=item;
        ref::NoHitDelayInput source;
        source.attack_click=click;source.jvm_ready=ready;
        source.left_click_counter=counter;source.mode=mode;
        source.target_present=target;source.target_type_ordinal=ordinal;
        source.player_using_item=item;
        auto a=decide_no_hit_delay(input),b=original.evaluate(source);
        CHECK(a.reset_counter==b.clear_click_counter &&
              a.propose_swing==b.swing_arm);
        ++cases;
    }
    CHECK(cases==768u);
    std::cout<<"R254_PINNED_NO_HIT_DELAY_POLICY_PARITY_768_PASS\n";
}
static void walk_parity(){
    ref::SafeWalkController original;
    std::uint64_t cases=0;
    for(bool game:{false,true})
    for(float pitch : {64.9f,65.0f,90.0f,
                      std::numeric_limits<float>::quiet_NaN()})
    for(float forward : {-0.5f,0.0f,0.5f})
    for(bool blocks:{false,true})
    for(bool ground:{false,true})
    for(bool look:{false,true})
    for(bool no_forward:{false,true})
    for(bool require_blocks:{false,true})
    for(float multiplier : {0.0f,0.5f,1.0f,2.0f,
                            std::numeric_limits<float>::infinity()}){
        RecoveredSafeWalkInput ours;
        ours.in_game=game;ours.pitch=pitch;ours.forward=forward;
        ours.holding_blocks=blocks;ours.on_ground=ground;
        ours.motion_x=0.8;ours.motion_y=0.25;ours.motion_z=-0.4;
        ours.settings={look,no_forward,require_blocks,multiplier};
        ref::SafeWalkInput theirs;
        theirs.in_game=game;theirs.pitch_degrees=pitch;
        theirs.move_forward=forward;theirs.holding_blocks=blocks;
        theirs.on_ground=ground;theirs.motion_x=0.8;
        theirs.motion_y=0.25;theirs.motion_z=-0.4;
        theirs.settings={look,no_forward,require_blocks,multiplier};
        const auto a=decide_safe_walk(ours),b=original.evaluate(theirs);
        CHECK(a.edge_guard==b.proposed_edge_guard_active);
        CHECK(a.motion.has_value()==b.proposed_motion.has_value());
        if(a.motion){
            CHECK(eq(a.motion->x,b.proposed_motion->x));
            CHECK(eq(a.motion->y,b.proposed_motion->y));
            CHECK(eq(a.motion->z,b.proposed_motion->z));
        }
        ++cases;
    }
    CHECK(cases==3840u);
    std::cout<<"R254_PINNED_SAFE_WALK_POLICY_PARITY_3840_PASS\n";
}
static void place_parity(){
    std::uint64_t ticks=0;
    for(float cps : {1.0f,2.0f,10.0f,20.0f,40.0f,100.0f})
    for(float delay : {0.0f,1.0f,3.0f})
    for(bool ladder:{false,true})
    for(bool overlap:{false,true}){
        ref::FastPlaceController original;
        RecoveredFastPlaceModel ours;
        // Simulate 60 source-order ticks including release, loss of
        // in-game context, a pending vanilla click and a high-CPS burst.
        for(int tick=0;tick<60;++tick){
            const bool held=tick>3 && tick<45;
            ref::FastPlaceInput a;
            a.in_game=tick<51;
            a.use_key_held=held;
            a.block_item_held=true;
            a.use_press_already_pending=tick==8;
            a.target_is_block=tick!=20;
            a.adjacent_block_overlaps_player=overlap;
            a.item_is_ladder=ladder;
            a.random_bypass_granted=tick%7==0;
            a.max_cps=cps;
            a.activation_delay_ticks=delay;
            RecoveredFastPlaceInput b;
            b.in_game=a.in_game;b.use_held=a.use_key_held;
            b.holds_block=a.block_item_held;
            b.press_pending=a.use_press_already_pending;
            b.target_block=a.target_is_block;
            b.placement_intersects_player=a.adjacent_block_overlaps_player;
            b.held_ladder=a.item_is_ladder;
            b.random_bypass=a.random_bypass_granted;
            b.cps=cps;b.activation_ticks=delay;
            const auto expected=original.evaluate(a),actual=ours.tick(b);
            CHECK(actual.proposed_presses==expected.extra_use_press_count);
            CHECK(actual.valid==expected.admitted);
            CHECK(eq(actual.timer,expected.timer_after));
            CHECK(actual.delay_left==expected.delay_remaining);
            CHECK(eq(ours.timer(),original.timer()));
            CHECK(ours.was_held()==original.was_held());
            ++ticks;
        }
    }
    CHECK(ticks==4320u);
    // Invalid reciprocal settings do not generate unbounded clicks.
    ref::FastPlaceController source;
    RecoveredFastPlaceModel ours;
    for(float cps : {0.0f,-1.0f,101.0f,
                     std::numeric_limits<float>::quiet_NaN()}){
        ref::FastPlaceInput a;
        a.in_game=true;a.use_key_held=true;a.max_cps=cps;
        RecoveredFastPlaceInput b;
        b.in_game=true;b.use_held=true;b.cps=cps;
        CHECK(ours.tick(b).valid==source.evaluate(a).admitted);
    }
    std::cout<<"R254_PINNED_FAST_PLACE_TICK_SEQUENCE_4320_PASS\n";
}
int main(){
    hit_parity();
    walk_parity();
    place_parity();
    std::cout<<"R254_THREE_CATEGORIES_SOURCE_PARITY_PROPOSALS_ONLY_PASS\n";
}
