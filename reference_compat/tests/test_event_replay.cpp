#include "slinkywinky/reference_event_model.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void verify(bool truth, const char* what, int line) {
    if (!truth) throw std::runtime_error(std::string("R174 line ")+std::to_string(line)+": "+what);
}
#define CHECK(expr) verify(static_cast<bool>(expr), #expr, __LINE__)
static ModuleSpec spec(std::string name, std::vector<Subscription> slots,
                       bool enabled = false, unsigned category = 0) {
    return {std::move(name), category, std::move(slots), enabled};
}
int main() {
    // Check event priorities, valid slots and safe refusal of invalid ones.
    EventReplay engine;
    const auto first = engine.add(spec("a", {{2, 1}, {5, 1}, {4, 1}}, true));
    const auto second = engine.add(spec("b", {{2, 10}, {5, 10}}, true));
    const auto third = engine.add(spec("c", {{2, 5}, {5, 5}}));
    CHECK(first == 0 && second == 1 && third == 2);
    CHECK((engine.handlers(2) == std::vector<std::size_t>{1, 0}));
    CHECK(engine.dispatch(2));
    CHECK(engine.trace().size() == 2 && engine.trace()[0].recipient == second);
    engine.clear_trace();
    CHECK(engine.dispatch(26));
    CHECK(!engine.dispatch(27));
    CHECK(!engine.dispatch(999));
    CHECK(engine.trace().empty());

    // Main-menu guard defers enables until world and player both exist.
    CHECK(engine.request(third, true));
    CHECK(engine.pending_enables() == 1);
    CHECK(!engine.request(third, true)); // deliberate duplicate rejection
    CHECK(!engine.apply(false, false));
    CHECK(!engine.apply(true, false));
    CHECK(!engine.enabled(third) && engine.pending_enables() == 1);
    CHECK(engine.apply(true, true));
    CHECK(engine.enabled(third) && engine.pending_enables() == 0);
    CHECK(engine.trace().size() == 3);
    CHECK(engine.trace()[0].recipient == second && engine.trace()[0].target == third);
    CHECK(engine.trace()[1].recipient == third && engine.trace()[2].recipient == first);
    CHECK(engine.trace()[0].new_state);
    engine.clear_trace();

    // Disables can be vetoed by event 4, then notify event 5 before rebuild.
    bool should_veto = true;
    EventReplay vetoer([&](const Delivery& d, bool& veto) {
        if (d.event == 4 && should_veto) veto = true;
    });
    auto guard = vetoer.add(spec("guard", {{4, 1}, {5, 1}}, true));
    auto victim = vetoer.add(spec("victim", {{5, 2}}, true));
    CHECK(!vetoer.request(victim, false));
    CHECK(vetoer.pending_disables() == 0);
    should_veto = false;
    vetoer.clear_trace();
    CHECK(vetoer.request(victim, false));
    CHECK(vetoer.trace().size() == 1 && vetoer.trace()[0].event == 4);
    CHECK(vetoer.enabled(victim));
    vetoer.clear_trace();
    CHECK(vetoer.apply(true, true));
    CHECK(vetoer.trace().size() == 2);
    CHECK(vetoer.trace()[0].recipient == victim && !vetoer.trace()[0].new_state);
    CHECK(vetoer.trace()[1].recipient == guard);
    CHECK(!vetoer.enabled(victim));
    CHECK((vetoer.handlers(5) == std::vector<std::size_t>{guard}));

    // Enable phases happen before disables regardless of request order.
    EventReplay phases;
    auto x = phases.add(spec("x", {{5, 1}}, true));
    auto y = phases.add(spec("y", {{5, 2}}));
    auto z = phases.add(spec("z", {{5, 3}}));
    CHECK(phases.request(x, false));
    CHECK(phases.request(y, true));
    CHECK(phases.request(z, true));
    CHECK(phases.apply(true, true));
    CHECK(phases.trace().size() == 9);
    CHECK(phases.trace()[0].target == y && phases.trace()[0].new_state);
    CHECK(phases.trace()[3].target == z && phases.trace()[3].new_state);
    CHECK(phases.trace()[6].target == x && !phases.trace()[6].new_state);
    CHECK(!phases.enabled(x) && phases.enabled(y) && phases.enabled(z));
    CHECK(phases.handlers(5).size() == 2);

    // Exactly 66 valid descriptors, never 67, with bounded pending queue.
    EventReplay bounded;
    for (std::size_t i = 0; i < kCandidateModules; ++i)
        CHECK(bounded.add(spec("module_" + std::to_string(i), {{0, 0}})) == i);
    bool rejected = false;
    try { bounded.add(spec("module_66", {{0, 0}})); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected && bounded.module_count() == kCandidateModules);
    CHECK(!bounded.request(66, true));
    for (std::size_t i = 0; i < kCandidateModules; ++i) CHECK(bounded.request(i, true));
    CHECK(bounded.pending_enables() == 66);
    CHECK(bounded.apply(true, true));
    CHECK(bounded.pending_enables() == 0 && bounded.handlers(0).size() == 66);
    bounded.stop();
    CHECK(!bounded.dispatch(0));
    CHECK(!bounded.request(0, false));
    CHECK(!bounded.apply(true, true));
    CHECK(bounded.handlers(0).empty());

    auto expect_invalid = [](ModuleSpec candidate) {
        EventReplay t;
        try { t.add(std::move(candidate)); }
        catch (const std::invalid_argument&) { return; }
        CHECK(false && "invalid descriptor accepted");
    };
    expect_invalid(spec("category", {}, false, 5));
    expect_invalid(spec("event", {{27, 1}}));
    expect_invalid(spec("duplicate_event", {{4, 1}, {4, 2}}));
    expect_invalid(spec("", {{4, 1}}));

    std::cout << "R174 event replay PASS: 66 modules, 27 event slots, "
                 "priority, cancellation, lifecycle and bounds\n";
}
