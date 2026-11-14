#pragma once
// R203: first-party offline Sprint proposals based on pinned native callback
// contract. No JNI, key injection, Minecraft writes or third-party source.
#include <cmath>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {

inline constexpr unsigned kSprintPlayerUpdateEvent = 17;
inline constexpr unsigned kSprintMovementInputEvent = 21;

struct SprintSettings {
    bool while_in_gui = false;
    bool while_using_item = false;
    bool while_backwards = false;
    bool while_sideways = false;
};

struct SprintUpdateInput {
    bool gui_open = false;
    bool player_present = true;
    bool player_sprinting = false;
    SprintSettings settings{};
};

struct SprintUpdateDecision {
    // The pinned native reference always sets the sprint binding on each
    // admitted update, even if the player object is unavailable.
    bool proposed_key_pressed = false;
    std::optional<bool> proposed_player_sprinting{};
    bool tracked_forced_state = false;
};

struct SprintMovementInput {
    bool player_present = true;
    float forward = 0.0f;
    float strafe = 0.0f;
    float speed = 0.0f;
    bool using_item = false;
    SprintSettings settings{};
};

struct SprintMovementDecision {
    std::optional<float> proposed_speed{};
    bool propose_set_sprinting = false;
};

class SprintController final {
public:
    SprintUpdateDecision on_player_update(const SprintUpdateInput& input) noexcept {
        const bool allowed = !input.gui_open || input.settings.while_in_gui;
        SprintUpdateDecision out;
        out.proposed_key_pressed = allowed;
        if (input.player_present) {
            if (!allowed) {
                // The native implementation returns early here: forced_ is
                // deliberately *not* cleared when a menu disallows sprint.
                if (forced_) out.proposed_player_sprinting = false;
                out.tracked_forced_state = forced_;
                return out;
            }
            if (input.player_sprinting)
                out.proposed_player_sprinting = true;
        }
        forced_ = allowed;
        out.tracked_forced_state = forced_;
        return out;
    }

    SprintMovementDecision on_movement_input(
        const SprintMovementInput& input) const noexcept {
        SprintMovementDecision out;
        // Defensive bounds: the candidate source assumes a live player and
        // finite movement scalars; reject invalid proposals instead.
        if (!input.player_present || !std::isfinite(input.forward) ||
            !std::isfinite(input.strafe) || !std::isfinite(input.speed) ||
            (input.forward == 0.0f && input.strafe == 0.0f))
            return out;

        if (input.using_item && !input.settings.while_using_item)
            return out;
        if (input.forward < 0.0f) {
            if (!input.settings.while_backwards) return out;
        } else if (input.forward == 0.0f) {
            if (!input.settings.while_sideways) return out;
        } else if (!input.using_item) {
            // Forward movement is already handled by vanilla sprint.
            return out;
        }

        const double proposed = static_cast<double>(input.speed) * 1.3;
        if (!std::isfinite(proposed) ||
            std::abs(proposed) > std::numeric_limits<float>::max())
            return out;
        out.proposed_speed = static_cast<float>(proposed);
        out.propose_set_sprinting = true;
        return out;
    }

    // This only clears the independent first-party state. Releasing an
    // actual JNI key binding on shutdown is still a separate runtime gate.
    void reset() noexcept { forced_ = false; }
    bool tracked_forced_state() const noexcept { return forced_; }

private:
    bool forced_ = false;
};

} // namespace slinkywinky::reference_compat
