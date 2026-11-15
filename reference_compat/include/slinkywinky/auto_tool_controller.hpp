#pragma once
// R228 first-party Auto Tool from pinned OpenSlinky C++ semantics.
// Block category 3; native inventory tick event25 and shutdown event26.
// All results are value-only proposals, not hotbar/player memory writes.
#include <array>
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {
enum class AutoToolHeldKind {fist, tool, sword, other};
// Exact item IDs from pinned Auto Tool reference; the caller supplies
// item ID, not a guessed GUI category. Empty hand is a separate state.
inline AutoToolHeldKind auto_tool_item_kind(
    bool has_item,int minecraft_item_id) noexcept {
    if(!has_item)return AutoToolHeldKind::fist;
    switch(minecraft_item_id) {
        case 256:case 257:case 258:
        case 269:case 270:case 271:
        case 273:case 274:case 275:
        case 277:case 278:case 279:
        case 284:case 285:case 286:
        case 359:return AutoToolHeldKind::tool;
        case 267:case 268:case 272:case 276:case 283:
            return AutoToolHeldKind::sword;
        default:return AutoToolHeldKind::other;
    }
}
struct AutoToolTickInput {
    bool in_game=false;
    int current_slot=0;
    AutoToolHeldKind held=AutoToolHeldKind::fist;
    bool allow_fists=true,allow_tools=true,allow_swords=true,allow_other=true;
    bool using_item=false,attack_held=false,sneak_held=false;
    bool require_sneak=false,look_at_block=false,block_exists=false;
    bool restrict_allowed_blocks=false,block_is_allowed=false;
    float activation_time=0.0f; // original (int) tick countdown
    bool switch_back=true,hotbar_claim_granted=false;
    std::array<bool,9> stack_present{};
    std::array<float,9> mining_speed{};
};
struct AutoToolTickDecision {
    bool admitted=false,conditions_met=false;
    bool propose_claim_hotbar=false,propose_release_hotbar=false;
    std::optional<int> proposed_selected_slot{};
    int remembered_slot=-1,owned_selected_slot=-1,countdown_remaining=0;
};
struct AutoToolShutdownDecision {
    bool proposed_release_allowed_block_list=false;
};
class AutoToolController final {
public:
    void reset() noexcept {
        remembered_slot_=-1;owned_selected_slot_=-1;countdown_=0;
        shutdown_=false;
    }
    AutoToolTickDecision on_tick(const AutoToolTickInput& in) noexcept {
        AutoToolTickDecision out;
        if(shutdown_ || !in.in_game || in.current_slot<0 ||
           in.current_slot>=9 || !std::isfinite(in.activation_time) ||
           in.activation_time<0.0f || in.activation_time>200.0f)
            return out;
        for(unsigned i=0;i<9;++i)
            if(!std::isfinite(in.mining_speed[i]) ||
               in.mining_speed[i]<0.0f ||
               in.mining_speed[i]>10000.0f)return out;
        out.admitted=true;
        auto finish=[&]() noexcept {
            out.remembered_slot=remembered_slot_;
            out.owned_selected_slot=owned_selected_slot_;
            out.countdown_remaining=countdown_;
            return out;
        };
        auto relinquish=[&]() noexcept {
            out.propose_release_hotbar=true;
            if(remembered_slot_!=-1) {
                out.proposed_selected_slot=remembered_slot_;
                remembered_slot_=-1;
            }
        };
        if(remembered_slot_!=-1 &&
           owned_selected_slot_!=in.current_slot) {
            // User manually selected a different slot: never override it.
            remembered_slot_=-1;
            relinquish();
            return finish();
        }
        bool allowed=true;
        if(remembered_slot_==-1) {
            switch(in.held) {
                case AutoToolHeldKind::fist:allowed=in.allow_fists;break;
                case AutoToolHeldKind::tool:allowed=in.allow_tools;break;
                case AutoToolHeldKind::sword:allowed=in.allow_swords;break;
                case AutoToolHeldKind::other:allowed=in.allow_other;break;
            }
        }
        const bool active=allowed && !in.using_item && in.attack_held &&
            (!in.require_sneak || in.sneak_held) &&
            in.look_at_block && in.block_exists &&
            (!in.restrict_allowed_blocks || in.block_is_allowed);
        out.conditions_met=active;
        if(!active) {
            countdown_=static_cast<int>(in.activation_time);
            relinquish();
            return finish();
        }
        if(countdown_>0) {
            --countdown_;
            relinquish();
            return finish();
        }
        int best=in.current_slot;
        float speed=1.0f;
        for(int i=0;i<9;++i) {
            const int slot=(in.current_slot+i)%9;
            if(!in.stack_present[slot])continue;
            if(in.mining_speed[slot]>speed) {
                speed=in.mining_speed[slot];
                best=slot;
            }
        }
        if(best!=in.current_slot && in.hotbar_claim_granted) {
            out.propose_claim_hotbar=true;
            out.proposed_selected_slot=best;
            owned_selected_slot_=best;
            if(remembered_slot_==-1 && in.switch_back)
                remembered_slot_=in.current_slot;
        }
        return finish();
    }
    AutoToolShutdownDecision on_shutdown() noexcept {
        // Native source releases an allowed-block allocation. This model
        // owns no pointers and requests the external owner to clear its list.
        if(shutdown_)return {};
        shutdown_=true;
        remembered_slot_=-1;
        owned_selected_slot_=-1;
        countdown_=0;
        return {true};
    }
    int remembered_slot() const noexcept{return remembered_slot_;}
    int owned_selected_slot() const noexcept{return owned_selected_slot_;}
    int countdown() const noexcept{return countdown_;}
private:
    int remembered_slot_=-1,owned_selected_slot_=-1,countdown_=0;
    bool shutdown_=false;
};
} // namespace slinkywinky::reference_compat
