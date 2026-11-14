#include "slinkywinky/menu_color_popup_preview.hpp"
#include <stdexcept>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("R198 failed at " + std::to_string(__LINE__) + ": " #x); } while(false)
#include <cmath>
#include <limits>
#include <string>
using namespace slinkywinky;

// Portable, standalone original-reference *behavior* fixture. No game,
// DLL injection, graphics context, OS color chooser or live input.
int main() {
    MenuColorPopupPreview p;
    CHECK(!p.active() && !p.current() && !p.close(true));
    CHECK(!p.open(0x1000000u));
    CHECK(!MenuColorPopupPreview::parse_hex6("#abcdef"));
    CHECK(!MenuColorPopupPreview::parse_hex6("abcde"));
    CHECK(!MenuColorPopupPreview::parse_hex6("abcdeg"));
    CHECK(MenuColorPopupPreview::parse_hex6("ABCDEF")==0xABCDEFu);
    CHECK(MenuColorPopupPreview::parse_hex6("123abc")==0x123ABCu);
    CHECK(p.open(0x8B7CF6u) && !p.open(0xFF00FFu));
    CHECK(p.hex6() && *p.hex6()=="8b7cf6");
    CHECK(!p.set_hex6("FFFFF") && p.current()==0x8B7CF6u);
    CHECK(p.set_hex6("ff0000") && p.current()==0xFF0000u);
    CHECK(p.hsl() && std::fabs(p.hsl()->hue-0.0f)<0.0001f);
    CHECK(std::fabs(p.hsl()->saturation-1.0f)<0.0001f);
    CHECK(std::fabs(p.hsl()->lightness-0.5f)<0.0001f);
    CHECK(p.set_hsl({1.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x00FF00u);
    CHECK(p.set_hsl({2.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x0000FFu);
    CHECK(!p.set_hsl({
        std::numeric_limits<float>::quiet_NaN(),0.5f,0.5f}));
    CHECK(!p.set_hsl({0.5f,1.1f,0.5f}));
    CHECK(p.close(false)==std::nullopt);
    CHECK(p.recent(0)==0x0000FFu); // cancelled edits still in recent
    CHECK(!p.select_recent(0));
    CHECK(p.open(0x555555u));
    CHECK(p.select_recent(0) && p.current()==0x0000FFu);
    CHECK(p.close(true)==0x0000FFu);
    CHECK(p.recent(0)==0x0000FFu); // duplicate not reinserted
    for(unsigned i=1;i<=9;++i){
        CHECK(p.open(i));
        CHECK(p.close(true)==i);
    }
    CHECK(p.recent(0)==9u);
    CHECK(p.recent(7)==2u);
    CHECK(!p.recent(8));
    for(unsigned c: {
            0x000000u,0xFFFFFFu,0xFF0000u,0x00FF00u,
            0x0000FFu,0x8B7CF6u,0xAB359Du}) {
        const auto h=MenuColorPopupPreview::rgb_to_hsl(c);
        const auto back=MenuColorPopupPreview::hsl_to_rgb(h);
        CHECK(back && *back==c);
    }
    CHECK(!p.current());
}
