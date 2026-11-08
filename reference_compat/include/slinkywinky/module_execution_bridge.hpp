#pragma once
// R177: independent offline adapter from candidate event registration to
// existing SlinkyWinky C++ decision models. No third-party source copied.
// NO JNI, Java hooks, input injection, game packets, or live player state.
#include "slinkywinky/core.hpp"
#include "slinkywinky/reference_event_model.hpp"
#include "slinkywinky/no_jump_delay_controller.hpp"
#include "slinkywinky/null_move_controller.hpp"
#include "slinkywinky/instant_stop_controller.hpp"

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

struct ExecutionFrame {
    unsigned event = 0;
    bool world_present = false;
    bool player_present = false;
    std::optional<ReachQueryPayload> reach{};
    std::optional<JumpDelayTickPayload> jump_delay{};
    std::optional<NullMoveTickPayload> null_move{};
    std::optional<InstantStopTickPayload> instant_stop{};
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
        // Event 16 may carry BOTH movement policies, unlike disjoint
        // event-8 Reach and event-17 jump payloads. Prevent conflicting
        // or fabricated game-state inputs on the same movement tick.
        const unsigned typed_count =
            static_cast<unsigned>(frame.reach.has_value()) +
            static_cast<unsigned>(frame.jump_delay.has_value()) +
            static_cast<unsigned>(frame.null_move.has_value()) +
            static_cast<unsigned>(frame.instant_stop.has_value());
        if (typed_count > 1 &&
            !(typed_count == 2 && frame.event == kMovementTickEvent &&
              frame.null_move && frame.instant_stop)) return false;
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

    void stop() noexcept {
        if (active_frame_) return; // caller must finish an active frame first
        stopped_ = true;
        replay_.stop();
        controller_.reset();
        null_move_.reset();
        instant_stop_.reset();
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
    std::vector<ModuleSpec> definitions_;
    std::vector<std::array<OfflineCallback, kEventSlots>> callbacks_;
    ExecutionFrame* active_frame_ = nullptr;
    std::size_t bound_count_ = 0;
    bool stopped_ = false;
};
}  // namespace slinkywinky::reference_compat
