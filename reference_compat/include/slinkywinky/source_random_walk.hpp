#pragma once
// R232: first-party deterministic reconstruction of OpenSlinky
// math_helpers.cpp's xoshiro128+ / walk_update pipeline.
// The original runtime chooses an entropy-backed 4-word seed; this offline
// model takes an explicit seed. No system clock, network, or game operations.
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {
struct SourceWalkState {
    double t0=0.0;
    float a=0.0f,b=0.0f;
};
class SourceWalkRng {
public:
    bool seed(const std::array<std::uint32_t,4>& state) noexcept {
        if((state[0]|state[1]|state[2]|state[3])==0)return false;
        words_=state;valid_=true;return true;
    }
    bool seeded() const noexcept {return valid_;}
    const std::array<std::uint32_t,4>& state() const noexcept {
        return words_;
    }
    std::uint32_t next() noexcept {
        // Source: result=s0+s3; t=s1<<9; xor shifts; rotl(s3,11).
        const std::uint32_t result=words_[0]+words_[3];
        const std::uint32_t t=words_[1]<<9u;
        words_[2]^=words_[0];
        words_[3]^=words_[1];
        words_[1]^=words_[2];
        words_[0]^=words_[3];
        words_[2]^=t;
        words_[3]=(words_[3]<<11u)|(words_[3]>>21u);
        return result;
    }
    float unit() noexcept {
        // Original rdata.s random_walk_range_epsilon = 2^-24f:
        // (float)(rng_next() >> 8) * 0x1p-24f.
        return static_cast<float>(next()>>8u)*0x1p-24f;
    }
private:
    std::array<std::uint32_t,4> words_{};
    bool valid_=false;
};

class SourceBlockInWalks {
public:
    bool configure(const std::array<std::uint32_t,4>& seed) noexcept {
        SourceWalkRng next{};
        if(!next.seed(seed))return false;
        rng_=next;
        walks_={};
        last_time_=0.0;
        configured_=true;
        return true;
    }
    bool configured() const noexcept{return configured_;}
    const SourceWalkState& state(unsigned index) const noexcept {
        return walks_[index];
    }
    const std::array<std::uint32_t,4>& rng_state() const noexcept {
        return rng_.state();
    }
    // R234: transfer only the four-word shared generator; walk t0/a/b
    // belong to Block In. Caller owns transaction/dispatch sequencing.
    SourceWalkRng source_rng_snapshot() const noexcept{return rng_;}
    bool adopt_source_rng(const SourceWalkRng& next) noexcept {
        if(!configured_ || !next.seeded())return false;
        rng_=next;
        return true;
    }
    std::optional<std::array<float,2>> sample_placement(
        unsigned bit,double now) noexcept {
        if(bit<12u)
            return sample_pair(2,3,now,0.5);
        if(bit>32u && bit<=41u)
            return sample_pair(0,1,now,0.5);
        return std::nullopt; // roof bit32 is never a place target
    }
    std::optional<std::array<float,2>> sample_jitter(
        double now) noexcept {
        return sample_pair(4,5,now,0.2);
    }
    // Exposed to independently exercise exact source timing/branch logic.
    std::optional<float> sample_unit(unsigned index,double period,
                                      double now) noexcept {
        if(!configured_ || !rng_.seeded() || index>=6 ||
           !std::isfinite(period)||period<=0.0||period>1000000.0||
           !std::isfinite(now)||now<0.0||now>1e12||
           now<last_time_)return std::nullopt;
        SourceWalkState& w=walks_[index];
        if(now>w.t0+period) {
            const bool both=(w.a==0.0f && w.b==0.0f);
            const float old_b=w.b;
            float candidate_a=both ? 0.0f : old_b;
            float candidate_b=old_b;
            bool found=false;
            // Source uses an unbounded do-while. Limit pathological
            // seeded traces to avoid hanging a client.
            for(unsigned tries=0;tries<4096;++tries) {
                if(both && tries==0)candidate_a=rng_.unit();
                candidate_b=rng_.unit();
                if(std::fabs(candidate_a-candidate_b)>=0.2f) {
                    found=true;break;
                }
                if(both)candidate_a=rng_.unit();
            }
            if(!found)return std::nullopt;
            w.t0=now;w.a=candidate_a;w.b=candidate_b;
        }
        const double fraction=(now-w.t0)/period;
        const double unit=static_cast<double>(w.b-w.a)*fraction+
                          static_cast<double>(w.a);
        if(!std::isfinite(unit)||
           unit< -0.000001||unit>1.000001)return std::nullopt;
        last_time_=now;
        return static_cast<float>(unit);
    }
private:
    std::optional<std::array<float,2>> sample_pair(
        unsigned a,unsigned b,double now,double period) noexcept {
        // Transactional: failure in the second walk must not consume
        // the first walk's new state or shared PRNG draws.
        SourceBlockInWalks next=*this;
        const auto first=next.sample_unit(a,period,now);
        if(!first)return std::nullopt;
        const auto second=next.sample_unit(b,period,now);
        if(!second)return std::nullopt;
        *this=next;
        return std::array<float,2>{*first,*second};
    }
    SourceWalkRng rng_{};
    std::array<SourceWalkState,6> walks_{};
    double last_time_=0.0;
    bool configured_=false;
};
} // namespace slinkywinky::reference_compat
