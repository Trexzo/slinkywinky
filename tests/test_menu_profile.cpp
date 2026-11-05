#include "slinkywinky/menu_profile.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x))throw std::runtime_error("R86 profile binding: " #x);}while(false)

using namespace slinkywinky;

int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    for(std::size_t i=0;i<official_module_catalog().size();++i) {
        const bool expected=(i==0||i==1||i==2||i==4||i==6||i==12||i==15);
        CHECK(module_has_profile_setting(i)==expected);
        CHECK(!profile.enabled(i));
    }
    CHECK(!module_has_profile_setting(65));
    CHECK(!profile.toggle(65));
    CHECK(!profile.toggle(3)); // Auto Weapon is catalog-only.
    CHECK(!profile.toggle(15)); // Initially outside the visible page.
    CHECK(!profile.toggle(17)); // Wrong tab even though the index exists.
    CHECK(profile.toggle(0));
    CHECK(profile.toggle(1));
    CHECK(profile.toggle(2));
    CHECK(profile.toggle(4));
    CHECK(profile.toggle(6));
    CHECK(profile.settings().combat.aim_enabled);
    CHECK(profile.settings().combat.auto_block_enabled);
    CHECK(profile.settings().combat.auto_clicker_enabled);
    CHECK(profile.settings().backtrack_enabled);
    CHECK(profile.settings().combat.hit_select_enabled);
    CHECK(menu.page()[0].preview_enabled);
    CHECK(!menu.page()[3].preview_enabled);
    CHECK(profile.toggle(0) && !profile.enabled(0));
    CHECK(!menu.page()[0].preview_enabled);
    CHECK(profile.scroll_by(9));
    CHECK(menu.scroll()==9 && menu.page().size()==8);
    CHECK(!profile.toggle(6)); // Stale off-screen row is refused.
    CHECK(profile.toggle(12));
    CHECK(profile.toggle(15));
    CHECK(profile.settings().combat.reach_enabled);
    CHECK(profile.settings().velocity.enabled);
    CHECK(menu.page()[3].preview_enabled); // Reach: catalog index 12.
    CHECK(menu.page()[6].preview_enabled); // Velocity: catalog index 15.
    CHECK(!profile.scroll_by(20));
    CHECK(profile.select(MenuTab::Move));
    CHECK(!profile.toggle(17));
    CHECK(!menu.page()[0].preview_enabled); // Unsupported Move module.
    CHECK(profile.select(MenuTab::Combat));
    CHECK(profile.scroll_by(9));
    CHECK(menu.page()[3].preview_enabled && menu.page()[6].preview_enabled);

    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r86-settings-profile.ini";
    std::error_code ignored;
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(std::filesystem::path(file.string()+".bak"),ignored);
    std::filesystem::remove(std::filesystem::path(file.string()+".tmp"),ignored);
    CHECK(profile.save(file));
    const auto saved=load_settings(file);
    CHECK(saved.valid && saved.settings.combat.auto_clicker_enabled);
    CHECK(saved.settings.velocity.enabled && saved.settings.combat.reach_enabled);
    CHECK(saved.settings.combat.aim_enabled==false);

    MenuState freshMenu;
    freshMenu.set_compact(true); // Appearance independent of game profile.
    MenuProfileBridge fresh(freshMenu);
    CHECK(fresh.load(file));
    CHECK(fresh.enabled(2));
    CHECK(!fresh.enabled(0));
    CHECK(freshMenu.page()[2].preview_enabled);
    CHECK(freshMenu.appearance().compact);
    CHECK(fresh.scroll_by(9));
    CHECK(freshMenu.page()[3].preview_enabled);
    CHECK(freshMenu.page()[6].preview_enabled);

    // A corrupted load must never partially apply its available keys.
    {
        std::ofstream broken(file,std::ios::binary|std::ios::trunc);
        broken<<"version=1\nauto_clicker_enabled=false\nclicks_per_second=oops\n";
    }
    CHECK(!fresh.load(file));
    CHECK(fresh.enabled(2) && fresh.enabled(12) && fresh.enabled(15));
    CHECK(freshMenu.page()[3].preview_enabled);
    ClientSettings invalid=fresh.settings();
    invalid.combat.clicks_per_second=9999.f;
    CHECK(!fresh.adopt(invalid));
    CHECK(fresh.enabled(2) && fresh.enabled(12));

    // Exported settings are native models only; no Minecraft actions.
    const std::string serialized=serialize_settings(fresh.settings());
    CHECK(serialized.find("auto_clicker_enabled=true")!=std::string::npos);
    CHECK(serialized.find("reach_enabled=true")!=std::string::npos);
    CHECK(serialized.find("velocity_enabled=true")!=std::string::npos);
    CHECK(serialized.find("net.minecraft")==std::string::npos);
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(std::filesystem::path(file.string()+".bak"),ignored);
    std::filesystem::remove(std::filesystem::path(file.string()+".tmp"),ignored);
    return 0;
}
