#include "slinkywinky/session.hpp"
#include "slinkywinky/settings.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace slinkywinky;
#define CHECK(c) do {if(!(c))throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+": " #c);}while(false)
int main(){
    HostSession session({17,19,23,29});
    CHECK(session.attach());
    std::uint64_t absolute_time=100;
    int attacks=0,rotations=0,packet_releases=0;
    for (int world=0;world<50;++world) {
        CHECK(session.start_world());
        ClientSettings settings;
        settings.combat.auto_clicker_enabled=true;
        settings.combat.aim_enabled=true;
        settings.combat.auto_block_enabled=true;
        settings.combat.aim_range=4;
        settings.combat.auto_block_enabled=true;
        settings.backtrack_enabled=true;
        settings.backtrack_ms=70;
        TickSnapshot snap;
        snap.world_valid=true;
        snap.generation=session.mailbox().generation();
        snap.holding_weapon=true;
        snap.player_eye={0,1,0};
        for(int frame=0;frame<500;++frame) {
            snap.elapsed_ms=absolute_time;
            snap.mouse_left=frame%30<20;
            snap.gui_open=frame%111<3;
            snap.sprinting=frame%7!=0;
            snap.hurt_time=frame%11;
            snap.last_attacker_time=frame/12;
            snap.yaw=static_cast<float>((frame*11)%360 - 180);
            snap.pitch=static_cast<float>((frame*3)%160 - 80);
            if(frame%17==0) snap.target.reset();
            else snap.target=Target{frame%4,{2.0,1.2 + 0.2*(frame%3),1.0},true,true};
            settings.combat.auto_clicker_enabled=frame%23!=0;
            settings.combat.auto_block_enabled=frame%19!=0;
            settings.combat.aim_enabled=frame%31!=0;
            settings.combat.hit_select_enabled=frame%8==0;
            settings.combat.hit_select_min_hurt=3;
            if(frame%13==0) CHECK(session.queue_packet_notice(static_cast<unsigned>(frame),absolute_time));
            auto batch=session.tick(snap,settings);
            CHECK(batch.valid && !batch.emergency_release);
            for(const auto& cmd:batch.commands) {
                CHECK(cmd.generation==session.mailbox().generation());
                CHECK(std::isfinite(cmd.first) && std::isfinite(cmd.second));
                if(cmd.kind==CommandKind::attack_pulse) ++attacks;
                if(cmd.kind==CommandKind::set_rotation) ++rotations;
            }
            for(const auto& p:batch.released_packet_notices) {
                CHECK(p.generation==snap.generation);
                CHECK(p.received_ms<=snap.elapsed_ms);
                ++packet_releases;
            }
            absolute_time+=20;
        }
        auto left=session.leave_world(false,false);
        CHECK(left.valid && left.emergency_release);
        CHECK(left.commands.size()==1 && left.commands[0].kind==CommandKind::release_all);
        CHECK(session.mailbox().queued()==0);
    }
    auto final=session.shutdown(false,false);
    CHECK(final.valid);
    CHECK(attacks>50 && rotations>50 && packet_releases>50);
    CHECK(!parse_settings("version=1\nclicks_per_second=1e999\n").valid);
    CHECK(!parse_settings("version=1\nvelocity_horizontal=-0.1\n").valid);
    std::cout<<"PASS 25,000 frame stress, 50 world transitions, packet generations, release controls\n";
}
