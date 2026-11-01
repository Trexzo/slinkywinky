#include "slinkywinky/hud.hpp"
#include <stdexcept>
#include <cmath>
#include <limits>
#define CHECK(c) do { if(!(c)) throw std::runtime_error("HUD test: " #c); } while(false)

using namespace slinkywinky;
int main() {
    auto p=make_hud_panel(std::nullopt);
    CHECK(!p.has_data && p.state=="NOT ATTACHED" && p.rows.size()==1);
    ObservationStatus s{};
    p=make_hud_panel(s);
    CHECK(!p.has_data && p.state=="INVALID TELEMETRY");

    s.valid_ticks=12;
    s.generation=2;
    s.session_transitions=1;
    p=make_hud_panel(s);
    CHECK(p.has_data && p.state=="MAIN MENU" && !p.has_ray_target);
    CHECK(p.rows.size()==6 && p.rows.back().value=="N/A");

    s.flags=1;
    p=make_hud_panel(s);
    CHECK(p.state=="WORLD LOADING" && !p.in_world);
    s.flags=3;
    s.eye_y=64.62;
    s.yaw=40.1f;
    s.pitch=-8.0f;
    p=make_hud_panel(s);
    CHECK(p.state=="IN WORLD" && p.in_world);
    CHECK(p.rows.size()==8 && p.rows[5].value=="64.62");
    CHECK(p.rows[6].value=="40.1 / -8.0");
    CHECK(p.rows[7].value=="No ray-selected entity");

    s.flags=7;
    s.target_distance=3.765;
    p=make_hud_panel(s);
    CHECK(p.state=="RAY TARGET OBSERVED" && p.has_ray_target);
    CHECK(p.rows[7].label=="Ray distance" && p.rows[7].value=="3.77 blocks");
    auto compact=make_hud_panel(s,{false,true,true});
    CHECK(compact.rows.size()==4);
    CHECK(compact.rows.back().value=="3.77 blocks");
    auto minimal=make_hud_panel(s,{false,false,true});
    CHECK(minimal.rows.size()==3);
    CHECK(minimal.rows.back().label=="Ray distance");

    s.flags=8;
    p=make_hud_panel(s);
    CHECK(!p.has_data && p.state=="INVALID TELEMETRY");
    s.flags=2;
    CHECK(!make_hud_panel(s).has_data);
    s.flags=4;
    CHECK(!make_hud_panel(s).has_data);
    s.flags=7;
    s.target_distance=std::numeric_limits<double>::infinity();
    CHECK(!make_hud_panel(s).has_data);
    s.target_distance=2.0;
    s.eye_y=std::numeric_limits<double>::quiet_NaN();
    CHECK(!make_hud_panel(s).has_data);
    s.eye_y=64.62;
    s.yaw=std::numeric_limits<float>::infinity();
    CHECK(!make_hud_panel(s).has_data);
    s.yaw=40.1f;
    CHECK(make_hud_panel(s).has_data);
    return 0;
}
