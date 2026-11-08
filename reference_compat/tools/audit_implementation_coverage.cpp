// R178: read-only 66-module catalog to independently written execution bridge.
// No original client executable, Minecraft process, native hooks or game I/O.
#include "slinkywinky/candidate_catalog.hpp"
#include "slinkywinky/module_execution_bridge.hpp"

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
        const auto c = bridge.coverage();
        require(c.registered_modules == manifest.modules.size(), "module coverage drift");
        require(c.native_callback_subscriptions == manifest.native_subscriptions,
                "callback coverage drift");
        require(c.modules_with_executable_bindings == 1 &&
                c.executable_callback_bindings == 1,
                "incorrectly counted unimplemented module functions");
        require(c.unimplemented_callback_subscriptions + c.executable_callback_bindings ==
                c.native_callback_subscriptions, "callback coverage does not reconcile");
        require(c.implemented_per_event[kReachQueryEvent] == 1,
                "Reach callback not routed through event 8");
        for (unsigned event = 0; event < kEventSlots; ++event)
            if (event != kReachQueryEvent)
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

        if (expect_candidate)
            require(c.registered_modules == 66 &&
                    c.native_callback_subscriptions == 171 &&
                    c.modules_with_native_handlers == 64 &&
                    c.unimplemented_callback_subscriptions == 170,
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
        std::cerr << "R178 coverage audit failed: " << e.what() << '\n';
        return 1;
    }
}
