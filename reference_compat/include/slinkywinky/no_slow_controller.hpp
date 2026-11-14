#pragma once
// R182: independent, pure offline No Slow input-scaling decision. No JVM/game I/O.
#include <cmath>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kNoSlowEvent = 21;

struct NoSlowSettings {
    float bow_multiplier = 1.0f;
    float sword_multiplier = 1.0f;
    float consumable_multiplier = 1.0f;
};

struct NoSlowInput {
    bool using_item = false;
    bool held_item_present = false;
    int item_id = -1;
    float strafe = 0.0f;
    float forward = 0.0f;
    NoSlowSettings settings{};
};

struct NoSlowDecision {
    bool replace_input = false;
    float strafe = 0.0f;
    float forward = 0.0f;
};

class NoSlowController final {
public:
    NoSlowDecision evaluate(const NoSlowInput& input) const noexcept {
        NoSlowDecision out{false, input.strafe, input.forward};
        if (!input.using_item || !input.held_item_present) return out;
        float multiplier = input.settings.consumable_multiplier;
        if (input.item_id == 261) {
            multiplier = input.settings.bow_multiplier;
        } else if (input.item_id == 267 || input.item_id == 268 ||
                   input.item_id == 272 || input.item_id == 276 ||
                   input.item_id == 283) {
            multiplier = input.settings.sword_multiplier;
        }
        // The reference hook rescales input already slowed to 0.2x.
        // We deliberately reject nonfinite/negative magnitudes instead
        // of propagating unsafe proposals into a downstream adapter.
        if (!std::isfinite(multiplier) || multiplier < 0.0f ||
            !std::isfinite(input.strafe) || !std::isfinite(input.forward))
            return out;
        const double factor = static_cast<double>(multiplier) / 0.2;
        const double strafe = static_cast<double>(input.strafe) * factor;
        const double forward = static_cast<double>(input.forward) * factor;
        if (!std::isfinite(strafe) || !std::isfinite(forward) ||
            std::abs(strafe) > 3.402823466e38 ||
            std::abs(forward) > 3.402823466e38)
            return out;
        out.replace_input = true;
        out.strafe = static_cast<float>(strafe);
        out.forward = static_cast<float>(forward);
        return out;
    }
};
} // namespace slinkywinky::reference_compat
