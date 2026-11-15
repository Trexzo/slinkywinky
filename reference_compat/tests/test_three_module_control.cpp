#include "slinkywinky/sprint_observation_preview.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

using namespace slinkywinky;
using namespace slinkywinky::reference_compat;

static void require(bool ok, const char* why, int line) {
    if (!ok) throw std::runtime_error(
        "R211 line " + std::to_string(line) + ": " + why);
}
#define CHECK(x) require(static_cast<bool>(x), #x, __LINE__)

static bool near(float a, float b) { return std::fabs(a-b) < 0.00001f; }

static SprintObservedTick in_world(std::uint64_t generation,
                                  std::uint64_t ms) {
    SprintObservedTick tick;
    tick.observation.generation = generation;
    tick.observation.time_ms = ms;
    tick.observation.flags = 3;
    tick.observation.xyz = {0,0,0,0,0,0};
    tick.observation.angles = {0,0};
    tick.update.player_present = true;
    tick.update.player_sprinting = true;
    tick.movement.player_present = true;
    tick.movement.forward = -0.2f;
    tick.movement.strafe = 0.1f;
    tick.movement.speed = 0.1f;
    tick.movement.using_item = true;
    tick.jump_delay = JumpDelayInput{true, 10};
    tick.no_slow = NoSlowInput{true, true, 261, 0.1f, -0.2f, {}};
    return tick;
}

static SprintObservedTick away(std::uint64_t generation,
                               std::uint64_t ms) {
    SprintObservedTick tick;
    tick.observation.generation = generation;
    tick.observation.time_ms = ms;
    tick.observation.flags = 0;
    tick.update.player_present = false;
    tick.movement.player_present = false;
    return tick;
}

int main() {
    SprintObservationPreview runtime;
    auto states = runtime.control_states();
    CHECK(states && states->size()==3);
    CHECK((*states)[0].canonical_name=="No Jump Delay");
    CHECK((*states)[1].canonical_name=="No Slow");
    CHECK((*states)[2].canonical_name=="Sprint");
    CHECK(!(*states)[0].requested_enabled);
    CHECK(!(*states)[1].requested_enabled);
    CHECK((*states)[2].requested_enabled);
    CHECK(!runtime.control_state("fake"));
    CHECK(!runtime.request_module({"Slow", true}));

    CHECK(runtime.request_module({"No Jump Delay", true}));
    CHECK(runtime.request_module({"No Slow", true}));
    CHECK(runtime.configure_no_slow({0.8f,0.4f,0.6f}));
    CHECK(!runtime.configure_no_slow({-1.0f,0.5f,0.5f}));
    CHECK(!runtime.configure_no_slow({
        std::numeric_limits<float>::infinity(),0.5f,0.5f}));
    CHECK(runtime.control_state("No Slow")->settings_override);
    CHECK(runtime.control_state("No Jump Delay")->requested_enabled);
    CHECK(!runtime.control_state("No Jump Delay")->active_in_world);
    auto accepted=runtime.observe(in_world(10,1000));
    CHECK(accepted.admitted && accepted.world_ready);
    CHECK(accepted.proposed_jump_ticks && *accepted.proposed_jump_ticks==0);
    CHECK(accepted.no_slow && accepted.no_slow->replace_input);
    CHECK(near(accepted.no_slow->strafe,0.4f));
    CHECK(near(accepted.no_slow->forward,-0.8f));
    CHECK(accepted.update.proposed_key_pressed);
    CHECK(!accepted.movement.proposed_speed);
    for (auto name : {"No Jump Delay","No Slow","Sprint"})
        CHECK(runtime.control_state(name)->active_in_world);

    // Same source movement payload drives No Slow and Sprint.
    auto bad=in_world(10,1001);
    bad.no_slow->forward=-0.3f;
    CHECK(!runtime.observe(bad).admitted);
    bad=in_world(10,1001);
    bad.jump_delay->in_game=false;
    CHECK(!runtime.observe(bad).admitted);
    accepted=runtime.observe(in_world(10,1001));
    CHECK(accepted.admitted && accepted.proposed_jump_ticks);

    CHECK(runtime.request_module({"No Slow", false}));
    CHECK(!runtime.control_state("No Slow")->active_in_world);
    CHECK(runtime.control_state("No Jump Delay")->active_in_world);
    accepted=runtime.observe(in_world(10,1002));
    CHECK(accepted.admitted && accepted.proposed_jump_ticks);
    CHECK(!accepted.no_slow);
    CHECK(accepted.update.proposed_key_pressed);

    CHECK(runtime.request_module({"Sprint", false}));
    accepted=runtime.observe(in_world(10,1003));
    CHECK(accepted.admitted && accepted.proposed_jump_ticks);
    CHECK(!accepted.no_slow && !accepted.update.proposed_key_pressed);
    CHECK(!accepted.movement.proposed_speed);
    CHECK(runtime.request_module({"No Jump Delay", false}));
    accepted=runtime.observe(in_world(10,1004));
    CHECK(accepted.admitted && !accepted.proposed_jump_ticks);
    CHECK(!accepted.no_slow && !accepted.update.proposed_key_pressed);

    CHECK(runtime.observe(away(11,1005)).admitted);
    CHECK(!runtime.control_state("No Jump Delay")->active_in_world);
    CHECK(!runtime.control_state("No Slow")->active_in_world);
    CHECK(runtime.control_state("No Slow")->settings_override);
    CHECK(runtime.request_module({"No Slow", true}));
    CHECK(runtime.request_module({"No Jump Delay", true}));
    CHECK(!runtime.control_state("No Slow")->active_in_world);
    accepted=runtime.observe(in_world(12,1006));
    CHECK(accepted.admitted && accepted.session_transition);
    CHECK(accepted.proposed_jump_ticks && accepted.no_slow &&
          accepted.no_slow->replace_input);
    CHECK(!accepted.update.proposed_key_pressed);
    CHECK(runtime.control_state("No Slow")->active_in_world);
    CHECK(runtime.control_state("No Jump Delay")->world_generation==12);

    bool foreign=false;
    std::thread other([&] {
        foreign=!runtime.request_module({"No Slow",false}) &&
            !runtime.configure_no_slow({1,1,1}) &&
            !runtime.control_state("No Slow") &&
            !runtime.control_states().has_value() && !runtime.shutdown();
    });
    other.join();
    CHECK(foreign && runtime.control_state("No Slow")->active_in_world);

    CHECK(runtime.shutdown());
    CHECK(!runtime.control_states());
    CHECK(!runtime.request_module({"No Jump Delay",true}));
    CHECK(!runtime.configure_no_slow({1,1,1}));
    CHECK(!runtime.observe(in_world(13,1007)).admitted);
    std::cout<<"R211_THREE_MODULE_CONTROL_PASS modules=3 callbacks=4 "
             <<"toggle=1 no_slow_settings=1 typed_events=17,21 "
             <<"world_reentry=1 owner_guard=1 game_actions=0\n";
}
