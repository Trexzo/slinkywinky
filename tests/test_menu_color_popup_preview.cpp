#include "slinkywinky/menu_color_popup_preview.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
using namespace slinkywinky;

// Portable, standalone original-reference *behavior* fixture. No game,
// DLL injection, graphics context, OS color chooser or live input.
int main() {
    MenuColorPopupPreview p;
    assert(!p.active() && !p.current() && !p.close(true));
    assert(!p.open(0x1000000u));
    assert(!MenuColorPopupPreview::parse_hex6("#abcdef"));
    assert(!MenuColorPopupPreview::parse_hex6("abcde"));
    assert(!MenuColorPopupPreview::parse_hex6("abcdeg"));
    assert(MenuColorPopupPreview::parse_hex6("ABCDEF")==0xABCDEFu);
    assert(MenuColorPopupPreview::parse_hex6("123abc")==0x123ABCu);
    assert(p.open(0x8B7CF6u) && !p.open(0xFF00FFu));
    assert(p.hex6() && *p.hex6()=="8b7cf6");
    assert(!p.set_hex6("FFFFF") && p.current()==0x8B7CF6u);
    assert(p.set_hex6("ff0000") && p.current()==0xFF0000u);
    assert(p.hsl() && std::fabs(p.hsl()->hue-0.0f)<0.0001f);
    assert(std::fabs(p.hsl()->saturation-1.0f)<0.0001f);
    assert(std::fabs(p.hsl()->lightness-0.5f)<0.0001f);
    assert(p.set_hsl({1.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x00FF00u);
    assert(p.set_hsl({2.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x0000FFu);
    assert(!p.set_hsl({
        std::numeric_limits<float>::quiet_NaN(),0.5f,0.5f}));
    assert(!p.set_hsl({0.5f,1.1f,0.5f}));
    assert(p.close(false)==std::nullopt);
    assert(p.recent(0)==0x0000FFu); // cancelled edits still in recent
    assert(!p.select_recent(0));
    assert(p.open(0x555555u));
    assert(p.select_recent(0) && p.current()==0x0000FFu);
    assert(p.close(true)==0x0000FFu);
    assert(p.recent(0)==0x0000FFu); // duplicate not reinserted
    for(unsigned i=1;i<=9;++i){
        assert(p.open(i));
        assert(p.close(true)==i);
    }
    assert(p.recent(0)==9u);
    assert(p.recent(7)==2u);
    assert(!p.recent(8));
    for(unsigned c: {
            0x000000u,0xFFFFFFu,0xFF0000u,0x00FF00u,
            0x0000FFu,0x8B7CF6u,0xAB359Du}) {
        const auto h=MenuColorPopupPreview::rgb_to_hsl(c);
        const auto back=MenuColorPopupPreview::hsl_to_rgb(h);
        assert(back && *back==c);
    }
    assert(!p.current());
}
