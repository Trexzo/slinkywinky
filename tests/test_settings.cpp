#include "slinkywinky/settings.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace slinkywinky;
#define CHECK(c) do { if(!(c)) throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+": " #c); } while(false)
int main() {
    ClientSettings cfg; cfg.combat.aim_enabled=true;cfg.combat.clicks_per_second=12.75f;
    cfg.velocity.enabled=true;cfg.backtrack_ms=250;
    const auto serial=serialize_settings(cfg);
    auto parsed=parse_settings(serial);
    CHECK(parsed.valid);
    CHECK(parsed.settings.combat.aim_enabled);
    CHECK(parsed.settings.combat.clicks_per_second==12.75f);
    CHECK(parsed.settings.backtrack_ms==250);
    CHECK(parsed.settings.velocity.enabled);
    CHECK(!parse_settings("version=2\n").valid);
    CHECK(!parse_settings("version=1\nclicks_per_second=NaN\n").valid);
    CHECK(!parse_settings("version=1\nclicks_per_second=0\n").valid);
    CHECK(!parse_settings("version=1\naim_enabled=true\naim_enabled=false\n").valid);
    CHECK(!parse_settings("aim_enabled=true\n").valid);
    CHECK(parse_settings("version=1\nfuture_setting=abc\n").valid);
    const auto dir=std::filesystem::temp_directory_path()/"slinkywinky-settings-test";
    std::filesystem::create_directories(dir);
    const auto path=dir/"profile.ini";
    std::string err;
    CHECK(save_settings(path,cfg,&err));
    auto loaded=load_settings(path);
    CHECK(loaded.valid && loaded.settings.combat.aim_enabled);
    cfg.combat.aim_enabled=false;
    CHECK(save_settings(path,cfg,&err));
    CHECK(!load_settings(path).settings.combat.aim_enabled);
    CHECK(std::filesystem::exists(dir/"profile.ini.bak"));
    std::filesystem::remove_all(dir);
    std::cout<<"PASS settings validation, round-trip, corruption rejection, backup\n";
}
