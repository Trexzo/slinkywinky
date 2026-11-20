#pragma once
// R257: owner-thread, source-backed event-8 QUERY-ONLY composition.
// Source fd9c592: Reach, Hitboxes and Ghost Hand share event slot +0xc8.
// The native priority/order between different source callbacks is not
// independently established; proposals are returned in DISTINCT FIELDS,
// never overwritten or committed to game/JVM memory.
#include "slinkywinky/core.hpp"
#include "slinkywinky/recovered_entity_query_policies.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <thread>

namespace slinkywinky {
inline constexpr unsigned kR257ReachQueryEvent=8u;

enum class RecoveredEvent8Module : unsigned char {
    Reach=0,Hitboxes=1,GhostHand=2
};
struct RecoveredEvent8Input {
    ReachInput reach{};
    ReachSettings reach_settings{};
    float expansion=0.0f;
    RecoveredGhostHandSettings ghost_settings{};
    RecoveredGhostHandQuery ghost_query{};
};
struct RecoveredEvent8Result {
    bool reach_callback_ran=false;
    std::optional<double> proposed_reach{};
    std::optional<RecoveredHitboxesDecision> hitboxes{};
    std::optional<RecoveredGhostHandDecision> ghost_hand{};
};

class RecoveredEvent8Owner final {
public:
    RecoveredEvent8Owner():owner_(std::this_thread::get_id()){}
    RecoveredEvent8Owner(const RecoveredEvent8Owner&)=delete;
    RecoveredEvent8Owner& operator=(const RecoveredEvent8Owner&)=delete;

    bool transition(std::uint64_t generation,bool ready) noexcept {
        if(!on_owner() || retired_ || generation==0 ||
           generation<generation_ ||
           (generation==generation_ && !ready_ && ready))
            return false;
        if(!entities_.transition(generation,ready))
            return false;
        if(generation!=generation_ || (ready_ && !ready))
            reach_.reset();
        generation_=generation;
        ready_=ready;
        return true;
    }
    bool select(RecoveredEvent8Module mod,bool enabled) noexcept {
        if(!on_owner() || retired_)return false;
        switch(mod){
            case RecoveredEvent8Module::Reach:
                if(reach_enabled_!=enabled)reach_.reset();
                reach_enabled_=enabled;
                return true;
            case RecoveredEvent8Module::Hitboxes:
                return entities_.select(RecoveredEntityQueryModule::Hitboxes,
                                        enabled);
            case RecoveredEvent8Module::GhostHand:
                return entities_.select(RecoveredEntityQueryModule::GhostHand,
                                        enabled);
        }
        return false;
    }
    bool selected(RecoveredEvent8Module mod) const noexcept {
        if(retired_)return false;
        switch(mod){
            case RecoveredEvent8Module::Reach:return reach_enabled_;
            case RecoveredEvent8Module::Hitboxes:
                return entities_.selected(RecoveredEntityQueryModule::Hitboxes);
            case RecoveredEvent8Module::GhostHand:
                return entities_.selected(RecoveredEntityQueryModule::GhostHand);
        }
        return false;
    }
    std::optional<RecoveredEvent8Result> query(
            unsigned event,const RecoveredEvent8Input& in) noexcept {
        if(!on_owner() || retired_ || !ready_ ||
           event!=kR257ReachQueryEvent)
            return std::nullopt;
        RecoveredEvent8Result out;
        if(reach_enabled_) {
            out.reach_callback_ran=true;
            // Restrict invalid settings before the stateful policy is
            // evaluated. Bounding 16 blocks is an intentional deviation
            // from the source's unchecked floating-point value.
            if(std::isfinite(in.reach_settings.distance) &&
               std::isfinite(in.reach_settings.activation_ticks) &&
               in.reach_settings.distance>=0.0f &&
               in.reach_settings.distance<=16.0f)
                out.proposed_reach=reach_.evaluate(
                    in.reach,in.reach_settings);
        }
        out.hitboxes=entities_.hitboxes(event,in.expansion);
        out.ghost_hand=entities_.ghost_hand(
            event,in.ghost_settings,in.ghost_query);
        return out;
    }
    int reach_cooldown_ticks() const noexcept{
        return reach_.cooldown_ticks();
    }
    std::optional<ModuleArrayListSnapshot> snapshot(
            std::uint64_t sequence) const {
        if(!on_owner() || retired_ || !generation_ || !sequence ||
           !candidate_native_manager_registered("Reach"))
            return std::nullopt;
        const auto entity=entities_.snapshot(sequence);
        if(!entity || entity->world_generation!=generation_ ||
           entity->world_ready!=ready_ || entity->modules.size()!=2)
            return std::nullopt;
        ModuleArrayListSnapshot result=*entity;
        result.modules.insert(result.modules.begin(),{
            "Reach",reach_enabled_,reach_enabled_&&ready_,generation_
        });
        if(!make_module_arraylist_display(result))
            return std::nullopt;
        return result;
    }
    bool retire() noexcept {
        if(!on_owner() || retired_ || !entities_.retire())
            return false;
        retired_=true;
        ready_=false;
        reach_enabled_=false;
        reach_.reset();
        return true;
    }
    bool retired() const noexcept{return retired_;}
    std::uint64_t generation() const noexcept{return generation_;}
private:
    bool on_owner() const noexcept{
        return std::this_thread::get_id()==owner_;
    }
    const std::thread::id owner_;
    RecoveredEntityQueryOwner entities_;
    ReachController reach_;
    std::uint64_t generation_=0;
    bool ready_=false;
    bool reach_enabled_=false;
    bool retired_=false;
};
} // namespace slinkywinky
