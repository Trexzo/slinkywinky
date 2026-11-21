#pragma once
// R264: source-derived Hitboxes event8 expansion proposal.
// No entity collision boxes, ray casts, game writes, or native hook.
#include <cmath>
namespace slinkywinky::reference_compat {
struct HitboxesObservation {
    bool dispatched_to_enabled_module=false;
    float expand=0.0f;
    bool expansion_setting_configured=false;
};
struct HitboxesProposal {
    bool admitted=false,proposed_enable_event=false;
    double proposed_expansion=0.0;
    bool proposed_status_visible=false;
};
class HitboxesController final {
public:
    static HitboxesProposal on_event8(const HitboxesObservation& obs) noexcept {
        HitboxesProposal out;
        // The source stores expand in a numeric slider. Its authenticated
        // native limits and event priority are not yet recovered.
        // Fail closed on unusable physics inputs, not on unknown slider min.
        if(!std::isfinite(obs.expand) ||
           std::fabs(obs.expand)>1000000.0f)
            return out;
        out.admitted=true;
        out.proposed_status_visible=obs.expansion_setting_configured;
        if(obs.dispatched_to_enabled_module) {
            out.proposed_enable_event=true;
            out.proposed_expansion=static_cast<double>(obs.expand);
        }
        return out;
    }
};
} // namespace slinkywinky::reference_compat
