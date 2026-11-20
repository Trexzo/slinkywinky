#pragma once
// R247: independently written, read-only reconstruction of the recovered
// Clutch event-0 click-timer and block-face gate. No game I/O or press API.
#include <cmath>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {

struct ClutchClickTarget {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct ClutchClickHit {
    bool is_block = false;
    ClutchClickTarget block{};
    // Vanilla 1.8.9 EnumFacing ordinal: down=0, up=1, north=2,
    // south=3, west=4, east=5.
    int face = -1;
};

struct ClutchClickState {
    float timer = 0.0f;
    bool placement = false;
    bool centered = false;
    ClutchClickTarget target{};
};

struct ClutchClickObservation {
    float clicks_per_second = 1.0f;
    bool game_ready = false;
    std::optional<ClutchClickHit> ray_hit{};
};

struct ClutchClickProposal {
    // A proposal never invokes any input function or changes game state.
    bool press_proposed = false;
    ClutchClickState next{};
};

// This function is deliberately stateless: the caller must bind its inputs to
// a single verified world/player/game-thread owner. It models source ordering,
// not an assertion that a Minecraft event hook or placement is implemented.
inline std::optional<ClutchClickProposal> evaluate_clutch_click(
    const ClutchClickState& state, const ClutchClickObservation& sample) noexcept {
    if (!std::isfinite(state.timer) || state.timer < -1.0f ||
        state.timer > 20.0f || !std::isfinite(sample.clicks_per_second) ||
        sample.clicks_per_second < 1.0f || sample.clicks_per_second > 20.0f)
        return std::nullopt;

    ClutchClickProposal result{};
    result.next = state;
    // Recovered event-0 source advances its timer *before* the ready,
    // placement and block-hit gates. One callback is one source timer step.
    if (result.next.timer > 0.0f) {
        result.next.timer -= 1.0f;
        if (result.next.timer > 0.0f) return result;
    }
    result.next.timer += 20.0f / sample.clicks_per_second;

    if (!sample.game_ready || !state.placement || !sample.ray_hit)
        return result;
    const ClutchClickHit& hit = *sample.ray_hit;
    if (!hit.is_block || hit.face < 0 || hit.face > 5 ||
        hit.block.x != state.target.x || hit.block.y != state.target.y ||
        hit.block.z != state.target.z ||
        // Centered target requires UP; sideways target requires NOT UP.
        state.centered != (hit.face == 1))
        return result;

    int dx = 0, dy = 0, dz = 0;
    switch (hit.face) {
        case 0: dy = -1; break;
        case 1: dy =  1; break;
        case 2: dz = -1; break;
        case 3: dz =  1; break;
        case 4: dx = -1; break;
        case 5: dx =  1; break;
        default: return std::nullopt;
    }
    const auto can_add = [](int coordinate, int delta) noexcept {
        return (delta != 1 || coordinate < std::numeric_limits<int>::max()) &&
               (delta != -1 || coordinate > std::numeric_limits<int>::min());
    };
    if (!can_add(state.target.x, dx) || !can_add(state.target.y, dy) ||
        !can_add(state.target.z, dz)) return std::nullopt;

    result.press_proposed = true;
    result.next.target = {state.target.x + dx,
                          state.target.y + dy,
                          state.target.z + dz};
    return result;
}

} // namespace slinkywinky::reference_compat
