#pragma once
// R235: source-ordered Clutch candidate table + 3D side/top aim policy.
// The 167 exact packed offsets come from pinned source rdata.s.
// World solidity, player AABB, eye and prior ray hit are caller OBSERVATIONS.
// Only value decisions are returned; never raycast, press, or place in game.
#include "slinkywinky/clutch_candidate_table.hpp"
#include "slinkywinky/source_walk_bank.hpp"
#include "slinkywinky/block_in_aim_geometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
namespace slinkywinky::reference_compat {
struct ClutchCandidatePos {int x=0,y=0,z=0;};
inline bool operator==(const ClutchCandidatePos& a,
                       const ClutchCandidatePos& b) noexcept {
    return a.x==b.x && a.y==b.y && a.z==b.z;
}
struct ClutchSourcePrior {
    ClutchCandidatePos block{};
    BlockInVec3 last_aim{};
    // Source ray-tracing can yield no hit; exact source falls back
    // to previously selected block when ray result is null.
    bool solid_known=false,solid=false,ray_known=false;
    std::optional<ClutchCandidatePos> ray_hit{};
};
struct ClutchCandidateWorld {
    BlockInVec3 box_lo{},box_hi{},eye{};
    float current_yaw=0.0f,current_pitch=0.0f;
    unsigned range_index=2; // Source indexes table 0..7, not distance scalar.
    float min_height=0.0f,max_aim_degrees=180.0f;
    bool sideways_only=false,multipoint=true;
    float randomisation=1.0f;
    double clock_seconds=0.0;
    // Required for EVERY tested candidate until source scan stops.
    std::array<std::optional<bool>,167> source_solid{};
    // When min_height>0, source probes descending through base.y.
    // Depth 1 observes base.y, depth 2 observes base.y-1, etc.
    std::array<std::optional<bool>,8> ground_by_depth{};
    std::optional<ClutchSourcePrior> prior{};
};
struct ClutchCandidatePlan {
    bool admitted=false,found=false,face_up=false;
    unsigned visited=0,source_index=0;
    ClutchCandidatePos selected{};
    BlockInVec3 aim{};
    float aim_yaw_diff=0.0f,aim_pitch_diff=0.0f;
    float angular_cost=0.0f;
    bool preserved_prior=false;
};
inline ClutchCandidatePos decode_clutch_candidate(
    std::uint16_t v) noexcept {
    return {static_cast<int>(v&15u)-8,
            static_cast<int>((v>>4u)&15u)-8,
            static_cast<int>((v>>8u)&15u)-8};
}
inline int clutch_candidate_rank(ClutchCandidatePos p) noexcept {
    return std::abs(p.x)+std::abs(p.y)+std::abs(p.z);
}
inline bool clutch_valid_vec(const BlockInVec3& v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
           std::fabs(v.x)<1e6&&std::fabs(v.y)<1e6&&std::fabs(v.z)<1e6;
}
inline float clutch_clamp_float(double val,float low,float high) noexcept {
    return std::clamp(static_cast<float>(val),low,high);
}
inline bool clutch_source_reachable(const BlockInVec3& eye,
                                    const BlockInVec3& point) noexcept {
    const double x=eye.x-point.x,y=eye.y-point.y,z=eye.z-point.z;
    return x*x+y*y+z*z < 4.49*4.49; // original strict squared reach
}
struct ClutchCandidateAim {
    BlockInVec3 target{};
    float yaw_diff=0.0f,pitch_diff=0.0f,cost=0.0f;
};
inline std::optional<ClutchCandidateAim> clutch_source_look(
    const BlockInVec3& eye,const BlockInVec3& point,
    float current_yaw,float current_pitch) noexcept {
    const auto a=block_in_source_look_at(eye,point);
    if(!a)return std::nullopt;
    const float dyaw=block_in_wrap_yaw(a->yaw-current_yaw);
    const float dpitch=a->pitch-current_pitch;
    const float cost=std::fabs(dyaw)+std::fabs(dpitch);
    if(!std::isfinite(cost))return std::nullopt;
    return ClutchCandidateAim{point,dyaw,dpitch,cost};
}
inline std::optional<ClutchCandidateAim> clutch_source_side_aim(
    ClutchCandidatePos relative,ClutchCandidatePos base,
    const BlockInVec3& center,const BlockInVec3& eye,
    float yaw,float pitch,float inset,float shift_x,float shift_z,
    bool multipoint,const BlockInVec3& last_aim) noexcept {
    if(!std::isfinite(inset)||!std::isfinite(shift_x)||
       !std::isfinite(shift_z)||inset<0.0f||inset>=0.5f)
        return std::nullopt;
    if(!multipoint) {
        double frac_x=std::fabs(std::fmod(center.x,1.0));
        double frac_z=std::fabs(std::fmod(center.z,1.0));
        frac_x=frac_x>0.5?std::fabs(frac_x-1.0):frac_x;
        frac_z=frac_z>0.5?std::fabs(frac_z-1.0):frac_z;
        shift_z+=static_cast<float>(frac_x*
            (last_aim.z<center.z?-1.0:1.0));
        shift_x+=static_cast<float>(frac_z*
            (last_aim.x<center.x?-1.0:1.0));
    }
    const int bx=base.x+relative.x,by=base.y+relative.y,
              bz=base.z+relative.z;
    const float far=1.0f-inset;
    float x_lo=inset,x_hi=far,z_lo=inset,z_hi=far;
    if(std::abs(relative.x)>std::abs(relative.z)) {
        x_lo=x_hi=relative.x<0?1.0f:0.0f;
    } else {
        z_lo=z_hi=relative.z<0?1.0f:0.0f;
    }
    const int distance=std::abs(relative.x)+std::abs(relative.z);
    const double ax=static_cast<double>(clutch_clamp_float(
        center.x+shift_x,static_cast<float>(bx)+x_lo,
        static_cast<float>(bx)+x_hi));
    const double az=static_cast<double>(clutch_clamp_float(
        center.z+shift_z,static_cast<float>(bz)+z_lo,
        static_cast<float>(bz)+z_hi));
    const double ay=static_cast<double>(by)+0.25+
        static_cast<double>(std::clamp(distance*.25f,0.0f,.65f));
    return clutch_source_look(eye,{ax,ay,az},yaw,pitch);
}
inline std::optional<ClutchCandidateAim> clutch_source_top_aim(
    ClutchCandidatePos base,ClutchCandidatePos relative,
    const BlockInVec3& center,const BlockInVec3& eye,
    float yaw,float pitch,float inset,float shift_x,float shift_z) noexcept {
    if(!std::isfinite(inset)||!std::isfinite(shift_x)||
       !std::isfinite(shift_z)||inset<0.0f||inset>=0.5f)
        return std::nullopt;
    const int bx=base.x+relative.x,by=base.y+relative.y,
              bz=base.z+relative.z;
    const float x=clutch_clamp_float(center.x+shift_x,
        static_cast<float>(bx)+inset,static_cast<float>(bx+1)-inset);
    const float z=clutch_clamp_float(center.z+shift_z,
        static_cast<float>(bz)+inset,static_cast<float>(bz+1)-inset);
    return clutch_source_look(eye,{x,static_cast<double>(by)+1.0,z},
                              yaw,pitch);
}
inline std::optional<std::array<float,3>> clutch_draw_face(
    SourceWalkBank<8>& walks,bool top,double now,
    float randomisation) noexcept {
    const auto inset=walks.sample_clock(0,now,1.0,0.0f,.05f);
    if(!inset)return std::nullopt;
    const auto x=walks.sample_clock(top?3:1,now,
        top?1.0:0.5,top?-.1f:-.01f,top?.1f:.01f);
    if(!x)return std::nullopt;
    const auto z=walks.sample_clock(top?4:2,now,
        top?1.0:0.5,top?-.1f:-.01f,top?.1f:.01f);
    if(!z)return std::nullopt;
    return std::array<float,3>{
        .1f+*inset*randomisation,*x*randomisation,
        *z*randomisation};
}
inline ClutchCandidatePlan clutch_plan_original(
    const ClutchCandidateWorld& in,SourceWalkBank<8>& owner) noexcept {
    ClutchCandidatePlan rejected{};
    if(!owner.configured()||in.range_index>=kClutchDistanceCounts.size()||
       !clutch_valid_vec(in.box_lo)||!clutch_valid_vec(in.box_hi)||
       !clutch_valid_vec(in.eye)||
       in.box_lo.x>in.box_hi.x||in.box_lo.y>in.box_hi.y||
       in.box_lo.z>in.box_hi.z||
       !std::isfinite(in.current_yaw)||!std::isfinite(in.current_pitch)||
       !std::isfinite(in.min_height)||in.min_height<0.0f||
       in.min_height>8.0f||
       !std::isfinite(in.max_aim_degrees)||
       in.max_aim_degrees<0.0f||in.max_aim_degrees>500.0f||
       !std::isfinite(in.randomisation)||
       in.randomisation<0.0f||in.randomisation>1.0f||
       !std::isfinite(in.clock_seconds)||in.clock_seconds<0.0||
       in.clock_seconds>1e12)
        return rejected;
    const SourceWalkBank<8> before=owner;
    const auto refuse=[&]() noexcept -> ClutchCandidatePlan {
        owner=before;
        return {};
    };
    ClutchCandidatePlan out{};
    out.admitted=true;
    const BlockInVec3 center={
        (in.box_lo.x+in.box_hi.x)*.5,
        in.box_lo.y-.99,
        (in.box_lo.z+in.box_hi.z)*.5};
    const ClutchCandidatePos base={
        static_cast<int>(std::floor(center.x)),
        static_cast<int>(std::floor(center.y)),
        static_cast<int>(std::floor(center.z))};
    if(!clutch_valid_vec(center) || !clutch_valid_vec(in.box_hi))
        return refuse();
    if(in.min_height>0.0f) {
        for(unsigned depth=1;depth<=8;++depth) {
            if(!in.ground_by_depth[depth-1])return refuse();
            if(*in.ground_by_depth[depth-1])
                return out; // ground too close
            if(!(in.min_height>static_cast<float>(depth)))break;
        }
    }
    std::size_t candidates=kClutchDistanceCounts[in.range_index];
    int best_rank=-3;
    float best_cost=std::numeric_limits<float>::max();
    ClutchCandidatePlan best=out;
    bool have_best=false;

    if(in.prior && in.prior->block.y<=center.y) {
        const auto& previous=*in.prior;
        // Caller coordinates must be representable and close to the
        // bounded original world. Reject before subtracting signed ints
        // (an INT_MIN prior block could otherwise overflow rel.x/y/z).
        if(std::fabs(static_cast<double>(previous.block.x))>1000000.0 ||
           std::fabs(static_cast<double>(previous.block.y))>1000000.0 ||
           std::fabs(static_cast<double>(previous.block.z))>1000000.0)
            return refuse();
        if(!previous.solid_known)return refuse();
        if(previous.solid) {
            // The original only calls world ray tracing AFTER the
            // prior candidate passes angle and strict reach. Requiring
            // a ray result before those guards rejects otherwise valid
            // fallback candidates when no ray trace was needed.
            if(!clutch_valid_vec(previous.last_aim))return refuse();
            const auto rel=ClutchCandidatePos{
                previous.block.x-base.x,
                previous.block.y-base.y,
                previous.block.z-base.z};
            const auto samples=clutch_draw_face(
                owner,false,in.clock_seconds,in.randomisation);
            if(!samples)return refuse();
            const auto aim=clutch_source_side_aim(
                rel,base,center,in.eye,in.current_yaw,in.current_pitch,
                (*samples)[0],(*samples)[1],(*samples)[2],
                in.multipoint,previous.last_aim);
            if(!aim)return refuse();
            const ClutchCandidatePos observed=previous.ray_hit.value_or(
                previous.block);
            if(aim->cost<=in.max_aim_degrees &&
               clutch_source_reachable(in.eye,aim->target)) {
                if(!previous.ray_known)return refuse();
                if(observed==previous.block) {
                have_best=true;
                best_rank=clutch_candidate_rank(rel)-2;
                best_cost=aim->cost;
                best.found=true;
                best.preserved_prior=true;
                best.face_up=false;
                best.selected=previous.block;
                best.aim=aim->target;
                best.aim_yaw_diff=aim->yaw_diff;
                best.aim_pitch_diff=aim->pitch_diff;
                best.angular_cost=aim->cost;
                }
            }
        }
    }
    for(std::size_t i=0;i<candidates;++i) {
        if(!in.source_solid[i])return refuse();
        ++out.visited;
        if(!*in.source_solid[i])continue;
        const auto relative=decode_clutch_candidate(
            kClutchSourceOffsets[i]);
        const auto candidate=ClutchCandidatePos{
            base.x+relative.x,base.y+relative.y,base.z+relative.z};
        if(relative.x==0 && relative.z==0) {
            // Pinned source returns immediately, without running reach,
            // angle-cost or ray-trace filters for the top candidate.
            if(in.sideways_only)return out;
            const auto samples=clutch_draw_face(
                owner,true,in.clock_seconds,in.randomisation);
            if(!samples)return refuse();
            const auto target=clutch_source_top_aim(
                base,relative,center,in.eye,
                in.current_yaw,in.current_pitch,
                (*samples)[0],(*samples)[1],(*samples)[2]);
            if(!target)return refuse();
            out.found=true;out.face_up=true;
            out.source_index=static_cast<unsigned>(i);
            out.selected=candidate;out.aim=target->target;
            out.aim_yaw_diff=target->yaw_diff;
            out.aim_pitch_diff=target->pitch_diff;
            out.angular_cost=target->cost;
            return out;
        }
        const int rank=clutch_candidate_rank(relative);
        if(rank<0||rank>=static_cast<int>(kClutchDistanceCounts.size()))
            return refuse();
        // Original narrows the loop AFTER finding an occupied side
        // candidate, even if that candidate later fails reach.
        candidates=kClutchDistanceCounts[rank];
        const auto samples=clutch_draw_face(
            owner,false,in.clock_seconds,in.randomisation);
        if(!samples)return refuse();
        const auto aim=clutch_source_side_aim(
            relative,base,center,in.eye,
            in.current_yaw,in.current_pitch,
            (*samples)[0],(*samples)[1],(*samples)[2],
            in.multipoint,in.prior?in.prior->last_aim:BlockInVec3{});
        if(!aim)return refuse();
        if(aim->cost>best_cost && best_rank<=rank)continue;
        if(!clutch_source_reachable(in.eye,aim->target))continue;
        have_best=true;best_cost=aim->cost;
        best.found=true;best.face_up=false;
        best.source_index=static_cast<unsigned>(i);
        best.selected=candidate;best.aim=aim->target;
        best.aim_yaw_diff=aim->yaw_diff;
        best.aim_pitch_diff=aim->pitch_diff;
        best.angular_cost=aim->cost;
        best.preserved_prior=false;
    }
    if(have_best && best_cost<=in.max_aim_degrees) {
        best.admitted=true;
        best.visited=out.visited;
        return best;
    }
    return out;
}
} // namespace slinkywinky::reference_compat
