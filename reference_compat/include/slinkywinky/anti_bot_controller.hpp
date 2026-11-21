#pragma once
// R270: first-party, offline Anti Bot event-0 relationship proposals.
// Derived from user-provided reconstructed Java. No entity writes.
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {
struct AntiBotUuid {
    std::uint64_t msb=0,lsb=0;
    bool operator<(const AntiBotUuid& other) const noexcept {
        return msb<other.msb || (msb==other.msb && lsb<other.lsb);
    }
    bool operator==(const AntiBotUuid& other) const noexcept {
        return msb==other.msb && lsb==other.lsb;
    }
};
struct AntiBotSettings {
    bool always_stationary=false;
    int invalid_uuid_mode=0;   // 0=off; 1=version-2 until movement; 2=version-2 always
    bool sleeping=false;
    bool always_invis=false;
    float min_age=0.0f;
    bool invalid_username=false;
};
struct AntiBotPlayer {
    AntiBotUuid uuid{};
    std::u16string name{};
    double previous_x=0.0,previous_z=0.0,x=0.0,z=0.0;
    int ticks_existed=0;
    bool sleeping=false,invisible_to_owner=false,has_armor=false;
};
struct AntiBotProposal {
    AntiBotUuid uuid{};
    bool rejected=false;
    double travelled_squared=0.0;
    bool armor_seen=false,age_accepted=false,name_accepted=false;
    unsigned relationship_flag=2;
};
struct AntiBotFrameResult {
    bool admitted=false,world_changed=false;
    std::vector<AntiBotProposal> proposals{};
};
class AntiBotController final {
public:
    struct History {
        double travelled_squared=0.0;
        bool armor_seen=false,age_accepted=false,name_accepted=false;
    };
    // Java validName: skips U+00A7 and its next UTF-16 unit; then
    // accepts ANY letter/digit/underscore, not a fully valid username.
    static bool valid_name(const std::u16string& text) noexcept {
        for(std::size_t i=0;i<text.size();++i) {
            const char16_t c=text[i];
            if(c==u'\u00a7') {
                if(i+1<text.size())++i;
            } else {
                const unsigned upper=static_cast<unsigned>(c)&0xffdfu;
                if(((upper-65u)&65535u)<26u || c==u'_' ||
                   ((static_cast<unsigned>(c)-
                     static_cast<unsigned>(u'0'))&65535u)<10u)
                    return true;
            }
        }
        return false;
    }
    AntiBotFrameResult on_event0(
        std::uint64_t world_token,
        bool context_ready,
        AntiBotUuid local_player,
        const AntiBotSettings& settings,
        const std::vector<AntiBotPlayer>& players) {
        AntiBotFrameResult out;
        if(world_token==0 || settings.invalid_uuid_mode<0 ||
           settings.invalid_uuid_mode>2 || !std::isfinite(settings.min_age) ||
           settings.min_age<0.0f || players.size()>256u)return out;
        // Perform all calculations against an isolated copy; an invalid
        // observation must never half-commit relationship history.
        auto draft=history_;
        const bool changed=world_!=0 && world_!=world_token;
        if(changed)draft.clear();
        out.world_changed=changed;
        out.admitted=true;
        if(!context_ready) {
            history_=std::move(draft);
            world_=world_token;
            return out;
        }
        for(const auto& p:players) {
            if(p.uuid==local_player)continue;
            if(p.ticks_existed<0 || !std::isfinite(p.previous_x) ||
               !std::isfinite(p.previous_z) || !std::isfinite(p.x) ||
               !std::isfinite(p.z))return {};
            auto& history=draft[p.uuid];
            const double dx=p.previous_x-p.x;
            const double dz=p.previous_z-p.z;
            const double travelled=history.travelled_squared+dx*dx+dz*dz;
            if(!std::isfinite(travelled))return {};
            history.travelled_squared=travelled;
            AntiBotProposal proposal;
            proposal.uuid=p.uuid;
            bool reject=false;
            if(settings.always_stationary && travelled<1.0)
                reject=true;
            // Preserve the recovered Java's unusual predicate EXACTLY:
            // mode!=0 AND UUID version nibble == 0x2000 AND
            // (mode==2 OR travelledSquared<1) => rejected.
            // A DIFFERENT nibble is explicitly admitted by this gate.
            else if(settings.invalid_uuid_mode!=0 &&
                    (static_cast<std::uint32_t>(p.uuid.msb)&0xf000u)==0x2000u &&
                    (settings.invalid_uuid_mode==2 || travelled<1.0))
                reject=true;
            else if(settings.sleeping && p.sleeping)
                reject=true;
            else {
                if(settings.always_invis && !history.armor_seen) {
                    if(p.invisible_to_owner && !p.has_armor) {
                        reject=true;
                    } else if(p.ticks_existed>=21) {
                        history.armor_seen=true;
                    }
                }
                if(!reject && settings.min_age>0.0f &&
                   !history.age_accepted) {
                    if(static_cast<float>(p.ticks_existed)<settings.min_age)
                        reject=true;
                    else history.age_accepted=true;
                }
                if(!reject && settings.invalid_username &&
                   !history.name_accepted) {
                    history.name_accepted=valid_name(p.name);
                    if(!history.name_accepted)reject=true;
                }
            }
            proposal.rejected=reject;
            proposal.travelled_squared=history.travelled_squared;
            proposal.armor_seen=history.armor_seen;
            proposal.age_accepted=history.age_accepted;
            proposal.name_accepted=history.name_accepted;
            out.proposals.push_back(proposal);
        }
        history_=std::move(draft);
        world_=world_token;
        return out;
    }
    unsigned on_disable_clear_mask() noexcept {
        reset();return 2u;
    }
    unsigned on_unload_clear_mask() noexcept {
        reset();return 2u;
    }
    void reset() noexcept {history_.clear();world_=0;}
    std::size_t history_entries() const noexcept {return history_.size();}
    std::uint64_t current_world() const noexcept {return world_;}
private:
    std::map<AntiBotUuid,History> history_{};
    std::uint64_t world_=0;
};
} // namespace slinkywinky::reference_compat
