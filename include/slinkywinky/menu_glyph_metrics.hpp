#pragma once
#include <cstdint>
#include <optional>
namespace slinkywinky {
// Platform-neutral GDI grayscale layout validation. Zero-byte outlines
// may retain a nonzero black box (e.g. whitespace): preserve the advance
// but normalize coverage dimensions to zero. No font files or GL calls.
struct MenuGlyphRasterLayout {
    unsigned width=0,height=0,pitch=0,advance=0;
};
inline std::optional<MenuGlyphRasterLayout> menu_glyph_raster_layout(
    std::uint32_t width,std::uint32_t height,std::int32_t advance,
    std::uint32_t bytes) noexcept {
    if(width>80u || height>80u || advance<=0 || advance>80 || bytes>16384u)
        return std::nullopt;
    if(bytes==0u)return MenuGlyphRasterLayout{0,0,0,static_cast<unsigned>(advance)};
    if(width==0u || height==0u)return std::nullopt;
    const auto pitch=(width+3u)&~3u;
    if(pitch*height>bytes)return std::nullopt;
    return MenuGlyphRasterLayout{width,height,pitch,static_cast<unsigned>(advance)};
}
}
