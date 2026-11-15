#pragma once
// R231: independently authored typed Block In target geometry derived from
// pinned OpenSlinky fd9c592 block_in.cpp + rdata.s. External, normalized
// walk observations replace the original engine RNG; NO game world reads.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace slinkywinky::reference_compat {
struct BlockInVec3 {
    double x=0.0,y=0.0,z=0.0;
};
struct BlockInSourceAim {
    // Preverified normalized results of the two source walk_update calls.
    // These are NOT an implementation of the source's random-walk RNG.
    float first_walk=0.5f;
    float second_walk=0.5f;
    // R232 opt-in source walk pipeline. Clock and seed must be supplied
    // explicitly by an offline owner. No live runtime seed/time assumed.
    bool use_reconstructed_walk=false;
    double clock_seconds=0.0;
    BlockInVec3 eye{};
    float current_yaw=0.0f,current_pitch=0.0f;
    float silent_yaw=0.0f,silent_pitch=0.0f;
};
struct BlockInSourceAimResult {
    BlockInVec3 target{};
    float absolute_yaw=0.0f,absolute_pitch=0.0f;
    float diff_yaw=0.0f,diff_pitch=0.0f;
};
inline bool valid_block_in_source_aim(const BlockInSourceAim& v) noexcept {
    const float f[]{v.first_walk,v.second_walk,v.current_yaw,
                    v.current_pitch,v.silent_yaw,v.silent_pitch};
    for(float n:f)if(!std::isfinite(n))return false;
    if(v.first_walk<0.0f||v.first_walk>1.0f||
       v.second_walk<0.0f||v.second_walk>1.0f)return false;
    if(v.use_reconstructed_walk &&
       (!std::isfinite(v.clock_seconds)||
        v.clock_seconds<0.0||v.clock_seconds>1e12))return false;
    const double eye[]{v.eye.x,v.eye.y,v.eye.z};
    for(double n:eye)if(!std::isfinite(n)||std::fabs(n)>1000000.0)
        return false;
    return true;
}
inline float source_walk_value(float lo,float hi,float unit) noexcept {
    // Ordered endpoints are retained even for the original's
    // negatively descending ranges [-0.1,-0.2] / [-0.25,-0.4].
    return lo+(hi-lo)*unit;
}
inline std::optional<BlockInVec3> block_in_source_target(
    unsigned placement_bit,int bx,int by,int bz,
    float first_walk,float second_walk) noexcept {
    if(!std::isfinite(first_walk)||!std::isfinite(second_walk)||
       first_walk<0.0f||first_walk>1.0f||
       second_walk<0.0f||second_walk>1.0f)return std::nullopt;
    BlockInVec3 t{};
    if(placement_bit>=32u && placement_bit<=41u) {
        const unsigned i=placement_bit-32u;
        if(i==0u)return std::nullopt; // roof anchor is not a place target
        const float height_a=source_walk_value(3.6f,3.8f,first_walk);
        const float height_b=source_walk_value(3.6f,3.8f,second_walk);
        const float negative_a=source_walk_value(-0.1f,-0.2f,first_walk);
        const float negative_b=source_walk_value(-0.1f,-0.2f,second_walk);
        const float positive_a=source_walk_value(1.1f,1.2f,first_walk);
        const float positive_b=source_walk_value(1.1f,1.2f,second_walk);
        if(i==9u) {
            t.x=static_cast<double>(bx)+
                source_walk_value(0.4f,0.6f,first_walk);
            t.y=static_cast<double>(by)+4.0;
            t.z=static_cast<double>(bz)+
                source_walk_value(0.4f,0.6f,second_walk);
            return t;
        }
        switch((i-1u)&3u) {
            case 0u:
                t={static_cast<double>(bx),
                   static_cast<double>(by)+height_a,
                   static_cast<double>(bz)+negative_b};
                break;
            case 1u:
                t={static_cast<double>(bx)+positive_a,
                   static_cast<double>(by)+height_b,
                   static_cast<double>(bz)};
                break;
            case 2u:
                t={static_cast<double>(bx)+1.0,
                   static_cast<double>(by)+height_a,
                   static_cast<double>(bz)+positive_b};
                break;
            default:
                t={static_cast<double>(bx)+negative_a,
                   static_cast<double>(by)+height_b,
                   static_cast<double>(bz)+1.0};
                break;
        }
        return t;
    }
    if(placement_bit>=12u)return std::nullopt;
    constexpr int dx[]{0,-1,0,1};
    constexpr int dz[]{1,0,-1,0};
    const unsigned dir=placement_bit/3u;
    const unsigned level=placement_bit%3u;
    const float low=level==0u?-0.25f:0.25f;
    const float high=level==0u?-0.4f:0.4f;
    const float x_walk=source_walk_value(low,high,first_walk);
    const float z_walk=source_walk_value(low,high,second_walk);
    t.x=static_cast<double>(bx+dx[dir])+0.5-
        static_cast<double>(x_walk*static_cast<float>(dx[dir]));
    t.z=static_cast<double>(bz+dz[dir])+0.5-
        static_cast<double>(z_walk*static_cast<float>(dz[dir]));
    t.y=static_cast<double>(by)+static_cast<double>(level);
    return t;
}
struct BlockInAngles {
    float yaw=0.0f,pitch=0.0f;
};
inline std::optional<BlockInAngles> block_in_source_look_at(
    const BlockInVec3& from,const BlockInVec3& to) noexcept {
    const double xyz[]{from.x,from.y,from.z,to.x,to.y,to.z};
    for(double n:xyz)if(!std::isfinite(n)||std::fabs(n)>1000000.0)
        return std::nullopt;
    const double dx=to.x-from.x,dy=to.y-from.y,dz=to.z-from.z;
    if(dx==0.0 && dy==0.0 && dz==0.0)return std::nullopt;
    constexpr double kRadiansToDegrees=
        57.295779513082320876798154814105;
    const float yaw=static_cast<float>(
        std::atan2(dz,dx)*kRadiansToDegrees-90.0);
    const float pitch=static_cast<float>(
        -std::atan2(dy,std::sqrt(dx*dx+dz*dz))*kRadiansToDegrees);
    if(!std::isfinite(yaw)||!std::isfinite(pitch))return std::nullopt;
    return BlockInAngles{yaw,pitch};
}
inline float block_in_wrap_yaw(float delta) noexcept {
    float r=std::fmod(delta,360.0f);
    if(r>=180.0f)r-=360.0f;
    else if(r< -180.0f)r+=360.0f;
    return r;
}
inline std::optional<BlockInSourceAimResult> block_in_source_look(
    unsigned bit,int bx,int by,int bz,
    const BlockInSourceAim& v) noexcept {
    if(!valid_block_in_source_aim(v))return std::nullopt;
    const auto target=block_in_source_target(
        bit,bx,by,bz,v.first_walk,v.second_walk);
    if(!target)return std::nullopt;
    const auto angles=block_in_source_look_at(v.eye,*target);
    if(!angles)return std::nullopt;
    const float current_yaw=v.current_yaw+v.silent_yaw;
    const float current_pitch=v.current_pitch+v.silent_pitch;
    if(!std::isfinite(current_yaw)||!std::isfinite(current_pitch))
        return std::nullopt;
    return BlockInSourceAimResult{
        *target,angles->yaw,angles->pitch,
        block_in_wrap_yaw(angles->yaw-current_yaw),
        angles->pitch-current_pitch};
}
} // namespace slinkywinky::reference_compat
