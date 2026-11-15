#pragma once
// R233 independent value-only shared-source xoshiro random-walk bank:
// exact pinned math_helpers.cpp clock/timed retarget/interpolation paths.
// Explicit seeded PRNG, explicit clock/time deltas: no system RNG, game or IO.
#include "slinkywinky/source_random_walk.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
namespace slinkywinky::reference_compat {
template <std::size_t N>
class SourceWalkBank final {
public:
    bool configure(const std::array<std::uint32_t,4>& words) noexcept {
        SourceWalkRng rng{};
        if(!rng.seed(words))return false;
        rng_=rng;walks_={};last_clock_=0.0;configured_=true;return true;
    }
    bool configured() const noexcept {return configured_;}
    const SourceWalkState& state(std::size_t i) const noexcept {
        return walks_[i];
    }
    const std::array<std::uint32_t,4>& rng_state() const noexcept {
        return rng_.state();
    }
    std::optional<float> unit() noexcept {
        if(!configured_)return std::nullopt;
        return rng_.unit();
    }
    std::optional<float> sample_clock(
        std::size_t index,double now,double period,
        float lower,float upper) noexcept {
        if(!inputs(index,period,lower,upper) ||
           !std::isfinite(now) || now<0.0||now>1e12||
           now<last_clock_)return std::nullopt;
        SourceWalkBank next=*this;
        auto& w=next.walks_[index];
        if(now>w.t0+period) {
            if(!next.retarget(w))return std::nullopt;
            w.t0=now;
        }
        const auto out=value(w,(now-w.t0)/period,lower,upper);
        if(!out)return std::nullopt;
        next.last_clock_=now;
        *this=next;
        return out;
    }
    std::optional<float> sample_timed(
        std::size_t index,double delta,double period,
        float lower,float upper) noexcept {
        if(!inputs(index,period,lower,upper) ||
           !std::isfinite(delta) || delta<0.0 || delta>period)
           return std::nullopt;
        SourceWalkBank next=*this;
        auto& w=next.walks_[index];
        w.t0+=delta;
        if(w.t0>period) {
            w.t0=0.0;
            if(!next.retarget(w))return std::nullopt;
        }
        const auto out=value(w,w.t0/period,lower,upper);
        if(!out)return std::nullopt;
        *this=next;
        return out;
    }
private:
    bool inputs(std::size_t index,double period,
                float lo,float hi) const noexcept {
        return configured_&&index<N&&std::isfinite(period)&&
               period>0.0&&period<=1000000.0&&
               std::isfinite(lo)&&std::isfinite(hi)&&
               std::fabs(lo)<1e6f&&std::fabs(hi)<1e6f;
    }
    bool retarget(SourceWalkState& w) noexcept {
        const bool both=w.a==0.0f&&w.b==0.0f;
        const float old_b=w.b;
        // Matches the source's do-while: initial draws a and b,
        // later sets a=old b and samples just the new b.
        for(unsigned tries=0;tries<4096;++tries) {
            const float a=both?rng_.unit():old_b;
            const float b=rng_.unit();
            if(std::fabs(a-b)>=0.2f) {
                w.a=a;w.b=b;
                return true;
            }
        }
        return false;
    }
    static std::optional<float> value(
        const SourceWalkState& w,double t,float lo,float hi) noexcept {
        const double unit=static_cast<double>(w.b-w.a)*t+
                          static_cast<double>(w.a);
        if(!std::isfinite(unit) || unit < -0.000001 ||
           unit > 1.000001)return std::nullopt;
        const float scaled=static_cast<float>(
            static_cast<double>(hi-lo)*unit+static_cast<double>(lo));
        return std::isfinite(scaled)?std::optional<float>(scaled):
                                     std::nullopt;
    }
    SourceWalkRng rng_{};
    std::array<SourceWalkState,N> walks_{};
    double last_clock_=0.0;
    bool configured_=false;
};
} // namespace slinkywinky::reference_compat
