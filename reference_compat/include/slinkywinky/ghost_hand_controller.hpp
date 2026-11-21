#pragma once
// R255: first-party, offline Ghost Hand query and interaction filter.
// Reconstructed from user-supplied Java; native registration proves only
// setting presence, not completed original gameplay behavior.
#include <cstdint>

namespace slinkywinky::reference_compat {
enum class GhostHandItem { fists,sword,tool,bucket,flint_steel,cobweb,other };
enum class GhostHandTarget { non_player,bot,friendly,enemy };
struct GhostHandSettings {
    bool non_players=false,friendlies=false,bots=false,enemies=false;
    bool allow_using_sword=false,allow_using_tools=false,allow_using_fists=false;
    bool allow_using_buckets=false,allow_using_fns=false,allow_using_cobweb=false;
    bool allow_using_other=false,require_mouse_pressed=false;
};
struct GhostHandObservation {
    bool module_enabled=false,game_ready=false,mouse_pressed=false;
    // A missing item is an actual fist; an unknown item is not a fist.
    bool holding_item=false;
    int item_id=-1;
    bool target_is_remote_player=false;
    unsigned relationship_flags=0;
};
struct GhostHandDecision {
    bool admitted=false,can_interact_through=false;
    GhostHandItem item=GhostHandItem::other;
    GhostHandTarget target=GhostHandTarget::non_player;
};
class GhostHandController final {
public:
    // Event 8 in the reconstructed source marks queryOverride on every
    // dispatched enabled callback, independent of canInteractThrough().
    static bool on_event8(bool dispatched_to_enabled_module) noexcept {
        return dispatched_to_enabled_module;
    }
    static GhostHandItem item_group(int id) noexcept {
        switch(id) {
            case 30: return GhostHandItem::cobweb;
            case 259: return GhostHandItem::flint_steel;
            case 325: case 326: case 327:return GhostHandItem::bucket;
            case 267: case 268: case 272: case 276: case 283:
                return GhostHandItem::sword;
            case 256: case 257: case 258: case 269: case 270:
            case 271: case 273: case 274: case 275: case 277:
            case 278: case 279: case 284: case 285: case 286:
            case 359:return GhostHandItem::tool;
            default:return GhostHandItem::other;
        }
    }
    static GhostHandTarget target_group(bool remote_player,
                                        unsigned flags) noexcept {
        if(!remote_player)return GhostHandTarget::non_player;
        // Reconstructed RelationshipService bits: bots=2,
        // friendlies=1|4, then enemies. Bot overrides friendly.
        if((flags&2u)!=0u)return GhostHandTarget::bot;
        if((flags&5u)!=0u)return GhostHandTarget::friendly;
        return GhostHandTarget::enemy;
    }
    static GhostHandDecision can_interact_through(
        const GhostHandSettings& cfg,
        const GhostHandObservation& obs) noexcept {
        GhostHandDecision out;
        if(obs.holding_item && obs.item_id<0)return out;
        out.admitted=true;
        out.item=obs.holding_item?item_group(obs.item_id):
                                  GhostHandItem::fists;
        out.target=target_group(obs.target_is_remote_player,
                                obs.relationship_flags);
        if(!obs.module_enabled || !obs.game_ready ||
           (cfg.require_mouse_pressed && !obs.mouse_pressed))
            return out;
        bool item_allowed=false;
        switch(out.item) {
            case GhostHandItem::fists:item_allowed=cfg.allow_using_fists;break;
            case GhostHandItem::sword:item_allowed=cfg.allow_using_sword;break;
            case GhostHandItem::tool:item_allowed=cfg.allow_using_tools;break;
            case GhostHandItem::bucket:item_allowed=cfg.allow_using_buckets;break;
            case GhostHandItem::flint_steel:item_allowed=cfg.allow_using_fns;break;
            case GhostHandItem::cobweb:item_allowed=cfg.allow_using_cobweb;break;
            case GhostHandItem::other:item_allowed=cfg.allow_using_other;break;
        }
        if(!item_allowed)return out;
        switch(out.target) {
            case GhostHandTarget::non_player:
                out.can_interact_through=cfg.non_players;break;
            case GhostHandTarget::bot:
                out.can_interact_through=cfg.bots;break;
            case GhostHandTarget::friendly:
                out.can_interact_through=cfg.friendlies;break;
            case GhostHandTarget::enemy:
                out.can_interact_through=cfg.enemies;break;
        }
        return out;
    }
    static bool sword_excluded(const GhostHandSettings& cfg) noexcept {
        return !cfg.allow_using_sword;
    }
};
} // namespace slinkywinky::reference_compat
