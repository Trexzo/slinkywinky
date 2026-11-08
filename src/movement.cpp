#include "slinkywinky/movement.hpp"
#include <cmath>
namespace slinkywinky {
bool decide_sprint(const SprintObservation& in,
                   const SprintSettings& cfg) noexcept {
    if(!cfg.enabled || !in.world_valid || !in.hunger_sufficient ||
       in.sneaking || in.collided || in.blinded ||
       !std::isfinite(in.forward) || !std::isfinite(in.strafe) ||
       in.forward < -1.0f || in.forward > 1.0f ||
       in.strafe < -1.0f || in.strafe > 1.0f)
        return false;
    if(in.gui_open && (!in.inventory_open || !cfg.allow_inventory))
        return false;
    if(in.using_item && !cfg.allow_using_item)return false;
    constexpr float threshold=0.01f;
    return in.forward>threshold ||
        (in.forward<-threshold && cfg.allow_backwards) ||
        (std::fabs(in.strafe)>threshold && cfg.allow_sideways);
}
} // namespace slinkywinky
