#pragma once
#include "slinkywinky/observation.hpp"
#include <optional>
#include <string>
#include <vector>

namespace slinkywinky {

// Immutable display-only data. Not connected to game input, render hooks,
// packet modification or a JVM.
struct HudRow {
    std::string label;
    std::string value;
};
struct HudPanel {
    std::string title;
    std::string subtitle;
    std::string state;
    std::vector<HudRow> rows;
    bool has_data = false;
    bool in_world = false;
    bool has_ray_target = false;
};
struct HudOptions {
    bool show_angles = true;
    bool show_eye_y = true;
    bool compact = false;
};

// Builds bounded, locale-independent plain text from a validated native
// observation. Never represents ray distance as combat reach or visibility.
HudPanel make_hud_panel(const std::optional<ObservationStatus>& observation,
                        HudOptions options = {});

} // namespace slinkywinky
