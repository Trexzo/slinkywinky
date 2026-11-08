#pragma once
namespace slinkywinky {
// R157 original, first-party Sprint decision model only.
// No OS key input, JVM hooks, packet changes or game state mutation.
struct SprintSettings {
    bool enabled=false;
    bool allow_inventory=false;
    bool allow_using_item=false;
    bool allow_backwards=false;
    bool allow_sideways=false;
};
struct SprintObservation {
    bool world_valid=false;
    bool gui_open=false;
    bool inventory_open=false;
    bool using_item=false;
    bool sneaking=false;
    bool collided=false;
    bool blinded=false;
    bool hunger_sufficient=false;
    float forward=0.0f;
    float strafe=0.0f;
};
// An intent, not a synthetic key press or an in-game sprint state.
bool decide_sprint(const SprintObservation& input,
                   const SprintSettings& settings) noexcept;
} // namespace slinkywinky
