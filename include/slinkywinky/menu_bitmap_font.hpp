#pragma once
#include <array>
#include <cstdint>

namespace slinkywinky {

// First-party small ASCII letterforms. Geometry only; no external fonts,
// textures or proprietary glyph assets. Rows use five low bits, MSB on left.
// Lowercase ASCII maps to uppercase for consistent UI labels.
std::array<std::uint8_t,7> menu_ascii_glyph(char character) noexcept;

} // namespace slinkywinky
