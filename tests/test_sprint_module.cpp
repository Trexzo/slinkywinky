#include "slinkywinky/movement.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#define CHECK(x) do {if(!(x))throw std::runtime_error(    "R157 Sprint intent line "+std::to_string(__LINE__));}while(false)
using namespace slinkywinky;
namespace {
std::size_t find(std::string_view label) {
    const auto& all=recovered_widget_catalog();
    for(std::size_t i=0;i<all.size();++i)
        if(std::string_view(all[i].module)=="Sprint" &&
           std::string_view(all[i].group)=="Allow while" &&
           std::string_view(all[i].label)==label &&
           all[i].kind==RecoveredWidgetKind::Boolean)return i;
    throw std::runtime_error("missing candidate Sprint widget");
}
}
int main() {
    SprintObservation obs;
    SprintSettings cfg;
    obs.world_valid=true;
    obs.hunger_sufficient=true;
    obs.forward=1.0f;
    CHECK(!decide_sprint(obs,cfg));
    cfg.enabled=true;
    CHECK(decide_sprint(obs,cfg));
    obs.world_valid=false;CHECK(!decide_sprint(obs,cfg));obs.world_valid=true;
    obs.hunger_sufficient=false;CHECK(!decide_sprint(obs,cfg));obs.hunger_sufficient=true;
    obs.sneaking=true;CHECK(!decide_sprint(obs,cfg));obs.sneaking=false;
    obs.collided=true;CHECK(!decide_sprint(obs,cfg));obs.collided=false;
    obs.blinded=true;CHECK(!decide_sprint(obs,cfg));obs.blinded=false;
    obs.gui_open=true;CHECK(!decide_sprint(obs,cfg));
    obs.inventory_open=true;CHECK(!decide_sprint(obs,cfg));
    cfg.allow_inventory=true;CHECK(decide_sprint(obs,cfg));
    obs.inventory_open=false;CHECK(!decide_sprint(obs,cfg));
    obs.gui_open=false;
    obs.using_item=true;CHECK(!decide_sprint(obs,cfg));
    cfg.allow_using_item=true;CHECK(decide_sprint(obs,cfg));
    obs.using_item=false;
    obs.forward=-1.0f;CHECK(!decide_sprint(obs,cfg));
    cfg.allow_backwards=true;CHECK(decide_sprint(obs,cfg));
    obs.forward=0.0f;
    obs.strafe=1.0f;CHECK(!decide_sprint(obs,cfg));
    cfg.allow_sideways=true;CHECK(decide_sprint(obs,cfg));
    obs.strafe=0.0f;CHECK(!decide_sprint(obs,cfg));
    obs.forward=std::numeric_limits<float>::quiet_NaN();
    CHECK(!decide_sprint(obs,cfg));
    obs.forward=2.0f;CHECK(!decide_sprint(obs,cfg));
    obs.forward=1.0f;
    CHECK(decide_sprint(obs,cfg));
    cfg.enabled=false;CHECK(!decide_sprint(obs,cfg));

    MenuState menu;
    MenuProfileBridge profile(menu);
    CHECK(!profile.enabled(25));
    CHECK(!profile.toggle(25)); // wrong tab and offscreen
    CHECK(profile.select(MenuTab::Move));
    CHECK(!profile.toggle(25)); // offscreen before scrolling
    CHECK(profile.scroll_by(1));
    CHECK(profile.toggle(25));
    CHECK(profile.settings().sprint.enabled && profile.enabled(25));
    const auto inv=find("In inventory");
    const auto item=find("Using item");
    const auto backwards=find("Backwards");
    const auto sideways=find("Sideways");
    CHECK(!profile.toggle_recovered_boolean(25,inv)); // collapsed
    CHECK(menu.toggle_expanded(25));
    CHECK(profile.toggle_recovered_boolean(25,inv));
    CHECK(profile.toggle_recovered_boolean(25,item));
    CHECK(profile.toggle_recovered_boolean(25,backwards));
    CHECK(profile.toggle_recovered_boolean(25,sideways));
    const auto& sprint=profile.settings().sprint;
    CHECK(sprint.allow_inventory && sprint.allow_using_item &&
          sprint.allow_backwards && sprint.allow_sideways);
    CHECK(profile.recovered_boolean_setting(inv)==true);
    CHECK(!profile.toggle_recovered_boolean(17,inv)); // mismatched owner
    CHECK(!profile.toggle_recovered_boolean(25,400u));
    MenuRenderSession frames;
    CHECK(frames.attach(157,900,630));
    const auto frame=frames.next(menu,&profile);
    CHECK(frame);
    CHECK(frame->recovered_boolean_overrides[inv]==true);
    CHECK(frame->recovered_boolean_overrides[item]==true);
    CHECK(frame->recovered_boolean_overrides[backwards]==true);
    CHECK(frame->recovered_boolean_overrides[sideways]==true);
    CHECK(profile.toggle(25) && !profile.enabled(25));
    CHECK(!decide_sprint(obs,profile.settings().sprint));
    CHECK(profile.toggle(25) && profile.enabled(25));
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r157-sprint-profile.ini";
    std::error_code ec;std::filesystem::remove(file,ec);
    CHECK(profile.save(file));
    MenuState loadedMenu;
    MenuProfileBridge loaded(loadedMenu);
    CHECK(loaded.load(file));
    CHECK(loaded.settings().sprint.enabled &&
          loaded.settings().sprint.allow_inventory &&
          loaded.settings().sprint.allow_using_item &&
          loaded.settings().sprint.allow_backwards &&
          loaded.settings().sprint.allow_sideways);
    CHECK(decide_sprint(obs,loaded.settings().sprint));
    auto invalid=loaded.settings();
    invalid.sprint.enabled=false;
    CHECK(loaded.adopt(invalid));
    CHECK(!decide_sprint(obs,loaded.settings().sprint));
    CHECK(loaded.load(file));
    CHECK(decide_sprint(obs,loaded.settings().sprint));
    CHECK(!parse_settings("version=1\nsprint_inventory=maybe\n").valid);
    CHECK(!parse_settings("version=1\nsprint_enabled=true\nsprint_enabled=false\n").valid);
    {std::ofstream broken(file,std::ios::binary|std::ios::trunc);
     broken<<"version=1\nsprint_sideways=INVALID\n";}
    CHECK(!loaded.load(file));
    CHECK(loaded.settings().sprint.enabled);
    CHECK(frames.detach());
    std::filesystem::remove(file,ec);
    std::filesystem::remove(file.string()+".bak",ec);
    std::filesystem::remove(file.string()+".tmp",ec);
    return 0;
}
