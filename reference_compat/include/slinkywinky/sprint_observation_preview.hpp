#pragma once
// R204 first-party observation-to-offline-policy bridge.
// Consumes the existing runtime's real ObservationFrame wire shape; it never
// writes to Minecraft, key bindings, packets, a native DLL or the OS.
#include "slinkywinky/module_execution_bridge.hpp"
#include "slinkywinky/observation.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>

namespace slinkywinky::reference_compat {

struct SprintObservedTick {
    slinkywinky::ObservationFrame observation{};
    SprintUpdateInput update{};
    SprintMovementInput movement{};
};

struct SprintObservedDecision {
    bool admitted = false;
    bool world_ready = false;
    bool session_transition = false;
    std::uint64_t generation = 0;
    std::uint64_t accepted_ticks = 0;
    SprintUpdateDecision update{};
    SprintMovementDecision movement{};
};

class SprintObservationPreview final {
public:
    SprintObservationPreview() : owner_(std::this_thread::get_id()) {}
    SprintObservationPreview(const SprintObservationPreview&) = delete;
    SprintObservationPreview& operator=(const SprintObservationPreview&) = delete;

    SprintObservedDecision observe(const SprintObservedTick& sample) {
        SprintObservedDecision result;
        if (stopped_ || std::this_thread::get_id() != owner_ ||
            !ledger_.can_accept(sample.observation)) return result;
        const auto flags = sample.observation.flags;
        const bool player = (flags & 3u) == 3u;
        // Both callbacks are optional decisions for the *same sampled player*.
        // Reject inconsistent externally supplied game context before updating
        // our session, event queue or read-only observation ledger.
        if (sample.update.player_present != player ||
            sample.movement.player_present != player)
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
            ExecutionFrame update_frame;
            update_frame.event = kSprintPlayerUpdateEvent;
            update_frame.world_present = update_frame.player_present = true;
            update_frame.sprint_update = SprintUpdatePayload{sample.update, {}, 0};
            if (!bridge_->dispatch(update_frame) ||
                update_frame.sprint_update->executions != 1) {
                shutdown();
                return result;
            }
            ExecutionFrame movement_frame;
            movement_frame.event = kSprintMovementInputEvent;
            movement_frame.world_present = movement_frame.player_present = true;
            movement_frame.sprint_movement =
                SprintMovementPayload{sample.movement, {}, 0};
            if (!bridge_->dispatch(movement_frame) ||
                movement_frame.sprint_movement->executions != 1) {
                shutdown();
                return result;
            }
            result.update = update_frame.sprint_update->decision;
            result.movement = movement_frame.sprint_movement->decision;
        } else {
            // No-player frames admit observation only. Session generation
            // changes were required by ObservationLedger before we got here.
            if (bridge_) {
                bridge_->stop();
                bridge_.reset();
            }
        }
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
        return true;
    }
    bool stopped() const noexcept { return stopped_; }
    std::uint64_t generation() const noexcept {
        return std::this_thread::get_id() == owner_ ? generation_ : 0;
    }

private:
    void start_session() {
        bridge_ = std::make_unique<ModuleExecutionBridge>();
        const auto id = bridge_->add(
            {"Sprint", 1,
             {{kSprintPlayerUpdateEvent, 100},
              {kSprintMovementInputEvent, 100}}, false});
        if (!bridge_->bind_sprint(id) ||
            !bridge_->request(id, true) ||
            !bridge_->apply(true, true))
            throw std::logic_error("R204 failed to configure offline Sprint session");
    }

    const std::thread::id owner_;
    slinkywinky::ObservationLedger ledger_;
    std::unique_ptr<ModuleExecutionBridge> bridge_;
    std::uint64_t generation_ = 0;
    bool stopped_ = false;
};

} // namespace slinkywinky::reference_compat
