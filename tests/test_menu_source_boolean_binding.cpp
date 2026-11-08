#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#define CHECK(x) do {if(!(x))throw std::runtime_error(    "R156 Boolean GUI binding line "+std::to_string(__LINE__));}while(false)
using namespace slinkywinky;
namespace {
std::size_t find(std::string_view module,std::string_view group,
                 std::string_view label) {
    const auto& source=recovered_widget_catalog();
    for(std::size_t i=0;i<source.size();++i)
        if(std::string_view(source[i].module)==module &&
           std::string_view(source[i].group)==group &&
           std::string_view(source[i].label)==label &&
           source[i].kind==RecoveredWidgetKind::Boolean)return i;
    throw std::runtime_error("source Boolean not found");
}
bool visible(const MenuState& menu,std::size_t module,std::size_t widget) {
    const auto& all=recovered_widget_catalog();
    const auto& catalog=official_module_catalog();
    if(module>=catalog.size() || widget>=all.size())return false;
    std::array<const RecoveredWidget*,5> page{};
    if(!recovered_widget_visible_page(catalog[module].name,
        menu.settings_offset(module),static_cast<int>(menu.aim_preview_mode()),
        static_cast<int>(menu.target_preview_mode()),page))return false;
    for(const auto* candidate:page)
        if(candidate==&all[widget])return true;
    return false;
}
void expose(MenuState& menu,std::size_t module,std::size_t widget) {
    if(!menu.expanded(module))CHECK(menu.toggle_expanded(module));
    for(int i=0;i<15 && !visible(menu,module,widget);++i)
        CHECK(menu.change_settings_page(module,1));
    CHECK(visible(menu,module,widget));
}
}
int main() {
    const auto aim=find("Aim Assist","Conditions","Mouse pressed");
    const auto block=find("Auto Block","Conditions","Damaged");
    const auto reach=find("Reach","Conditions","Sprinting");
    const auto other=find("Auto Clicker","In-game","Randomize");
    MenuState menu;
    MenuProfileBridge profile(menu);
    CHECK(!profile.recovered_boolean_setting(other));
    CHECK(profile.recovered_boolean_setting(aim)==false);
    CHECK(profile.recovered_boolean_setting(block)==false);
    CHECK(profile.recovered_boolean_setting(reach)==false);
    CHECK(!profile.toggle_recovered_boolean(0,aim)); // not expanded
    CHECK(!profile.toggle_recovered_boolean(0,recovered_widget_catalog().size()));
    expose(menu,0,aim);
    CHECK(profile.toggle_recovered_boolean(0,aim));
    CHECK(profile.settings().combat.aim_requires_mouse);
    CHECK(menu.recovered_boolean_override(aim)==true);
    CHECK(profile.toggle_recovered_boolean(0,aim));
    CHECK(!profile.settings().combat.aim_requires_mouse);
    CHECK(menu.recovered_boolean_override(aim)==false);
    CHECK(profile.toggle_recovered_boolean(0,aim));
    CHECK(!profile.toggle_recovered_boolean(1,aim)); // wrong module
    CHECK(!profile.toggle_recovered_boolean(1,block)); // not expanded
    expose(menu,1,block);
    CHECK(profile.toggle_recovered_boolean(1,block));
    CHECK(profile.settings().combat.block_requires_damage);
    CHECK(menu.recovered_boolean_override(block)==true);
    MenuRenderSession frames;
    CHECK(frames.attach(157,900,630));
    auto frame=frames.next(menu,&profile);
    CHECK(frame && frame->recovered_boolean_overrides[aim]==true &&
          frame->recovered_boolean_overrides[block]==true);
    CHECK(profile.scroll_by(9));
    CHECK(!profile.toggle_recovered_boolean(1,block)); // offscreen
    expose(menu,12,reach);
    CHECK(profile.toggle_recovered_boolean(12,reach));
    CHECK(profile.settings().combat.reach.require_sprint);
    CHECK(menu.recovered_boolean_override(reach)==true);
    CHECK(profile.select(MenuTab::Move));
    CHECK(!profile.toggle_recovered_boolean(12,reach)); // wrong tab
    CHECK(profile.select(MenuTab::Combat));
    CHECK(!profile.toggle_recovered_boolean(12,reach)); // offscreen
    CHECK(profile.scroll_by(9));
    CHECK(profile.toggle_recovered_boolean(12,reach)); // visible again
    CHECK(!profile.settings().combat.reach.require_sprint);
    CHECK(profile.toggle_recovered_boolean(12,reach));
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r156-native-boolean.ini";
    std::error_code ec;
    std::filesystem::remove(file,ec);
    CHECK(profile.save(file));
    MenuState restoredMenu;
    MenuProfileBridge restored(restoredMenu);
    CHECK(restored.load(file));
    CHECK(restored.recovered_boolean_setting(aim)==true);
    CHECK(restored.recovered_boolean_setting(block)==true);
    CHECK(restored.recovered_boolean_setting(reach)==true);
    MenuRenderSession restoredFrames;
    CHECK(restoredFrames.attach(158,900,630));
    auto restoredFrame=restoredFrames.next(restoredMenu,&restored);
    CHECK(restoredFrame);
    CHECK(restoredFrame->recovered_boolean_overrides[aim]==true);
    CHECK(restoredFrame->recovered_boolean_overrides[block]==true);
    CHECK(restoredFrame->recovered_boolean_overrides[reach]==true);
    expose(restoredMenu,0,aim);
    CHECK(restored.toggle_recovered_boolean(0,aim));
    CHECK(restored.recovered_boolean_setting(aim)==false);
    CHECK(restored.load(file));
    restoredFrame=restoredFrames.next(restoredMenu,&restored);
    CHECK(restoredFrame->recovered_boolean_overrides[aim]==true);
    {std::ofstream broken(file,std::ios::binary|std::ios::trunc);
     broken<<"version=1\naim_requires_mouse=INVALID\n";}
    CHECK(!restored.load(file));
    CHECK(restored.recovered_boolean_setting(aim)==true);
    CHECK(frames.detach() && restoredFrames.detach());
    std::filesystem::remove(file,ec);
    std::filesystem::remove(file.string()+".bak",ec);
    std::filesystem::remove(file.string()+".tmp",ec);
    return 0;
}
