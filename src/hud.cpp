#include "slinkywinky/hud.hpp"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace slinkywinky {
namespace {
std::string fixed(double value, int precision) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}
bool coherent(const ObservationStatus& s) {
    if ((s.flags & ~7u) != 0 ||
        ((s.flags & 2u) && !(s.flags & 1u)) ||
        ((s.flags & 4u) && (s.flags & 3u) != 3u) ||
        !std::isfinite(s.target_distance) || s.target_distance < 0.0 ||
        !std::isfinite(s.eye_y) || !std::isfinite(s.yaw) ||
        !std::isfinite(s.pitch))
        return false;
    if (s.valid_ticks == 0 || s.generation == 0)
        return false;
    return true;
}
void add(HudPanel& panel,const char* label,std::string value) {
    // Hard cap prevents accidental unbounded HUD content.
    if (panel.rows.size() < 8) panel.rows.push_back({label,std::move(value)});
}
} // namespace

HudPanel make_hud_panel(const std::optional<ObservationStatus>& observation,
                        HudOptions options) {
    HudPanel panel;
    panel.title = "SLINKYWINKY";
    panel.subtitle = "READ-ONLY  /  NATIVE DIAGNOSTICS";
    if (!observation) {
        panel.state = "NOT ATTACHED";
        add(panel,"Status","No observation session");
        return panel;
    }
    if (!coherent(*observation)) {
        panel.state = "INVALID TELEMETRY";
        add(panel,"Status","Discarded malformed observation");
        return panel;
    }
    const auto& s=*observation;
    panel.has_data = true;
    panel.in_world = (s.flags & 3u) == 3u;
    panel.has_ray_target = (s.flags & 4u) != 0;
    if (panel.has_ray_target) panel.state="RAY TARGET OBSERVED";
    else if (panel.in_world) panel.state="IN WORLD";
    else if (s.flags & 1u) panel.state="WORLD LOADING";
    else panel.state="MAIN MENU";

    add(panel,"Accepted ticks",std::to_string(s.valid_ticks));
    add(panel,"World transitions",std::to_string(s.session_transitions));
    if (!options.compact) {
        add(panel,"Generation",std::to_string(s.generation));
        add(panel,"World",(s.flags & 1u) ? "Available" : "Unavailable");
        add(panel,"Player",(s.flags & 2u) ? "Available" : "Unavailable");
    }
    if (panel.in_world) {
        if (options.show_eye_y) add(panel,"Player eye Y",fixed(s.eye_y,2));
        if (options.show_angles)
            add(panel,"Yaw / pitch",fixed(s.yaw,1)+" / "+fixed(s.pitch,1));
        add(panel,"Ray distance",panel.has_ray_target ?
            fixed(s.target_distance,2)+" blocks" : "No ray-selected entity");
    } else {
        add(panel,"Ray distance","N/A");
    }
    return panel;
}
} // namespace slinkywinky
