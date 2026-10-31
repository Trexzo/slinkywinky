#include "slinkywinky/core.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace slinkywinky;

#define CHECK(condition) do { \
    if (!(condition)) throw std::runtime_error("Check failed at line " + \
        std::to_string(__LINE__) + ": " #condition); \
} while (false)

int main() {
    Random128 rng({0x12345678, 0x9abcdef0, 0xdeadbeef, 0x0f1e2d3c});
    constexpr std::array<std::uint32_t, 10> expected = {
        0x215283b4, 0x9f350a61, 0xa0c4498b, 0x86768c27, 0x1e19cb7f,
        0xc0b6b118, 0xb79d2832, 0xfea7eb69, 0x1d779958, 0x5b9c3b21
    };
    for (const auto value : expected) CHECK(rng.next() == value);
    bool caught = false;
    try { Random128 invalid({0, 0, 0, 0}); } catch (const std::invalid_argument&) { caught = true; }
    CHECK(caught);
    Random128 bounds({1, 2, 3, 4});
    for (int i = 0; i < 10000; ++i) {
        const float v = bounds.unit_float();
        CHECK(v >= 0.0f && v < 1.0f);
    }

    ReachController reach;
    ReachSettings settings;
    CHECK(reach.evaluate({true, 0}, settings).has_value());
    CHECK(!reach.evaluate({true, 1}, settings).has_value());
    CHECK(reach.cooldown_ticks() == 9);
    for (int i = 0; i < 6; ++i) CHECK(!reach.evaluate({true, 1}, settings).has_value());
    CHECK(reach.evaluate({true, 1}, settings).has_value()); // counter now 2 (<3)
    settings.require_sprint = true;
    CHECK(!reach.evaluate({false, 1}, settings).has_value());
    reach.reset();
    CHECK(reach.cooldown_ticks() == 0);
    CHECK(reach.evaluate({true, 0}, settings).has_value());
    CHECK(!reach.evaluate({true, 2}, settings).has_value());
    CHECK(reach.cooldown_ticks() == 9);

    RotationArbiter arbiter;
    CHECK(arbiter.submit({"aim", 100, 20, 15}));
    CHECK(!arbiter.submit({"manual", 100, 40, 20}));
    CHECK(arbiter.winner()->source == "aim");
    CHECK(arbiter.submit({"higher", 101, -5, -10}));
    CHECK(arbiter.winner()->source == "higher");
    CHECK(!arbiter.submit({"nan", 900, std::numeric_limits<float>::quiet_NaN(), 0}));
    arbiter.start_frame();
    CHECK(!arbiter.winner().has_value());

    InputOwnership input;
    CHECK(input.update(false, true) == KeyEdge::press);
    CHECK(input.update(true, true) == KeyEdge::none);
    CHECK(input.disable_synthetic(true) == KeyEdge::none);
    CHECK(input.update(false, false) == KeyEdge::release);
    CHECK(input.update(true, false) == KeyEdge::press);
    CHECK(input.update(false, false) == KeyEdge::release);
    CHECK(!input.effective_down());

    ReferenceProfile profile;
    CHECK(profile.click_target_cps == 12.0f);
    CHECK(profile.reach_activation_ticks == 3.0f);
    std::cout << "PASS: deterministic RNG, reach state, arbitration and input ownership\n";
}
