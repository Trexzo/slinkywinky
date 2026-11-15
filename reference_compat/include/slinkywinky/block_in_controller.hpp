#pragma once
// R230: independently authored Slinky Block In native policy, pinned to
// OpenSlinky fd9c592 block_in.cpp + data.s/rdata.s. Value proposals only.
// No Minecraft world reads, keyboard writes, rotation hooks or injection.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include "slinkywinky/block_in_aim_geometry.hpp"
#include "slinkywinky/source_random_walk.hpp"

namespace slinkywinky::reference_compat {

struct BlockInPoint { int x=0,y=0,z=0; };
struct BlockInDirection { int x=0,z=0; };
inline constexpr std::array<BlockInPoint,10> kBlockInRoofOffsets{{
    {0,3,0},{0,3,-1},{1,3,0},{0,3,1},{-1,3,0},
    {-1,3,-1},{1,3,-1},{1,3,1},{-1,3,1},{0,4,0}
}};
inline constexpr std::array<BlockInDirection,4> kBlockInWalls{{
    {0,1},{-1,0},{0,-1},{1,0}
}};
inline bool block_in_is_open_id(unsigned id) noexcept {
    // Pinned original: only air (0), water (8,9) and tall grass (31).
    return id==0 || id==8 || id==9 || id==31;
}
inline bool block_in_middle(double x,double z) noexcept {
    if(!std::isfinite(x)||!std::isfinite(z))return false;
    const double fx=std::fabs(std::fmod(x,1.0));
    const double fz=std::fabs(std::fmod(z,1.0));
    return fx>=0.31 && fx<=0.69 && fz>=0.31 && fz<=0.69;
}
struct BlockInToggleInput {
    bool own_module=false,enabled=false;
    float player_yaw=0.0f;
    std::array<bool,4> physical_movement_keys{};
};
struct BlockInToggleDecision {
    bool admitted=false;
    bool proposed_restore_keys=false;
    std::array<bool,4> restored_keys{};
    int remembered_quadrant=0;
    std::uint64_t placed_mask=0;
};
struct BlockInRotationDecision {
    bool override_yaw=false,override_pitch=false;
    std::optional<float> proposed_yaw{},proposed_pitch{};
};
struct BlockInMovementDecision {
    bool proposed_override_keys=false;
    std::array<bool,4> proposed_keys{}; // forward/back/left/right
};
struct BlockInWorldInput {
    bool in_game=false,on_ground=false,require_ground=false;
    bool rotation_priority_granted=false,held_block_item=false;
    double player_x=0.5,player_y=64.0,player_z=0.5;
    float aim_speed=0.5f;
    float diff_yaw=0.0f,diff_pitch=0.0f;
    float yaw_jitter=1.0f,pitch_jitter=1.0f;
    // R231 source-backed option: compute actual world-space aim point,
    // rot_look_at yaw/pitch and silent-offset delta from first-party
    // pure native math, instead of trusting caller-supplied diff angles.
    // Legacy R230 receives diff_yaw/diff_pitch if not supplied.
    std::optional<BlockInSourceAim> source_aim{};
    // The game world's original block-id and bounding-box observations,
    // supplied by a caller. Missing required cells fail closed.
    std::array<std::optional<unsigned>,10> roof_block_ids{};
    // Four compass directions; columns: below feet, feet, head, above.
    std::array<std::array<std::optional<unsigned>,4>,4> wall_block_ids{};
    std::array<std::optional<bool>,4> wall_intersects_player{};
};
struct BlockInTickDecision {
    bool admitted=false,proposed_disable=false,active=false;
    bool proposed_select_block=false,proposed_click=false;
    std::optional<unsigned> proposed_placement_bit{};
    std::optional<BlockInPoint> proposed_target_offset{};
    std::optional<BlockInVec3> proposed_world_target{};
    // R232: exact source-walk normalized proposals, surfaced for tests.
    std::optional<float> proposed_walk_first{},proposed_walk_second{};
    std::optional<float> proposed_jitter_yaw{},proposed_jitter_pitch{};
    std::optional<float> proposed_absolute_yaw{},proposed_absolute_pitch{};
    std::optional<float> proposed_yaw_delta{},proposed_pitch_delta{};
    int movement_mode=0; // 0 idle; 1 walk to centre; 2 hold centre
    std::uint64_t placed_mask=0;
};
class BlockInController final {
public:
    void reset() noexcept {
        active_=false;mode_=0;quadrant_=0;placed_=0;
        last_y_=0.0;have_last_y_=false;
    }
    bool on_empty_tick() const noexcept { return true; }
    bool configure_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        return source_walks_.configure(seed);
    }
    const SourceBlockInWalks& source_walks() const noexcept {
        return source_walks_;
    }
    BlockInToggleDecision on_toggle(const BlockInToggleInput& in) noexcept {
        BlockInToggleDecision out;
        if(!in.own_module)return out;
        if(in.enabled && !std::isfinite(in.player_yaw))return out;
        out.admitted=true;
        if(in.enabled) {
            placed_=0;
            // The pinned C++ uses roundf(yaw/90) then %4.
            const int rounded=static_cast<int>(std::round(in.player_yaw/90.0f));
            quadrant_=(rounded%4+4)%4;
            active_=false;mode_=0;
        } else {
            out.proposed_restore_keys=true;
            out.restored_keys=in.physical_movement_keys;
            active_=false;mode_=0;
        }
        out.remembered_quadrant=quadrant_;out.placed_mask=placed_;
        return out;
    }
    BlockInRotationDecision on_rotation() const noexcept {
        BlockInRotationDecision out;
        if(!active_)return out;
        out.override_yaw=out.override_pitch=true;
        if(mode_==1) {
            out.proposed_yaw=center_yaw_;
            out.proposed_pitch=center_pitch_;
        }
        return out;
    }
    BlockInMovementDecision on_movement() const noexcept {
        BlockInMovementDecision out;
        if(mode_==0)return out;
        out.proposed_override_keys=true;
        out.proposed_keys={mode_==1,false,false,false};
        return out;
    }
    BlockInTickDecision on_tick(const BlockInWorldInput& in) noexcept {
        BlockInTickDecision out;
        if(!in.in_game) {
            active_=false;mode_=0;
            out.admitted=true;out.proposed_disable=true;
            return complete(out);
        }
        if(!std::isfinite(in.player_x)||!std::isfinite(in.player_y)||
           !std::isfinite(in.player_z)||!std::isfinite(in.aim_speed)||
           !std::isfinite(in.diff_yaw)||!std::isfinite(in.diff_pitch)||
           !std::isfinite(in.yaw_jitter)||!std::isfinite(in.pitch_jitter)||
           std::fabs(in.player_x)>1000000.0||
           std::fabs(in.player_y)>1000000.0||
           std::fabs(in.player_z)>1000000.0||
           in.aim_speed<0.0f||in.aim_speed>2.0f||
           in.yaw_jitter<0.0f||in.yaw_jitter>2.0f||
           in.pitch_jitter<0.0f||in.pitch_jitter>2.0f)
            return out;
        // Reject malformed caller-provided source geometry before mutating
        // either placement state or last-height/mask data.
        if(in.source_aim && !valid_block_in_source_aim(*in.source_aim))
            return {};
        const BlockInController rollback=*this;
        auto reject=[&]() noexcept {
            *this=rollback;
            return BlockInTickDecision{};
        };
        active_=false;mode_=0;
        out.admitted=true;
        if(in.require_ground&&!in.on_ground) {
            out.proposed_disable=true;
            return complete(out);
        }
        if(have_last_y_&&std::fabs(in.player_y-last_y_)>0.01)
            placed_=0;
        last_y_=in.player_y;have_last_y_=true;
        std::optional<unsigned> bit;
        BlockInPoint offset{};
        // roof first. If first roof spot is occupied, try walls instead.
        for(unsigned i=0;i<10;++i) {
            if(!in.roof_block_ids[i])return reject(); // fail closed, no state drift
            const unsigned id=*in.roof_block_ids[i];
            if(id>4095)return reject();
            const bool marked=(placed_&(std::uint64_t{1}<<(32+i)))!=0;
            if(!marked&&block_in_is_open_id(id))continue;
            if(i!=0) {
                bit=32+i;
                offset=kBlockInRoofOffsets[i];
            }
            break;
        }
        if(!bit) {
            for(unsigned step=0;step<4&&!bit;++step) {
                const unsigned dir=(static_cast<unsigned>(quadrant_)+step)&3u;
                if(!in.wall_intersects_player[dir])return reject();
                if(*in.wall_intersects_player[dir])continue;
                const auto& cells=in.wall_block_ids[dir];
                // source scans each side from feet to head, requiring a
                // *solid supporting block immediately below each air cell*.
                for(unsigned level=0;level<3;++level) {
                    if(placed_&(std::uint64_t{1}<<(dir*3+level)))continue;
                    if(!cells[level]||!cells[level+1])return reject();
                    if(*cells[level]>4095||*cells[level+1]>4095)
                        return reject();
                    if(block_in_is_open_id(*cells[level+1]) &&
                       !block_in_is_open_id(*cells[level])) {
                        bit=dir*3+level;
                        offset={kBlockInWalls[dir].x,
                                static_cast<int>(level),
                                kBlockInWalls[dir].z};
                        break;
                    }
                }
            }
        }
        if(!bit) {
            // Nothing to place: the original refuses automatic disable
            // until grounded and centered in the block.
            out.proposed_disable=in.on_ground &&
                block_in_middle(in.player_x,in.player_z);
            return complete(out);
        }
        out.proposed_placement_bit=*bit;
        out.proposed_target_offset=offset;
        float diff_yaw=in.diff_yaw,diff_pitch=in.diff_pitch;
        if(in.source_aim) {
            BlockInSourceAim effective=*in.source_aim;
            if(effective.use_reconstructed_walk) {
                const auto samples=source_walks_.sample_placement(
                    *bit,effective.clock_seconds);
                if(!samples)return reject();
                effective.first_walk=(*samples)[0];
                effective.second_walk=(*samples)[1];
                out.proposed_walk_first=effective.first_walk;
                out.proposed_walk_second=effective.second_walk;
            }
            const auto target=block_in_source_look(
                *bit,static_cast<int>(std::floor(in.player_x)),
                static_cast<int>(std::floor(in.player_y))-1,
                static_cast<int>(std::floor(in.player_z)),
                effective);
            if(!target)return reject();
            out.proposed_world_target=target->target;
            out.proposed_absolute_yaw=target->absolute_yaw;
            out.proposed_absolute_pitch=target->absolute_pitch;
            diff_yaw=target->diff_yaw;
            diff_pitch=target->diff_pitch;
        }
        if(!in.rotation_priority_granted||!in.held_block_item) {
            out.proposed_disable=true;
            return complete(out);
        }
        out.proposed_select_block=true;
        float yaw_jitter=in.yaw_jitter,pitch_jitter=in.pitch_jitter;
        if(in.source_aim && in.source_aim->use_reconstructed_walk) {
            // Original Block In calls shared RNG states #5/#6 only
            // after source placement selection and hotbar admission.
            const auto jitter=source_walks_.sample_jitter(
                in.source_aim->clock_seconds);
            if(!jitter)return reject();
            yaw_jitter=0.8f+0.4f*(*jitter)[0];
            pitch_jitter=0.8f+0.4f*(*jitter)[1];
            out.proposed_jitter_yaw=yaw_jitter;
            out.proposed_jitter_pitch=pitch_jitter;
        }
        const float yaw_limit=180.0f*in.aim_speed*yaw_jitter;
        const float pitch_limit=180.0f*in.aim_speed*pitch_jitter;
        const float scale=in.aim_speed*0.5f+0.5f;
        const float yaw=0.5f*(diff_yaw*scale+
            std::clamp(diff_yaw,-yaw_limit,yaw_limit));
        const float pitch=0.5f*(diff_pitch*scale+
            std::clamp(diff_pitch,-pitch_limit,pitch_limit));
        if(!std::isfinite(yaw)||!std::isfinite(pitch))return reject();
        out.proposed_yaw_delta=yaw;
        out.proposed_pitch_delta=pitch;
        if(std::fabs(diff_yaw-yaw)+std::fabs(diff_pitch-pitch)<1.0f) {
            out.proposed_click=true;
            placed_|=std::uint64_t{1}<<(*bit);
        }
        active_=true;
        mode_=block_in_middle(in.player_x,in.player_z)?2:1;
        if(mode_==1 && in.source_aim) {
            const auto center=block_in_source_look_at(
                {in.player_x,in.player_y,in.player_z},
                {std::floor(in.player_x)+0.5,in.player_y,
                 std::floor(in.player_z)+0.5});
            if(!center)return reject();
            center_yaw_=center->yaw;
            center_pitch_=center->pitch;
        }
        out.active=true;
        return complete(out);
    }
    std::uint64_t placed_mask() const noexcept{return placed_;}
    int quadrant() const noexcept{return quadrant_;}
    int movement_mode() const noexcept{return mode_;}
    void set_center_rotation_for_preview(float yaw,float pitch) noexcept {
        if(std::isfinite(yaw)&&std::isfinite(pitch)) {
            center_yaw_=yaw;center_pitch_=pitch;
        }
    }
private:
    BlockInTickDecision complete(BlockInTickDecision out) const noexcept {
        out.placed_mask=placed_;
        out.movement_mode=mode_;
        return out;
    }
    bool active_=false,have_last_y_=false;
    int mode_=0,quadrant_=0;
    std::uint64_t placed_=0;
    double last_y_=0.0;
    float center_yaw_=0.0f,center_pitch_=0.0f;
    SourceBlockInWalks source_walks_{};
};
} // namespace slinkywinky::reference_compat
