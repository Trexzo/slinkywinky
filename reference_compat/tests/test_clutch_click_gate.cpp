#include "slinkywinky/clutch_click_gate.hpp"
#include <cstdio>
#include <cstdlib>
#define CHECK(condition) do { if (!(condition)) {\
    std::fprintf(stderr, "R247 CHECK FAILED line %d: %s\n", __LINE__, #condition); \
    std::exit(1); \
} } while (false)
#include <cmath>
#include <limits>
#include <optional>

using namespace slinkywinky::reference_compat;

static ClutchClickObservation sample(float cps, bool ready, int face,
                                      int x=1, int y=2, int z=3) {
    return {cps,ready,ClutchClickHit{true,{x,y,z},face}};
}
static ClutchClickState state(bool centered=false) {
    return {0.0f,true,centered,{1,2,3}};
}
static bool near(float a,float b) { return std::fabs(a-b)<0.0001f; }

int main() {
    // Source's event-0 semantics: 20/CPS timer, minus one per callback,
    // with remainder carried across clicks (no arbitrary rounding).
    auto step = evaluate_clutch_click(state(),sample(10.0f,true,5));
    CHECK(step && step->press_proposed && near(step->next.timer,2.0f));
    CHECK(step->next.target.x == 2 && step->next.target.y == 2);
    auto next = step->next;
    next.target = {1,2,3}; // Model source event-18 re-targeting separately.
    step = evaluate_clutch_click(next,sample(10.0f,true,5));
    CHECK(step && !step->press_proposed && near(step->next.timer,1.0f));
    step = evaluate_clutch_click(step->next,sample(10.0f,true,5));
    CHECK(step && step->press_proposed && near(step->next.timer,2.0f));

    // Fractional CPS is retained as a float remainder across callbacks.
    next=state();
    const bool expected[] = {true,false,true,false,true,true,false,true};
    for(bool pressed:expected) {
        step=evaluate_clutch_click(next,sample(12.5f,true,2));
        CHECK(step && step->press_proposed==pressed);
        next=step->next;next.target={1,2,3};
    }
    // Exact facing gate: UP only for centered, any non-UP for sideways.
    for(int face=0;face<6;++face) {
        step=evaluate_clutch_click(state(true),sample(20,true,face));
        CHECK(step && step->press_proposed==(face==1));
        step=evaluate_clutch_click(state(false),sample(20,true,face));
        CHECK(step && step->press_proposed==(face!=1));
    }
    // World readiness/placement and bad hit gate do not stop timer advance.
    step=evaluate_clutch_click(state(),sample(20,false,5));
    CHECK(step && !step->press_proposed && near(step->next.timer,1));
    auto not_placing=state();not_placing.placement=false;
    step=evaluate_clutch_click(not_placing,sample(20,true,5));
    CHECK(step && !step->press_proposed && near(step->next.timer,1));
    step=evaluate_clutch_click(state(),sample(20,true,5,9,2,3));
    CHECK(step && !step->press_proposed);
    auto no_block=sample(20,true,5); no_block.ray_hit->is_block=false;
    step=evaluate_clutch_click(state(),no_block);
    CHECK(step && !step->press_proposed);
    auto absent=sample(20,true,5);absent.ray_hit.reset();
    step=evaluate_clutch_click(state(),absent);
    CHECK(step && !step->press_proposed);
    // Face-to-adjacent destination mapping, retaining the source target.
    const ClutchClickTarget expected_pos[] = {
        {1,1,3},{1,3,3},{1,2,2},{1,2,4},{0,2,3},{2,2,3}
    };
    for(int face=0;face<6;++face) {
        auto s=state(face==1);
        step=evaluate_clutch_click(s,sample(20,true,face));
        CHECK(step && step->press_proposed);
        CHECK(step->next.target.x==expected_pos[face].x &&
               step->next.target.y==expected_pos[face].y &&
               step->next.target.z==expected_pos[face].z);
    }
    // Malformed evidence refuses the complete transaction; no game action.
    auto invalid=sample(0,true,5);
    CHECK(!evaluate_clutch_click(state(),invalid));
    invalid.clicks_per_second=std::numeric_limits<float>::quiet_NaN();
    CHECK(!evaluate_clutch_click(state(),invalid));
    invalid=sample(21,true,5);
    CHECK(!evaluate_clutch_click(state(),invalid));
    invalid=sample(20,true,6);
    step=evaluate_clutch_click(state(),invalid);
    CHECK(step && !step->press_proposed);
    auto bad_state=state();bad_state.timer=std::numeric_limits<float>::infinity();
    CHECK(!evaluate_clutch_click(bad_state,sample(20,true,5)));
    bad_state=state();bad_state.timer=-2.0f;
    CHECK(!evaluate_clutch_click(bad_state,sample(20,true,5)));
    bad_state=state();bad_state.target.x=std::numeric_limits<int>::max();
    CHECK(!evaluate_clutch_click(bad_state,sample(20,true,5,
                 std::numeric_limits<int>::max(),2,3)));
    // The gate never has a Minecraft pointer, press callback or output port.
}
