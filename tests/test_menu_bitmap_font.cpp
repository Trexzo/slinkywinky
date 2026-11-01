#include "slinkywinky/menu_bitmap_font.hpp"
#include "slinkywinky/menu.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>

#define CHECK(x) do {if(!(x))throw std::runtime_error("R68 bitmap glyph check: " #x);}while(false)

int main() {
    // All documented public ASCII module/tab labels must be rendered with
    // deterministic first-party data and no fonts or platform APIs.
    std::size_t pixels=0;
    const auto check_string=[&](const std::string& text) {
        for(char c:text) {
            const auto glyph=slinkywinky::menu_ascii_glyph(c);
            std::size_t glyphPixels=0;
            for(std::uint8_t row:glyph) {
                CHECK((row&0xe0u)==0); // five-bit wide, seven rows
                for(unsigned column=0;column<5;++column) {
                    if((row&(1u<<(4-column)))!=0u) {
                        ++glyphPixels;
                        ++pixels;
                    }
                }
            }
            if(c!=' ')CHECK(glyphPixels>0);
        }
    };
    for(const auto& module:slinkywinky::official_module_catalog())
        check_string(module.name);
    for(int i=0;i<7;++i)
        check_string(slinkywinky::menu_tab_name(
            static_cast<slinkywinky::MenuTab>(i)));
    check_string("SLINKYWINKY - PREVIEW ONLY / UI DEMO");
    CHECK(pixels>2000);
    CHECK(slinkywinky::menu_ascii_glyph('a')==
          slinkywinky::menu_ascii_glyph('A'));
    CHECK(slinkywinky::menu_ascii_glyph('z')==
          slinkywinky::menu_ascii_glyph('Z'));
    CHECK(slinkywinky::menu_ascii_glyph(' ')==
          std::array<std::uint8_t,7>{{0,0,0,0,0,0,0}});
    CHECK(slinkywinky::menu_ascii_glyph('~')==
          slinkywinky::menu_ascii_glyph('?'));
    return 0;
}
