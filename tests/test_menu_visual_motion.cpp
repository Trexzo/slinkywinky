#include "slinkywinky/menu_visual_motion.hpp"
#include "slinkywinky/menu_render.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R149 source motion line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    MenuVisualMotionClock timeline;
    CHECK(!timeline.initialized());
    CHECK(!timeline.settled());
    auto first=timeline.advance(MenuTab::Combat,0.0f,630.0f);
    CHECK(timeline.initialized() && !timeline.settled());
    CHECK(first.tab_position==0.0f);
    CHECK(first.opening_offset_y==-50.0f &&
          first.opening_opacity==0.0f);
    CHECK(first.page_offset_y==0.0f && first.page_opacity==1.0f);
    CHECK(menu_visual_motion_valid(first,MenuTab::Combat,630.0f));
    CHECK(!menu_visual_motion_valid(first,MenuTab::Move,630.0f));
    CHECK(!menu_visual_motion_valid(first,MenuTab::Combat,0.0f));
    CHECK(!menu_visual_motion_valid(
        MenuVisualMotion{
            std::numeric_limits<float>::quiet_NaN(),
            0.0f,1.0f,0.0f,1.0f,false},
        MenuTab::Combat,630.0f));
    const auto ignored=timeline.advance(
        MenuTab::Combat,-0.5f,630.0f);
    CHECK(ignored.tab_position==first.tab_position &&
          ignored.opening_opacity==first.opening_opacity);
    const auto atFifty=timeline.advance(
        MenuTab::Combat,0.05f,630.0f);
    CHECK(atFifty.opening_opacity>0.45f &&
          atFifty.opening_opacity<0.51f);
    CHECK(atFifty.opening_offset_y<0.0f &&
          atFifty.opening_offset_y>-30.0f);
    CHECK(!atFifty.settled);
    CHECK(menu_visual_motion_valid(atFifty,MenuTab::Combat,630.0f));
    // The source tab lerp advances (target-anim)*min(1,dt/.05).
    const auto moving=timeline.advance(MenuTab::Move,0.01f,630.0f);
    CHECK(std::fabs(moving.tab_position-0.2f)<0.0001f);
    CHECK(std::fabs(moving.page_offset_y+50.4f)<0.01f);
    CHECK(std::fabs(moving.page_opacity-0.0f)<0.001f);
    CHECK(menu_visual_motion_valid(moving,MenuTab::Move,630.0f));
    CHECK(!moving.settled);
    // Visual state cannot be the source of input; the model only
    // snapshots presentation values, never updates MenuState::tab().
    MenuState menu;
    CHECK(menu.tab()==MenuTab::Combat);
    auto completed=timeline.advance(MenuTab::Move,0.05f,630.0f);
    CHECK(completed.tab_position==1.0f);
    CHECK(completed.page_offset_y==0.0f);
    CHECK(completed.page_opacity==1.0f);
    CHECK(!completed.settled); // the opening animation is still active
    for(int i=0;i<4;++i)
        completed=timeline.advance(MenuTab::Move,0.05f,630.0f);
    CHECK(completed.settled);
    CHECK(completed.opening_offset_y==0.0f &&
          completed.opening_opacity==1.0f);
    CHECK(timeline.settled() && menu.tab()==MenuTab::Combat);
    CHECK(menu_visual_motion_valid(completed,MenuTab::Move,630.0f));
    // Switching back from tab 1 immediately follows the 50ms source
    // easing; page moves in the opposite direction and is 50% opaque.
    const auto reversed=timeline.advance(MenuTab::Combat,0.025f,630.0f);
    CHECK(std::fabs(reversed.tab_position-0.5f)<0.0001f);
    CHECK(std::fabs(reversed.page_offset_y-31.5f)<0.01f);
    CHECK(std::fabs(reversed.page_opacity-0.0f)<0.01f);
    CHECK(menu_visual_motion_valid(reversed,MenuTab::Combat,630.0f));
    const auto halfHeight=timeline.advance(
        MenuTab::Combat,0.0f,315.0f);
    CHECK(std::fabs(halfHeight.page_offset_y-15.75f)<0.01f);
    CHECK(menu_visual_motion_valid(halfHeight,MenuTab::Combat,315.0f));
    // Invalid tab and invalid delta are ignored without corrupting a
    // previously valid model snapshot or running external timers.
    const auto bad=timeline.advance(
        MenuTab::Unload,std::numeric_limits<float>::infinity(),
        315.0f);
    CHECK(bad.tab_position==halfHeight.tab_position);
    timeline.reset();
    CHECK(!timeline.initialized() && !timeline.settled());
    return 0;
}
