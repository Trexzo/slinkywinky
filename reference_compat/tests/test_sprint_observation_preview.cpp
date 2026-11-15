#include "slinkywinky/sprint_observation_preview.hpp"
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

using namespace slinkywinky::reference_compat;

static void require(bool condition, const char* expression, int line) {
    if (!condition)
        throw std::runtime_error("R204 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(expression) require(static_cast<bool>(expression), #expression, __LINE__)

static SprintObservedTick tick(std::uint64_t generation, std::uint64_t ms,
                               std::uint32_t flags = 3) {
    SprintObservedTick result;
    result.observation.generation = generation;
    result.observation.time_ms = ms;
    result.observation.flags = flags;
    result.observation.xyz = {1, 2, 3, 0, 0, 0};
    result.observation.angles = {15.0f, 35.0f};
    result.update.player_present = (flags & 3u) == 3u;
    result.update.player_sprinting = true;
    result.movement.player_present = result.update.player_present;
    result.movement.forward = -1.0f;
    result.movement.speed = 0.1f;
    result.movement.settings.while_backwards = true;
    return result;
}

int main() {
    SprintObservationPreview preview;
    auto sample = tick(1, 1000);
    auto first = preview.observe(sample);
    CHECK(first.admitted && first.world_ready && first.session_transition);
    CHECK(first.generation == 1 && first.accepted_ticks == 1);
    CHECK(first.update.proposed_key_pressed);
    CHECK(first.update.proposed_player_sprinting == true);
    CHECK(first.movement.proposed_speed.has_value());
    CHECK(std::abs(*first.movement.proposed_speed - 0.13f) < 0.000001f);
    CHECK(first.movement.propose_set_sprinting);

    sample = tick(1, 1001);
    sample.update.gui_open = true;
    auto gui = preview.observe(sample);
    CHECK(gui.admitted && !gui.session_transition && gui.accepted_ticks == 2);
    CHECK(!gui.update.proposed_key_pressed);
    CHECK(gui.update.proposed_player_sprinting == false);

    sample = tick(1, 1002);
    sample.update.gui_open = true;
    sample.update.settings.while_in_gui = true;
    auto allowed = preview.observe(sample);
    CHECK(allowed.admitted && allowed.update.proposed_key_pressed);
    CHECK(allowed.update.proposed_player_sprinting == true);

    // Clock rollback, invalid identity and contradictory world/player
    // identity must never produce new proposals or commit a sample.
    sample = tick(1, 1001);
    CHECK(!preview.observe(sample).admitted);
    sample = tick(1, 1003, 2); // player without a world
    sample.update.player_present = true;
    sample.movement.player_present = true;
    CHECK(!preview.observe(sample).admitted);
    sample = tick(1, 1003);
    sample.movement.player_present = false;
    CHECK(!preview.observe(sample).admitted);
    sample = tick(0, 1003);
    CHECK(!preview.observe(sample).admitted);
    CHECK(preview.generation() == 1);

    // A world-player loss requires the host's session generation bump.
    auto stale_absence = tick(1, 1003, 0);
    CHECK(!preview.observe(stale_absence).admitted);
    auto absent = tick(2, 1003, 0);
    auto lost = preview.observe(absent);
    CHECK(lost.admitted && !lost.world_ready && lost.session_transition);
    CHECK(lost.generation == 2 && lost.accepted_ticks == 4);
    CHECK(!lost.update.proposed_player_sprinting);
    CHECK(!lost.movement.proposed_speed);

    // Returning in the same generation is refused. The valid new
    // generation must start with no stale forced-sprint state.
    CHECK(!preview.observe(tick(2, 1004)).admitted);
    sample = tick(3, 1004);
    sample.update.gui_open = true;
    auto returned = preview.observe(sample);
    CHECK(returned.admitted && returned.world_ready &&
          returned.session_transition && returned.generation == 3);
    CHECK(!returned.update.proposed_key_pressed);
    CHECK(!returned.update.proposed_player_sprinting);
    CHECK(returned.movement.proposed_speed.has_value());

    // Foreign thread cannot create a session, observe, or shut down;
    // no cross-thread writes to the non-atomic ledger occur.
    std::atomic<int> denied{0};
    std::thread foreign([&] {
        if (!preview.observe(tick(3, 1005)).admitted) ++denied;
        if (!preview.shutdown()) ++denied;
        if (preview.generation() == 0) ++denied;
    });
    foreign.join();
    CHECK(denied == 3);
    CHECK(preview.generation() == 3);

    sample = tick(3, 1005);
    sample.observation.xyz[1] = std::numeric_limits<double>::infinity(); // rejected nonfinite observation
    CHECK(!preview.observe(sample).admitted);
    sample = tick(3, 1005);
    sample.movement.forward = 0.0f;
    sample.movement.strafe = 0.0f;
    auto stationary = preview.observe(sample);
    CHECK(stationary.admitted && stationary.world_ready);
    CHECK(!stationary.movement.proposed_speed);
    CHECK(stationary.accepted_ticks == 6);

    CHECK(preview.shutdown() && preview.stopped());
    CHECK(!preview.shutdown());
    CHECK(!preview.observe(tick(4, 1006)).admitted);

    SprintObservationPreview fresh;
    auto fresh_first = fresh.observe(tick(11, 500));
    CHECK(fresh_first.admitted && fresh_first.session_transition);
    CHECK(fresh_first.accepted_ticks == 1 && fresh.generation() == 11);
    CHECK(fresh.shutdown());

    std::cout << "R204_SPRINT_OBSERVATION_PREVIEW_PASS "
              << "owner_thread=1 observation_ledger=1 session_reset=1 "
              << "cross_thread_denial=1 rollback_denial=1 zero_game_writes=1\n";
}
