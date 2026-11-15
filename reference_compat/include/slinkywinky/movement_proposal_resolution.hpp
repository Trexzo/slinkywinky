#pragma once
// R223: read-only owner arbitration for simultaneous event-22 movement
// proposals. This is a defensive first-party policy, NOT the original
// commercial binary's execution order. No game motion writes occur.
#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {

enum class MovementProposalOwner {
    none,
    strafe_priority_100,
    fast_accel_priority_0
};
struct MovementResolution {
    MovementProposalOwner owner=MovementProposalOwner::none;
    std::optional<StrafeMotionProposal> motion{};
};

// Must only be called on the SAME successfully dispatched ExecutionFrame.
// Source coordinates, event and admission must agree before a single value
// is exposed. A stale or forged payload never becomes an actionable command.
inline MovementResolution resolve_event22_motion(
    const ExecutionFrame& frame) noexcept {
    MovementResolution out;
    if (frame.event!=kStrafeSteeringEvent ||
        !frame.world_present || !frame.player_present ||
        frame.executed_modules.empty()) return out;

    const auto* strafe=frame.strafe_steering ?
        &*frame.strafe_steering : nullptr;
    const auto* accel=frame.fast_accel ?
        &*frame.fast_accel : nullptr;
    if (!strafe && !accel) return out;
    if ((strafe && strafe->executions>1) ||
        (accel && accel->executions>1)) return out;

    if (strafe && accel) {
        const auto& a=strafe->input;
        const auto& b=accel->input;
        if (a.forward!=b.forward || a.strafe!=b.strafe ||
            a.on_ground!=b.on_ground || a.motion_x!=b.motion_x ||
            a.motion_y!=b.motion_y || a.motion_z!=b.motion_z)
            return out;
    }
    if ((strafe && strafe->executions==0 &&
         strafe->decision.proposed_motion) ||
        (accel && accel->executions==0 &&
         accel->decision.proposed_motion))
        return out; // never accept a decision without its callback

    const auto* selected_source=strafe ? &strafe->input : nullptr;
    const double base_x=selected_source ? selected_source->motion_x :
                        accel->input.motion_x;
    const double base_y=selected_source ? selected_source->motion_y :
                        accel->input.motion_y;
    const double base_z=selected_source ? selected_source->motion_z :
                        accel->input.motion_z;
    if (!std::isfinite(base_x) || !std::isfinite(base_y) ||
        !std::isfinite(base_z)) return out;

    // Even an otherwise unselected proposal must be valid: never hide a
    // broken source producer behind priority-based arbitration.
    if (strafe && strafe->decision.proposed_motion) {
        const auto& p=*strafe->decision.proposed_motion;
        if (strafe->executions!=1 ||
            !std::isfinite(p.x) || !std::isfinite(p.y) ||
            !std::isfinite(p.z) || p.y!=base_y) return out;
    }
    if (accel && accel->decision.proposed_motion) {
        const auto& p=*accel->decision.proposed_motion;
        if (accel->executions!=1 ||
            !std::isfinite(p.x) || !std::isfinite(p.y) ||
            !std::isfinite(p.z) || p.y!=base_y) return out;
    }
    // Event-22 source metadata: Strafe priority 100 > Fast Accel 0.
    // This chooses one copied proposal, not an action or claim that
    // commercial Slinky applies the same composition semantics.
    if (strafe && strafe->executions==1 &&
        strafe->decision.proposed_motion) {
        out.owner=MovementProposalOwner::strafe_priority_100;
        out.motion=*strafe->decision.proposed_motion;
    } else if (accel && accel->executions==1 &&
               accel->decision.proposed_motion) {
        out.owner=MovementProposalOwner::fast_accel_priority_0;
        const auto& p=*accel->decision.proposed_motion;
        out.motion=StrafeMotionProposal{p.x,p.y,p.z};
    }
    return out;
}
} // namespace slinkywinky::reference_compat
