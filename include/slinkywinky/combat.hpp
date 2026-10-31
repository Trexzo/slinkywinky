#pragma once
#include "slinkywinky/core.hpp"
#include "slinkywinky/runtime.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace slinkywinky {
struct Vector3 { double x = 0, y = 0, z = 0; };
struct Target {
    int entity_id = -1;
    Vector3 position;
    bool visible = false;
    bool alive = false;
};
struct TickSnapshot {
    std::uint64_t elapsed_ms = 0;
    std::uint64_t generation = 0;
    bool world_valid = false;
    bool gui_open = false;
    bool mouse_left = false;
    bool mouse_right = false;
    bool holding_weapon = false;
    bool sprinting = false;
    int hurt_time = 0;
    int last_attacker_time = 0;
    Vector3 player_eye;
    float yaw = 0;
    float pitch = 0;
    std::optional<Target> target;
};
struct CombatSettings {
    bool auto_clicker_enabled = false;
    float clicks_per_second = 12;
    bool require_mouse_down = true;
    bool aim_enabled = false;
    float aim_range = 4.0f;
    float aim_horizontal_speed = 15.0f;
    float aim_vertical_speed = 15.0f;
    float aim_fov = 90.0f;
    bool aim_requires_mouse = false;
    bool auto_block_enabled = false;
    float block_range = 3.1f;
    bool block_requires_damage = false;
    float block_max_hurt_time = 5.0f;
    bool reach_enabled = false;
    ReachSettings reach;
    bool hit_select_enabled = false;
    int hit_select_min_hurt = 0;
};
struct CombatResult {
    KeyEdge left_edge = KeyEdge::none;
    KeyEdge right_edge = KeyEdge::none;
    bool left_held = false;
    bool right_held = false;
    bool attempt_attack = false;
    std::optional<float> requested_reach;
    std::optional<RotationRequest> rotation;
};

// Deliberately standalone decision model. Integration must bind these requests
// to the exact Minecraft client thread after independent acceptance.
class CombatCoordinator {
public:
    explicit CombatCoordinator(std::array<std::uint32_t, 4> seed);
    CombatResult tick(const TickSnapshot& snapshot, const CombatSettings& settings);
    CombatResult disable(bool physical_left, bool physical_right);
    void reset_world() noexcept;
private:
    Random128 random_;
    ReachController reach_;
    RotationArbiter rotations_;
    InputOwnership left_;
    InputOwnership right_;
    std::uint64_t next_press_ms_ = 0;
    std::uint64_t release_ms_ = 0;
    std::uint64_t last_ms_ = 0;
    std::uint64_t generation_ = 0;
    int last_target_id_ = -1;
    int block_cooldown_ = 0;
    bool click_down_ = false;
    bool block_down_ = false;
    bool have_snapshot_ = false;
};
struct Knockback { float x = 0, y = 0, z = 0; };
struct VelocitySettings {
    bool enabled = false;
    float horizontal = 1.0f;
    float vertical = 1.0f;
    float chance_percent = 100.0f;
};
// Pure packet transformation model; leaves original input untouched if disabled.
std::optional<Knockback> decide_velocity(Knockback incoming,
                                          const VelocitySettings& settings,
                                          float unit_random);
} // namespace slinkywinky
