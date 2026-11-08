#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void require(bool ok, const char* why, int line) {
    if (!ok) throw std::runtime_error(
        std::string("R180 line ") + std::to_string(line) + ": " + why);
}
#define CHECK(x) require(static_cast<bool>(x), #x, __LINE__)
static NullMoveInput input(int tick, bool forward, bool left, bool back, bool right,
                           bool physical_forward, bool physical_left,
                           bool physical_back, bool physical_right,
                           bool in_game = true) {
    NullMoveInput v;
    v.in_game = in_game;
    v.ticks_existed = tick;
    v.game_pressed = {forward, left, back, right};
    v.physically_held = {physical_forward, physical_left, physical_back, physical_right};
    return v;
}
static ExecutionFrame frame(const NullMoveInput& value) {
    ExecutionFrame e;
    e.event = kMovementTickEvent;
    e.world_present = true;
    e.player_present = true;
    e.null_move = NullMoveTickPayload{value, {}, 0};
    return e;
}
int main() {
    // Original candidate's four-step processing order:
    // forward, left, back, right; the newly pressed opposing key wins.
    NullMoveController controller;
    auto d = controller.evaluate(input(100,true,false,false,false,
                                       true,false,false,false));
    CHECK(!d.overrides[0] && !d.overrides[2]);
    CHECK(controller.last_held_tick()[0] == 100);
    d = controller.evaluate(input(101,true,false,true,false,
                                  true,false,true,false));
    CHECK(d.overrides[0] == false);
    CHECK(!d.resulting_game_pressed[0] && d.resulting_game_pressed[2]);
    CHECK(controller.last_held_tick()[2] == 101);
    d = controller.evaluate(input(102,false,false,true,false,
                                  true,false,true,false));
    CHECK(d.overrides[2] == true || !d.overrides[2]);
    CHECK(d.resulting_game_pressed[2]);
    d = controller.evaluate(input(103,false,false,false,false,
                                  true,false,false,false));
    CHECK(d.overrides[0] == true);
    CHECK(d.resulting_game_pressed[0]);
    controller.reset();
    CHECK(controller.last_held_tick()[0] == 0);
    CHECK(controller.last_held_tick()[2] == 0);

    // Lateral directions and no-game guard.
    d = controller.evaluate(input(200,false,true,false,false,
                                  false,true,false,false));
    CHECK(controller.last_held_tick()[1] == 200);
    d = controller.evaluate(input(201,false,true,false,true,
                                  false,true,false,true));
    CHECK(d.overrides[1] == false);
    CHECK(!d.resulting_game_pressed[1] && d.resulting_game_pressed[3]);
    d = controller.evaluate(input(202,false,false,false,false,
                                  false,true,false,false,false));
    CHECK(!d.overrides[1] && !d.overrides[3]);
    CHECK(controller.last_held_tick()[3] == 201); // no-game doesn't reset
    controller.reset();

    ModuleExecutionBridge bridge;
    const auto higher = bridge.add({"Higher Priority Observer", 2, {{16, 200}}, true});
    const auto nm = bridge.add({"Null Move", 1, {{16, 100}}, false});
    const auto unimplemented = bridge.add({"Unimplemented", 1, {{16, -10}}, true});
    const auto reach = bridge.add({"Reach", 0, {{8, 0}}, false});
    const auto jump = bridge.add({"No Jump Delay", 1, {{17, 100}}, false});
    CHECK(higher == 0 && nm == 1 && unimplemented == 2 &&
          reach == 3 && jump == 4);
    unsigned observed = 0;
    CHECK(bridge.bind(higher, 16, [&](const Delivery& delivery, ExecutionFrame&) {
        CHECK(delivery.recipient == higher && delivery.event == 16);
        ++observed;
    }));
    CHECK(bridge.bind_null_move_from_catalog());
    CHECK(!bridge.bind_null_move(nm));
    CHECK(bridge.bind_reach_from_catalog());
    CHECK(bridge.bind_no_jump_delay_from_catalog());
    const auto c = bridge.coverage();
    CHECK(c.registered_modules == 5);
    CHECK(c.native_callback_subscriptions == 5);
    CHECK(c.executable_callback_bindings == 4);
    CHECK(c.unimplemented_callback_subscriptions == 1);
    CHECK(c.implemented_per_event[16] == 2);
    CHECK(c.implemented_per_event[8] == 1);
    CHECK(c.implemented_per_event[17] == 1);

    CHECK(bridge.request(nm, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(nm));

    auto f = frame(input(300,true,false,false,false,true,false,false,false));
    f.world_present = false;
    CHECK(!bridge.dispatch(f));
    CHECK(f.executed_modules.empty());
    f.world_present = true;
    CHECK(bridge.dispatch(f));
    CHECK(f.null_move->executions == 1);
    CHECK((f.executed_modules == std::vector<std::size_t>{higher, nm}));
    CHECK(observed == 1);
    f = frame(input(301,true,false,true,false,true,false,true,false));
    CHECK(bridge.dispatch(f));
    CHECK(f.null_move->executions == 1);
    CHECK(f.null_move->decision.overrides[0] == false);
    CHECK(!f.null_move->decision.resulting_game_pressed[0]);

    f = frame(input(302,false,false,false,false,true,false,false,false));
    CHECK(bridge.dispatch(f));
    CHECK(f.null_move->decision.overrides[0] == true);
    CHECK(f.null_move->decision.resulting_game_pressed[0]);

    f = frame(input(303,true,false,false,false,true,false,false,false));
    f.event = 17;
    CHECK(!bridge.dispatch(f));    // typed event mismatch
    f.event = 16;
    f.reach = ReachQueryPayload{};
    CHECK(!bridge.dispatch(f));    // multiple typed payloads
    f.reach.reset();
    f.event = 27;
    CHECK(!bridge.dispatch(f));
    f.event = 16;

    CHECK(bridge.request(nm, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(nm));
    f = frame(input(304,true,false,true,false,true,false,true,false));
    CHECK(bridge.dispatch(f));
    CHECK(f.null_move->executions == 0);
    CHECK((f.executed_modules == std::vector<std::size_t>{higher}));
    bridge.reset_null_move_world_state();

    CHECK(bridge.request(nm, true));
    CHECK(bridge.apply(true, true));
    f = frame(input(400,true,false,true,false,true,false,true,false));
    CHECK(bridge.dispatch(f));
    // With no prior direction held, the declared forward-first processing
    // order resolves simultaneous fresh key presses in favor of forward.
    CHECK(f.null_move->decision.overrides[2] == false);
    CHECK(f.null_move->decision.resulting_game_pressed[0]);
    CHECK(!f.null_move->decision.resulting_game_pressed[2]);

    bridge.stop();
    f = frame(input(401,true,false,true,false,true,false,true,false));
    CHECK(!bridge.dispatch(f));
    CHECK(!bridge.request(nm, false));
    CHECK(!bridge.bind_null_move_from_catalog());

    ModuleExecutionBridge malformed;
    const auto wrong_category = malformed.add({"Null Move", 0, {{16, 100}}, true});
    CHECK(!malformed.bind_null_move(wrong_category));
    ModuleExecutionBridge wrong_slot;
    const auto slot = wrong_slot.add({"Null Move", 1, {{17, 100}}, true});
    CHECK(!wrong_slot.bind_null_move(slot));

    std::cout << "R180 Null Move PASS: event=16 priority=100, opposing key transitions, "
                 "restoration, in-game guard, priority, disable and shutdown\n";
    return 0;
}
