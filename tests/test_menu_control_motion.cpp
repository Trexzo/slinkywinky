#include "slinkywinky/menu_control_motion.hpp"
#include "slinkywinky/menu.hpp"
#include <limits>
#include <cstdio>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,    "R150 FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(false)
using namespace slinkywinky;
int main() {
    MenuControlMotionClock c;
    MenuState menu;
    CHECK(c.settled() && menu_control_motion_valid(c.value()));
    CHECK(c.retarget(true,19u));
    CHECK(!c.value().hovered_boolean && c.value().popup_opacity==0.0f);
    CHECK(c.advance(0.06f));
    CHECK(c.value().popup_opacity>0.0f && c.value().popup_opacity<1.0f);
    CHECK(!c.retarget(true,std::nullopt));
    for(int i=0;i<6;++i)CHECK(c.advance(0.1f) || c.settled());
    CHECK(c.settled() && c.value().popup_opacity==1.0f);
    CHECK(c.retarget(false,19u));
    CHECK(c.value().popup_opacity==1.0f);
    CHECK(c.value().hovered_boolean==19u);
    CHECK(c.advance(0.05f));
    CHECK(c.value().switch_hover_opacity>0.0f &&
          c.value().switch_hover_opacity<1.0f);
    for(int i=0;i<8;++i)c.advance(0.1f);
    CHECK(c.settled() && c.value().switch_hover_opacity==1.0f);
    CHECK(menu_control_hover_tint(0x404040u,1.0f)!=0x404040u);
    CHECK(menu_control_hover_tint(0x404040u,0.0f)==0x404040u);
    CHECK(menu_control_hover_tint(0x404040u,
        std::numeric_limits<float>::quiet_NaN())==0x404040u);
    CHECK(c.retarget(false,std::nullopt));
    CHECK(c.value().hovered_boolean==19u); // exit fade
    CHECK(c.advance(0.05f));
    CHECK(c.value().switch_hover_opacity>0.0f &&
          c.value().switch_hover_opacity<1.0f);
    for(int i=0;i<8;++i)c.advance(0.1f);
    CHECK(c.settled() && !c.value().hovered_boolean);
    CHECK(c.retarget(false,20u));
    CHECK(c.retarget(false,21u));
    CHECK(c.value().hovered_boolean==21u);
    CHECK(c.value().switch_hover_opacity==0.0f);
    CHECK(c.retarget(false,385u) && c.settled());
    CHECK(!c.value().hovered_boolean);
    CHECK(c.retarget(false,std::nullopt,2u));
    CHECK(c.value().hovered_expand==2u);
    CHECK(!c.retarget(false,std::nullopt,2u));
    CHECK(c.retarget(false,std::nullopt,66u));
    CHECK(c.value().hovered_expand==66u);
    CHECK(c.retarget(false,std::nullopt,67u));
    CHECK(!c.value().hovered_expand);
    CHECK(c.retarget(false,21u,2u));
    CHECK(c.value().hovered_expand==2u);
    CHECK(c.retarget(true,21u,2u));
    CHECK(!c.value().hovered_expand);
    CHECK(c.value().popup_opacity==0.0f);
    CHECK(!c.advance(std::numeric_limits<float>::infinity()));
    CHECK(!c.advance(-1.0f));
    CHECK(menu_control_center_text_y(10.0f,22.0f,12.0f)==15.0f);
    CHECK(menu_control_center_text_y(5.0f,10.0f,15.0f)==5.0f);
    CHECK(!menu_control_motion_valid(MenuControlMotion{385u,1.0f,1.0f}));
    CHECK(!menu_control_motion_valid(MenuControlMotion{
        std::nullopt,0.0f,1.0f,67u}));
    CHECK(menu_control_motion_valid(MenuControlMotion{
        std::nullopt,0.0f,1.0f,2u}));
    CHECK(!menu_control_motion_valid(MenuControlMotion{
        std::nullopt,0.5f,1.0f}));
    CHECK(!menu_control_motion_valid(MenuControlMotion{
        std::nullopt,0.0f,
        std::numeric_limits<float>::quiet_NaN()}));
    CHECK(menu.tab()==MenuTab::Combat && !menu.page().empty());
    return 0;
}
