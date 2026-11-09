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

struct ReachQueryPayload {
    slinkywinky::ReachInput input{};
    slinkywinky::ReachSettings settings{};
    std::optional<double> proposed_distance{};
    unsigned executions = 0;
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

struct ExecutionFrame {
    unsigned event = 0;
    bool world_present = false;
    bool player_present = false;
    std::optional<ReachQueryPayload> reach{};
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
        // Event 16 may carry BOTH movement policies, unlike disjoint
        // event-8 Reach and event-17 jump payloads. Prevent conflicting
        // or fabricated game-state inputs on the same movement tick.
        const unsigned typed_count =
            static_cast<unsigned>(frame.reach.has_value()) +
            static_cast<unsigned>(frame.jump_delay.has_value()) +
            static_cast<unsigned>(frame.null_move.has_value()) +
            static_cast<unsigned>(frame.instant_stop.has_value()) +
            static_cast<unsigned>(frame.no_slow.has_value()) +
            static_cast<unsigned>(frame.no_hit_delay.has_value()) +
            static_cast<unsigned>(frame.no_use_delay.has_value()) +
            static_cast<unsigned>(frame.item_use_fix.has_value()) +
            static_cast<unsigned>(frame.ping_fix.has_value()) +
            static_cast<unsigned>(frame.no_item_release.has_value()) +
            static_cast<unsigned>(frame.anti_debuff.has_value());
        const bool combined_movement =
            typed_count == 2 && frame.event == kMovementTickEvent &&
            frame.null_move && frame.instant_stop;
        const bool combined_mouse_click =
            typed_count == 2 && frame.event == kItemUseFixBeforeEvent &&
            frame.no_hit_delay && frame.item_use_fix;
        const bool combined_outgoing_packet =
            typed_count == 2 && frame.event == kOutgoingPacketEvent &&
            frame.ping_fix && frame.no_item_release;
        const bool combined_late_tick =
            typed_count == 2 && frame.event == kAntiDebuffEvent &&
            frame.no_use_delay && frame.anti_debuff;
        if (typed_count > 1 && !combined_movement && !combined_mouse_click &&
            !combined_outgoing_packet && !combined_late_tick)
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
        struct ClearFrame {
            ModuleExecutionBridge& self;
            ~ClearFrame() { self.active_frame_ = nullptr; }
        } guard{*this};
        active_frame_ = &frame;
        return replay_.dispatch(frame.event);
    }

    bool request(std::size_t id, bool enabled) {
        return !stopped_ && !active_frame_ && replay_.request(id, enabled);
    }

    bool apply(bool world, bool player) {
        return !stopped_ && !active_frame_ && replay_.apply(world, player);
    }

    void reset_reach_world_state() noexcept { controller_.reset(); }
    void reset_null_move_world_state() noexcept { null_move_.reset(); }
    void reset_instant_stop_world_state() noexcept { instant_stop_.reset(); }
    void reset_no_use_delay_world_state() noexcept { no_use_delay_.reset(); }
    void reset_item_use_fix_world_state() noexcept { item_use_fix_.reset(); }
    void reset_no_item_release_world_state() noexcept { no_item_release_.reset(); }

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
        callbacks_.clear();
        definitions_.clear();
        bound_count_ = 0;
    }

    bool enabled(std::size_t id) const { return replay_.enabled(id); }
    std::size_t registered_modules() const noexcept { return definitions_.size(); }
    std::size_t implemented_event_bindings() const noexcept { return bound_count_; }
    int reach_cooldown_ticks() const noexcept { return controller_.cooldown_ticks(); }

private:
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
    std::vector<ModuleSpec> definitions_;
    std::vector<std::array<OfflineCallback, kEventSlots>> callbacks_;
    ExecutionFrame* active_frame_ = nullptr;
    std::size_t bound_count_ = 0;
    bool stopped_ = false;
};
}  // namespace slinkywinky::reference_compat
