#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_control_motion.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x))throw std::runtime_error("R151 CSS "+std::to_string(__LINE__));} while(false)
using namespace slinkywinky;
int main(){
    for(const float width:{430.0f,600.0f,900.0f}){
        const float x=37.0f;
        const auto c=menu_setting_column(x,width);
        CHECK(std::fabs(c.x-(x+width*0.5f))<0.001f);
        CHECK(std::fabs(c.width-(width*0.5f-8.0f))<0.001f);
        CHECK(std::fabs(menu_setting_switch_x(c)+40.0f-(x+width-8.0f))<0.001f);
    }
    CHECK(menu_setting_row_height==20.0f);
    CHECK(menu_setting_switch_width==40.0f);
    CHECK(menu_setting_control_font==15.0f);
    CHECK(menu_setting_column(0.0f,10.0f).width==0.0f);
    CHECK(menu_setting_column(std::numeric_limits<float>::infinity(),430.0f).width==0.0f);
    CHECK(menu_control_hover_tint(0x404040u,1.0f)==0x505050u);
    CHECK(menu_control_hover_tint(0x404040u,0.0f)==0x404040u);
    CHECK(menu_control_hover_tint(0x404040u,0.5f)==0x484848u);
    return 0;
}
