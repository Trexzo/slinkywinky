#pragma once
#ifdef _WIN32
#include <string_view>
namespace slinkywinky {
// Windows system font, rendered as GL1.1 grayscale geometry; no font files,
// texture uploads or GL objects. Called on the admitted WGL owner only.
// Returns false if an operating-system font could not be initialized.
bool draw_menu_system_text(std::string_view text,float x,float top,
                           float font_px,float max_width) noexcept;
float measure_menu_system_text(std::string_view text,float font_px) noexcept;
}
#endif
