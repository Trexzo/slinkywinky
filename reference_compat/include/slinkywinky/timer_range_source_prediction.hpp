#pragma once
// R333: value-only reconstruction of candidate Timer Range's per-frame ray
// simulation. Caller provides a copied player/target snapshot. No JNI.
#include "slinkywinky/timer_range_controller.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace slinkywinky::reference_compat {
struct SourceVec3{double x=0,y=0,z=0;};
inline SourceVec3 source_add(SourceVec3 a,SourceVec3 b)noexcept{
    return {a.x+b.x,a.y+b.y,a.z+b.z};
}
inline SourceVec3 source_sub(SourceVec3 a,SourceVec3 b)noexcept{
    return {a.x-b.x,a.y-b.y,a.z-b.z};
}
inline SourceVec3 source_scale(SourceVec3 a,double k)noexcept{
    return {a.x*k,a.y*k,a.z*k};
}
inline bool source_finite(SourceVec3 p)noexcept{
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
struct TimerRangeSourceObservation {
    SourceVec3 player_eye{};
    SourceVec3 initial_motion{};
    SourceVec3 target_client_pos{};
    SourceVec3 target_server_pos{};
    double yaw_degrees=0,pitch_degrees=0;
    double movement_factor=0.1;
    float forward_input=0,strafe_input=0;
    int target_interpolation_steps=0;
};
inline SourceVec3 source_closest_aabb_point(SourceVec3 p,SourceVec3 lo,SourceVec3 hi)noexcept{
    return {std::clamp(p.x,lo.x,hi.x),
            std::clamp(p.y,lo.y,hi.y),
            std::clamp(p.z,lo.z,hi.z)};
}
inline double source_dist_sq(SourceVec3 a,SourceVec3 b)noexcept{
    const auto d=source_sub(a,b);return d.x*d.x+d.y*d.y+d.z*d.z;
}
inline bool source_segment_hits_aabb(SourceVec3 eye,SourceVec3 end,
                                     SourceVec3 lo,SourceVec3 hi)noexcept{
    auto d=source_sub(end,eye);
    double tlo=0,thi=1;
    const double a[3]={eye.x,eye.y,eye.z};
    const double v[3]={d.x,d.y,d.z};
    const double mn[3]={lo.x,lo.y,lo.z};
    const double mx[3]={hi.x,hi.y,hi.z};
    for(int k=0;k<3;++k){
        if(std::abs(v[k])<1e-12){
            if(a[k]<mn[k]||a[k]>mx[k])return false;
            continue;
        }
        double x=(mn[k]-a[k])/v[k],y=(mx[k]-a[k])/v[k];
        if(x>y)std::swap(x,y);
        tlo=std::max(tlo,x);thi=std::min(thi,y);
        if(tlo>thi)return false;
    }
    return true;
}
inline SourceVec3 source_movement_push(double yaw_degrees,float forward,
                                        float strafe,double speed)noexcept{
    const double sum=static_cast<double>(forward)*forward+
                     static_cast<double>(strafe)*strafe;
    if(sum<1e-4)return {};
    const double mag=std::max(1.0,std::sqrt(sum));
    const double f=static_cast<double>(forward)*speed/mag;
    const double s=static_cast<double>(strafe)*speed/mag;
    constexpr double pi=3.14159265358979323846;
    const double yaw=yaw_degrees*(pi/180.0);
    return {s*std::cos(yaw)-f*std::sin(yaw),0.0,
            f*std::cos(yaw)+s*std::sin(yaw)};
}
inline std::vector<TimerRangeRayStep>
source_timer_range_predict(const TimerRangeSourceObservation& o,
                           const TimerRangeSettings& settings){
    if(!source_finite(o.player_eye)||!source_finite(o.initial_motion)||
       !source_finite(o.target_client_pos)||!source_finite(o.target_server_pos)||
       !std::isfinite(o.yaw_degrees)||!std::isfinite(o.pitch_degrees)||
       !std::isfinite(o.movement_factor)||!std::isfinite(o.forward_input)||
       !std::isfinite(o.strafe_input)||o.movement_factor<0||
       o.movement_factor>2.0||o.target_interpolation_steps<0||
       o.target_interpolation_steps>10000)
        throw std::invalid_argument("bad Timer Range source snapshot");
    const float source_max=settings.mode==TimerRangeMode::freeze_after?
        settings.duration_ticks:settings.duration_legit_ticks;
    if(!std::isfinite(source_max)||source_max<0||source_max>128)
        throw std::invalid_argument("Timer Range prediction exceeds bounded 128-step source window");
    const auto max_steps=static_cast<int>(source_max);
    constexpr double pi=3.14159265358979323846;
    const double yaw=-static_cast<double>(static_cast<float>(pi))-
                     o.yaw_degrees*(pi/180.0);
    const double pitch=-o.pitch_degrees*(pi/180.0);
    const auto cp=std::cos(pitch);
    const SourceVec3 look{std::sin(yaw)*(-cp),std::sin(pitch),
                          std::cos(yaw)*(-cp)};
    const auto ray=source_scale(look,3.0);
    SourceVec3 motion=o.initial_motion,eye=o.player_eye;
    SourceVec3 target=o.target_client_pos,target_server=o.target_server_pos;
    int remaining=o.target_interpolation_steps;
    std::vector<TimerRangeRayStep> out;
    out.reserve(static_cast<std::size_t>(max_steps));
    for(int i=1;i<=max_steps;++i){
        const auto lo=source_sub(target,{0.4,0.1,0.4});
        const auto hi=source_add(target,{0.4,1.9,0.4});
        const bool intersects=source_segment_hits_aabb(
            eye,source_add(eye,ray),lo,hi);
        const double near=source_dist_sq(source_closest_aabb_point(eye,lo,hi),eye);
        if(!std::isfinite(near))throw std::invalid_argument("ray distance overflow");
        out.push_back({intersects,near});
        // The original exits on its first qualifying candidate. Further
        // steps are unnecessary when the controller will choose this one.
        if(intersects&&near>0)return out;

        const auto push=source_movement_push(o.yaw_degrees,o.forward_input,
                                             o.strafe_input,o.movement_factor);
        motion=source_add(motion,push);
        eye=source_add(eye,motion);
        motion=source_scale(motion,0.546);
        if(settings.mode==TimerRangeMode::freeze_before){
            const double speed=std::hypot(motion.x,motion.z);
            const auto towards=source_sub(eye,target_server);
            const double horiz=std::hypot(towards.x,towards.z);
            // Source divides by horiz with no guard, producing NaN on exact
            // overlap. Fail closed instead of emitting invalid coordinates.
            if(horiz<1e-12&&speed>0)
                throw std::invalid_argument("undefined zero-horizontal-distance target walk");
            if(horiz>=1e-12){
                auto step=source_scale(towards,speed/horiz);
                step.y=0;
                target_server=source_add(target_server,step);
            }
        }
        if(remaining>0){
            const auto delta=source_scale(source_sub(target_server,target),
                                          1.0/static_cast<double>(remaining));
            target=source_add(target,delta);
            --remaining;
        }
        if(!source_finite(eye)||!source_finite(motion)||
           !source_finite(target)||!source_finite(target_server))
            throw std::invalid_argument("Timer Range prediction overflow");
    }
    return out;
}
} // namespace slinkywinky::reference_compat
