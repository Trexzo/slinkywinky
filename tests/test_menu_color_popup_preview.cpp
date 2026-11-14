#include "slinkywinky/menu_color_popup_preview.hpp"
#include "slinkywinky/menu_color_modal_geometry.hpp"
#include <stdexcept>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("R198 failed at " + std::to_string(__LINE__) + ": " #x); } while(false)
#include <cmath>
#include <limits>
#include <string>
using namespace slinkywinky;

// Portable, standalone original-reference *behavior* fixture. No game,
// DLL injection, graphics context, OS color chooser or live input.
int main() {
    // R202: source-backed title cross must not intersect hex editing,
    // HSL bars or eight history swatches. There is no footer button.
    const auto dialog=menu_color_modal_geometry(420.0f,380.0f);
    CHECK(dialog.has_value());
    CHECK(dialog->outer.height==294.0f);
    // R203 measured native candidate layout constants: 15px HSL
    // rows and 25px recent colors, no synthetic footer.
    for(const auto& h:dialog->hsl)CHECK(h.height==15.0f);
    for(const auto& h:dialog->recent)CHECK(h.height==25.0f);
    const auto center=[](const MenuColorModalRect& r) {
        return std::array<float,2>{r.x+r.width*0.5f,
                                   r.y+r.height*0.5f};
    };
    const auto x=center(dialog->close);
    CHECK(dialog->close.contains(x[0],x[1]));
    CHECK(!dialog->hex.contains(x[0],x[1]));
    CHECK(!dialog->outer.contains(-1.0f,-1.0f));
    for(const auto& r:dialog->hsl)
        CHECK(!r.contains(x[0],x[1]));
    for(const auto& r:dialog->recent)
        CHECK(!r.contains(x[0],x[1]));
    CHECK(!menu_color_modal_geometry(300.0f,380.0f));
    CHECK(!menu_color_modal_geometry(420.0f,270.0f));
    CHECK(!menu_color_modal_geometry(
        std::numeric_limits<float>::quiet_NaN(),380.0f));
    MenuColorPopupPreview p;
    CHECK(!p.active() && !p.current() && !p.close(true));
    // R199: both supplied references initialize eight BLACK swatches.
    // An all-zero history cell is present, not a missing optional.
    for(unsigned i=0u;i<8u;++i)CHECK(p.recent(i)==0u);
    CHECK(!p.recent(8u));
    CHECK(p.open(0u));
    CHECK(p.close(true)==0u);
    CHECK(p.recent(0)==0u && p.recent(7)==0u);
    CHECK(p.open(0x112233u));
    CHECK(p.close(true)==0x112233u);
    CHECK(p.recent(0)==0x112233u && p.recent(1)==0u);
    CHECK(p.open(0u));
    CHECK(p.close(false)==std::nullopt);
    CHECK(p.recent(0)==0x112233u); // closing black does not reinsert it
    CHECK(p.open(0x555555u));
    CHECK(p.select_recent(1) && p.current()==0u);
    CHECK(p.hsl() && p.hsl()->lightness==0.0f);
    CHECK(p.close(false)==std::nullopt);
    CHECK(p.recent(0)==0x112233u);
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
    // R201: source-constrained six-digit keyboard editing, no partial
    // RGB changes, no invalid character or overflow, and safe blur.
    CHECK(p.begin_hex_edit() && p.hex_editing());
    CHECK(!p.begin_hex_edit());
    CHECK(!p.append_hex_digit('g'));
    CHECK(!p.append_hex_digit('#'));
    CHECK(p.append_hex_digit('1'));
    CHECK(p.append_hex_digit('A'));
    CHECK(p.append_hex_digit('2'));
    CHECK(p.append_hex_digit('b'));
    CHECK(p.append_hex_digit('3'));
    CHECK(p.current()==0xFF0000u);
    CHECK(p.hex_buffer()=="1a2b3");
    CHECK(p.append_hex_digit('c'));
    CHECK(p.hex_buffer()=="1a2b3c" && p.current()==0x1A2B3Cu);
    CHECK(!p.append_hex_digit('d'));
    CHECK(p.erase_hex_digit() && p.hex_buffer()=="1a2b3");
    CHECK(p.current()==0x1A2B3Cu);
    CHECK(p.append_hex_digit('d') && p.current()==0x1A2B3Du);
    CHECK(p.end_hex_edit() && !p.hex_editing());
    CHECK(!p.erase_hex_digit());
    CHECK(p.hex_buffer().empty());
    CHECK(p.set_hex6("ff0000") && p.current()==0xFF0000u);
    CHECK(p.hsl() && std::fabs(p.hsl()->hue-0.0f)<0.0001f);
    CHECK(std::fabs(p.hsl()->saturation-1.0f)<0.0001f);
    CHECK(std::fabs(p.hsl()->lightness-0.5f)<0.0001f);
    CHECK(p.set_hsl({1.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x00FF00u);
    CHECK(p.set_hsl({2.0f/3.0f,1.0f,0.5f}) &&
           p.current()==0x0000FFu);
    // R199: keep precise HSL slider position separate from rounded RGB.
    const MenuPopupHsl fine{0.172345f,0.456789f,0.623457f};
    CHECK(p.set_hsl(fine));
    CHECK(p.hsl() && p.hsl()->hue==fine.hue &&
          p.hsl()->saturation==fine.saturation &&
          p.hsl()->lightness==fine.lightness);
    CHECK(p.set_hex6("0000ff"));
    CHECK(p.hsl() && std::fabs(p.hsl()->hue-2.0f/3.0f)<0.0001f);
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
