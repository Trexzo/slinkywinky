#pragma once
// R177: independent offline adapter from candidate event registration to
// existing SlinkyWinky C++ decision models. No third-party source copied.
// NO JNI, Java hooks, input injection, game packets, or live player state.
#include "slinkywinky/core.hpp"
#include "slinkywinky/reference_event_model.hpp"
#include "slinkywinky/no_jump_delay_controller.hpp"
#include "slinkywinky/null_move_controller.hpp"
#include "slinkywinky/instant_stop_controller.hpp"
#include "slinkywinky/no_slow_controller.hpp"
#include "slinkywinky/no_hit_delay_controller.hpp"
#include "slinkywinky/no_use_delay_controller.hpp"
#include "slinkywinky/item_use_fix_controller.hpp"
#include "slinkywinky/ping_fix_controller.hpp"
#include "slinkywinky/no_item_release_controller.hpp"
#include "slinkywinky/anti_debuff_controller.hpp"
#include "slinkywinky/fast_accel_controller.hpp"
#include "slinkywinky/safe_walk_controller.hpp"
#include "slinkywinky/fast_place_controller.hpp"
#include "slinkywinky/fast_mine_controller.hpp"
#include "slinkywinky/bridge_assist_controller.hpp"
#include "slinkywinky/auto_tool_controller.hpp"
#include "slinkywinky/auto_weapon_controller.hpp"
#include "slinkywinky/weapons_controller.hpp"
#include "slinkywinky/weapons_tick_cache.hpp"
#include "slinkywinky/clutch_controller.hpp"
#include "slinkywinky/free_look_controller.hpp"
#include "slinkywinky/ghost_hand_controller.hpp"
#include "slinkywinky/hitboxes_controller.hpp"
#include "slinkywinky/teams_controller.hpp"
#include "slinkywinky/anti_bot_controller.hpp"
#include "slinkywinky/piercing_controller.hpp"
#include "slinkywinky/block_in_controller.hpp"
#include "slinkywinky/timer_controller.hpp"
#include "slinkywinky/keep_sprint_controller.hpp"
#include "slinkywinky/sprint_controller.hpp"
#include "slinkywinky/strafe_controller.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {

// Candidate slot +0xC8 is event 8: reach query.
// This mapping is a reference hypothesis, not commercial parity evidence.
inline constexpr unsigned kReachQueryEvent = 8;
inline constexpr unsigned kPlayerUpdateEvent = 17;
inline constexpr unsigned kMovementTickEvent = 16;
inline constexpr unsigned kAutoToolInventoryEvent = 25;
inline constexpr unsigned kAutoToolShutdownEvent = 26;
inline constexpr unsigned kClutchRotationFlagEvent = 13;

struct ReachQueryPayload {
    slinkywinky::ReachInput input{};
    slinkywinky::ReachSettings settings{};
    std::optional<double> proposed_distance{};
    unsigned executions = 0;
};
struct GhostHandQueryPayload {
    GhostHandObservation input{};
    GhostHandSettings settings{};
    GhostHandDecision decision{};
    bool query_override=false;
    unsigned executions=0;
};
struct HitboxesQueryPayload {
    HitboxesObservation input{};
    HitboxesProposal decision{};
    unsigned executions=0;
};
struct PiercingQueryPayload {
    bool query_override=false;
    unsigned executions=0;
};
struct TeamsEvent0Payload {
    bool world_ready=false;
    TeamsOwnerName owner{};
    TeamsSettings settings{};
    std::vector<TeamsEntityName> other_players{};
    // R275: optional for a standalone Teams fixture, but mandatory
    // when Teams and Anti Bot share a physical event-0 snapshot.
    std::uint64_t source_world_token=0;
    AntiBotUuid source_owner_uuid{};
    std::vector<AntiBotUuid> other_player_uuids{};
    std::vector<TeamsDecision> decisions{};
    unsigned executions=0;
};
struct AntiBotEvent0Payload {
    bool context_ready=false;
    std::uint64_t world_token=0;
    AntiBotUuid local_player{};
    AntiBotSettings settings{};
    std::vector<AntiBotPlayer> other_players{};
    AntiBotFrameResult decision{};
    unsigned executions=0;
};

struct JumpDelayTickPayload {
    JumpDelayInput input{};
    std::optional<int> proposed_jump_ticks{};
    unsigned executions = 0;
};

struct NullMoveTickPayload {
    NullMoveInput input{};
    NullMoveDecision decision{};
    unsigned executions = 0;
};

struct InstantStopTickPayload {
    InstantStopInput input{};
    InstantStopDecision decision{};
    unsigned executions = 0;
};

struct NoSlowTickPayload {
    NoSlowInput input{};
    NoSlowDecision decision{};
    unsigned executions = 0;
};

struct NoHitDelayTickPayload {
    NoHitDelayInput input{};
    NoHitDelayDecision decision{};
    unsigned executions = 0;
};

struct NoUseDelayTickPayload {
    NoUseDelayInput input{};
    NoUseDelayDecision decision{};
    unsigned executions = 0;
};

struct ItemUseFixTickPayload {
    ItemUseFixInput input{};
    ItemUseFixDecision decision{};
    unsigned executions = 0;
};

struct PingFixTickPayload {
    PingFixInput input{};
    PingFixDecision decision{};
    unsigned executions = 0;
};

struct NoItemReleaseTickPayload {
    NoItemReleaseInput input{};
    NoItemReleaseDecision decision{};
    unsigned executions = 0;
};

struct AntiDebuffTickPayload {
    AntiDebuffInput input{};
    AntiDebuffDecision decision{};
    unsigned executions = 0;
};

struct FastAccelTickPayload {
    FastAccelInput input{};
    FastAccelDecision decision{};
    unsigned executions = 0;
};

struct SafeWalkTickPayload {
    SafeWalkInput input{};
    SafeWalkDecision decision{};
    unsigned executions = 0;
};

struct FastPlaceTickPayload {
    FastPlaceInput input{};
    FastPlaceDecision decision{};
    unsigned executions = 0;
};
struct FastMineEarlyPayload {
    FastMineEarlyInput input{};
    FastMineEarlyDecision decision{};
    unsigned executions=0;
};
struct FastMineLatePayload {
    FastMineLateInput input{};
    FastMineLateDecision decision{};
    unsigned executions=0;
};
struct BridgeAssistDisablePayload {
    BridgeAssistDisableInput input{};
    BridgeAssistDisableDecision decision{};
    unsigned executions=0;
};
struct BridgeAssistPlacePayload {
    BridgeAssistPlaceInput input{};
    BridgeAssistPlaceDecision decision{};
    unsigned executions=0;
};
struct BridgeAssistTickPayload {
    BridgeAssistTickInput input{};
    BridgeAssistTickDecision decision{};
    unsigned executions=0;
};
struct BridgeAssistLatePayload {
    BridgeAssistLateDecision decision{};
    unsigned executions=0;
};
struct AutoToolInventoryPayload {
    AutoToolTickInput input{};
    AutoToolTickDecision decision{};
    unsigned executions=0;
};
// R261: supplies exact hotbar/setting value snapshots to the first-party
// Weapons selector, so the Auto Weapon event does NOT trust caller-provided
// first_allowed_slot or current_is_weapon hints.
struct AutoWeaponInventoryPayload {
    AutoWeaponInput input{};
    WeaponsInventory inventory{};
    WeaponsSettings settings{};
    int player_ticks_existed=0;
    WeaponsChoice source_choice{};
    WeaponsTickReceipt source_cache{};
    bool source_cache_queried=false;
    bool source_current_is_weapon=false;
    AutoWeaponDecision decision{};
    unsigned executions=0;
};
struct WeaponsClearExtrasPayload {
    bool proposed_clear_extra_items=false;
    unsigned executions=0;
};
struct AutoToolShutdownPayload {
    AutoToolShutdownDecision decision{};
    unsigned executions=0;
};
struct ClutchClickPayload {
    ClutchClickInput input{};
    ClutchClickDecision decision{};
    unsigned executions=0;
};
struct ClutchRotationPayload {
    ClutchRotationDecision decision{};
    unsigned executions=0;
};
struct FreeLookRotationPayload {
    FreeLookFrame input{};
    FreeLookProposal decision{};
    unsigned executions=0;
};
struct ClutchLatePayload {
    ClutchLateInput input{};
    ClutchLateDecision decision{};
    unsigned executions=0;
};
struct BlockInEmptyPayload {
    unsigned executions=0;
};
struct BlockInTogglePayload {
    BlockInToggleInput input{};
    BlockInToggleDecision decision{};
    unsigned executions=0;
};
struct BlockInRotationPayload {
    BlockInRotationDecision decision{};
    unsigned executions=0;
};
struct BlockInMovementPayload {
    BlockInMovementDecision decision{};
    unsigned executions=0;
};
struct BlockInTickPayload {
    BlockInWorldInput input{};
    BlockInTickDecision decision{};
    unsigned executions=0;
};

struct TimerFramePayload {
    TimerFrameInput input{};
    TimerFrameDecision decision{};
    unsigned executions = 0;
};

struct TimerTogglePayload {
    TimerToggleInput input{};
    TimerToggleDecision decision{};
    unsigned executions = 0;
};

struct KeepSprintAttackPayload {
    KeepSprintAttackInput input{};
    KeepSprintAttackDecision decision{};
    unsigned executions = 0;
};

struct KeepSprintPostPayload {
    KeepSprintPostInput input{};
    KeepSprintPostDecision decision{};
    unsigned executions = 0;
};

struct KeepSprintUpdatePayload {
    KeepSprintUpdateInput input{};
    KeepSprintUpdateDecision decision{};
    unsigned executions = 0;
};

struct SprintUpdatePayload {
    SprintUpdateInput input{};
    SprintUpdateDecision decision{};
    unsigned executions = 0;
};

struct SprintMovementPayload {
    SprintMovementInput input{};
    SprintMovementDecision decision{};
    unsigned executions = 0;
};

struct StrafeSnapshotPayload {
    bool sprinting = false;
    unsigned executions = 0;
};

struct StrafeSteeringPayload {
    StrafeMotionInput input{};
    StrafeMotionDecision decision{};
    unsigned executions = 0;
};

struct ExecutionFrame {
    unsigned event = 0;
    bool world_present = false;
    bool player_present = false;
    // Offline event replay's native toggle-notification target/state.
    // Defaults preserve historical event fixtures without explicit target.
    std::size_t notification_target=0;
    bool notification_new_state=false;
    std::optional<ReachQueryPayload> reach{};
    std::optional<GhostHandQueryPayload> ghost_hand{};
    std::optional<HitboxesQueryPayload> hitboxes{};
    std::optional<PiercingQueryPayload> piercing{};
    std::optional<TeamsEvent0Payload> teams_event0{};
    std::optional<AntiBotEvent0Payload> anti_bot_event0{};
    std::optional<JumpDelayTickPayload> jump_delay{};
    std::optional<NullMoveTickPayload> null_move{};
    std::optional<InstantStopTickPayload> instant_stop{};
    std::optional<NoSlowTickPayload> no_slow{};
    std::optional<NoHitDelayTickPayload> no_hit_delay{};
    std::optional<NoUseDelayTickPayload> no_use_delay{};
    std::optional<ItemUseFixTickPayload> item_use_fix{};
    std::optional<PingFixTickPayload> ping_fix{};
    std::optional<NoItemReleaseTickPayload> no_item_release{};
    std::optional<AntiDebuffTickPayload> anti_debuff{};
    std::optional<FastAccelTickPayload> fast_accel{};
    std::optional<SafeWalkTickPayload> safe_walk{};
    std::optional<FastPlaceTickPayload> fast_place{};
    std::optional<FastMineEarlyPayload> fast_mine_early{};
    std::optional<FastMineLatePayload> fast_mine_late{};
    std::optional<BridgeAssistDisablePayload> bridge_disable{};
    std::optional<BridgeAssistPlacePayload> bridge_place{};
    std::optional<BridgeAssistTickPayload> bridge_tick{};
    std::optional<BridgeAssistLatePayload> bridge_late{};
    std::optional<AutoToolInventoryPayload> auto_tool_inventory{};
    std::optional<AutoWeaponInventoryPayload> auto_weapon_inventory{};
    std::optional<WeaponsClearExtrasPayload> weapons_clear_extras{};
    std::optional<AutoToolShutdownPayload> auto_tool_shutdown{};
    std::optional<ClutchClickPayload> clutch_click{};
    std::optional<ClutchRotationPayload> clutch_rotation{};
    std::optional<FreeLookRotationPayload> free_look_rotation{};
    std::optional<ClutchLatePayload> clutch_late{};
    std::optional<BlockInEmptyPayload> block_in_empty{};
    std::optional<BlockInTogglePayload> block_in_toggle{};
    std::optional<BlockInRotationPayload> block_in_rotation{};
    std::optional<BlockInMovementPayload> block_in_movement{};
    std::optional<BlockInTickPayload> block_in_tick{};
    std::optional<TimerFramePayload> timer_frame{};
    std::optional<TimerTogglePayload> timer_toggle{};
    std::optional<KeepSprintAttackPayload> keep_sprint_attack{};
    std::optional<KeepSprintPostPayload> keep_sprint_post{};
    std::optional<KeepSprintUpdatePayload> keep_sprint_update{};
    std::optional<SprintUpdatePayload> sprint_update{};
    std::optional<SprintMovementPayload> sprint_movement{};
    std::optional<StrafeSnapshotPayload> strafe_snapshot{};
    std::optional<StrafeSteeringPayload> strafe_steering{};
    std::vector<std::size_t> executed_modules{};
};

using OfflineCallback = std::function<void(const Delivery&, ExecutionFrame&)>;

// Registration evidence and functioning implementation are counted separately.
// "Bound" means a real first-party offline callback is installed, not that a
// Minecraft hook, injected module, or native original binary was tested.
struct ExecutionCoverage {
    std::size_t registered_modules = 0;
    std::size_t modules_with_native_handlers = 0;
    std::size_t native_callback_subscriptions = 0;
    std::size_t modules_with_executable_bindings = 0;
    std::size_t executable_callback_bindings = 0;
    std::size_t unimplemented_callback_subscriptions = 0;
    std::array<std::size_t, kEventSlots> declared_per_event{};
    std::array<std::size_t, kEventSlots> implemented_per_event{};
};

// Every live action is absent by design. EventReplay holds metadata, this
// adapter owns offline callbacks, and only an explicitly admitted frame can
// invoke callbacks. No callback is implicitly inferred from a descriptor.
class ModuleExecutionBridge final {
public:
    ModuleExecutionBridge()
        : replay_([this](const Delivery& delivery, bool& veto) {
              (void)veto;
              if (active_frame_ == nullptr || delivery.event != active_frame_->event)
                  return;
              // All registrations were individually validated by bind().
              auto& callback = callbacks_[delivery.recipient][delivery.event];
              if (callback) {
                  active_frame_->executed_modules.push_back(delivery.recipient);
                  callback(delivery, *active_frame_);
              }
          }) {}

    std::size_t add(ModuleSpec module) {
        if (active_frame_) throw std::logic_error("cannot add during dispatch");
        const std::size_t id = replay_.add(module);
        definitions_.push_back(std::move(module));
        callbacks_.emplace_back();
        return id;
    }

    bool bind(std::size_t id, unsigned event, OfflineCallback callback) {
        if (active_frame_ || stopped_ || !callback ||
            id >= definitions_.size() || event >= kEventSlots)
            return false;
        bool subscribed = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == event) subscribed = true;
        if (!subscribed || callbacks_[id][event]) return false;
        callbacks_[id][event] = std::move(callback);
        ++bound_count_;
        return true;
    }

    // Unlike a synthetic reimplementation, this invokes the existing R-series
    // ReachController in ../src/core.cpp (linked by this project's CMake).
    bool bind_reach(std::size_t id) {
        if (id >= definitions_.size() || definitions_[id].name != "Reach" ||
            definitions_[id].category != 0)
            return false;
        return bind(id, kReachQueryEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.reach) return;
                        auto& reach = *frame.reach;
                        ++reach.executions;
                        reach.proposed_distance = controller_.evaluate(
                            reach.input, reach.settings);
                    });
    }

    // R256: reconstructed Ghost Hand event8 query override. Native
    // callback priority is unverified; descriptor order is a fixture.
    bool bind_ghost_hand(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Ghost Hand" ||
           definitions_[id].category!=0 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kReachQueryEvent ||
           callbacks_[id][kReachQueryEvent])return false;
        return bind(id,kReachQueryEvent,
            [](const Delivery&,ExecutionFrame& frame){
                if(!frame.ghost_hand)return;
                auto& q=*frame.ghost_hand;
                ++q.executions;
                q.query_override=GhostHandController::on_event8(true);
                q.decision=GhostHandController::can_interact_through(
                    q.settings,q.input);
            });
    }
    bool bind_ghost_hand_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Ghost Hand")
                return bind_ghost_hand(id);
        return false;
    }

    // R265: the reconstructed Hitboxes combat module listens to event8.
    // Original native callback priority has not been recovered; the
    // subscription's supplied priority controls offline fixture ordering.
    bool bind_hitboxes(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Hitboxes" ||
           definitions_[id].category!=0 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kReachQueryEvent ||
           callbacks_[id][kReachQueryEvent])return false;
        return bind(id,kReachQueryEvent,
            [](const Delivery&,ExecutionFrame& frame){
                if(!frame.hitboxes)return;
                auto& p=*frame.hitboxes;
                ++p.executions;
                // EventReplay only delivers to enabled handlers. Never
                // trust a synthetic "disabled" flag in the payload.
                HitboxesObservation source=p.input;
                source.dispatched_to_enabled_module=true;
                p.decision=HitboxesController::on_event8(source);
            });
    }
    bool bind_hitboxes_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Hitboxes")
                return bind_hitboxes(id);
        return false;
    }

    // R273: Piercing Java onEvent8 only sets queryOverride; actual
    // ray-candidate ranking is a SEPARATE source call and is never
    // performed implicitly by this event subscription.
    bool bind_piercing(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Piercing" ||
           definitions_[id].category!=0 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kReachQueryEvent ||
           callbacks_[id][kReachQueryEvent])return false;
        return bind(id,kReachQueryEvent,
            [](const Delivery&,ExecutionFrame& frame){
                if(!frame.piercing)return;
                auto& p=*frame.piercing;
                ++p.executions;
                p.query_override=PiercingController::on_event8(true);
            });
    }
    bool bind_piercing_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Piercing")
                return bind_piercing(id);
        return false;
    }
    PiercingDecision preview_piercing_rank(
        const PiercingSettings& settings,bool is_weapon,bool inside,
        const PiercingEntity& first,const PiercingEntity& second) const noexcept {
        if(stopped_ || active_frame_ || !piercing_bound_enabled())return {};
        return PiercingController::rank(settings,is_weapon,inside,first,second);
    }
    bool preview_pierce_blocks(const PiercingSettings& settings,
                               bool is_weapon) const noexcept {
        return !stopped_ && !active_frame_ && piercing_bound_enabled() &&
               PiercingController::pierce_blocks(settings,is_weapon);
    }

    // R267: Teams uses event0/category UTILITY in the recovered Java.
    // Native callback priority is unknown: preserve caller descriptor
    // as a test fixture, never assert it is commercially authenticated.
    bool bind_teams(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Teams" ||
           definitions_[id].category!=4 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kSafeWalkEvent ||
           callbacks_[id][kSafeWalkEvent])return false;
        return bind(id,kSafeWalkEvent,
            [](const Delivery&,ExecutionFrame& frame){
                if(!frame.teams_event0)return;
                auto& p=*frame.teams_event0;
                ++p.executions;
                if(!p.world_ready || p.other_players.size()>256u)return;
                p.decisions.reserve(p.other_players.size());
                for(const auto& player:p.other_players)
                    p.decisions.push_back(TeamsController::on_event0(
                        true,p.owner,player,p.settings));
                // No RelationshipService.set(1) or player memory writes.
            });
    }
    bool bind_teams_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Teams")
                return bind_teams(id);
        return false;
    }

    // R271: Anti Bot's reconstructed Java module is UTILITY/event0.
    // Never infer its original native callback priority or write flags.
    bool bind_anti_bot(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Anti Bot" ||
           definitions_[id].category!=4 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kSafeWalkEvent ||
           callbacks_[id][kSafeWalkEvent])return false;
        return bind(id,kSafeWalkEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.anti_bot_event0)return;
                auto& p=*frame.anti_bot_event0;
                ++p.executions;
                p.decision=anti_bot_.on_event0(p.world_token,
                    p.context_ready,p.local_player,p.settings,
                    p.other_players);
            });
    }
    bool bind_anti_bot_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Anti Bot")
                return bind_anti_bot(id);
        return false;
    }

    // Second independent offline decision implementation. Native candidate:
    // "No Jump Delay" is a movement module subscribed to event 17.
    bool bind_no_jump_delay(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "No Jump Delay" ||
            definitions_[id].category != 1)
            return false;
        return bind(id, kPlayerUpdateEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.jump_delay) return;
                        auto& tick = *frame.jump_delay;
                        ++tick.executions;
                        const JumpDelayDecision decision =
                            no_jump_delay_.evaluate(tick.input);
                        if (decision.override_jump_ticks)
                            tick.proposed_jump_ticks = decision.proposed_jump_ticks;
                    });
    }

    bool bind_no_jump_delay_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "No Jump Delay")
                return bind_no_jump_delay(id);
        return false;
    }

    // Instant Stop is the higher-priority event-16 module (200 vs
    // Null Move's 100). Its proposed key state is made visible to the
    // lower-priority offline Null Move policy in the same frame.
    bool bind_instant_stop(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "Instant Stop" ||
            definitions_[id].category != 1)
            return false;
        bool has_original_priority = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kMovementTickEvent && sub.priority == 200)
                has_original_priority = true;
        if (!has_original_priority) return false;
        return bind(id, kMovementTickEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.instant_stop) return;
                        auto& tick = *frame.instant_stop;
                        ++tick.executions;
                        tick.decision = instant_stop_.evaluate(tick.input);
                        if (frame.null_move) {
                            for (auto key : {MoveKey::back, MoveKey::left, MoveKey::right}) {
                                const unsigned index = static_cast<unsigned>(key);
                                if (tick.decision.overrides[index])
                                    frame.null_move->input.game_pressed[index] =
                                        *tick.decision.overrides[index];
                            }
                        }
                    });
    }

    bool bind_instant_stop_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Instant Stop")
                return bind_instant_stop(id);
        return false;
    }

    // Offline Null Move runs on original candidate movement event 16.
    bool bind_null_move(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "Null Move" ||
            definitions_[id].category != 1)
            return false;
        return bind(id, kMovementTickEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.null_move) return;
                        auto& tick = *frame.null_move;
                        ++tick.executions;
                        tick.decision = null_move_.evaluate(tick.input);
                    });
    }

    bool bind_null_move_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Null Move")
                return bind_null_move(id);
        return false;
    }

    // Candidate No Slow: category move, event slot 21, priority 200.
    bool bind_no_slow(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "No Slow" ||
            definitions_[id].category != 1)
            return false;
        bool original_priority = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kNoSlowEvent && sub.priority == 200)
                original_priority = true;
        if (!original_priority) return false;
        return bind(id, kNoSlowEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.no_slow) return;
                        auto& payload = *frame.no_slow;
                        ++payload.executions;
                        payload.decision = no_slow_.evaluate(payload.input);
                    });
    }

    bool bind_no_slow_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "No Slow")
                return bind_no_slow(id);
        return false;
    }

    // Original reference: utility "No Hit Delay", event 14, priority 0.
    // The callback proposes a zero left-click counter or arm swing only;
    // no Minecraft field write or input action is possible here.
    bool bind_no_hit_delay(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "No Hit Delay" ||
            definitions_[id].category != 4)
            return false;
        bool correct_subscription = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kNoHitDelayEvent && sub.priority == 0)
                correct_subscription = true;
        if (!correct_subscription) return false;
        return bind(id, kNoHitDelayEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.no_hit_delay) return;
                        auto& payload = *frame.no_hit_delay;
                        ++payload.executions;
                        payload.decision = no_hit_delay_.evaluate(payload.input);
                    });
    }

    bool bind_no_hit_delay_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "No Hit Delay")
                return bind_no_hit_delay(id);
        return false;
    }

    // Original candidate utility callback: module "No Use Delay" is
    // registered on event 18 at priority 0. Only typed, offline proposals.
    bool bind_no_use_delay(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "No Use Delay" ||
            definitions_[id].category != 4)
            return false;
        bool candidate_slot = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kNoUseDelayEvent && sub.priority == 0)
                candidate_slot = true;
        if (!candidate_slot) return false;
        return bind(id, kNoUseDelayEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.no_use_delay) return;
                        auto& use = *frame.no_use_delay;
                        ++use.executions;
                        use.decision = no_use_delay_.evaluate(use.input);
                    });
    }

    bool bind_no_use_delay_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "No Use Delay")
                return bind_no_use_delay(id);
        return false;
    }

    // Item Use Fix has TWO original callbacks on event 14 (before)
    // and event 15 (after), both priority zero. Registration is atomic:
    // never count a half-implemented module as a completed pair.
    bool bind_item_use_fix(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Item Use Fix" ||
            definitions_[id].category != 4)
            return false;
        bool before = false, after = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kItemUseFixBeforeEvent && sub.priority == 0)
                before = true;
            if (sub.event == kItemUseFixAfterEvent && sub.priority == 0)
                after = true;
        }
        if (!before || !after ||
            callbacks_[id][kItemUseFixBeforeEvent] ||
            callbacks_[id][kItemUseFixAfterEvent])
            return false;
        const bool first = bind(id, kItemUseFixBeforeEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.item_use_fix) return;
                auto& payload = *frame.item_use_fix;
                ++payload.executions;
                payload.decision = item_use_fix_.before(payload.input);
            });
        const bool second = bind(id, kItemUseFixAfterEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.item_use_fix) return;
                auto& payload = *frame.item_use_fix;
                ++payload.executions;
                payload.decision = item_use_fix_.after(payload.input);
            });
        if (!first || !second)
            throw std::logic_error("validated Item Use Fix pair failed");
        return true;
    }

    bool bind_item_use_fix_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Item Use Fix")
                return bind_item_use_fix(id);
        return false;
    }

    // Original candidate Ping Fix: receive slot 2 and send slot 3,
    // both signed priority -1000. Validate BOTH before binding either.
    bool bind_ping_fix(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Ping Fix" ||
            definitions_[id].category != 4)
            return false;
        bool inbound = false, outbound = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kIncomingPacketEvent && sub.priority == -1000)
                inbound = true;
            if (sub.event == kOutgoingPacketEvent && sub.priority == -1000)
                outbound = true;
        }
        if (!inbound || !outbound ||
            callbacks_[id][kIncomingPacketEvent] ||
            callbacks_[id][kOutgoingPacketEvent])
            return false;
        const bool first = bind(id, kIncomingPacketEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.ping_fix) return;
                auto& packet = *frame.ping_fix;
                ++packet.executions;
                packet.decision = ping_fix_.on_receive(packet.input);
            });
        const bool second = bind(id, kOutgoingPacketEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.ping_fix) return;
                auto& packet = *frame.ping_fix;
                ++packet.executions;
                packet.decision = ping_fix_.on_send(packet.input);
            });
        if (!first || !second)
            throw std::logic_error("validated Ping Fix event pair failed");
        return true;
    }

    bool bind_ping_fix_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Ping Fix")
                return bind_ping_fix(id);
        return false;
    }

    // Original candidate module 44: No Item Release, event 3 priority 0.
    // State advances on simulated C08/C07 outgoing packets. No actual I/O.
    bool bind_no_item_release(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "No Item Release" ||
            definitions_[id].category != 4)
            return false;
        bool correct = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kNoItemReleaseEvent && sub.priority == 0)
                correct = true;
        if (!correct) return false;
        return bind(id, kNoItemReleaseEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.no_item_release) return;
                        auto& packet = *frame.no_item_release;
                        ++packet.executions;
                        packet.decision = no_item_release_.evaluate(packet.input);
                    });
    }

    bool bind_no_item_release_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "No Item Release")
                return bind_no_item_release(id);
        return false;
    }

    // Candidate original: Anti Debuff descriptor 2, visual category 2,
    // event 18 with priority 0. Pure offline screen-state proposals only.
    bool bind_anti_debuff(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "Anti Debuff" ||
            definitions_[id].category != 2)
            return false;
        bool correct = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kAntiDebuffEvent && sub.priority == 0)
                correct = true;
        if (!correct) return false;
        return bind(id, kAntiDebuffEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.anti_debuff) return;
                        auto& payload = *frame.anti_debuff;
                        ++payload.executions;
                        payload.decision = anti_debuff_.evaluate(payload.input);
                    });
    }

    bool bind_anti_debuff_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Anti Debuff")
                return bind_anti_debuff(id);
        return false;
    }

    // Original candidate Fast Accel: movement category 1, event 22,
    // priority 0. All motion changes are proposed offline only.
    bool bind_fast_accel(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "Fast Accel" ||
            definitions_[id].category != 1)
            return false;
        bool original_event = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kFastAccelEvent && sub.priority == 0)
                original_event = true;
        if (!original_event) return false;
        return bind(id, kFastAccelEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.fast_accel) return;
                        auto& payload = *frame.fast_accel;
                        ++payload.executions;
                        payload.decision = fast_accel_.evaluate(payload.input);
                    });
    }

    bool bind_fast_accel_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Fast Accel")
                return bind_fast_accel(id);
        return false;
    }

    // Candidate original Safe Walk module 56, movement category 1,
    // event 0 at priority 0. Returns only simulated edge-guard and
    // horizontal motion proposals; edge collision belongs to the engine.
    bool bind_safe_walk(std::size_t id) {
        if (id >= definitions_.size() ||
            definitions_[id].name != "Safe Walk" ||
            definitions_[id].category != 1)
            return false;
        bool correct = false;
        for (const auto& sub : definitions_[id].subscriptions)
            if (sub.event == kSafeWalkEvent && sub.priority == 0)
                correct = true;
        if (!correct) return false;
        return bind(id, kSafeWalkEvent,
                    [this](const Delivery&, ExecutionFrame& frame) {
                        if (!frame.safe_walk) return;
                        auto& payload = *frame.safe_walk;
                        ++payload.executions;
                        payload.decision = safe_walk_.evaluate(payload.input);
                    });
    }

    bool bind_safe_walk_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Safe Walk")
                return bind_safe_walk(id);
        return false;
    }

    // R225: source-backed Fast Place, block category 3, event 0 priority
    // -1000. Exact single callback binding; never posts actual key presses.
    bool bind_fast_place(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Fast Place" ||
            definitions_[id].category != 3 ||
            definitions_[id].subscriptions.size() != 1u ||
            definitions_[id].subscriptions[0].event != kSafeWalkEvent ||
            definitions_[id].subscriptions[0].priority != -1000)
            return false;
        return bind(id,kSafeWalkEvent,
                    [this](const Delivery&,ExecutionFrame& frame) {
                        if (!frame.fast_place) return;
                        auto& payload = *frame.fast_place;
                        ++payload.executions;
                        payload.decision = fast_place_.evaluate(payload.input);
                    });
    }

    bool bind_fast_place_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Fast Place")
                return bind_fast_place(id);
        return false;
    }

    // R226: original Fast Mine block category 3, two callbacks at
    // descriptor +0x48 (event0, priority0) and +0x168 (event18,
    // priority0). The controller preserves early->late progress history.
    bool bind_fast_mine(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Fast Mine" ||
           definitions_[id].category!=3 ||
           definitions_[id].subscriptions.size()!=2u ||
           callbacks_[id][kSafeWalkEvent] ||
           callbacks_[id][kAntiDebuffEvent]) return false;
        bool early=false,late=false;
        for(const auto& sub:definitions_[id].subscriptions) {
            if(sub.event==kSafeWalkEvent && sub.priority==0)early=true;
            if(sub.event==kAntiDebuffEvent && sub.priority==0)late=true;
        }
        if(!early || !late)return false;
        const bool first=bind(id,kSafeWalkEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.fast_mine_early)return;
                auto& p=*frame.fast_mine_early;
                ++p.executions;
                p.decision=fast_mine_.on_early(p.input);
            });
        const bool second=bind(id,kAntiDebuffEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.fast_mine_late)return;
                auto& p=*frame.fast_mine_late;
                ++p.executions;
                p.decision=fast_mine_.on_late(p.input);
            });
        if(!first || !second)
            throw std::logic_error("validated Fast Mine callback pair failed");
        return true;
    }

    bool bind_fast_mine_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Fast Mine")
                return bind_fast_mine(id);
        return false;
    }

    // R227: original Block category3 Bridge Assist descriptor: event5
    // disable, event14 place, event17 tick, event18 late. Four exact
    // source subscriptions, zero priority, with owner-only value output.
    bool bind_bridge_assist(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Bridge Assist" ||
           definitions_[id].category!=3 ||
           definitions_[id].subscriptions.size()!=4u)return false;
        bool disable=false,place=false,tick=false,late=false;
        for(const auto& sub:definitions_[id].subscriptions) {
            if(sub.priority!=0)return false;
            if(sub.event==kTimerToggleEvent)disable=true;
            else if(sub.event==kKeepSprintAttackEvent)place=true;
            else if(sub.event==kPlayerUpdateEvent)tick=true;
            else if(sub.event==kAntiDebuffEvent)late=true;
            else return false;
        }
        if(!disable || !place || !tick || !late ||
           callbacks_[id][kTimerToggleEvent] ||
           callbacks_[id][kKeepSprintAttackEvent] ||
           callbacks_[id][kPlayerUpdateEvent] ||
           callbacks_[id][kAntiDebuffEvent])return false;
        const bool a=bind(id,kTimerToggleEvent,
            [this,id](const Delivery& d,ExecutionFrame& frame){
                if(!frame.bridge_disable)return;
                auto& p=*frame.bridge_disable;
                ++p.executions;
                auto input=p.input;
                input.own_module=d.target==id;
                input.enabled=d.new_state;
                p.decision=bridge_assist_.on_disable(input);
            });
        const bool b=bind(id,kKeepSprintAttackEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.bridge_place)return;
                auto& p=*frame.bridge_place;
                ++p.executions;
                p.decision=bridge_assist_.on_place(p.input);
            });
        const bool c=bind(id,kPlayerUpdateEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.bridge_tick)return;
                auto& p=*frame.bridge_tick;
                ++p.executions;
                if(shared_source_rng_enabled_ && p.input.use_source_walk &&
                   !bridge_assist_.adopt_source_rng(shared_source_rng_)) {
                    p.decision={};
                    return;
                }
                p.decision=bridge_assist_.on_tick(p.input);
                if(shared_source_rng_enabled_ && p.input.use_source_walk &&
                   p.decision.admitted)
                    shared_source_rng_=bridge_assist_.source_rng_snapshot();
            });
        const bool e=bind(id,kAntiDebuffEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.bridge_late)return;
                auto& p=*frame.bridge_late;
                ++p.executions;
                p.decision=bridge_assist_.on_late();
            });
        if(!a||!b||!c||!e)
            throw std::logic_error("validated Bridge Assist subscriptions failed");
        return true;
    }
    bool bind_bridge_assist_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Bridge Assist")
                return bind_bridge_assist(id);
        return false;
    }

    // R228: original Block category3 Auto Tool slots +0x1d8/+0x1e8,
    // candidate events 25 and 26, each priority0. A two-event
    // first-party C++ state machine; no actual hotbar/game writes.
    bool bind_auto_tool(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Auto Tool" ||
           definitions_[id].category!=3 ||
           definitions_[id].subscriptions.size()!=2u ||
           callbacks_[id][kAutoToolInventoryEvent] ||
           callbacks_[id][kAutoToolShutdownEvent])return false;
        bool inventory=false,shutdown=false;
        for(const auto& sub:definitions_[id].subscriptions) {
            if(sub.priority!=0)return false;
            if(sub.event==kAutoToolInventoryEvent)inventory=true;
            else if(sub.event==kAutoToolShutdownEvent)shutdown=true;
            else return false;
        }
        if(!inventory || !shutdown)return false;
        const bool first=bind(id,kAutoToolInventoryEvent,
            [this](const Delivery&,ExecutionFrame& frame) {
                if(!frame.auto_tool_inventory)return;
                auto& p=*frame.auto_tool_inventory;
                ++p.executions;
                p.decision=auto_tool_.on_tick(p.input);
            });
        const bool second=bind(id,kAutoToolShutdownEvent,
            [this](const Delivery&,ExecutionFrame& frame) {
                if(!frame.auto_tool_shutdown)return;
                auto& p=*frame.auto_tool_shutdown;
                ++p.executions;
                p.decision=auto_tool_.on_shutdown();
            });
        if(!first || !second)
            throw std::logic_error("validated Auto Tool callback pair failed");
        return true;
    }
    bool bind_auto_tool_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Auto Tool")
                return bind_auto_tool(id);
        return false;
    }

    // R261: Auto Weapon Java source listens to event25/category COMBAT.
    // Priority is not established: fixture ordering follows metadata.
    // Source firstAllowedSlot and isWeapon are calculated here from the
    // actual value-only Weapons inventory, never an untrusted hint.
    bool bind_auto_weapon(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Auto Weapon" ||
           definitions_[id].category!=0 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kAutoToolInventoryEvent ||
           callbacks_[id][kAutoToolInventoryEvent])return false;
        return bind(id,kAutoToolInventoryEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.auto_weapon_inventory)return;
                auto& p=*frame.auto_weapon_inventory;
                ++p.executions;
                AutoWeaponInput verified=p.input;
                verified.current_is_weapon=false; // never trust caller's hint
                verified.first_allowed_slot=-1;   // never trust caller's slot
                // Java AutoWeaponModule tests the player/target gates
                // BEFORE calling WeaponsModule.isWeapon. Cache must not
                // advance on a missed or friendly target.
                const bool could_query=verified.input_ready &&
                    !verified.is_using_item &&
                    (!verified.require_mouse_pressed || verified.mouse_pressed) &&
                    verified.hit_is_entity && verified.remote_other_player &&
                    verified.hostile;
                if(could_query) {
                    if(verified.current_slot<0 || verified.current_slot>=9) {
                        auto_weapon_.reset();
                        return;
                    }
                    const WeaponsTickCache before=weapons_tick_cache_;
                    WeaponsTickObservation observed;
                    observed.world_token=verified.world_token;
                    observed.player_token=verified.player_token;
                    observed.player_ticks_existed=p.player_ticks_existed;
                    observed.current_slot=verified.current_slot;
                    observed.inventory=p.inventory;
                    observed.settings=p.settings;
                    p.source_cache_queried=true;
                    p.source_cache=weapons_tick_cache_.on_query(observed);
                    if(!p.source_cache.admitted) {
                        auto_weapon_.reset();
                        return;
                    }
                    verified.current_is_weapon=p.source_cache.cached_allowed;
                    p.source_current_is_weapon=verified.current_is_weapon;
                    // The source only reads firstAllowedSlot after
                    // isWeapon returned false.
                    if(!verified.current_is_weapon) {
                        p.source_choice=WeaponsController::first_allowed_slot(
                            p.inventory,p.settings);
                        if(!p.source_choice.admitted) {
                            weapons_tick_cache_=before;
                            auto_weapon_.reset();
                            return;
                        }
                        verified.first_allowed_slot=
                            p.source_choice.first_allowed_slot;
                    }
                }
                p.decision=auto_weapon_.on_event25(verified);
            });
    }
    bool bind_auto_weapon_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Auto Weapon")
                return bind_auto_weapon(id);
        return false;
    }
    // Weapons source listens to event26 and clears its extra-item names.
    // Here it emits an explicit value-only request; no settings are changed.
    bool bind_weapons(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Weapons" ||
           definitions_[id].category!=0 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kAutoToolShutdownEvent ||
           callbacks_[id][kAutoToolShutdownEvent])return false;
        return bind(id,kAutoToolShutdownEvent,
            [](const Delivery&,ExecutionFrame& frame){
                if(!frame.weapons_clear_extras)return;
                auto& p=*frame.weapons_clear_extras;
                ++p.executions;
                p.proposed_clear_extra_items=
                    WeaponsController::event26_requests_clear_extras(true);
            });
    }
    bool bind_weapons_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Weapons")
                return bind_weapons(id);
        return false;
    }

    // R229: pinned Clutch Block-category descriptor has native event 0
    // click priority0; event13 rotation-flag priority-100; event18 late
    // phase priority-1000. No original client input or rotation writes.
    bool bind_clutch(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Clutch" ||
           definitions_[id].category!=3 ||
           definitions_[id].subscriptions.size()!=3u ||
           callbacks_[id][kSafeWalkEvent] ||
           callbacks_[id][kClutchRotationFlagEvent] ||
           callbacks_[id][kAntiDebuffEvent])return false;
        bool click=false,rotation=false,late=false;
        for(const auto& sub:definitions_[id].subscriptions) {
            if(sub.event==kSafeWalkEvent && sub.priority==0)click=true;
            else if(sub.event==kClutchRotationFlagEvent &&
                    sub.priority==-100)rotation=true;
            else if(sub.event==kAntiDebuffEvent &&
                    sub.priority==-1000)late=true;
            else return false;
        }
        if(!click||!rotation||!late)return false;
        const bool first=bind(id,kSafeWalkEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.clutch_click)return;
                auto& p=*frame.clutch_click;
                ++p.executions;
                p.decision=clutch_.on_tick(p.input);
            });
        const bool second=bind(id,kClutchRotationFlagEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.clutch_rotation)return;
                auto& p=*frame.clutch_rotation;
                ++p.executions;
                p.decision=clutch_.on_rotation_flag();
            });
        const bool third=bind(id,kAntiDebuffEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.clutch_late)return;
                auto& p=*frame.clutch_late;
                ++p.executions;
                if(shared_source_rng_enabled_ && p.input.use_source_walk &&
                   !clutch_.adopt_source_rng(shared_source_rng_)) {
                    p.decision={};
                    return;
                }
                p.decision=clutch_.on_late(p.input);
                if(shared_source_rng_enabled_ && p.input.use_source_walk &&
                   p.decision.admitted)
                    shared_source_rng_=clutch_.source_rng_snapshot();
            });
        if(!first||!second||!third)
            throw std::logic_error("validated Clutch callback triple failed");
        return true;
    }
    bool bind_clutch_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Clutch")
                return bind_clutch(id);
        return false;
    }

    // R254: reconstructed Java Free Look listens on event13; native
    // priority is unknown, so preserve caller descriptor's priority.
    bool bind_free_look(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Free Look" ||
           definitions_[id].category!=2 ||
           definitions_[id].subscriptions.size()!=1u ||
           definitions_[id].subscriptions[0].event!=kClutchRotationFlagEvent ||
           callbacks_[id][kClutchRotationFlagEvent])return false;
        return bind(id,kClutchRotationFlagEvent,
            [this](const Delivery&,ExecutionFrame& frame){
                if(!frame.free_look_rotation)return;
                auto& p=*frame.free_look_rotation;
                ++p.executions;
                p.decision=free_look_.on_event13(p.input);
            });
    }
    bool bind_free_look_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Free Look")
                return bind_free_look(id);
        return false;
    }
    // Never write a live camera; these explicit lifecycle receipts are
    // value-only and require an enabled, correctly bound module.
    FreeLookProposal preview_free_look_enable(const FreeLookFrame& in) noexcept {
        if(stopped_ || active_frame_ || !free_look_bound_enabled())return {};
        return free_look_.on_enable(in);
    }
    FreeLookProposal preview_free_look_disable(std::uint64_t world,
                                                std::uint64_t player) noexcept {
        if(stopped_ || active_frame_ || !free_look_bound_enabled())return {};
        return free_look_.on_disable(world,player);
    }
    FreeLookProposal preview_free_look_world_unload(std::uint64_t world,
                                                     std::uint64_t player) noexcept {
        if(stopped_ || active_frame_ || !free_look_bound_enabled())return {};
        return free_look_.on_world_unload(world,player);
    }

    // R230: the pinned source Block In descriptor has exactly five hooks:
    // event 0 empty (priority0); event5 toggle (0); event13 rotation
    // priority -2000; event15 keys priority -1000; event16 tick
    // priority -500. All proposals are native-only, no gameplay writes.
    bool bind_block_in(std::size_t id) {
        if(stopped_ || active_frame_ || id>=definitions_.size() ||
           definitions_[id].name!="Block In" ||
           definitions_[id].category!=3 ||
           definitions_[id].subscriptions.size()!=5u)return false;
        bool empty=false,toggle=false,rotation=false,movement=false,tick=false;
        for(const auto& sub:definitions_[id].subscriptions) {
            if(sub.event==kSafeWalkEvent && sub.priority==0)empty=true;
            else if(sub.event==kTimerToggleEvent && sub.priority==0)toggle=true;
            else if(sub.event==kClutchRotationFlagEvent &&
                    sub.priority==-2000)rotation=true;
            else if(sub.event==kKeepSprintPostEvent &&
                    sub.priority==-1000)movement=true;
            else if(sub.event==kMovementTickEvent &&
                    sub.priority==-500)tick=true;
            else return false;
        }
        if(!empty||!toggle||!rotation||!movement||!tick ||
           callbacks_[id][kSafeWalkEvent] ||
           callbacks_[id][kTimerToggleEvent] ||
           callbacks_[id][kClutchRotationFlagEvent] ||
           callbacks_[id][kKeepSprintPostEvent] ||
           callbacks_[id][kMovementTickEvent])return false;
        const bool a=bind(id,kSafeWalkEvent,
            [this](const Delivery&,ExecutionFrame& f){
                if(f.block_in_empty && block_in_.on_empty_tick())
                    ++f.block_in_empty->executions;
            });
        const bool b=bind(id,kTimerToggleEvent,
            [this,id](const Delivery& delivery,ExecutionFrame& f){
                if(!f.block_in_toggle)return;
                auto& p=*f.block_in_toggle;
                ++p.executions;
                auto input=p.input;
                input.own_module=delivery.target==id;
                input.enabled=delivery.new_state;
                p.decision=block_in_.on_toggle(input);
            });
        const bool c=bind(id,kClutchRotationFlagEvent,
            [this](const Delivery&,ExecutionFrame& f){
                if(!f.block_in_rotation)return;
                auto& p=*f.block_in_rotation;
                ++p.executions;
                p.decision=block_in_.on_rotation();
            });
        const bool e=bind(id,kKeepSprintPostEvent,
            [this](const Delivery&,ExecutionFrame& f){
                if(!f.block_in_movement)return;
                auto& p=*f.block_in_movement;
                ++p.executions;
                p.decision=block_in_.on_movement();
            });
        const bool g=bind(id,kMovementTickEvent,
            [this](const Delivery&,ExecutionFrame& f){
                if(!f.block_in_tick)return;
                auto& p=*f.block_in_tick;
                ++p.executions;
                const bool share=shared_source_rng_enabled_ &&
                    p.input.source_aim &&
                    p.input.source_aim->use_reconstructed_walk;
                if(share && !block_in_.adopt_source_rng(shared_source_rng_)) {
                    p.decision={};
                    return;
                }
                p.decision=block_in_.on_tick(p.input);
                if(share && p.decision.admitted)
                    shared_source_rng_=block_in_.source_rng_snapshot();
            });
        if(!a||!b||!c||!e||!g)
            throw std::logic_error("Block In source callback five-tuple rejected");
        return true;
    }
    bool bind_block_in_from_catalog() {
        for(std::size_t id=0;id<definitions_.size();++id)
            if(definitions_[id].name=="Block In")
                return bind_block_in(id);
        return false;
    }

    // Source-linked Timer: utility category 4, frame event 1 priority 200
    // and toggle notification event 5 priority 0. Both must be present before
    // accepting the offline binding; no actual Minecraft timer is modified.
    bool bind_timer(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Timer" ||
            definitions_[id].category != 4) return false;
        bool frame = false, toggle = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kTimerFrameEvent && sub.priority == 200)
                frame = true;
            if (sub.event == kTimerToggleEvent && sub.priority == 0)
                toggle = true;
        }
        if (!frame || !toggle ||
            callbacks_[id][kTimerFrameEvent] ||
            callbacks_[id][kTimerToggleEvent]) return false;

        const bool first = bind(id, kTimerFrameEvent,
            [this](const Delivery&, ExecutionFrame& tick) {
                if (!tick.timer_frame) return;
                auto& payload = *tick.timer_frame;
                ++payload.executions;
                payload.decision = timer_.on_frame(payload.input);
            });
        const bool second = bind(id, kTimerToggleEvent,
            [this](const Delivery&, ExecutionFrame& tick) {
                if (!tick.timer_toggle) return;
                auto& payload = *tick.timer_toggle;
                ++payload.executions;
                payload.decision = timer_.on_toggle(payload.input);
            });
        if (!first || !second)
            throw std::logic_error("validated Timer event pair failed");
        return true;
    }

    bool bind_timer_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Timer")
                return bind_timer(id);
        return false;
    }

    // R202: source-grounded Keep Sprint movement category 1,
    // native event slots 14/15/17 all with zero signed priority.
    // Register the three-phase offline state machine atomically.
    bool bind_keep_sprint(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Keep Sprint" ||
            definitions_[id].category != 1) return false;
        bool pre = false, post = false, update = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kKeepSprintAttackEvent && sub.priority == 0) pre = true;
            if (sub.event == kKeepSprintPostEvent && sub.priority == 0) post = true;
            if (sub.event == kKeepSprintUpdateEvent && sub.priority == 0) update = true;
        }
        if (!pre || !post || !update ||
            callbacks_[id][kKeepSprintAttackEvent] ||
            callbacks_[id][kKeepSprintPostEvent] ||
            callbacks_[id][kKeepSprintUpdateEvent]) return false;
        const bool first = bind(id, kKeepSprintAttackEvent,
            [this](const Delivery&, ExecutionFrame& f) {
                if (!f.keep_sprint_attack) return;
                auto& payload = *f.keep_sprint_attack;
                ++payload.executions;
                payload.decision = keep_sprint_.on_attack(payload.input);
            });
        const bool second = bind(id, kKeepSprintPostEvent,
            [this](const Delivery&, ExecutionFrame& f) {
                if (!f.keep_sprint_post) return;
                auto& payload = *f.keep_sprint_post;
                ++payload.executions;
                payload.decision = keep_sprint_.on_attack_post(payload.input);
            });
        const bool third = bind(id, kKeepSprintUpdateEvent,
            [this](const Delivery&, ExecutionFrame& f) {
                if (!f.keep_sprint_update) return;
                auto& payload = *f.keep_sprint_update;
                ++payload.executions;
                payload.decision = keep_sprint_.on_player_update(payload.input);
            });
        if (!first || !second || !third)
            throw std::logic_error("validated Keep Sprint event triple failed");
        return true;
    }

    bool bind_keep_sprint_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Keep Sprint")
                return bind_keep_sprint(id);
        return false;
    }

    // R203 pinned native Sprint: movement category 1, player update 17/100
    // and movement input 21/100. Validate the *pair* before binding either.
    bool bind_sprint(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Sprint" ||
            definitions_[id].category != 1 ||
            definitions_[id].subscriptions.size() != 2)
            return false;
        bool update = false, movement = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kSprintPlayerUpdateEvent && sub.priority == 100)
                update = true;
            if (sub.event == kSprintMovementInputEvent && sub.priority == 100)
                movement = true;
        }
        if (!update || !movement ||
            callbacks_[id][kSprintPlayerUpdateEvent] ||
            callbacks_[id][kSprintMovementInputEvent])
            return false;
        const bool first = bind(id, kSprintPlayerUpdateEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.sprint_update) return;
                auto& p = *frame.sprint_update;
                ++p.executions;
                p.decision = sprint_.on_player_update(p.input);
            });
        const bool second = bind(id, kSprintMovementInputEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.sprint_movement) return;
                auto& p = *frame.sprint_movement;
                ++p.executions;
                p.decision = sprint_.on_movement_input(p.input);
            });
        if (!first || !second)
            throw std::logic_error("validated Sprint callback pair failed");
        return true;
    }

    bool bind_sprint_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Sprint")
                return bind_sprint(id);
        return false;
    }

    // R222 pinned candidate row 59: Strafe is movement category 1, with
    // event21/1000 snapshot and event22/100 steering. Validate the complete
    // native registration before installing either independent callback.
    bool bind_strafe(std::size_t id) {
        if (stopped_ || active_frame_ || id >= definitions_.size() ||
            definitions_[id].name != "Strafe" ||
            definitions_[id].category != 1 ||
            definitions_[id].subscriptions.size() != 2)
            return false;
        bool snapshot = false, steering = false;
        for (const auto& sub : definitions_[id].subscriptions) {
            if (sub.event == kStrafeSnapshotEvent && sub.priority == 1000)
                snapshot = true;
            if (sub.event == kStrafeSteeringEvent && sub.priority == 100)
                steering = true;
        }
        if (!snapshot || !steering ||
            callbacks_[id][kStrafeSnapshotEvent] ||
            callbacks_[id][kStrafeSteeringEvent])
            return false;

        const bool first = bind(id, kStrafeSnapshotEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.strafe_snapshot) return;
                auto& p = *frame.strafe_snapshot;
                ++p.executions;
                strafe_.observe_sprinting(p.sprinting);
            });
        const bool second = bind(id, kStrafeSteeringEvent,
            [this](const Delivery&, ExecutionFrame& frame) {
                if (!frame.strafe_steering) return;
                auto& p = *frame.strafe_steering;
                ++p.executions;
                p.decision = strafe_.evaluate(p.input);
            });
        if (!first || !second)
            throw std::logic_error("validated Strafe callback pair failed");
        return true;
    }

    bool bind_strafe_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Strafe")
                return bind_strafe(id);
        return false;
    }

    // A full 66-module manifest is registered by add(); Reach is located by
    // validated name/category/slot, never assumed to occupy a fixed ID.
    bool bind_reach_from_catalog() {
        for (std::size_t id = 0; id < definitions_.size(); ++id)
            if (definitions_[id].name == "Reach")
                return bind_reach(id);
        return false;
    }

    ExecutionCoverage coverage() const noexcept {
        ExecutionCoverage result;
        result.registered_modules = definitions_.size();
        for (std::size_t id = 0; id < definitions_.size(); ++id) {
            bool any_declaration = false;
            bool any_binding = false;
            for (const auto& sub : definitions_[id].subscriptions) {
                any_declaration = true;
                ++result.native_callback_subscriptions;
                ++result.declared_per_event[sub.event];
                if (callbacks_[id][sub.event]) {
                    any_binding = true;
                    ++result.executable_callback_bindings;
                    ++result.implemented_per_event[sub.event];
                } else {
                    ++result.unimplemented_callback_subscriptions;
                }
            }
            if (any_declaration) ++result.modules_with_native_handlers;
            if (any_binding) ++result.modules_with_executable_bindings;
        }
        return result;
    }

    bool dispatch(ExecutionFrame& frame) {
        if (stopped_ || active_frame_ || frame.event >= kEventSlots ||
            !frame.world_present || !frame.player_present)
            return false;
        // Fail closed: only the recognized typed payload for event 8 is
        // allowed; other event slots may be observed with an empty payload.
        if (frame.reach && frame.event != kReachQueryEvent) return false;
        if (frame.ghost_hand && frame.event != kReachQueryEvent) return false;
        if (frame.hitboxes && frame.event != kReachQueryEvent) return false;
        if (frame.piercing && frame.event != kReachQueryEvent) return false;
        if (frame.teams_event0 && frame.event != kSafeWalkEvent) return false;
        if (frame.anti_bot_event0 && frame.event != kSafeWalkEvent) return false;
        if (frame.jump_delay && frame.event != kPlayerUpdateEvent) return false;
        if (frame.null_move && frame.event != kMovementTickEvent) return false;
        if (frame.instant_stop && frame.event != kMovementTickEvent) return false;
        if (frame.no_slow && frame.event != kNoSlowEvent) return false;
        if (frame.no_hit_delay && frame.event != kNoHitDelayEvent) return false;
        if (frame.no_use_delay && frame.event != kNoUseDelayEvent) return false;
        if (frame.item_use_fix &&
            frame.event != kItemUseFixBeforeEvent &&
            frame.event != kItemUseFixAfterEvent) return false;
        if (frame.ping_fix &&
            frame.event != kIncomingPacketEvent &&
            frame.event != kOutgoingPacketEvent) return false;
        if (frame.no_item_release && frame.event != kNoItemReleaseEvent)
            return false;
        if (frame.anti_debuff && frame.event != kAntiDebuffEvent)
            return false;
        if (frame.fast_accel && frame.event != kFastAccelEvent)
            return false;
        if (frame.safe_walk && frame.event != kSafeWalkEvent)
            return false;
        if (frame.fast_place && frame.event != kSafeWalkEvent)
            return false;
        if (frame.fast_mine_early && frame.event != kSafeWalkEvent)
            return false;
        if (frame.fast_mine_late && frame.event != kAntiDebuffEvent)
            return false;
        if (frame.bridge_disable && frame.event != kTimerToggleEvent)
            return false;
        if (frame.bridge_place && frame.event != kKeepSprintAttackEvent)
            return false;
        if (frame.bridge_tick && frame.event != kPlayerUpdateEvent)
            return false;
        if (frame.bridge_late && frame.event != kAntiDebuffEvent)
            return false;
        if (frame.auto_tool_inventory &&
            frame.event!=kAutoToolInventoryEvent)return false;
        if (frame.auto_weapon_inventory &&
            frame.event!=kAutoToolInventoryEvent)return false;
        if (frame.weapons_clear_extras &&
            frame.event!=kAutoToolShutdownEvent)return false;
        if (frame.auto_tool_shutdown &&
            frame.event!=kAutoToolShutdownEvent)return false;
        if(frame.clutch_click && frame.event!=kSafeWalkEvent)return false;
        if(frame.clutch_rotation &&
           frame.event!=kClutchRotationFlagEvent)return false;
        if(frame.free_look_rotation &&
           frame.event!=kClutchRotationFlagEvent)return false;
        if(frame.clutch_late && frame.event!=kAntiDebuffEvent)return false;
        if(frame.block_in_empty && frame.event!=kSafeWalkEvent)return false;
        if(frame.block_in_toggle && frame.event!=kTimerToggleEvent)return false;
        if(frame.block_in_rotation &&
           frame.event!=kClutchRotationFlagEvent)return false;
        if(frame.block_in_movement &&
           frame.event!=kKeepSprintPostEvent)return false;
        if(frame.block_in_tick && frame.event!=kMovementTickEvent)return false;
        if (frame.timer_frame && frame.event != kTimerFrameEvent)
            return false;
        if (frame.timer_toggle && frame.event != kTimerToggleEvent)
            return false;
        if (frame.keep_sprint_attack && frame.event != kKeepSprintAttackEvent)
            return false;
        if (frame.keep_sprint_post && frame.event != kKeepSprintPostEvent)
            return false;
        if (frame.keep_sprint_update && frame.event != kKeepSprintUpdateEvent)
            return false;
        if (frame.sprint_update && frame.event != kSprintPlayerUpdateEvent)
            return false;
        if (frame.sprint_movement && frame.event != kSprintMovementInputEvent)
            return false;
        if (frame.strafe_snapshot && frame.event != kStrafeSnapshotEvent)
            return false;
        if (frame.strafe_steering && frame.event != kStrafeSteeringEvent)
            return false;
        // Event 16 may carry BOTH movement policies, unlike disjoint
        // event-8 Reach and event-17 jump payloads. Prevent conflicting
        // or fabricated game-state inputs on the same movement tick.
        const unsigned typed_count =
            static_cast<unsigned>(frame.reach.has_value()) +
            static_cast<unsigned>(frame.ghost_hand.has_value()) +
            static_cast<unsigned>(frame.hitboxes.has_value()) +
            static_cast<unsigned>(frame.piercing.has_value()) +
            static_cast<unsigned>(frame.teams_event0.has_value()) +
            static_cast<unsigned>(frame.anti_bot_event0.has_value()) +
            static_cast<unsigned>(frame.jump_delay.has_value()) +
            static_cast<unsigned>(frame.null_move.has_value()) +
            static_cast<unsigned>(frame.instant_stop.has_value()) +
            static_cast<unsigned>(frame.no_slow.has_value()) +
            static_cast<unsigned>(frame.no_hit_delay.has_value()) +
            static_cast<unsigned>(frame.no_use_delay.has_value()) +
            static_cast<unsigned>(frame.item_use_fix.has_value()) +
            static_cast<unsigned>(frame.ping_fix.has_value()) +
            static_cast<unsigned>(frame.no_item_release.has_value()) +
            static_cast<unsigned>(frame.anti_debuff.has_value()) +
            static_cast<unsigned>(frame.fast_accel.has_value()) +
            static_cast<unsigned>(frame.safe_walk.has_value()) +
            static_cast<unsigned>(frame.fast_place.has_value()) +
            static_cast<unsigned>(frame.fast_mine_early.has_value()) +
            static_cast<unsigned>(frame.fast_mine_late.has_value()) +
            static_cast<unsigned>(frame.bridge_disable.has_value()) +
            static_cast<unsigned>(frame.bridge_place.has_value()) +
            static_cast<unsigned>(frame.bridge_tick.has_value()) +
            static_cast<unsigned>(frame.bridge_late.has_value()) +
            static_cast<unsigned>(frame.auto_tool_inventory.has_value()) +
            static_cast<unsigned>(frame.auto_weapon_inventory.has_value()) +
            static_cast<unsigned>(frame.weapons_clear_extras.has_value()) +
            static_cast<unsigned>(frame.auto_tool_shutdown.has_value()) +
            static_cast<unsigned>(frame.clutch_click.has_value()) +
            static_cast<unsigned>(frame.clutch_rotation.has_value()) +
            static_cast<unsigned>(frame.free_look_rotation.has_value()) +
            static_cast<unsigned>(frame.clutch_late.has_value()) +
            static_cast<unsigned>(frame.block_in_empty.has_value()) +
            static_cast<unsigned>(frame.block_in_toggle.has_value()) +
            static_cast<unsigned>(frame.block_in_rotation.has_value()) +
            static_cast<unsigned>(frame.block_in_movement.has_value()) +
            static_cast<unsigned>(frame.block_in_tick.has_value()) +
            static_cast<unsigned>(frame.timer_frame.has_value()) +
            static_cast<unsigned>(frame.timer_toggle.has_value()) +
            static_cast<unsigned>(frame.keep_sprint_attack.has_value()) +
            static_cast<unsigned>(frame.keep_sprint_post.has_value()) +
            static_cast<unsigned>(frame.keep_sprint_update.has_value()) +
            static_cast<unsigned>(frame.sprint_update.has_value()) +
            static_cast<unsigned>(frame.sprint_movement.has_value()) +
            static_cast<unsigned>(frame.strafe_snapshot.has_value()) +
            static_cast<unsigned>(frame.strafe_steering.has_value());
        const unsigned movement_count=
            static_cast<unsigned>(frame.null_move.has_value()) +
            static_cast<unsigned>(frame.instant_stop.has_value()) +
            static_cast<unsigned>(frame.block_in_tick.has_value());
        const bool combined_movement =
            typed_count>=2 && typed_count<=3 &&
            frame.event==kMovementTickEvent &&
            movement_count==typed_count;
        // Event14 may simultaneously carry No Hit Delay, Item Use Fix
        // and Keep Sprint attack proposals. Each typed payload is validated
        // against event14 above; no other event payload is admitted.
        const unsigned mouse_click_count =
            static_cast<unsigned>(frame.no_hit_delay.has_value()) +
            static_cast<unsigned>(frame.item_use_fix.has_value()) +
            static_cast<unsigned>(frame.keep_sprint_attack.has_value()) +
            static_cast<unsigned>(frame.bridge_place.has_value());
        const bool combined_mouse_click =
            typed_count>=2 && typed_count<=4 &&
            frame.event==kItemUseFixBeforeEvent &&
            mouse_click_count==typed_count;
        const unsigned attack_post_count=
            static_cast<unsigned>(frame.item_use_fix.has_value()) +
            static_cast<unsigned>(frame.keep_sprint_post.has_value()) +
            static_cast<unsigned>(frame.block_in_movement.has_value());
        const bool combined_attack_post =
            typed_count>=2 && typed_count<=3 &&
            frame.event==kKeepSprintPostEvent &&
            attack_post_count==typed_count;
        // Both reconstructed modules subscribe to semantic event25, but
        // block and entity cursor observations must not contradict.
        const bool combined_inventory =
            typed_count==2 && frame.event==kAutoToolInventoryEvent &&
            frame.auto_tool_inventory && frame.auto_weapon_inventory;
        const bool combined_cleanup =
            typed_count==2 && frame.event==kAutoToolShutdownEvent &&
            frame.auto_tool_shutdown && frame.weapons_clear_extras;
        const unsigned query_count =
            static_cast<unsigned>(frame.reach.has_value()) +
            static_cast<unsigned>(frame.ghost_hand.has_value()) +
            static_cast<unsigned>(frame.hitboxes.has_value()) +
            static_cast<unsigned>(frame.piercing.has_value());
        const bool combined_source_query =
            typed_count>=2 && typed_count<=4 &&
            frame.event==kReachQueryEvent && query_count==typed_count;
        const unsigned rotation_count=
            static_cast<unsigned>(frame.clutch_rotation.has_value()) +
            static_cast<unsigned>(frame.block_in_rotation.has_value()) +
            static_cast<unsigned>(frame.free_look_rotation.has_value());
        const bool combined_rotation =
            typed_count>=2 && typed_count<=3 &&
            frame.event==kClutchRotationFlagEvent &&
            rotation_count==typed_count;
        // Event17 can contain all three update policies together;
        // event21 contains No Slow and Sprint's movement proposals.
        const bool combined_player_update =
            typed_count>=2 && typed_count<=4 &&
            frame.event==kSprintPlayerUpdateEvent &&
            (static_cast<unsigned>(frame.jump_delay.has_value()) +
             static_cast<unsigned>(frame.keep_sprint_update.has_value()) +
             static_cast<unsigned>(frame.sprint_update.has_value()) +
             static_cast<unsigned>(frame.bridge_tick.has_value())==typed_count);
        const bool combined_movement_sprint =
            typed_count >= 2 && typed_count <= 3 &&
            frame.event == kSprintMovementInputEvent &&
            (static_cast<unsigned>(frame.no_slow.has_value()) +
             static_cast<unsigned>(frame.sprint_movement.has_value()) +
             static_cast<unsigned>(frame.strafe_snapshot.has_value()) ==
             typed_count);
        // Strafe and Fast Accel both observe the SAME event22 movement hook,
        // and their input frames must agree before either is evaluated.
        const bool combined_steering_accel =
            typed_count == 2 && frame.event == kStrafeSteeringEvent &&
            frame.strafe_steering && frame.fast_accel;
        // Safe Walk priority 0 precedes Fast Place priority -1000 on
        // native event0. Both payloads must describe the same game state.
        const unsigned edge_place_count =
            static_cast<unsigned>(frame.safe_walk.has_value()) +
            static_cast<unsigned>(frame.fast_place.has_value()) +
            static_cast<unsigned>(frame.fast_mine_early.has_value()) +
            static_cast<unsigned>(frame.clutch_click.has_value()) +
            static_cast<unsigned>(frame.block_in_empty.has_value()) +
            static_cast<unsigned>(frame.teams_event0.has_value()) +
            static_cast<unsigned>(frame.anti_bot_event0.has_value());
        const bool combined_edge_place =
            typed_count>=2 && typed_count<=7 &&
            frame.event==kSafeWalkEvent &&
            typed_count==edge_place_count;
        const bool combined_outgoing_packet =
            typed_count == 2 && frame.event == kOutgoingPacketEvent &&
            frame.ping_fix && frame.no_item_release;
        const unsigned late_tick_count =
            static_cast<unsigned>(frame.no_use_delay.has_value()) +
            static_cast<unsigned>(frame.anti_debuff.has_value()) +
            static_cast<unsigned>(frame.fast_mine_late.has_value()) +
            static_cast<unsigned>(frame.bridge_late.has_value()) +
            static_cast<unsigned>(frame.clutch_late.has_value());
        const bool combined_late_tick =
            typed_count>=2 && typed_count<=5 &&
            frame.event==kAntiDebuffEvent &&
            typed_count==late_tick_count;
        const unsigned toggle_count=
            static_cast<unsigned>(frame.timer_toggle.has_value()) +
            static_cast<unsigned>(frame.bridge_disable.has_value()) +
            static_cast<unsigned>(frame.block_in_toggle.has_value());
        const bool combined_toggle =
            typed_count>=2 && typed_count<=3 &&
            frame.event==kTimerToggleEvent &&
            toggle_count==typed_count;
        if (typed_count > 1 && !combined_movement && !combined_mouse_click &&
            !combined_attack_post && !combined_player_update &&
            !combined_movement_sprint && !combined_steering_accel &&
            !combined_edge_place && !combined_outgoing_packet &&
            !combined_late_tick && !combined_toggle &&
            !combined_rotation && !combined_source_query &&
            !combined_inventory && !combined_cleanup)
            return false;
        // Both recovered Java modules iterate the SAME playerEntities
        // collection. Equal names do not prove equal players: duplicate
        // display names or a reordered roster can be mistaken as the
        // same event. Bind world, local owner and every UUID explicitly.
        if(frame.teams_event0 && frame.anti_bot_event0) {
            const auto& teams=*frame.teams_event0;
            const auto& bots=*frame.anti_bot_event0;
            if(teams.world_ready!=bots.context_ready ||
               bots.world_token==0 ||
               teams.source_world_token!=bots.world_token ||
               !(teams.source_owner_uuid==bots.local_player) ||
               teams.other_players.size()!=bots.other_players.size() ||
               teams.other_player_uuids.size()!=bots.other_players.size())
                return false;
            for(std::size_t i=0;i<teams.other_players.size();++i)
                if(teams.other_players[i].raw_name!=bots.other_players[i].name ||
                   !(teams.other_player_uuids[i]==bots.other_players[i].uuid))
                    return false;
        }
        if(combined_inventory) {
            const auto& tool=frame.auto_tool_inventory->input;
            const auto& weapon=frame.auto_weapon_inventory->input;
            if(tool.current_slot!=weapon.current_slot ||
               tool.using_item!=weapon.is_using_item ||
               (tool.look_at_block && weapon.hit_is_entity))
                return false;
        }
        if (combined_edge_place) {
            // Only action policies own in_game evidence. Teams and
            // Block In empty-event callbacks have no such field.
            // Avoid dereferencing a missing Fast Mine payload when
            // the frame contains only Clutch, Block In and/or Teams.
            std::optional<bool> state;
            if(frame.safe_walk)state=frame.safe_walk->input.in_game;
            else if(frame.fast_place)state=frame.fast_place->input.in_game;
            else if(frame.fast_mine_early)
                state=frame.fast_mine_early->input.in_game;
            else if(frame.clutch_click)
                state=frame.clutch_click->input.in_game;
            if(state &&
               ((frame.safe_walk && frame.safe_walk->input.in_game!=*state) ||
                (frame.fast_place && frame.fast_place->input.in_game!=*state) ||
                (frame.fast_mine_early &&
                 frame.fast_mine_early->input.in_game!=*state) ||
                (frame.clutch_click &&
                 frame.clutch_click->input.in_game!=*state)))
                return false;
        }
        if(frame.fast_mine_late && frame.no_use_delay &&
           frame.fast_mine_late->input.in_game!=
           frame.no_use_delay->input.in_game)
            return false;
        if(frame.bridge_tick && frame.jump_delay &&
           frame.bridge_tick->input.in_game!=
           frame.jump_delay->input.in_game)
            return false;
        if (combined_outgoing_packet) {
            const bool c00 =
                frame.no_item_release->input.packet ==
                NoItemReleasePacketKind::c00_keepalive;
            const auto ping_class = frame.ping_fix->input.packet;
            // These two views must describe the SAME outgoing packet:
            // only C00 keepalives can be labelled as such in Ping Fix.
            if (c00 != (ping_class == PingPacketClass::c00_client_keepalive))
                return false;
            if (ping_class == PingPacketClass::s00_server_keepalive)
                return false;
        }
        if (frame.no_slow && frame.sprint_movement) {
            const auto& no_slow = frame.no_slow->input;
            const auto& sprint = frame.sprint_movement->input;
            if (no_slow.forward != sprint.forward ||
                no_slow.strafe != sprint.strafe ||
                no_slow.using_item != sprint.using_item)
                return false;
        }
        if (combined_steering_accel) {
            const auto& a = frame.strafe_steering->input;
            const auto& b = frame.fast_accel->input;
            if (a.forward != b.forward || a.strafe != b.strafe ||
                a.on_ground != b.on_ground ||
                a.motion_x != b.motion_x || a.motion_y != b.motion_y ||
                a.motion_z != b.motion_z)
                return false;
        }
        if (frame.keep_sprint_update && frame.sprint_update &&
            frame.keep_sprint_update->input.player_present !=
                frame.sprint_update->input.player_present)
            return false;
        if (frame.null_move && frame.instant_stop) {
            const auto& a = frame.instant_stop->input;
            const auto& b = frame.null_move->input;
            if (a.in_game != b.in_game ||
                a.forward_pressed != b.game_pressed[static_cast<unsigned>(MoveKey::forward)] ||
                a.physically_held != b.physically_held)
                return false;
        }
        frame.executed_modules.clear();
        if (frame.reach) {
            frame.reach->proposed_distance.reset();
            frame.reach->executions = 0;
        }
        if (frame.ghost_hand) {
            frame.ghost_hand->decision={};
            frame.ghost_hand->query_override=false;
            frame.ghost_hand->executions=0;
        }
        if (frame.hitboxes) {
            frame.hitboxes->decision={};
            frame.hitboxes->executions=0;
        }
        if (frame.piercing) {
            frame.piercing->query_override=false;
            frame.piercing->executions=0;
        }
        if (frame.teams_event0) {
            frame.teams_event0->decisions.clear();
            frame.teams_event0->executions=0;
        }
        if (frame.anti_bot_event0) {
            frame.anti_bot_event0->decision={};
            frame.anti_bot_event0->executions=0;
        }
        if (frame.jump_delay) {
            frame.jump_delay->proposed_jump_ticks.reset();
            frame.jump_delay->executions = 0;
        }
        if (frame.null_move) {
            frame.null_move->decision = {};
            frame.null_move->executions = 0;
        }
        if (frame.instant_stop) {
            frame.instant_stop->decision = {};
            frame.instant_stop->executions = 0;
        }
        if (frame.no_slow) {
            frame.no_slow->decision = {};
            frame.no_slow->executions = 0;
        }
        if (frame.no_hit_delay) {
            frame.no_hit_delay->decision = {};
            frame.no_hit_delay->executions = 0;
        }
        if (frame.no_use_delay) {
            frame.no_use_delay->decision = {};
            frame.no_use_delay->executions = 0;
        }
        if (frame.item_use_fix) {
            frame.item_use_fix->decision = {};
            frame.item_use_fix->executions = 0;
        }
        if (frame.ping_fix) {
            frame.ping_fix->decision = {};
            frame.ping_fix->executions = 0;
        }
        if (frame.no_item_release) {
            frame.no_item_release->decision = {};
            frame.no_item_release->executions = 0;
        }
        if (frame.anti_debuff) {
            frame.anti_debuff->decision = {};
            frame.anti_debuff->executions = 0;
        }
        if (frame.fast_accel) {
            frame.fast_accel->decision = {};
            frame.fast_accel->executions = 0;
        }
        if (frame.safe_walk) {
            frame.safe_walk->decision = {};
            frame.safe_walk->executions = 0;
        }
        if (frame.fast_place) {
            frame.fast_place->decision = {};
            frame.fast_place->executions = 0;
        }
        if(frame.fast_mine_early) {
            frame.fast_mine_early->decision={};
            frame.fast_mine_early->executions=0;
        }
        if(frame.fast_mine_late) {
            frame.fast_mine_late->decision={};
            frame.fast_mine_late->executions=0;
        }
        if(frame.bridge_disable) {
            frame.bridge_disable->decision={};
            frame.bridge_disable->executions=0;
        }
        if(frame.bridge_place) {
            frame.bridge_place->decision={};
            frame.bridge_place->executions=0;
        }
        if(frame.bridge_tick) {
            frame.bridge_tick->decision={};
            frame.bridge_tick->executions=0;
        }
        if(frame.bridge_late) {
            frame.bridge_late->decision={};
            frame.bridge_late->executions=0;
        }
        if(frame.auto_tool_inventory) {
            frame.auto_tool_inventory->decision={};
            frame.auto_tool_inventory->executions=0;
        }
        if(frame.auto_weapon_inventory) {
            frame.auto_weapon_inventory->source_choice={};
            frame.auto_weapon_inventory->source_cache={};
            frame.auto_weapon_inventory->source_cache_queried=false;
            frame.auto_weapon_inventory->source_current_is_weapon=false;
            frame.auto_weapon_inventory->decision={};
            frame.auto_weapon_inventory->executions=0;
        }
        if(frame.weapons_clear_extras) {
            frame.weapons_clear_extras->proposed_clear_extra_items=false;
            frame.weapons_clear_extras->executions=0;
        }
        if(frame.auto_tool_shutdown) {
            frame.auto_tool_shutdown->decision={};
            frame.auto_tool_shutdown->executions=0;
        }
        if(frame.clutch_click) {
            frame.clutch_click->decision={};
            frame.clutch_click->executions=0;
        }
        if(frame.clutch_rotation) {
            frame.clutch_rotation->decision={};
            frame.clutch_rotation->executions=0;
        }
        if(frame.free_look_rotation) {
            frame.free_look_rotation->decision={};
            frame.free_look_rotation->executions=0;
        }
        if(frame.clutch_late) {
            frame.clutch_late->decision={};
            frame.clutch_late->executions=0;
        }
        if(frame.block_in_empty)
            frame.block_in_empty->executions=0;
        if(frame.block_in_toggle) {
            frame.block_in_toggle->decision={};
            frame.block_in_toggle->executions=0;
        }
        if(frame.block_in_rotation) {
            frame.block_in_rotation->decision={};
            frame.block_in_rotation->executions=0;
        }
        if(frame.block_in_movement) {
            frame.block_in_movement->decision={};
            frame.block_in_movement->executions=0;
        }
        if(frame.block_in_tick) {
            frame.block_in_tick->decision={};
            frame.block_in_tick->executions=0;
        }
        if (frame.timer_frame) {
            frame.timer_frame->decision = {};
            frame.timer_frame->executions = 0;
        }
        if (frame.timer_toggle) {
            frame.timer_toggle->decision = {};
            frame.timer_toggle->executions = 0;
        }
        if (frame.keep_sprint_attack) {
            frame.keep_sprint_attack->decision = {};
            frame.keep_sprint_attack->executions = 0;
        }
        if (frame.keep_sprint_post) {
            frame.keep_sprint_post->decision = {};
            frame.keep_sprint_post->executions = 0;
        }
        if (frame.keep_sprint_update) {
            frame.keep_sprint_update->decision = {};
            frame.keep_sprint_update->executions = 0;
        }
        if (frame.sprint_update) {
            frame.sprint_update->decision = {};
            frame.sprint_update->executions = 0;
        }
        if (frame.sprint_movement) {
            frame.sprint_movement->decision = {};
            frame.sprint_movement->executions = 0;
        }
        if (frame.strafe_snapshot)
            frame.strafe_snapshot->executions = 0;
        if (frame.strafe_steering) {
            frame.strafe_steering->decision = {};
            frame.strafe_steering->executions = 0;
        }
        struct ClearFrame {
            ModuleExecutionBridge& self;
            ~ClearFrame() { self.active_frame_ = nullptr; }
        } guard{*this};
        active_frame_ = &frame;
        // R261: even outside the global RNG transaction, an exception in
        // a later offline observer must not commit an Auto Weapon delay.
        const AutoWeaponController auto_weapon_before=auto_weapon_;
        const WeaponsTickCache weapons_cache_before=weapons_tick_cache_;
        const AntiBotController anti_bot_before=anti_bot_;
        if(!shared_source_rng_enabled_) {
            try {
                const bool delivered=replay_.dispatch(
                    frame.event,frame.notification_target,
                    frame.notification_new_state);
                if(!delivered) {
                    auto_weapon_=auto_weapon_before;
                    weapons_tick_cache_=weapons_cache_before;
                    anti_bot_=anti_bot_before;
                }
                return delivered;
            } catch(...) {
                auto_weapon_=auto_weapon_before;
                weapons_tick_cache_=weapons_cache_before;
                anti_bot_=anti_bot_before;
                throw;
            }
        }
        // R234 frame-atomic shared RNG transaction. Each module retains
        // its OWN walk timing state, but draws and optional random_chance
        // advance exactly one bridge-owned xoshiro generator in source
        // callback order. Rejected or exceptional dispatch restores all
        // three source controllers plus global generator.
        const SourceWalkRng previous_rng=shared_source_rng_;
        const BridgeAssistController previous_bridge=bridge_assist_;
        const ClutchController previous_clutch=clutch_;
        const BlockInController previous_block=block_in_;
        const FreeLookController previous_free_look=free_look_;
        const auto rollback=[&]() noexcept {
            shared_source_rng_=previous_rng;
            bridge_assist_=previous_bridge;
            clutch_=previous_clutch;
            block_in_=previous_block;
            free_look_=previous_free_look;
            auto_weapon_=auto_weapon_before;
            weapons_tick_cache_=weapons_cache_before;
            anti_bot_=anti_bot_before;
        };
        try {
            const bool delivered=replay_.dispatch(frame.event,
                frame.notification_target,frame.notification_new_state);
            if(!delivered)rollback();
            return delivered;
        } catch(...) {
            rollback();
            throw;
        }
    }

    bool request(std::size_t id, bool enabled) {
        return !stopped_ && !active_frame_ && replay_.request(id, enabled);
    }

    bool apply(bool world, bool player) {
        if(stopped_ || active_frame_ || !replay_.apply(world,player))return false;
        if(!free_look_bound_enabled())free_look_.reset();
        if(!auto_weapon_bound_enabled()) {
            auto_weapon_.reset();
            weapons_tick_cache_.reset();
        }
        if(!anti_bot_bound_enabled())anti_bot_.reset();
        return true;
    }

    void reset_reach_world_state() noexcept { controller_.reset(); }
    void reset_null_move_world_state() noexcept { null_move_.reset(); }
    void reset_instant_stop_world_state() noexcept { instant_stop_.reset(); }
    void reset_no_use_delay_world_state() noexcept { no_use_delay_.reset(); }
    void reset_item_use_fix_world_state() noexcept { item_use_fix_.reset(); }
    void reset_no_item_release_world_state() noexcept { no_item_release_.reset(); }
    void reset_keep_sprint_world_state() noexcept { keep_sprint_.reset(); }
    void reset_sprint_world_state() noexcept { sprint_.reset(); }
    void reset_strafe_world_state() noexcept { strafe_.reset(); }
    void reset_fast_place_world_state() noexcept { fast_place_.reset(); }
    void reset_fast_mine_world_state() noexcept { fast_mine_.reset(); }
    void reset_bridge_assist_world_state() noexcept { bridge_assist_.reset(); }
    void reset_auto_tool_world_state() noexcept { auto_tool_.reset(); }
    void reset_auto_weapon_world_state() noexcept {
        auto_weapon_.reset();
        weapons_tick_cache_.reset();
    }
    void reset_anti_bot_world_state() noexcept { anti_bot_.reset(); }
    void reset_clutch_world_state() noexcept { clutch_.reset(); }
    void reset_free_look_world_state() noexcept { free_look_.reset(); }
    void reset_block_in_world_state() noexcept { block_in_.reset(); }
    // R232: offline owner passes a reproducible seed; never infer the
    // original client's entropy-backed xoshiro128+ runtime state.
    bool configure_block_in_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        if(stopped_ || active_frame_ || shared_source_rng_enabled_)
            return false;
        return block_in_.configure_source_walk_seed(seed);
    }
    // R233: opt-in offline seeds for source-native Bridge Assist
    // and Clutch. Production runtime RNG is not guessed or intercepted.
    bool configure_bridge_assist_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        if(stopped_ || active_frame_ || shared_source_rng_enabled_)
            return false;
        return bridge_assist_.configure_source_walk_seed(seed);
    }
    bool configure_clutch_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        if(stopped_ || active_frame_ || shared_source_rng_enabled_)
            return false;
        return clutch_.configure_source_walk_seed(seed);
    }
    // R234 source's ONE global generator. Atomic enable initializes
    // independent interpolation states together; afterward only the
    // bridge may advance this seed in source dispatch order.
    bool configure_shared_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        if(stopped_ || active_frame_ || shared_source_rng_enabled_)
            return false;
        SourceWalkRng rng;
        if(!rng.seed(seed))return false;
        auto bridge=bridge_assist_;
        auto clutch=clutch_;
        auto block=block_in_;
        if(!bridge.configure_source_walk_seed(seed) ||
           !clutch.configure_source_walk_seed(seed) ||
           !block.configure_source_walk_seed(seed))return false;
        bridge_assist_=bridge;
        clutch_=clutch;
        block_in_=block;
        shared_source_rng_=rng;
        shared_source_rng_enabled_=true;
        return true;
    }
    bool has_shared_source_walk() const noexcept {
        return shared_source_rng_enabled_;
    }
    std::optional<std::array<std::uint32_t,4>>
    shared_source_rng_state() const noexcept {
        if(!shared_source_rng_enabled_)return std::nullopt;
        return shared_source_rng_.state();
    }

    void stop() noexcept {
        if (active_frame_) return; // caller must finish an active frame first
        stopped_ = true;
        replay_.stop();
        controller_.reset();
        null_move_.reset();
        instant_stop_.reset();
        no_use_delay_.reset();
        item_use_fix_.reset();
        no_item_release_.reset();
        keep_sprint_.reset();
        sprint_.reset();
        strafe_.reset();
        fast_place_.reset();
        fast_mine_.reset();
        bridge_assist_.reset();
        auto_tool_.reset();
        auto_weapon_.reset();
        weapons_tick_cache_.reset();
        anti_bot_.reset();
        clutch_.reset();
        block_in_.reset();
        free_look_.reset();
        shared_source_rng_enabled_=false;
        shared_source_rng_=SourceWalkRng{};
        callbacks_.clear();
        definitions_.clear();
        bound_count_ = 0;
    }

    bool enabled(std::size_t id) const { return replay_.enabled(id); }
    std::size_t registered_modules() const noexcept { return definitions_.size(); }
    std::size_t implemented_event_bindings() const noexcept { return bound_count_; }
    int reach_cooldown_ticks() const noexcept { return controller_.cooldown_ticks(); }

private:
    bool piercing_bound_enabled() const noexcept {
        for(std::size_t i=0;i<definitions_.size();++i)
            if(definitions_[i].name=="Piercing" &&
               definitions_[i].category==0 &&
               callbacks_[i][kReachQueryEvent] &&
               replay_.enabled(i))return true;
        return false;
    }
    bool anti_bot_bound_enabled() const noexcept {
        for(std::size_t i=0;i<definitions_.size();++i)
            if(definitions_[i].name=="Anti Bot" &&
               definitions_[i].category==4 &&
               callbacks_[i][kSafeWalkEvent] &&
               replay_.enabled(i))return true;
        return false;
    }
    bool auto_weapon_bound_enabled() const noexcept {
        for(std::size_t i=0;i<definitions_.size();++i)
            if(definitions_[i].name=="Auto Weapon" &&
               definitions_[i].category==0 &&
               callbacks_[i][kAutoToolInventoryEvent] &&
               replay_.enabled(i))return true;
        return false;
    }
    bool free_look_bound_enabled() const noexcept {
        for(std::size_t i=0;i<definitions_.size();++i)
            if(definitions_[i].name=="Free Look" &&
               definitions_[i].category==2 &&
               callbacks_[i][kClutchRotationFlagEvent] &&
               replay_.enabled(i))return true;
        return false;
    }
    EventReplay replay_;
    slinkywinky::ReachController controller_;
    NoJumpDelayController no_jump_delay_;
    NullMoveController null_move_;
    InstantStopController instant_stop_;
    NoSlowController no_slow_;
    NoHitDelayController no_hit_delay_;
    NoUseDelayController no_use_delay_;
    ItemUseFixController item_use_fix_;
    PingFixController ping_fix_;
    NoItemReleaseController no_item_release_;
    AntiDebuffController anti_debuff_;
    FastAccelController fast_accel_;
    SafeWalkController safe_walk_;
    FastPlaceController fast_place_;
    FastMineController fast_mine_;
    BridgeAssistController bridge_assist_;
    AutoToolController auto_tool_;
    AutoWeaponController auto_weapon_;
    WeaponsTickCache weapons_tick_cache_;
    AntiBotController anti_bot_;
    ClutchController clutch_;
    BlockInController block_in_;
    FreeLookController free_look_;
    SourceWalkRng shared_source_rng_{};
    bool shared_source_rng_enabled_=false;
    TimerController timer_;
    KeepSprintController keep_sprint_;
    SprintController sprint_;
    StrafeController strafe_;
    std::vector<ModuleSpec> definitions_;
    std::vector<std::array<OfflineCallback, kEventSlots>> callbacks_;
    ExecutionFrame* active_frame_ = nullptr;
    std::size_t bound_count_ = 0;
    bool stopped_ = false;
};
}  // namespace slinkywinky::reference_compat
