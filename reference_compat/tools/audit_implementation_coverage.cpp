// R178-R179: read-only 66-module catalog to independently written execution bridge.
// No original client executable, Minecraft process, native hooks or game I/O.
#include "slinkywinky/candidate_catalog.hpp"
#include "slinkywinky/module_execution_bridge.hpp"

#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;

static void require(bool condition, const char* label) {
    if (!condition) throw std::runtime_error(label);
}

static ExecutionFrame reach_frame(int last_attack = 0) {
    ExecutionFrame frame;
    frame.event = kReachQueryEvent;
    frame.world_present = true;
    frame.player_present = true;
    ReachQueryPayload value;
    value.input.sprinting = true;
    value.input.last_attacker_time = last_attack;
    value.settings.distance = 3.2f;
    value.settings.activation_ticks = 4.0f;
    frame.reach = value;
    return frame;
}

static ExecutionFrame jump_frame(bool in_game = true, int current_ticks = 10) {
    ExecutionFrame frame;
    frame.event = kPlayerUpdateEvent;
    frame.world_present = true;
    frame.player_present = true;
    JumpDelayTickPayload payload;
    payload.input = {in_game, current_ticks};
    frame.jump_delay = payload;
    return frame;
}

static ExecutionFrame null_move_frame(int tick, bool forward, bool back) {
    ExecutionFrame frame;
    frame.event = kMovementTickEvent;
    frame.world_present = true;
    frame.player_present = true;
    NullMoveTickPayload payload;
    payload.input.in_game = true;
    payload.input.ticks_existed = tick;
    payload.input.game_pressed = {forward, false, back, false};
    payload.input.physically_held = {true, false, back, false};
    frame.null_move = payload;
    return frame;
}

static ExecutionFrame combined_movement_frame(int tick, bool forward, double x, double z,
                                              bool include_null_move) {
    ExecutionFrame frame;
    frame.event = kMovementTickEvent;
    frame.world_present = true;
    frame.player_present = true;
    InstantStopTickPayload payload;
    payload.input.in_game = true;
    payload.input.forward_pressed = forward;
    payload.input.on_ground = true;
    payload.input.motion_x = x;
    payload.input.motion_z = z;
    payload.input.yaw_degrees = 0.0f;
    payload.input.land_movement_factor = 0.1f;
    payload.input.physically_held = {forward, false, false, false};
    frame.instant_stop = payload;
    if (include_null_move) {
        NullMoveTickPayload null_tick;
        null_tick.input.in_game = true;
        null_tick.input.ticks_existed = tick;
        null_tick.input.game_pressed = {forward, false, false, false};
        null_tick.input.physically_held = payload.input.physically_held;
        frame.null_move = null_tick;
    }
    return frame;
}

static ExecutionFrame no_slow_frame(int item_id, bool using_item = true,
                                          bool item_present = true) {
    ExecutionFrame frame;
    frame.event = kNoSlowEvent;
    frame.world_present = true;
    frame.player_present = true;
    NoSlowTickPayload payload;
    payload.input.item_id = item_id;
    payload.input.using_item = using_item;
    payload.input.held_item_present = item_present;
    payload.input.strafe = 0.1f;
    payload.input.forward = -0.2f;
    payload.input.settings = {1.0f, 0.4f, 0.8f};
    frame.no_slow = payload;
    return frame;
}

static ExecutionFrame no_hit_delay_frame(unsigned mode = 0, bool target_present = false,
                                              int target_type = 0, bool using_item = false) {
    ExecutionFrame frame;
    frame.event = kNoHitDelayEvent;
    frame.world_present = true;
    frame.player_present = true;
    NoHitDelayTickPayload payload;
    payload.input.attack_click = true;
    payload.input.jvm_ready = true;
    payload.input.left_click_counter = 10;
    payload.input.mode = mode;
    payload.input.target_present = target_present;
    payload.input.target_type_ordinal = target_type;
    payload.input.player_using_item = using_item;
    frame.no_hit_delay = payload;
    return frame;
}

static ExecutionFrame no_use_delay_frame(int ticks = 3, int slot = 0,
                                             bool desync = false) {
    ExecutionFrame frame;
    frame.event = kNoUseDelayEvent;
    frame.world_present = true;
    frame.player_present = true;
    NoUseDelayTickPayload payload;
    payload.input.in_game = true;
    payload.input.use_key_down = true;
    payload.input.physical_use_held = true;
    payload.input.selected_slot = slot;
    payload.input.use_ticks_left = ticks;
    payload.input.allow_desync = desync;
    frame.no_use_delay = payload;
    return frame;
}

static ExecutionFrame item_use_fix_frame(unsigned event, bool hitting, bool skip = false) {
    ExecutionFrame frame;
    frame.event = event;
    frame.world_present = true;
    frame.player_present = true;
    frame.item_use_fix = ItemUseFixTickPayload{{skip, hitting}, {}, 0};
    return frame;
}

static ExecutionFrame ping_fix_frame(unsigned event, PingPacketClass packet_type) {
    ExecutionFrame frame;
    frame.event = event;
    frame.world_present = true;
    frame.player_present = true;
    frame.ping_fix = PingFixTickPayload{{packet_type}, {}, 0};
    return frame;
}

static std::string coverage_json(const ExecutionCoverage& c,
                                 bool checked_candidate_catalog) {
    return "{\n"
           "  \"candidate_catalog_fingerprint_checked\": " +
           std::string(checked_candidate_catalog ? "true" : "false") + ",\n" +
           "  \"registered_modules\": " + std::to_string(c.registered_modules) + ",\n" +
           "  \"modules_with_native_handlers\": " + std::to_string(c.modules_with_native_handlers) + ",\n" +
           "  \"declared_native_callback_subscriptions\": " +
                std::to_string(c.native_callback_subscriptions) + ",\n" +
           "  \"modules_with_firstparty_offline_bindings\": " +
                std::to_string(c.modules_with_executable_bindings) + ",\n" +
           "  \"firstparty_offline_callback_bindings\": " +
                std::to_string(c.executable_callback_bindings) + ",\n" +
           "  \"unimplemented_native_callback_subscriptions\": " +
                std::to_string(c.unimplemented_callback_subscriptions) + ",\n" +
           "  \"minecraft_runtime_verified\": false,\n"
           "  \"original_module_algorithm_equivalence_verified\": false\n"
           "}\n";
}

int main(int argc, char** argv) {
    try {
        // Offline manifest is private only when explicitly supplied. The
        // public synthetic CI fixture contains invented module records.
        if (argc < 2 || argc > 5)
            throw std::invalid_argument("Usage: audit_implementation_coverage TSV [--expect-candidate] [--json-out FILE]");
        bool expect_candidate = false;
        std::string json_path;
        for (int arg = 2; arg < argc; ++arg) {
            const std::string option = argv[arg];
            if (option == "--expect-candidate") expect_candidate = true;
            else if (option == "--json-out" && arg + 1 < argc) json_path = argv[++arg];
            else throw std::invalid_argument("unknown/duplicate command argument");
        }
        const CandidateCatalog manifest = read_candidate_catalog(argv[1], expect_candidate);
        ModuleExecutionBridge bridge;
        for (const auto& descriptor : manifest.modules)
            bridge.add(descriptor);
        const auto before = bridge.coverage();
        require(before.registered_modules == manifest.modules.size(),
                "manifest module count lost during registration");
        require(before.native_callback_subscriptions == manifest.native_subscriptions,
                "manifest callback count lost during registration");
        require(before.executable_callback_bindings == 0,
                "unbound candidate callback was counted as implemented");
        for (unsigned event = 0; event < kEventSlots; ++event)
            require(before.declared_per_event[event] == manifest.subscriptions_per_event[event],
                    "declared per-event callback histogram differs");
        require(bridge.bind_reach_from_catalog(),
                "no independently implemented Reach event-8 binding available");
        std::size_t jump_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "No Jump Delay") jump_id = i;
        const bool has_jump = jump_id < manifest.modules.size();
        if (has_jump)
            require(bridge.bind_no_jump_delay_from_catalog(),
                    "No Jump Delay is present but native event 17/movement binding failed");
        std::size_t null_move_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "Null Move") null_move_id = i;
        const bool has_null_move = null_move_id < manifest.modules.size();
        if (has_null_move)
            require(bridge.bind_null_move_from_catalog(),
                    "Null Move exists but original event 16/movement binding failed");
        std::size_t instant_stop_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "Instant Stop") instant_stop_id = i;
        const bool has_instant_stop = instant_stop_id < manifest.modules.size();
        if (has_instant_stop)
            require(bridge.bind_instant_stop_from_catalog(),
                    "Instant Stop exists but event 16/priority 200/movement binding failed");
        std::size_t no_slow_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "No Slow") no_slow_id = i;
        const bool has_no_slow = no_slow_id < manifest.modules.size();
        if (has_no_slow)
            require(bridge.bind_no_slow_from_catalog(),
                    "No Slow exists but native event 21/priority 200/movement binding failed");
        std::size_t no_hit_delay_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "No Hit Delay") no_hit_delay_id = i;
        const bool has_no_hit_delay = no_hit_delay_id < manifest.modules.size();
        if (has_no_hit_delay)
            require(bridge.bind_no_hit_delay_from_catalog(),
                    "No Hit Delay exists but event 14/priority 0/utility binding failed");
        std::size_t no_use_delay_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "No Use Delay") no_use_delay_id = i;
        const bool has_no_use_delay = no_use_delay_id < manifest.modules.size();
        if (has_no_use_delay)
            require(bridge.bind_no_use_delay_from_catalog(),
                    "No Use Delay exists but event 18/priority 0/utility binding failed");
        std::size_t item_use_fix_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "Item Use Fix") item_use_fix_id = i;
        const bool has_item_use_fix = item_use_fix_id < manifest.modules.size();
        if (has_item_use_fix)
            require(bridge.bind_item_use_fix_from_catalog(),
                    "Item Use Fix exists but original 14+15/utility binding failed");
        std::size_t ping_fix_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "Ping Fix") ping_fix_id = i;
        const bool has_ping_fix = ping_fix_id < manifest.modules.size();
        if (has_ping_fix)
            require(bridge.bind_ping_fix_from_catalog(),
                    "Ping Fix exists but original incoming/outgoing priorities mismatch");
        const std::size_t expected_bindings = 1 + (has_jump ? 1u : 0u) +
                                               (has_null_move ? 1u : 0u) +
                                               (has_instant_stop ? 1u : 0u) +
                                               (has_no_slow ? 1u : 0u) +
                                               (has_no_hit_delay ? 1u : 0u) +
                                               (has_no_use_delay ? 1u : 0u) +
                                               (has_item_use_fix ? 2u : 0u) +
                                               (has_ping_fix ? 2u : 0u);
        const auto c = bridge.coverage();
        require(c.registered_modules == manifest.modules.size(), "module coverage drift");
        require(c.native_callback_subscriptions == manifest.native_subscriptions,
                "callback coverage drift");
        require(c.modules_with_executable_bindings ==
                    expected_bindings - (has_item_use_fix ? 1u : 0u) -
                    (has_ping_fix ? 1u : 0u) &&
                c.executable_callback_bindings == expected_bindings,
                "incorrectly counted unimplemented module functions");
        require(c.unimplemented_callback_subscriptions + c.executable_callback_bindings ==
                c.native_callback_subscriptions, "callback coverage does not reconcile");
        require(c.implemented_per_event[kReachQueryEvent] == 1,
                "Reach callback not routed through event 8");
        require(c.implemented_per_event[kPlayerUpdateEvent] == (has_jump ? 1u : 0u),
                "No Jump Delay callback not routed through event 17");
        require(c.implemented_per_event[kMovementTickEvent] ==
                    (has_null_move ? 1u : 0u) + (has_instant_stop ? 1u : 0u),
                "Movement callbacks not routed through event 16");
        require(c.implemented_per_event[kNoSlowEvent] == (has_no_slow ? 1u : 0u),
                "No Slow callback not routed through event 21");

        require(c.implemented_per_event[kNoUseDelayEvent] == (has_no_use_delay ? 1u : 0u),
                "No Use Delay callback not routed through event 18");
        require(c.implemented_per_event[kItemUseFixBeforeEvent] ==
                    (has_no_hit_delay ? 1u : 0u) + (has_item_use_fix ? 1u : 0u),
                "Shared event 14 module binding inventory mismatch");
        require(c.implemented_per_event[kItemUseFixAfterEvent] == (has_item_use_fix ? 1u : 0u),
                "Item Use Fix after callback not routed through event 15");
        require(c.implemented_per_event[kIncomingPacketEvent] == (has_ping_fix ? 1u : 0u),
                "Ping Fix incoming event 2 not routed");
        require(c.implemented_per_event[kOutgoingPacketEvent] == (has_ping_fix ? 1u : 0u),
                "Ping Fix outgoing event 3 not routed");
        for (unsigned event = 0; event < kEventSlots; ++event)
            if (event != kReachQueryEvent && event != kPlayerUpdateEvent &&
                event != kMovementTickEvent && event != kNoSlowEvent &&
                event != kNoHitDelayEvent && event != kNoUseDelayEvent &&
                event != kItemUseFixAfterEvent &&
                event != kIncomingPacketEvent && event != kOutgoingPacketEvent)
                require(c.implemented_per_event[event] == 0,
                        "unimplemented native callback incorrectly counted");

        std::size_t reach_id = manifest.modules.size();
        for (std::size_t i = 0; i < manifest.modules.size(); ++i)
            if (manifest.modules[i].name == "Reach") reach_id = i;
        require(reach_id < manifest.modules.size(), "Reach module absent");
        require(bridge.request(reach_id, true), "Reach module enable rejected");
        require(!bridge.apply(false, false), "world admission guard bypassed");
        require(bridge.apply(true, true), "world admission failed");
        auto frame = reach_frame();
        require(bridge.dispatch(frame), "offline Reach event rejected");
        require(frame.reach && frame.reach->executions == 1 &&
                frame.reach->proposed_distance &&
                *frame.reach->proposed_distance == 3.2f,
                "first-party ReachController did not execute correctly");
        require(frame.executed_modules.size() == 1 &&
                frame.executed_modules.front() == reach_id,
                "an unimplemented module callback was invoked");

        frame = reach_frame(123);
        require(bridge.dispatch(frame) && frame.reach &&
                frame.reach->executions == 1 &&
                !frame.reach->proposed_distance,
                "first-party Reach cooldown did not suppress the query");
        require(bridge.reach_cooldown_ticks() == 9,
                "first-party Reach cooldown not owned by bridge");
        require(bridge.request(reach_id, false), "Reach disable rejected");
        require(bridge.apply(true, true), "Reach disable not applied");
        frame = reach_frame(123);
        require(bridge.dispatch(frame) && frame.reach &&
                frame.reach->executions == 0 && frame.executed_modules.empty(),
                "disabled Reach callback still executable");

        if (has_jump) {
            require(bridge.request(jump_id, true), "No Jump Delay enable rejected");
            require(bridge.apply(true, true), "No Jump Delay enable not applied");
            auto tick = jump_frame(true);
            require(bridge.dispatch(tick), "No Jump Delay tick rejected");
            require(tick.jump_delay && tick.jump_delay->executions == 1 &&
                    tick.jump_delay->proposed_jump_ticks == 0,
                    "first-party No Jump Delay did not reset the cooldown");
            require(tick.executed_modules.size() == 1 &&
                    tick.executed_modules.front() == jump_id,
                    "unimplemented event-17 module executed");
            tick = jump_frame(false);
            require(bridge.dispatch(tick) && tick.jump_delay &&
                    tick.jump_delay->executions == 1 &&
                    !tick.jump_delay->proposed_jump_ticks,
                    "No Jump Delay wrote to an out-of-game input");
            require(bridge.request(jump_id, false), "No Jump Delay disable rejected");
            require(bridge.apply(true, true), "No Jump Delay disable not applied");
            tick = jump_frame(true);
            require(bridge.dispatch(tick) && tick.jump_delay &&
                    tick.jump_delay->executions == 0 &&
                    !tick.jump_delay->proposed_jump_ticks,
                    "disabled No Jump Delay callback still executed");
        }

        if (has_null_move) {
            require(bridge.request(null_move_id, true), "Null Move enable rejected");
            require(bridge.apply(true, true), "Null Move enable not applied");
            auto movement = null_move_frame(100, true, false);
            require(bridge.dispatch(movement) && movement.null_move &&
                    movement.null_move->executions == 1,
                    "Null Move first movement tick not executed");
            movement = null_move_frame(101, true, true);
            require(bridge.dispatch(movement) && movement.null_move &&
                    movement.null_move->executions == 1 &&
                    movement.null_move->decision.overrides[0] == false &&
                    !movement.null_move->decision.resulting_game_pressed[0],
                    "Null Move failed to prioritize newly pressed back key");
            require(movement.executed_modules.size() == 1 &&
                    movement.executed_modules.front() == null_move_id,
                    "unimplemented event-16 module executed");
            require(bridge.request(null_move_id, false), "Null Move disable rejected");
            require(bridge.apply(true, true), "Null Move disable not applied");
            movement = null_move_frame(102, false, false);
            require(bridge.dispatch(movement) && movement.null_move &&
                    movement.null_move->executions == 0 &&
                    movement.executed_modules.empty(),
                    "disabled Null Move still executed");
        }

        if (has_instant_stop) {
            if (has_null_move) {
                bridge.reset_null_move_world_state();
                require(bridge.request(null_move_id, true),
                        "Null Move could not be readmitted for shared movement event");
            }
            bridge.reset_instant_stop_world_state();
            require(bridge.request(instant_stop_id, true),
                    "Instant Stop enable rejected");
            require(bridge.apply(true, true), "Instant Stop enable not applied");
            auto movement = combined_movement_frame(200, true, 0.0, 0.10,
                                                   has_null_move);
            require(bridge.dispatch(movement) && movement.instant_stop &&
                    movement.instant_stop->executions == 1 &&
                    movement.instant_stop->decision.phase ==
                        InstantStopPhase::watching_forward,
                    "Instant Stop failed to observe held forward key");
            movement = combined_movement_frame(201, false, 0.0, 0.10,
                                               has_null_move);
            require(bridge.dispatch(movement) && movement.instant_stop &&
                    movement.instant_stop->executions == 1 &&
                    movement.instant_stop->decision.choice ==
                        InstantStopChoice::back_only,
                    "Instant Stop failed to minimize forward slide");
            require(movement.executed_modules.size() ==
                    (has_null_move ? 2u : 1u) &&
                    movement.executed_modules.front() == instant_stop_id,
                    "Instant Stop priority was not first");
            if (has_null_move)
                require(movement.executed_modules.back() == null_move_id &&
                        movement.null_move &&
                        movement.null_move->input.game_pressed[
                            static_cast<unsigned>(MoveKey::back)] &&
                        movement.null_move->executions == 1,
                        "Null Move did not see high-priority counter input");
            movement = combined_movement_frame(202, false, 0.0, 0.0,
                                               has_null_move);
            require(bridge.dispatch(movement) &&
                    movement.instant_stop->decision.phase == InstantStopPhase::idle &&
                    movement.instant_stop->decision.overrides[
                        static_cast<unsigned>(MoveKey::back)] == false,
                    "Instant Stop failed to restore physical key state");
            require(bridge.request(instant_stop_id, false),
                    "Instant Stop disable rejected");
            require(bridge.apply(true, true), "Instant Stop disable not applied");
            movement = combined_movement_frame(203, false, 0.0, 0.10,
                                               has_null_move);
            require(bridge.dispatch(movement) &&
                    movement.instant_stop->executions == 0 &&
                    movement.executed_modules.size() == (has_null_move ? 1u : 0u),
                    "disabled Instant Stop still executed");
        }

        if (has_no_slow) {
            require(bridge.request(no_slow_id, true), "No Slow enable rejected");
            require(!bridge.apply(false, false), "No Slow bypassed world gate");
            require(bridge.apply(true, true), "No Slow enable not applied");
            auto slowed = no_slow_frame(261);
            require(bridge.dispatch(slowed) && slowed.no_slow &&
                    slowed.no_slow->executions == 1 &&
                    slowed.no_slow->decision.replace_input &&
                    std::abs(slowed.no_slow->decision.strafe - 0.5f) < 0.00001f,
                    "No Slow bow multiplier mismatch");
            require(slowed.executed_modules.size() == 1 &&
                    slowed.executed_modules.front() == no_slow_id,
                    "unimplemented native event-21 module executed");
            slowed = no_slow_frame(267);
            require(bridge.dispatch(slowed) && slowed.no_slow &&
                    slowed.no_slow->decision.replace_input &&
                    std::abs(slowed.no_slow->decision.strafe - 0.2f) < 0.00001f,
                    "No Slow sword multiplier mismatch");
            slowed = no_slow_frame(349);
            require(bridge.dispatch(slowed) && slowed.no_slow &&
                    slowed.no_slow->decision.replace_input &&
                    std::abs(slowed.no_slow->decision.strafe - 0.4f) < 0.00001f,
                    "No Slow consumable multiplier mismatch");
            slowed = no_slow_frame(261, false);
            require(bridge.dispatch(slowed) && slowed.no_slow &&
                    !slowed.no_slow->decision.replace_input,
                    "No Slow acted without item use");
            require(bridge.request(no_slow_id, false), "No Slow disable rejected");
            require(bridge.apply(true, true), "No Slow disable not applied");
            slowed = no_slow_frame(261);
            require(bridge.dispatch(slowed) && slowed.no_slow &&
                    slowed.no_slow->executions == 0 &&
                    slowed.executed_modules.empty(),
                    "disabled No Slow still executed");
        }

        if (has_no_hit_delay) {
            require(bridge.request(no_hit_delay_id, true), "No Hit Delay enable rejected");
            require(!bridge.apply(false, false), "No Hit Delay bypassed world gate");
            require(bridge.apply(true, true), "No Hit Delay enable not applied");
            auto hit = no_hit_delay_frame();
            require(bridge.dispatch(hit) && hit.no_hit_delay &&
                    hit.no_hit_delay->executions == 1 &&
                    hit.no_hit_delay->decision.clear_click_counter &&
                    !hit.no_hit_delay->decision.swing_arm,
                    "No Hit Delay mode 0 did not clear counter");
            require(hit.executed_modules.size() == 1 &&
                    hit.executed_modules.front() == no_hit_delay_id,
                    "unimplemented event 14 module executed");
            hit = no_hit_delay_frame(1, false);
            require(bridge.dispatch(hit) && hit.no_hit_delay &&
                    !hit.no_hit_delay->decision.clear_click_counter &&
                    hit.no_hit_delay->decision.swing_arm,
                    "No Hit Delay selective-miss did not propose swing");
            hit = no_hit_delay_frame(1, false, 0, true);
            require(bridge.dispatch(hit) && hit.no_hit_delay &&
                    !hit.no_hit_delay->decision.clear_click_counter &&
                    !hit.no_hit_delay->decision.swing_arm,
                    "No Hit Delay swung while using item");
            hit = no_hit_delay_frame(1, true, 1);
            require(bridge.dispatch(hit) && hit.no_hit_delay &&
                    hit.no_hit_delay->decision.clear_click_counter,
                    "No Hit Delay selective-hit did not clear counter");
            require(bridge.request(no_hit_delay_id, false), "No Hit Delay disable rejected");
            require(bridge.apply(true, true), "No Hit Delay disable not applied");
            hit = no_hit_delay_frame();
            require(bridge.dispatch(hit) && hit.no_hit_delay &&
                    hit.no_hit_delay->executions == 0 &&
                    hit.executed_modules.empty(),
                    "disabled No Hit Delay still executed");
        }

        if (has_no_use_delay) {
            require(bridge.request(no_use_delay_id, true), "No Use Delay enable rejected");
            require(!bridge.apply(false, false), "No Use Delay bypassed world gate");
            require(bridge.apply(true, true), "No Use Delay enable not applied");
            auto use = no_use_delay_frame(3);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->executions == 1 &&
                    use.no_use_delay->decision.tracked_remaining_ticks == 3 &&
                    !use.no_use_delay->decision.proposed_use_key_pressed,
                    "No Use Delay failed to initialize countdown");
            require(use.executed_modules.size() == 1 &&
                    use.executed_modules.front() == no_use_delay_id,
                    "unimplemented event-18 callback executed");
            use = no_use_delay_frame(2);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->decision.tracked_remaining_ticks == 2 &&
                    !use.no_use_delay->decision.proposed_use_key_pressed,
                    "No Use Delay tick did not decrement countdown");
            use = no_use_delay_frame(1);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->decision.tracked_remaining_ticks == 1,
                    "No Use Delay countdown second tick drift");
            use = no_use_delay_frame(0);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->decision.proposed_use_key_pressed == false,
                    "No Use Delay did not release completed item-use");
            use = no_use_delay_frame(5, 1);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->decision.tracked_slot == 1 &&
                    use.no_use_delay->decision.proposed_use_key_pressed == true,
                    "No Use Delay did not restore use key on slot change");
            use = no_use_delay_frame(0, 1, true);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->decision.proposed_use_key_pressed == false,
                    "No Use Delay desync setting did not release key early");
            require(bridge.request(no_use_delay_id, false),
                    "No Use Delay disable rejected");
            require(bridge.apply(true, true), "No Use Delay disable not applied");
            use = no_use_delay_frame(3);
            require(bridge.dispatch(use) && use.no_use_delay &&
                    use.no_use_delay->executions == 0 &&
                    use.executed_modules.empty(),
                    "disabled No Use Delay callback executed");
        }

        if (has_item_use_fix) {
            require(bridge.request(item_use_fix_id, true), "Item Use Fix enable rejected");
            require(!bridge.apply(false, false), "Item Use Fix bypassed world gate");
            require(bridge.apply(true, true), "Item Use Fix enable failed");
            auto use = item_use_fix_frame(14, true);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    use.item_use_fix->executions == 1 &&
                    use.item_use_fix->decision.proposed_hitting_block == false,
                    "Item Use Fix failed to clear hitting-block on pre-click");
            require(use.executed_modules.size() == 1 &&
                    use.executed_modules.front() == item_use_fix_id,
                    "unimplemented event14 callback ran");
            use = item_use_fix_frame(15, false);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    use.item_use_fix->decision.proposed_hitting_block == true,
                    "Item Use Fix failed to restore hitting-block on post-click");
            use = item_use_fix_frame(14, false);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    !use.item_use_fix->decision.proposed_hitting_block,
                    "Item Use Fix unnecessarily cleared hitting-block");
            use = item_use_fix_frame(15, false);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    !use.item_use_fix->decision.proposed_hitting_block,
                    "Item Use Fix restored stale hitting-block flag");
            require(bridge.request(item_use_fix_id, false),
                    "Item Use Fix disable rejected");
            require(bridge.apply(true, true), "Item Use Fix disable failed");
            use = item_use_fix_frame(14, true);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    use.item_use_fix->executions == 0 &&
                    use.executed_modules.empty(),
                    "disabled Item Use Fix pre-handler executed");
            use = item_use_fix_frame(15, false);
            require(bridge.dispatch(use) && use.item_use_fix &&
                    use.item_use_fix->executions == 0,
                    "disabled Item Use Fix post-handler executed");
        }

        if (has_ping_fix) {
            require(bridge.request(ping_fix_id, true), "Ping Fix enable rejected");
            require(!bridge.apply(false, false), "Ping Fix bypassed world gate");
            require(bridge.apply(true, true), "Ping Fix enable failed");
            auto incoming = ping_fix_frame(
                kIncomingPacketEvent, PingPacketClass::s00_server_keepalive);
            require(bridge.dispatch(incoming) && incoming.ping_fix &&
                    incoming.ping_fix->executions == 1 &&
                    incoming.ping_fix->decision.proposed_packet_state == 3 &&
                    incoming.executed_modules.size() == 1 &&
                    incoming.executed_modules.front() == ping_fix_id,
                    "Ping Fix failed to identify incoming S00 keep-alive");
            auto outgoing = ping_fix_frame(
                kOutgoingPacketEvent, PingPacketClass::c00_client_keepalive);
            require(bridge.dispatch(outgoing) && outgoing.ping_fix &&
                    outgoing.ping_fix->executions == 1 &&
                    outgoing.ping_fix->decision.proposed_packet_state == 3 &&
                    outgoing.executed_modules.size() == 1 &&
                    outgoing.executed_modules.front() == ping_fix_id,
                    "Ping Fix failed to identify outgoing C00 keep-alive");
            incoming = ping_fix_frame(
                kIncomingPacketEvent, PingPacketClass::c00_client_keepalive);
            require(bridge.dispatch(incoming) && incoming.ping_fix &&
                    !incoming.ping_fix->decision.proposed_packet_state,
                    "Ping Fix incorrectly selected opposite-direction packet");
            outgoing = ping_fix_frame(
                kOutgoingPacketEvent, PingPacketClass::other);
            require(bridge.dispatch(outgoing) && outgoing.ping_fix &&
                    !outgoing.ping_fix->decision.proposed_packet_state,
                    "Ping Fix incorrectly selected unrelated packet");
            require(bridge.request(ping_fix_id, false),
                    "Ping Fix disable rejected");
            require(bridge.apply(true, true), "Ping Fix disable failed");
            outgoing = ping_fix_frame(
                kOutgoingPacketEvent, PingPacketClass::c00_client_keepalive);
            require(bridge.dispatch(outgoing) && outgoing.ping_fix &&
                    outgoing.ping_fix->executions == 0 &&
                    outgoing.executed_modules.empty(),
                    "disabled Ping Fix outgoing callback still executed");
            incoming = ping_fix_frame(
                kIncomingPacketEvent, PingPacketClass::s00_server_keepalive);
            require(bridge.dispatch(incoming) && incoming.ping_fix &&
                    incoming.ping_fix->executions == 0 &&
                    incoming.executed_modules.empty(),
                    "disabled Ping Fix incoming callback still executed");
        }

        if (expect_candidate)
            require(has_jump && has_null_move && has_instant_stop &&
                    has_no_slow && has_no_hit_delay && has_no_use_delay &&
                    has_item_use_fix && has_ping_fix &&
                    c.registered_modules == 66 &&
                    c.native_callback_subscriptions == 171 &&
                    c.modules_with_native_handlers == 64 &&
                    c.modules_with_executable_bindings == 9 &&
                    c.executable_callback_bindings == 11 &&
                    c.unimplemented_callback_subscriptions == 160,
                    "private 66-module catalog implementation gap was misreported");

        const std::string report = coverage_json(c, expect_candidate);
        if (!json_path.empty()) {
            std::ofstream out(json_path);
            require(out.good(), "cannot write coverage report");
            out << report;
            require(out.good(), "coverage report write failed");
        }
        std::cout << report;
        bridge.stop();
        require(bridge.implemented_event_bindings() == 0,
                "callback binding leaked after shutdown");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "R186 coverage audit failed: " << e.what() << '\n';
        return 1;
    }
}
