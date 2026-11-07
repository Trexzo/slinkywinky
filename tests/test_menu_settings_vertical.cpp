#include "slinkywinky/menu_settings_vertical.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_render.hpp"
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    "R152 CSS settings layout "+std::to_string(__LINE__));} while(false)
using namespace slinkywinky;
int main() {
    const auto& catalog=recovered_widget_catalog();
    const RecoveredWidget *first=nullptr,*same=nullptr,*other=nullptr;
    for(const auto& x:catalog) {
        if(!first)first=&x;
        else if(!same && std::string_view(x.group)==first->group)same=&x;
        else if(!other && std::string_view(x.group)!=first->group)other=&x;
        if(first && same && other)break;
    }
    CHECK(first && same && other);
    std::array<const RecoveredWidget*,5> page{first,same,other,same,other};
    CHECK(menu_settings_section_changes(page,0)==0u);
    CHECK(menu_settings_section_changes(page,1)==0u);
    CHECK(menu_settings_section_changes(page,2)==1u);
    CHECK(menu_settings_control_offset(page,0)==35.0f);
    CHECK(menu_settings_control_offset(page,1)==67.0f);
    CHECK(menu_settings_control_offset(page,2)==130.0f);
    CHECK(menu_settings_section_text_offset(page,0)==8.0f);
    CHECK(menu_settings_section_text_offset(page,2)==103.0f);
    CHECK(menu_settings_expanded_extra(page,2,false)==95.0f);
    CHECK(menu_settings_next_row_offset(page,2)==99.0f);
    CHECK(menu_settings_expanded_extra(page,3,false)==158.0f);
    CHECK(menu_settings_expanded_extra(page,3,true)==190.0f);
    CHECK(menu_settings_control_offset(page,3)==193.0f);
    // Verify both compact and regular add exactly 20px row heights
    // and 12px gaps, not the old 27px approximation.
    for(const bool compact:{false,true}) {
        MenuRenderFrame frame;
        frame.appearance.compact=compact;
        CHECK(menu_row_body_height(frame)==(compact?36.0f:68.0f));
        const float body=menu_row_body_height(frame);
        CHECK(body+menu_settings_control_offset(page,0) ==
              (compact?71.0f:103.0f));
    }
    return 0;
}
