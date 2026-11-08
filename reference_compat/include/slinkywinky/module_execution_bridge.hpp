#pragma once
// R177: independent offline adapter from candidate event registration to
// existing SlinkyWinky C++ decision models. No third-party source copied.
// NO JNI, Java hooks, input injection, game packets, or live player state.
#include "slinkywinky/core.hpp"
#include "slinkywinky/reference_event_model.hpp"

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

struct ReachQueryPayload {
    slinkywinky::ReachInput input{};
    slinkywinky::ReachSettings settings{};
    std::optional<double> proposed_distance{};
    unsigned executions = 0;
};

struct ExecutionFrame {
    unsigned event = 0;
    bool world_present = false;
    bool player_present = false;
    std::optional<ReachQueryPayload> reach{};
    std::vector<std::size_t> executed_modules{};
};

using OfflineCallback = std::function<void(const Delivery&, ExecutionFrame&)>;

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

    bool dispatch(ExecutionFrame& frame) {
        if (stopped_ || active_frame_ || frame.event >= kEventSlots ||
            !frame.world_present || !frame.player_present)
            return false;
        // Fail closed: only the recognized typed payload for event 8 is
        // allowed; other event slots may be observed with an empty payload.
        if (frame.reach && frame.event != kReachQueryEvent) return false;
        frame.executed_modules.clear();
        if (frame.reach) {
            frame.reach->proposed_distance.reset();
            frame.reach->executions = 0;
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

    void stop() noexcept {
        if (active_frame_) return; // caller must finish an active frame first
        stopped_ = true;
        replay_.stop();
        controller_.reset();
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
    std::vector<ModuleSpec> definitions_;
    std::vector<std::array<OfflineCallback, kEventSlots>> callbacks_;
    ExecutionFrame* active_frame_ = nullptr;
    std::size_t bound_count_ = 0;
    bool stopped_ = false;
};
}  // namespace slinkywinky::reference_compat
