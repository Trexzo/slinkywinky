#pragma once
#include <array>
#include <cstddef>
#include <string_view>
namespace slinkywinky {
// R116: Recovered widget *presentation* metadata from the user's candidate
// decrypted archive re/mods_block.js. Names/control kinds/groupings are
// useful forensic hints; numeric defaults/ranges are fixture guesses and
// NOT authenticated or live game configurations. No executable code copied.
enum class RecoveredWidgetKind : unsigned char {
    Boolean,Slider,Dropdown,Range,Color,MultiSelect,Text,Button
};
struct RecoveredWidget {
    const char* module;
    const char* group;
    const char* label;
    RecoveredWidgetKind kind;
    const char* options; // pipe-separated reference labels, display-only
    const char* auxiliary; // hex swatch / placeholder, display-only
    float min_value, max_value, step, example_value, example_other;
    bool example_enabled;
};
const std::array<RecoveredWidget,378>& recovered_widget_catalog() noexcept;
std::size_t recovered_widget_count(std::string_view module) noexcept;
std::size_t recovered_widget_preview(const char* module,
                                     std::array<const RecoveredWidget*,5>& output) noexcept;
// Page is five controls, offset must be five-aligned and in range.
// Returns the total module control count, or zero for invalid admission.
std::size_t recovered_widget_page(const char* module,std::size_t offset,
                                  std::array<const RecoveredWidget*,5>& output) noexcept;
} // namespace slinkywinky
