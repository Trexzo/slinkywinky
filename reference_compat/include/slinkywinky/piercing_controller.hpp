#pragma once
// R272: source-derived Piercing rank policy, read-only/offline.
// No entity bounding-box changes, raycasts, inventory writes or packets.
#include <cmath>
#include <cstdint>
namespace slinkywinky::reference_compat {
struct PiercingSettings {
    bool enabled=false;
    bool blocks=false,non_players=false,friendlies=false,bots=false;
    bool enemies=false,require_weapon=false,require_inside_bb=false;
    int sort_by=0;
};
struct PiercingEntity {
    bool remote_player=false;
    unsigned relationship_flags=0;
    float health=20.0f;
    int hurt_time=0,ticks_existed=0,last_attacker_time=0;
};
struct PiercingDecision {
    bool admitted=false;
    int ranking=2;   // Source rank 0 prefer, 1 defer, 2 ignore
};
class PiercingController final {
public:
    // The Java mix() uses signed int with defined 32-bit overflow and
    // >>>16 logical shifts; unsigned arithmetic gives equivalent bits.
    static std::uint32_t mix_bits(std::uint32_t value) noexcept {
        value=(value^(value>>16))*73244475u;
        value=(value^(value>>16))*73244475u;
        return value^(value>>16);
    }
    static int compare(int mode,float first_health,float second_health,
                       int first_hurt,int second_hurt,
                       std::uint32_t first_counter,
                       std::uint32_t second_counter) noexcept {
        if(std::isnan(first_health))first_health=20.0f;
        if(std::isnan(second_health))second_health=20.0f;
        if(mode==1) {
            if(second_health>first_health)return 1;
            if(first_health>second_health)return 0;
        } else if(mode==0) {
            if(second_hurt>first_hurt)return 1;
            if(second_hurt<first_hurt)return 0;
        }
        // Compare Java signed int results without C++ signed overflow.
        const auto sa=mix_bits(first_counter)^0x80000000u;
        const auto sb=mix_bits(second_counter)^0x80000000u;
        return sb<=sa?1:0;
    }
    static bool on_event8(bool callback_delivered_enabled) noexcept {
        return callback_delivered_enabled;
    }
    static bool pierce_blocks(const PiercingSettings& s,bool is_weapon) noexcept {
        return s.enabled && s.blocks && (!s.require_weapon || is_weapon);
    }
    static PiercingDecision rank(const PiercingSettings& s,bool is_weapon,
                                 bool inside_bb,const PiercingEntity& first,
                                 const PiercingEntity& second) noexcept {
        PiercingDecision result;
        if(s.sort_by<0 || s.sort_by>2 ||
           !std::isfinite(first.health) && !std::isnan(first.health) ||
           !std::isfinite(second.health) && !std::isnan(second.health))
            return result;
        result.admitted=true;
        if(!s.enabled || (!inside_bb && s.require_inside_bb) ||
           (s.require_weapon && !is_weapon))
            return result;
        if(!second.remote_player) {
            result.ranking=s.non_players?1:2;
            return result;
        }
        if((s.friendlies && (second.relationship_flags&5u)!=0u) ||
           (s.bots && (second.relationship_flags&2u)!=0u)) {
            result.ranking=1;return result;
        }
        if(!first.remote_player) {
            result.ranking=0;return result;
        }
        if(!s.enemies)return result;
        const auto counter=[&](const PiercingEntity& entity) noexcept {
            return static_cast<std::uint32_t>(entity.ticks_existed)+
                   static_cast<std::uint32_t>(entity.last_attacker_time);
        };
        result.ranking=compare(s.sort_by,first.health,second.health,
            first.hurt_time,second.hurt_time,counter(first),counter(second));
        return result;
    }
};
} // namespace slinkywinky::reference_compat
