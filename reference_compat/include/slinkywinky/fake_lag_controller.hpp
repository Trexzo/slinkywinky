#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
enum class FakeLagMode : unsigned { constant = 0, pulse = 1 };
struct FakeLagSettings {
    float inbound_ms = 0;
    float outbound_ms = 0;
    FakeLagMode mode = FakeLagMode::constant;
    bool realtime_damage = false;
    bool require_ingame = false;
    bool require_weapon = false;
    bool require_attack = false;
};
struct FakeLagTick {
    bool world = false;
    bool in_game = false;
    bool holding_weapon = false;
    int last_attacker_time = 0;
};
enum class LagVerdict { unchanged, hold, pass };
struct LagPacketProposal {
    LagVerdict verdict = LagVerdict::unchanged;
    std::int64_t residual_budget_ms = 0;
};

// No network transport, packet data, or actual delayed-packet ownership.
class FakeLagController {
public:
    void configure(FakeLagSettings s) {
        if (!std::isfinite(s.inbound_ms) || !std::isfinite(s.outbound_ms) ||
            s.inbound_ms < 0 || s.outbound_ms < 0 ||
            s.inbound_ms > 65535 || s.outbound_ms > 65535 ||
            static_cast<unsigned>(s.mode) > 1u)
            throw std::invalid_argument("invalid Fake Lag settings");
        settings_ = s;
    }
    void reset_world() noexcept {
        cooldown_ = 0;
        inbound_counter_ = outbound_counter_ = 0;
        inbound_window_ = outbound_window_ = false;
        inbound_budget_ = outbound_budget_ = 0;
        has_last_attack_ = false;
    }
    void tick(const FakeLagTick& frame) noexcept {
        if (cooldown_ > 0) --cooldown_;
        if (settings_.mode == FakeLagMode::pulse) {
            inbound_window_ = pulse_step(inbound_counter_, settings_.inbound_ms);
            outbound_window_ = pulse_step(outbound_counter_, settings_.outbound_ms);
        }
        if (!frame.world || (settings_.require_ingame && !frame.in_game)) {
            cooldown_ = 0;
            inbound_budget_ = outbound_budget_ = 0;
            return;
        }
        bool lag = false;
        if (settings_.require_weapon && !frame.holding_weapon) {
            cooldown_ = 0;
        } else {
            lag = !settings_.require_attack || cooldown_ != 0;
        }
        // Source order: the newly observed attack starts the NEXT tick's lag.
        if (has_last_attack_ && frame.last_attacker_time != last_attack_)
            cooldown_ = 20;
        last_attack_ = frame.last_attacker_time;
        has_last_attack_ = true;
        inbound_budget_ = lag ? static_cast<std::uint16_t>(settings_.inbound_ms) : 0;
        outbound_budget_ = lag ? static_cast<std::uint16_t>(settings_.outbound_ms) : 0;
    }
    LagPacketProposal packet(bool incoming, std::int64_t held_ms,
                             bool entity_status = false,
                             bool update_health = false,
                             bool sound_effect = false) const noexcept {
        LagPacketProposal answer{LagVerdict::unchanged, held_ms};
        if (incoming && settings_.realtime_damage &&
            (entity_status || update_health || sound_effect)) {
            answer.verdict = LagVerdict::pass;
            return answer;
        }
        if (incoming ? inbound_window_ : outbound_window_) return answer;
        const auto delay = incoming ? inbound_budget_ : outbound_budget_;
        // Candidate callback subtracts the configured allowance from current budget.
        // Prevent signed overflow for caller-provided extreme negative time.
        if (held_ms < std::numeric_limits<std::int64_t>::min() +
                           static_cast<std::int64_t>(delay)) {
            answer.residual_budget_ms = 0;
            answer.verdict = LagVerdict::hold;
            return answer;
        }
        answer.residual_budget_ms = held_ms - delay;
        if (answer.residual_budget_ms < 0) {
            answer.residual_budget_ms = 0;
            answer.verdict = LagVerdict::hold;
        }
        return answer;
    }
    int cooldown() const noexcept { return cooldown_; }
    std::uint16_t inbound_budget() const noexcept { return inbound_budget_; }
    std::uint16_t outbound_budget() const noexcept { return outbound_budget_; }
    bool inbound_window() const noexcept { return inbound_window_; }
    bool outbound_window() const noexcept { return outbound_window_; }
private:
    static bool pulse_step(int& counter, float length) noexcept {
        if (counter > 0) {
            const int old = counter;
            counter -= 50;
            return old <= 50;
        }
        counter = static_cast<int>(static_cast<float>(counter) + length);
        return false;
    }
    FakeLagSettings settings_{};
    int cooldown_ = 0;
    int last_attack_ = 0;
    bool has_last_attack_ = false;
    int inbound_counter_ = 0, outbound_counter_ = 0;
    bool inbound_window_ = false, outbound_window_ = false;
    std::uint16_t inbound_budget_ = 0, outbound_budget_ = 0;
};
} // namespace slinkywinky::reference_compat
