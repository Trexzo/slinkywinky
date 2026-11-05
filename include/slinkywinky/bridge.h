#ifndef SLINKYWINKY_BRIDGE_H
#define SLINKYWINKY_BRIDGE_H
#include <stdint.h>
#ifdef _WIN32
 #ifdef SLINKYWINKY_BRIDGE_BUILD
  #define SW_API __declspec(dllexport)
 #else
  #define SW_API __declspec(dllimport)
 #endif
#else
 #define SW_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define SW_ABI_VERSION 1u
// All calls must come from the admitted game thread. No JVM attachment,
// injection, arbitrary process reading or hook installation is performed.
typedef struct SWContext SWContext;
typedef struct SWFrame {
 uint32_t abi_version;
 uint64_t time_ms, generation;
 int32_t world_valid, gui_open, mouse_left, mouse_right, weapon, sprinting;
 int32_t hurt_time, last_attacker_time, target_present, target_visible, target_alive;
 int32_t target_id;
 double eye_x, eye_y, eye_z, target_x, target_y, target_z;
 float yaw, pitch;
} SWFrame;
typedef struct SWSettings {
 uint32_t abi_version;
 int32_t clicker, require_mouse, aim, aim_require_mouse, auto_block, block_require_damage;
 int32_t reach, hit_select;
 float cps, aim_range, aim_hspeed, aim_vspeed, aim_fov, block_range, block_max_hurt;
 float reach_distance, reach_activation_ticks;
 int32_t reach_require_sprint, hit_select_min_hurt;
} SWSettings;
typedef struct SWOutput {
 uint32_t abi_version;
 int32_t left_edge, right_edge, left_held, right_held, attempt_attack;
 int32_t has_reach, has_rotation;
 float reach_distance, yaw, pitch;
} SWOutput;
// Returns 0 on success; negative error on invalid arguments/version/thread.
SW_API int32_t sw_create(const uint32_t seed[4], SWContext** out);
SW_API int32_t sw_step(SWContext* context, const SWFrame* frame,
                       const SWSettings* settings, SWOutput* out);
SW_API int32_t sw_disable(SWContext* context, int32_t manual_left,
                          int32_t manual_right, SWOutput* out);
SW_API int32_t sw_destroy(SWContext** context);
#ifdef __cplusplus
}
#endif
#endif
