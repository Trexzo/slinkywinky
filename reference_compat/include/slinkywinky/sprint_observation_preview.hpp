#pragma once
// R204 first-party observation-to-offline-policy bridge.
// Consumes the existing runtime's real ObservationFrame wire shape; it never
// writes to Minecraft, key bindings, packets, a native DLL or the OS.
#include "slinkywinky/module_execution_bridge.hpp"
#include "slinkywinky/module_control_contract.hpp"
#include "slinkywinky/observation.hpp"
#include <cstdint>
#include <cmath>
#include <optional>
#include <string_view>
#include <vector>
#include <memory>
#include <stdexcept>
#include <thread>

namespace slinkywinky::reference_compat {

struct SprintObservedTick {
    slinkywinky::ObservationFrame observation{};
    SprintUpdateInput update{};
    SprintMovementInput movement{};
    // Optional, typed inputs: legacy R204-R210 Sprint JNI callers do not
    // invent a jump counter or current Minecraft held-item state.
    std::optional<JumpDelayInput> jump_delay{};
    std::optional<NoSlowInput> no_slow{};
};

struct SprintObservedDecision {
    bool admitted = false;
    bool world_ready = false;
    bool session_transition = false;
    std::uint64_t generation = 0;
    std::uint64_t accepted_ticks = 0;
    SprintUpdateDecision update{};
    SprintMovementDecision movement{};
    std::optional<int> proposed_jump_ticks{};
    std::optional<NoSlowDecision> no_slow{};
};

class SprintObservationPreview final {
public:
    SprintObservationPreview() : owner_(std::this_thread::get_id()) {}
    SprintObservationPreview(const SprintObservationPreview&) = delete;
    SprintObservationPreview& operator=(const SprintObservationPreview&) = delete;

    // Game-thread-owned control entry point for an eventual ClickGUI adapter.
    // The selection survives absent-world frames and generation transitions.
    bool request_module(slinkywinky::ModuleControlRequest request) {
        if (stopped_ || std::this_thread::get_id() != owner_) return false;
        const auto id = module_id(request.canonical_name);
        if (!id) return false;
        bool& selected = selection_[*id];
        if (selected == request.enable) return true;
        if (bridge_) {
            if (!bridge_->request(*id, request.enable) ||
                !bridge_->apply(true, true))
                return false;
            if (*id == kSprintModule) bridge_->reset_sprint_world_state();
        }
        selected = request.enable;
        return true;
    }

    bool configure_sprint(SprintSettings settings) noexcept {
        if (stopped_ || std::this_thread::get_id() != owner_) return false;
        sprint_settings_ = settings;
        sprint_override_ = true;
        return true;
    }

    bool configure_no_slow(NoSlowSettings settings) noexcept {
        if (stopped_ || std::this_thread::get_id() != owner_ ||
            !std::isfinite(settings.bow_multiplier) ||
            !std::isfinite(settings.sword_multiplier) ||
            !std::isfinite(settings.consumable_multiplier) ||
            settings.bow_multiplier < 0.0f ||
            settings.sword_multiplier < 0.0f ||
            settings.consumable_multiplier < 0.0f)
            return false;
        no_slow_settings_ = settings;
        no_slow_override_ = true;
        return true;
    }

    std::optional<slinkywinky::ModuleControlState> control_state(
        std::string_view name) const noexcept {
        if (stopped_ || std::this_thread::get_id() != owner_) return std::nullopt;
        const auto id = module_id(name);
        if (!id) return std::nullopt;
        slinkywinky::ModuleControlState state;
        state.canonical_name = std::string(name);
        state.requested_enabled = selection_[*id];
        state.active_in_world =
            bridge_ && selection_[*id] && bridge_->enabled(*id);
        state.settings_override =
            *id == kSprintModule ? sprint_override_ :
            *id == kNoSlowModule ? no_slow_override_ : false;
        state.world_generation = generation_;
        return state;
    }

    // R210 legacy call site always represents Sprint.
    std::optional<slinkywinky::ModuleControlState> control_state() const noexcept {
        return control_state("Sprint");
    }

    // Deterministic order for a future immutable GUI/ArrayList snapshot.
    std::optional<std::vector<slinkywinky::ModuleControlState>>
    control_states() const {
        if (stopped_ || std::this_thread::get_id() != owner_)
            return std::nullopt;
        std::vector<slinkywinky::ModuleControlState> states;
        states.reserve(kModuleCount);
        for (auto name : {"No Jump Delay", "No Slow", "Sprint"}) {
            auto state = control_state(name);
            if (!state) return std::nullopt;
            states.push_back(std::move(*state));
        }
        return states;
    }

    SprintObservedDecision observe(const SprintObservedTick& sample) {
        SprintObservedDecision result;
        if (stopped_ || std::this_thread::get_id() != owner_ ||
            !ledger_.can_accept(sample.observation)) return result;
        const auto flags = sample.observation.flags;
        const bool player = (flags & 3u) == 3u;
        // An observation must represent one coherent player and shared
        // movement state. Reject contradictions before session changes.
        if (sample.update.player_present != player ||
            sample.movement.player_present != player ||
            (sample.jump_delay && sample.jump_delay->in_game != player) ||
            (sample.no_slow && !player) ||
            (sample.no_slow &&
             (sample.no_slow->forward != sample.movement.forward ||
              sample.no_slow->strafe != sample.movement.strafe ||
              sample.no_slow->using_item != sample.movement.using_item)))
            return result;

        const std::uint64_t generation = sample.observation.generation;
        const bool transitioned = generation != generation_;
        if (transitioned || !bridge_) {
            if (bridge_) {
                bridge_->stop();
                bridge_.reset();
            }
            generation_ = generation;
            if (player) start_session();
        }
        if (player) {
            if (!bridge_) return result;
            const bool sprint = selection_[kSprintModule];
            const bool jump = selection_[kJumpModule] && sample.jump_delay.has_value();
            const bool no_slow =
                selection_[kNoSlowModule] && sample.no_slow.has_value();

            if (sprint || jump) {
                ExecutionFrame update_frame;
                update_frame.event = kSprintPlayerUpdateEvent;
                update_frame.world_present = update_frame.player_present = true;
                if (sprint) {
                    SprintUpdateInput input = sample.update;
                    if (sprint_override_) input.settings = sprint_settings_;
                    update_frame.sprint_update = SprintUpdatePayload{input, {}, 0};
                }
                if (jump)
                    update_frame.jump_delay =
                        JumpDelayTickPayload{*sample.jump_delay, {}, 0};
                if (!bridge_->dispatch(update_frame)) {
                    shutdown();
                    return result;
                }
                if (sprint) {
                    if (update_frame.sprint_update->executions != 1) {
                        shutdown();
                        return result;
                    }
                    result.update = update_frame.sprint_update->decision;
                }
                if (jump) {
                    if (update_frame.jump_delay->executions != 1) {
                        shutdown();
                        return result;
                    }
                    result.proposed_jump_ticks =
                        update_frame.jump_delay->proposed_jump_ticks;
                }
            }

            if (sprint || no_slow) {
                ExecutionFrame movement_frame;
                movement_frame.event = kSprintMovementInputEvent;
                movement_frame.world_present = movement_frame.player_present = true;
                if (sprint) {
                    SprintMovementInput input = sample.movement;
                    if (sprint_override_) input.settings = sprint_settings_;
                    movement_frame.sprint_movement =
                        SprintMovementPayload{input, {}, 0};
                }
                if (no_slow) {
                    NoSlowInput input = *sample.no_slow;
                    if (no_slow_override_) input.settings = no_slow_settings_;
                    movement_frame.no_slow = NoSlowTickPayload{input, {}, 0};
                }
                if (!bridge_->dispatch(movement_frame)) {
                    shutdown();
                    return result;
                }
                if (sprint) {
                    if (movement_frame.sprint_movement->executions != 1) {
                        shutdown();
                        return result;
                    }
                    result.movement = movement_frame.sprint_movement->decision;
                }
                if (no_slow) {
                    if (movement_frame.no_slow->executions != 1) {
                        shutdown();
                        return result;
                    }
                    result.no_slow = movement_frame.no_slow->decision;
                }
            }
        } else {
            // Inactive worlds retire callbacks and preserve all three UI
            // selections plus explicit settings for subsequent generation.
            if (bridge_) {
                bridge_->stop();
                bridge_.reset();
            }
        }

        // When disabled in a valid world the observation is still admitted,
        // but no callback runs and every proposal remains its default.
        ledger_.commit(sample.observation);
        const auto snapshot = ledger_.snapshot();
        result.admitted = true;
        result.world_ready = player;
        result.session_transition = transitioned;
        result.generation = generation_;
        result.accepted_ticks = snapshot ? snapshot->valid_ticks : 0;
        return result;
    }

    bool shutdown() noexcept {
        if (stopped_ || std::this_thread::get_id() != owner_) return false;
        stopped_ = true;
        if (bridge_) {
            bridge_->stop();
            bridge_.reset();
        }
        generation_ = 0;
        selection_ = {};
        return true;
    }
    bool stopped() const noexcept { return stopped_; }
    std::uint64_t generation() const noexcept {
        return std::this_thread::get_id() == owner_ ? generation_ : 0;
    }

private:
    static constexpr std::size_t kJumpModule = 0;
    static constexpr std::size_t kNoSlowModule = 1;
    static constexpr std::size_t kSprintModule = 2;
    static constexpr std::size_t kModuleCount = 3;

    static std::optional<std::size_t> module_id(
        std::string_view name) noexcept {
        if (name == "No Jump Delay") return kJumpModule;
        if (name == "No Slow") return kNoSlowModule;
        if (name == "Sprint") return kSprintModule;
        return std::nullopt;
    }

    void start_session() {
        bridge_ = std::make_unique<ModuleExecutionBridge>();
        const auto jump = bridge_->add(
            {"No Jump Delay", 1, {{kPlayerUpdateEvent, 100}}, false});
        const auto slow = bridge_->add(
            {"No Slow", 1, {{kNoSlowEvent, 200}}, false});
        const auto sprint = bridge_->add(
            {"Sprint", 1,
             {{kSprintPlayerUpdateEvent, 100},
              {kSprintMovementInputEvent, 100}}, false});
        if (jump != kJumpModule || slow != kNoSlowModule ||
            sprint != kSprintModule ||
            !bridge_->bind_no_jump_delay(jump) ||
            !bridge_->bind_no_slow(slow) ||
            !bridge_->bind_sprint(sprint))
            throw std::logic_error("R211 failed to bind three native modules");
        for (std::size_t id = 0; id < kModuleCount; ++id) {
            if (selection_[id] && !bridge_->request(id, true))
                throw std::logic_error("R211 module enable request rejected");
        }
        if (!bridge_->apply(true, true))
            throw std::logic_error("R211 failed to activate module session");
    }

    const std::thread::id owner_;
    slinkywinky::ObservationLedger ledger_;
    std::unique_ptr<ModuleExecutionBridge> bridge_;
    std::uint64_t generation_ = 0;
    SprintSettings sprint_settings_{};
    NoSlowSettings no_slow_settings_{};
    std::array<bool, kModuleCount> selection_{{false, false, true}};
    bool sprint_override_ = false;
    bool no_slow_override_ = false;
    bool stopped_ = false;
};

} // namespace slinkywinky::reference_compat
