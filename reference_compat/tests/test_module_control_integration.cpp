#include "slinkywinky/sprint_observation_preview.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace slinkywinky;
using namespace slinkywinky::reference_compat;

static void check(bool condition, const char* expression, int line) {
    if (!condition) throw std::runtime_error(
        "R210 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

static SprintObservedTick frame(std::uint64_t generation,
                                std::uint64_t ms, bool in_world = true) {
    SprintObservedTick tick;
    tick.observation.generation = generation;
    tick.observation.time_ms = ms;
    tick.observation.flags = in_world ? 3 : 0;
    tick.observation.xyz = {0, 0, 0, 0, 0, 0};
    tick.observation.angles = {0.0f, 0.0f};
    tick.update.player_present = in_world;
    tick.update.player_sprinting = in_world;
    tick.movement.player_present = in_world;
    tick.movement.forward = -1.0f;
    tick.movement.speed = 0.1f;
    tick.movement.settings.while_backwards = true; // legacy passive inputs
    return tick;
}

int main() {
    SprintObservationPreview preview;
    auto state = preview.control_state();
    CHECK(state && state->canonical_name == "Sprint");
    CHECK(state->requested_enabled && !state->active_in_world);
    CHECK(!state->settings_override && state->world_generation == 0);
    CHECK(!preview.request_module({"Fake Module", true}));
    CHECK(preview.request_module({"Sprint", false}));
    CHECK(preview.request_module({"Sprint", false})); // idempotent
    state = preview.control_state();
    CHECK(state && !state->requested_enabled && !state->active_in_world);

    auto disabled = preview.observe(frame(10, 100));
    CHECK(disabled.admitted && disabled.world_ready);
    CHECK(!disabled.update.proposed_key_pressed);
    CHECK(!disabled.update.proposed_player_sprinting);
    CHECK(!disabled.movement.proposed_speed);
    state = preview.control_state();
    CHECK(state && state->world_generation == 10 && !state->active_in_world);

    CHECK(preview.request_module({"Sprint", true}));
    state = preview.control_state();
    CHECK(state && state->requested_enabled && state->active_in_world);
    // Without explicit settings, original passive JNI flag behavior remains.
    auto legacy = preview.observe(frame(10, 101));
    CHECK(legacy.admitted && legacy.update.proposed_key_pressed);
    CHECK(legacy.movement.proposed_speed);

    SprintSettings off;
    CHECK(preview.configure_sprint(off));
    state = preview.control_state();
    CHECK(state && state->settings_override);
    auto restricted = preview.observe(frame(10, 102));
    CHECK(restricted.admitted && restricted.update.proposed_key_pressed);
    CHECK(!restricted.movement.proposed_speed); // no backwards allowed
    SprintSettings backwards;
    backwards.while_backwards = true;
    CHECK(preview.configure_sprint(backwards));
    auto allowed = preview.observe(frame(10, 103));
    CHECK(allowed.admitted && allowed.movement.proposed_speed &&
          std::abs(*allowed.movement.proposed_speed - 0.13f) < 1e-5f);

    CHECK(preview.request_module({"Sprint", false}));
    auto after_disable = preview.observe(frame(10, 104));
    CHECK(after_disable.admitted && !after_disable.update.proposed_key_pressed);
    CHECK(!after_disable.movement.proposed_speed);
    CHECK(!preview.control_state()->active_in_world);
    auto away = preview.observe(frame(11, 105, false));
    CHECK(away.admitted && !away.world_ready && away.session_transition);
    CHECK(!preview.control_state()->requested_enabled);
    CHECK(!preview.control_state()->active_in_world);
    CHECK(preview.request_module({"Sprint", true}));
    CHECK(preview.control_state()->requested_enabled);
    CHECK(!preview.control_state()->active_in_world);
    auto return_world = preview.observe(frame(12, 106));
    CHECK(return_world.admitted && return_world.session_transition);
    CHECK(return_world.update.proposed_key_pressed &&
          return_world.movement.proposed_speed);
    state = preview.control_state();
    CHECK(state && state->world_generation == 12 &&
          state->active_in_world && state->settings_override);

    bool foreign_denied = false;
    std::thread other([&] {
        foreign_denied =
            !preview.request_module({"Sprint", false}) &&
            !preview.configure_sprint(off) &&
            !preview.control_state().has_value() &&
            !preview.shutdown();
    });
    other.join();
    CHECK(foreign_denied);
    CHECK(preview.control_state()->active_in_world);
    CHECK(preview.request_module({"Sprint", false})); // owner request succeeds

    CHECK(preview.shutdown());
    CHECK(!preview.control_state());
    CHECK(!preview.request_module({"Sprint", true}));
    CHECK(!preview.configure_sprint(backwards));
    CHECK(!preview.observe(frame(13, 107)).admitted);
    std::cout << "R210_NATIVE_MODULE_CONTROL_PASS selection=1 settings=1"
              << " lifecycle=1 owner_guard=1 no_game_mutation=1\n";
}
