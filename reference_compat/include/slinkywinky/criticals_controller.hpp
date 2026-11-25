#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace slinkywinky::reference_compat {
// Value-only model of candidate Criticals callbacks. No packet or timer writes.
enum class CriticalsMode : unsigned { packet = 0, timer = 1 };
struct CriticalsSettings {
    CriticalsMode mode = CriticalsMode::packet;
    float max_packet_delay_ms = 150;
    bool require_mouse = false, require_weapon = false;
    float timer_speed = 0.5f;
    float boost_ticks = 3;
};
struct CriticalsTickFrame {
    bool in_game = false, mouse = false, weapon = false;
    int player_ticks = 0, last_attacker_tick = 0, hurt_time = 0;
    bool on_ground = true, entity_under_crosshair = false;
    double motion_y = 0;
    bool sampled_chance_roll = false; // source RNG supplied by caller
};
struct CriticalsPacketFrame {
    bool player_packet = false, positional = false, on_ground = false;
    int sequence = 0;
    double y = 0;
    std::int64_t held_ms = 0;
};
struct CriticalsTimerProposal {
    bool update_speed = false;
    float speed = 1.0f;
    int add_elapsed_ticks = 0;
};

class CriticalsController {
public:
    void configure(CriticalsSettings s) {
        if (static_cast<unsigned>(s.mode)>1 || !std::isfinite(s.max_packet_delay_ms) ||
            !std::isfinite(s.timer_speed) || !std::isfinite(s.boost_ticks) ||
            s.max_packet_delay_ms < 0 || s.max_packet_delay_ms > 65535 ||
            s.timer_speed <= 0 || s.timer_speed > 1 ||
            s.boost_ticks < 0 || s.boost_ticks > 10000)
            throw std::invalid_argument("invalid Criticals settings");
        settings_ = s;
    }
    void reset_world() noexcept {
        packet_armed_=timer_armed_=timer_modified_=falling_=false;
        packet_delay_=hold_budget_=roll_cooldown_=0;
        last_roll_=false;
        last_sequence_=0;
        last_y_=0;
        accumulated_boost_=0;
    }
    void tick(const CriticalsTickFrame& f) {
        if (!std::isfinite(f.motion_y))
            throw std::invalid_argument("invalid Criticals motion");
        bool act=false;
        if (f.in_game) {
            if (roll_cooldown_ != 0) --roll_cooldown_;
            else {roll_cooldown_=9;last_roll_=f.sampled_chance_roll;}
            act = last_roll_ && (!settings_.require_mouse || f.mouse) &&
                (!settings_.require_weapon || f.weapon) &&
                (static_cast<std::int64_t>(f.player_ticks) -
                 static_cast<std::int64_t>(f.last_attacker_tick) < 11);
        }
        if (settings_.mode == CriticalsMode::packet) {
            packet_armed_=act;
            packet_delay_=static_cast<int>(settings_.max_packet_delay_ms);
            timer_armed_=false;
        } else {
            packet_armed_=false;
            timer_armed_=act && !f.on_ground && f.motion_y<0 &&
                f.hurt_time!=10 && f.entity_under_crosshair;
        }
    }
    bool packet(const CriticalsPacketFrame& f) {
        if (!std::isfinite(f.y)) throw std::invalid_argument("invalid packet height");
        if (f.player_packet && f.positional && f.sequence>last_sequence_) {
            if (f.on_ground || last_y_>f.y) falling_=!f.on_ground;
            int budget=0;
            if(packet_armed_ && falling_)
                budget=std::min(packet_delay_,hold_budget_+packet_delay_/5);
            hold_budget_=budget;
            last_y_=f.y;last_sequence_=f.sequence;
        }
        return f.held_ms < hold_budget_;
    }
    CriticalsTimerProposal frame(double dt_seconds) {
        if(!std::isfinite(dt_seconds) || dt_seconds<0 || dt_seconds>120)
            throw std::invalid_argument("invalid Criticals frame delta");
        CriticalsTimerProposal out{};
        if (timer_armed_) {
            timer_modified_=true;
            accumulated_boost_=std::min(static_cast<double>(settings_.boost_ticks),
                accumulated_boost_+(1.0-settings_.timer_speed)*dt_seconds*20.0);
            out.update_speed=true;out.speed=settings_.timer_speed;
            return out;
        }
        if(accumulated_boost_>1.0) {
            out.add_elapsed_ticks=static_cast<int>(accumulated_boost_);
            accumulated_boost_-=out.add_elapsed_ticks;
        }
        if(timer_modified_){out.update_speed=true;out.speed=1.0f;timer_modified_=false;}
        return out;
    }
    CriticalsTimerProposal disable() noexcept {
        CriticalsTimerProposal out{};
        if(timer_modified_){out.update_speed=true;out.speed=1.0f;}
        reset_world();
        return out;
    }
    bool packet_armed() const noexcept {return packet_armed_;}
    bool timer_armed() const noexcept {return timer_armed_;}
    int packet_hold_budget() const noexcept {return hold_budget_;}
    int roll_cooldown() const noexcept {return roll_cooldown_;}
    double boost_debt() const noexcept {return accumulated_boost_;}
private:
    CriticalsSettings settings_{};
    bool packet_armed_=false,timer_armed_=false,timer_modified_=false,falling_=false;
    bool last_roll_=false;
    int packet_delay_=0,hold_budget_=0,roll_cooldown_=0,last_sequence_=0;
    double last_y_=0,accumulated_boost_=0;
};
} // namespace slinkywinky::reference_compat
