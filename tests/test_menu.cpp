#include "slinkywinky/menu.hpp"
#include <set>
#include <stdexcept>
#include <cmath>
#include <limits>
#define CHECK(x) do {if(!(x))throw std::runtime_error("Menu test failure: " #x);}while(false)

using namespace slinkywinky;
int main() {
    const auto& catalog=official_module_catalog();
    CHECK(catalog.size()==65);
    std::set<std::string> unique;
    std::size_t counts[5]{};
    for(const auto& entry:catalog){
        CHECK(entry.name!=nullptr && std::string(entry.name).size()>1);
        CHECK(unique.insert(entry.name).second);
        auto category=static_cast<std::size_t>(entry.tab);
        CHECK(category<5);
        ++counts[category];
    }
    CHECK(counts[0]==17 && counts[1]==10 && counts[2]==15 &&
          counts[3]==7 && counts[4]==16);
    CHECK(std::string(menu_tab_name(MenuTab::Combat))=="Combat");
    CHECK(std::string(menu_tab_name(MenuTab::Profiles))=="Profiles");
    CHECK(std::string(menu_tab_name(MenuTab::Count))=="Unknown");

    MenuState state;
    CHECK(state.tab()==MenuTab::Combat && state.scroll()==0);
    CHECK(state.tab_count(MenuTab::Combat)==17);
    CHECK(state.tab_count(MenuTab::Block)==7);
    CHECK(state.tab_count(MenuTab::Profiles)==0);
    CHECK(state.page().size()==8);
    CHECK(state.page(200).size()==8); // bounded presentation
    CHECK(state.page(2).size()==2);
    CHECK(state.page(0).empty());
    CHECK(!state.preview_enabled(0) && !state.expanded(0));
    CHECK(!state.toggle_preview(65) && !state.toggle_expanded(65));
    CHECK(!state.toggle_preview(17)); // cannot mutate off-category entries
    CHECK(!state.preview_enabled(17));
    CHECK(state.toggle_preview(0) && state.preview_enabled(0));
    CHECK(state.toggle_expanded(0) && state.expanded(0));
    CHECK(state.page()[0].preview_enabled && state.page()[0].expanded);
    CHECK(state.toggle_preview(0) && !state.preview_enabled(0));
    CHECK(!state.scroll_by(-20));
    CHECK(state.scroll_by(9999));
    CHECK(state.scroll()==9 && state.page().size()==8);
    CHECK(std::string(state.page().front().title)=="Knockback Displace");
    CHECK(!state.scroll_by(2));
    CHECK(state.scroll_by(-99999) && state.scroll()==0);
    CHECK(state.select(MenuTab::Move) && state.scroll()==0);
    CHECK(state.page().size()==8);
    CHECK(state.page().front().title=="Fast Accel");
    CHECK(!state.toggle_preview(0)); // deny cross-tab modification
    CHECK(state.select(MenuTab::Block) && state.page().size()==7);
    CHECK(!state.scroll_by(100));
    CHECK(state.select(MenuTab::Profiles) && state.page().empty());
    CHECK(!state.select(MenuTab::Unload) && state.tab()==MenuTab::Profiles);
    CHECK(!state.select(MenuTab::Count) && state.tab()==MenuTab::Profiles);
    CHECK(state.select(MenuTab::Combat) && state.expanded(0));
    CHECK(!state.appearance().compact && !state.appearance().wide);
    CHECK(!state.appearance().allow_input_while_open);
    CHECK(state.appearance().accent_rgb==0xE88C46u);
    state.set_compact(true);
    state.set_wide(true);
    state.set_allow_input(true);
    CHECK(state.appearance().compact && state.appearance().wide &&
          state.appearance().allow_input_while_open);
    CHECK(state.set_scale(0.65f) && state.set_scale(1.8f));
    CHECK(!state.set_scale(0.64f) && !state.set_scale(1.81f));
    CHECK(!state.set_scale(std::numeric_limits<float>::quiet_NaN()));
    CHECK(!state.set_scale(std::numeric_limits<float>::infinity()));
    CHECK(std::abs(state.appearance().scale-1.8f)<0.001f);
    CHECK(state.set_accent(0xFF7D30u));
    CHECK(!state.set_accent(0x1000000u));
    CHECK(state.appearance().accent_rgb==0xFF7D30u);
    // UI is strictly an inert model: no game/JVM or network dependencies.
    return 0;
}
