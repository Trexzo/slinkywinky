#pragma once
// R188: original independently authored, OFFLINE ONLY Anti Debuff decision.
// No potion effects or Minecraft screen fields are actually modified.
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kAntiDebuffEvent = 18;

struct AntiDebuffInput {
    bool blindness_setting = false;
    bool nausea_setting = false;
    bool blindness_effect_active = false;
};

struct AntiDebuffDecision {
    std::optional<int> proposed_blindness_duration{};
    std::optional<float> proposed_time_in_portal{};
    std::optional<float> proposed_prev_time_in_portal{};
};

class AntiDebuffController final {
public:
    AntiDebuffDecision evaluate(const AntiDebuffInput& in) const noexcept {
        AntiDebuffDecision result;
        if (in.blindness_setting && in.blindness_effect_active)
            result.proposed_blindness_duration = 0;
        if (in.nausea_setting) {
            result.proposed_time_in_portal = 0.0f;
            result.proposed_prev_time_in_portal = 0.0f;
        }
        return result;
    }
};
} // namespace slinkywinky::reference_compat
