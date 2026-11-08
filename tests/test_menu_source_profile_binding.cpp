#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#define CHECK(x) do {if(!(x))throw std::runtime_error(    "R155 source-native GUI binding line "+std::to_string(__LINE__));}while(false)
using namespace slinkywinky;
namespace {
std::size_t find(std::string_view mod,std::string_view group,
                 std::string_view label) {
    const auto& all=recovered_widget_catalog();
    for(std::size_t i=0;i<all.size();++i)
        if(std::string_view(all[i].module)==mod &&
           std::string_view(all[i].group)==group &&
           std::string_view(all[i].label)==label &&
           all[i].kind==RecoveredWidgetKind::Slider)return i;
    throw std::runtime_error("source fixture callsite missing");
}
bool close(float a,float b) {return std::fabs(a-b)<0.0002f;}
}
int main() {
    const auto cps=find("Auto Clicker","In-game","Target CPS");
    const auto back=find("Backtrack","Main","Maximum delay");
    const auto reach=find("Reach","Main","Distance");
    const auto chance=find("Velocity","Main","Chance");
    const auto block=find("Auto Block","Main","Range");
    const auto predict=find("Aim Assist","Aim","Predict");
    MenuState menu;
    MenuProfileBridge profile(menu);
    CHECK(!profile.recovered_slider_setting(predict));
    CHECK(profile.recovered_slider_setting(cps)==12.0f);
    CHECK(profile.recovered_slider_setting(back)==100.0f);
    CHECK(close(*profile.recovered_slider_setting(block),3.1f));
    CHECK(!profile.set_recovered_slider(12,reach,4.0f)); // not visible
    CHECK(!profile.set_recovered_slider(2,predict,12.f)); // wrong module
    CHECK(!profile.set_recovered_slider(2,cps,-1.f));
    CHECK(!profile.set_recovered_slider(2,cps,21.f));
    CHECK(menu.toggle_expanded(2));
    CHECK(profile.set_recovered_slider(2,cps,13.27f));
    CHECK(close(profile.settings().combat.clicks_per_second,13.3f));
    CHECK(close(*menu.recovered_slider_override(cps),13.3f));
    CHECK(!profile.set_recovered_slider(2,cps,30.f));
    CHECK(close(profile.settings().combat.clicks_per_second,13.3f));
    CHECK(menu.toggle_expanded(1));
    CHECK(!profile.set_recovered_slider(1,block,7.f));
    CHECK(!menu.recovered_slider_override(block));
    CHECK(close(profile.settings().combat.block_range,3.1f));
    CHECK(profile.set_recovered_slider(1,block,5.1f));
    CHECK(close(profile.settings().combat.block_range,5.1f));
    CHECK(menu.toggle_expanded(4));
    CHECK(profile.set_recovered_slider(4,back,235.f));
    CHECK(profile.settings().backtrack_ms==240u);
    MenuRenderSession frames;
    CHECK(frames.attach(155,900,630));
    auto frame=frames.next(menu,&profile);
    CHECK(frame);
    CHECK(close(*frame->recovered_slider_overrides[cps],13.3f));
    CHECK(close(*frame->recovered_slider_overrides[block],5.1f));
    CHECK(close(*frame->recovered_slider_overrides[back],240.0f));
    CHECK(menu.toggle_expanded(0));
    const auto before=serialize_settings(profile.settings());
    CHECK(profile.set_recovered_slider(0,predict,17.0f));
    CHECK(profile.settings().combat.aim_range==4.f);
    CHECK(serialize_settings(profile.settings())==before);
    CHECK(menu.recovered_slider_override(predict)==17.f);
    CHECK(profile.scroll_by(9));
    CHECK(!profile.set_recovered_slider(2,cps,15.f)); // off-page
    CHECK(!profile.set_recovered_slider(12,reach,3.678f)); // collapsed
    CHECK(menu.toggle_expanded(12));
    CHECK(profile.set_recovered_slider(12,reach,3.678f));
    CHECK(close(profile.settings().combat.reach.distance,3.68f));
    CHECK(menu.toggle_expanded(15));
    CHECK(profile.set_recovered_slider(15,chance,65.1f));
    CHECK(close(profile.settings().velocity.chance_percent,65.f));
    CHECK(!profile.set_recovered_slider(15,chance,101.f));
    CHECK(profile.select(MenuTab::Move));
    CHECK(!profile.set_recovered_slider(15,chance,30.f)); // wrong tab
    CHECK(!profile.set_recovered_slider(12,reach,4.f));
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r155-native-binding.ini";
    std::error_code error;
    std::filesystem::remove(file,error);
    CHECK(profile.save(file));
    MenuState restoredMenu;
    MenuProfileBridge restored(restoredMenu);
    CHECK(restored.load(file));
    CHECK(close(*restored.recovered_slider_setting(cps),13.3f));
    CHECK(close(*restored.recovered_slider_setting(block),5.1f));
    CHECK(restored.recovered_slider_setting(back)==240.f);
    CHECK(close(*restored.recovered_slider_setting(reach),3.68f));
    CHECK(restored.recovered_slider_setting(chance)==65.f);
    MenuRenderSession restoredFrames;
    CHECK(restoredFrames.attach(156,900,630));
    auto restoredFrame=restoredFrames.next(restoredMenu,&restored);
    CHECK(restoredFrame);
    CHECK(close(*restoredFrame->recovered_slider_overrides[cps],13.3f));
    CHECK(!restoredFrame->recovered_slider_overrides[predict]);
    CHECK(restoredMenu.toggle_expanded(2));
    CHECK(restored.set_recovered_slider(2,cps,14.f));
    CHECK(restored.load(file)); // persisted settings win over stale GUI
    restoredFrame=restoredFrames.next(restoredMenu,&restored);
    CHECK(restoredFrame);
    CHECK(close(*restoredFrame->recovered_slider_overrides[cps],13.3f));
    {std::ofstream damaged(file,std::ios::binary|std::ios::trunc);
     damaged<<"version=1\nclicks_per_second=oops\n";}
    CHECK(!restored.load(file));
    CHECK(close(*restored.recovered_slider_setting(cps),13.3f));
    CHECK(frames.detach() && restoredFrames.detach());
    std::filesystem::remove(file,error);
    std::filesystem::remove(file.string()+".bak",error);
    std::filesystem::remove(file.string()+".tmp",error);
    return 0;
}
