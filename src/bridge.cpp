#define SLINKYWINKY_BRIDGE_BUILD
#include "slinkywinky/bridge.h"
#include "slinkywinky/combat.hpp"
#include <array>
#include <new>
#include <thread>

struct SWContext {
    std::thread::id owner;
    slinkywinky::CombatCoordinator coordinator;
    explicit SWContext(std::array<std::uint32_t,4> seed)
        : owner(std::this_thread::get_id()), coordinator(seed) {}
};
namespace {
void fill(const slinkywinky::CombatResult& result, SWOutput* output) {
    *output = {};
    output->abi_version = SW_ABI_VERSION;
    output->left_edge = static_cast<int>(result.left_edge);
    output->right_edge = static_cast<int>(result.right_edge);
    output->left_held = result.left_held;
    output->right_held = result.right_held;
    output->attempt_attack = result.attempt_attack;
    if(result.requested_reach) {
        output->has_reach=1;
        output->reach_distance=*result.requested_reach;
    }
    if(result.rotation) {
        output->has_rotation=1;
        output->yaw=result.rotation->yaw;
        output->pitch=result.rotation->pitch;
    }
}
}
extern "C" {
int32_t sw_create(const uint32_t seed[4], SWContext** out) {
    if(!seed || !out || *out) return -1;
    try {
        *out = new SWContext({seed[0],seed[1],seed[2],seed[3]});
        return 0;
    } catch (...) { return -2; }
}
int32_t sw_step(SWContext* context, const SWFrame* frame,
                const SWSettings* settings, SWOutput* out) {
    if(!context || !frame || !settings || !out) return -1;
    if(frame->abi_version!=SW_ABI_VERSION || settings->abi_version!=SW_ABI_VERSION) return -3;
    if(context->owner!=std::this_thread::get_id()) return -4;
    try {
        slinkywinky::TickSnapshot snap;
        snap.elapsed_ms=frame->time_ms;
        snap.generation=frame->generation;
        snap.world_valid=frame->world_valid!=0;
        snap.gui_open=frame->gui_open!=0;
        snap.mouse_left=frame->mouse_left!=0;
        snap.mouse_right=frame->mouse_right!=0;
        snap.holding_weapon=frame->weapon!=0;
        snap.sprinting=frame->sprinting!=0;
        snap.hurt_time=frame->hurt_time;
        snap.last_attacker_time=frame->last_attacker_time;
        snap.player_eye={frame->eye_x,frame->eye_y,frame->eye_z};
        snap.yaw=frame->yaw;
        snap.pitch=frame->pitch;
        if(frame->target_present) snap.target=slinkywinky::Target{
            frame->target_id,{frame->target_x,frame->target_y,frame->target_z},
            frame->target_visible!=0,frame->target_alive!=0};
        slinkywinky::CombatSettings cfg;
        cfg.auto_clicker_enabled=settings->clicker!=0;
        cfg.require_mouse_down=settings->require_mouse!=0;
        cfg.clicks_per_second=settings->cps;
        cfg.aim_enabled=settings->aim!=0;
        cfg.aim_requires_mouse=settings->aim_require_mouse!=0;
        cfg.aim_range=settings->aim_range;
        cfg.aim_horizontal_speed=settings->aim_hspeed;
        cfg.aim_vertical_speed=settings->aim_vspeed;
        cfg.aim_fov=settings->aim_fov;
        cfg.auto_block_enabled=settings->auto_block!=0;
        cfg.block_requires_damage=settings->block_require_damage!=0;
        cfg.block_range=settings->block_range;
        cfg.block_max_hurt_time=settings->block_max_hurt;
        cfg.reach_enabled=settings->reach!=0;
        cfg.reach.distance=settings->reach_distance;
        cfg.reach.activation_ticks=settings->reach_activation_ticks;
        cfg.reach.require_sprint=settings->reach_require_sprint!=0;
        cfg.hit_select_enabled=settings->hit_select!=0;
        cfg.hit_select_min_hurt=settings->hit_select_min_hurt;
        fill(context->coordinator.tick(snap,cfg),out);
        return 0;
    } catch (...) { return -2; }
}
int32_t sw_disable(SWContext* context,int32_t manual_left,
                    int32_t manual_right,SWOutput* out) {
    if(!context || !out) return -1;
    if(context->owner!=std::this_thread::get_id()) return -4;
    try { fill(context->coordinator.disable(manual_left!=0,manual_right!=0),out); return 0; }
    catch (...) { return -2; }
}
int32_t sw_destroy(SWContext** context) {
    if(!context || !*context) return -1;
    if((*context)->owner != std::this_thread::get_id()) return -4;
    delete *context;
    *context=nullptr;
    return 0;
}
}
