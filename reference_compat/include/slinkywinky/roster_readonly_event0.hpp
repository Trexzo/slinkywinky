#pragma once
// R281: owner-bound, read-only roster -> existing Teams/AntiBot event0.
// Not a JNI hook, Minecraft pointer, packet or RelationshipService writer.
#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {
struct RosterSourcePlayer {
    AntiBotPlayer bot{};
    // Complete rawName plus optional actual ChatComponentText display
    // evidence, NOT a name fabricated from the player's UUID.
    TeamsEntityName teams{};
};
struct RosterSourceSnapshot {
    std::uint64_t world_token=0,owner_token=0,generation=0,tick=0;
    AntiBotUuid owner_uuid{};
    std::vector<RosterSourcePlayer> remote{};
    bool complete_team_color_evidence=false;
    TeamsOwnerName owner_team{};
};
struct RosterSourceDecisions {
    bool teams_evaluated=false,antibot_evaluated=false;
    std::uint64_t generation=0,tick=0;
    // Each UUID and bit proposal stays distinct. No game writes.
    struct PerPlayer {
        AntiBotUuid uuid{};
        bool team_member=false,antibot_rejected=false;
    };
    std::vector<PerPlayer> remote{};
};
class RosterReadOnlyEvent0 final {
public:
    RosterReadOnlyEvent0()
       : owner_thread_(std::this_thread::get_id()) {
        anti_id_=bridge_.add({"Anti Bot",4,{{0,100}},false});
        teams_id_=bridge_.add({"Teams",4,{{0,50}},false});
        if(!bridge_.bind_anti_bot(anti_id_) ||
           !bridge_.bind_teams(teams_id_))
            throw std::logic_error("Roster native callbacks missing");
    }
    bool set_enabled(bool anti,bool teams) {
        if(!owned() || closed_)return false;
        return bridge_.request(anti_id_,anti) &&
               bridge_.request(teams_id_,teams) &&
               bridge_.apply(true,true) &&
               bridge_.enabled(anti_id_)==anti &&
               bridge_.enabled(teams_id_)==teams;
    }
    std::optional<RosterSourceDecisions> observe(
        const RosterSourceSnapshot& source,
        const AntiBotSettings& antiSettings,
        const TeamsSettings& teamsSettings) {
        if(!owned() || closed_)return std::nullopt;
        const bool anti=bridge_.enabled(anti_id_);
        const bool teams=bridge_.enabled(teams_id_);
        if(!anti && !teams)return std::nullopt;
        if(!source.world_token || !source.owner_token ||
           source.world_token==source.owner_token ||
           !source.generation || source.remote.size()>256u ||
           (teams && !source.complete_team_color_evidence) ||
           antiSettings.invalid_uuid_mode<0 ||
           antiSettings.invalid_uuid_mode>2 ||
           !std::isfinite(antiSettings.min_age) ||
           antiSettings.min_age<0.0f)
            return std::nullopt;
        const bool first=!have_scope_;
        const bool changed=!first &&
            (source.world_token!=world_token_ ||
             source.owner_token!=owner_token_ ||
             !(source.owner_uuid==owner_uuid_));
        if(!first && (changed ?
            source.generation<=generation_ :
            source.generation!=generation_ || source.tick<last_tick_))
            return std::nullopt;
        std::set<AntiBotUuid> seen;
        for(const auto& p:source.remote) {
            if(p.bot.uuid==source.owner_uuid ||
               !seen.insert(p.bot.uuid).second ||
               p.bot.ticks_existed<0 ||
               !std::isfinite(p.bot.previous_x) ||
               !std::isfinite(p.bot.previous_z) ||
               !std::isfinite(p.bot.x) ||
               !std::isfinite(p.bot.z) ||
               p.bot.name!=p.teams.raw_name)return std::nullopt;
        }
        ExecutionFrame frame;
        frame.event=kSafeWalkEvent;
        frame.world_present=frame.player_present=true;
        if(anti) {
            AntiBotEvent0Payload p;
            p.context_ready=true;
            p.world_token=source.world_token;
            p.local_player=source.owner_uuid;
            p.settings=antiSettings;
            p.other_players.reserve(source.remote.size());
            for(const auto& v:source.remote)p.other_players.push_back(v.bot);
            frame.anti_bot_event0=std::move(p);
        }
        if(teams) {
            TeamsEvent0Payload p;
            p.world_ready=true;
            p.source_world_token=source.world_token;
            p.source_owner_uuid=source.owner_uuid;
            p.settings=teamsSettings;
            p.owner=source.owner_team;
            p.other_players.reserve(source.remote.size());
            p.other_player_uuids.reserve(source.remote.size());
            for(const auto& v:source.remote) {
                p.other_players.push_back(v.teams);
                p.other_player_uuids.push_back(v.bot.uuid);
            }
            frame.teams_event0=std::move(p);
        }
        RosterSourceDecisions out;
        out.generation=source.generation;out.tick=source.tick;
        out.remote.reserve(source.remote.size());
        if(changed)bridge_.reset_anti_bot_world_state();
        if(!bridge_.dispatch(frame))return std::nullopt;
        if((anti && (!frame.anti_bot_event0->decision.admitted ||
             frame.anti_bot_event0->executions!=1 ||
             frame.anti_bot_event0->decision.proposals.size()!=source.remote.size())) ||
           (teams && (frame.teams_event0->executions!=1 ||
             frame.teams_event0->decisions.size()!=source.remote.size())))
            return std::nullopt;
        out.antibot_evaluated=anti;out.teams_evaluated=teams;
        for(std::size_t i=0;i<source.remote.size();++i) {
            RosterSourceDecisions::PerPlayer p;
            p.uuid=source.remote[i].bot.uuid;
            if(anti) {
                const auto& proposed=frame.anti_bot_event0->decision.proposals[i];
                if(!(proposed.uuid==p.uuid) || proposed.relationship_flag!=2u)
                    return std::nullopt;
                p.antibot_rejected=proposed.rejected;
            }
            if(teams) {
                const auto& proposed=frame.teams_event0->decisions[i];
                if(!proposed.admitted || proposed.relationship_flag!=1u)
                    return std::nullopt;
                p.team_member=proposed.proposed_team_member;
            }
            out.remote.push_back(p);
        }
        world_token_=source.world_token;
        owner_token_=source.owner_token;
        owner_uuid_=source.owner_uuid;
        generation_=source.generation;
        last_tick_=source.tick;
        have_scope_=true;
        return out;
    }
    bool close() {
        if(!owned() || closed_)return false;
        bridge_.stop();closed_=true;
        have_scope_=false;
        return true;
    }
private:
    bool owned() const noexcept {return owner_thread_==std::this_thread::get_id();}
    std::thread::id owner_thread_;
    ModuleExecutionBridge bridge_{};
    std::size_t anti_id_=0,teams_id_=0;
    bool closed_=false,have_scope_=false;
    std::uint64_t world_token_=0,owner_token_=0,generation_=0,last_tick_=0;
    AntiBotUuid owner_uuid_{};
};
} // namespace slinkywinky::reference_compat
