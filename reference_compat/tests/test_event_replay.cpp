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


    // R191: an observer must not re-enter dispatch, queue mutation or
    // registration while an event is being delivered.
    EventReplay* during_callback = nullptr;
    int reentrant_observations = 0;
    EventReplay reentrant([&](const Delivery& delivery, bool&) {
        if (delivery.event != 2) return;
        ++reentrant_observations;
        CHECK(!during_callback->dispatch(2));
        CHECK(!during_callback->request(0, false));
        CHECK(!during_callback->apply(true, true));
        bool rejected_registration = false;
        try { during_callback->add(spec("late", {{2, 1}})); }
        catch (const std::invalid_argument&) { rejected_registration = true; }
        CHECK(rejected_registration);
    });
    during_callback = &reentrant;
    reentrant.add(spec("observer", {{2, 5}}, true));
    reentrant.add(spec("other", {{2, 1}}, true));
    CHECK(reentrant.dispatch(2));
    CHECK(reentrant_observations == 2);
    CHECK(reentrant.module_count() == 2 && reentrant.pending_disables() == 0);
    CHECK(reentrant.trace().size() == 2);

    // stop() during a delivery must prevent remaining recipients from firing.
    EventReplay* during_stop = nullptr;
    int stop_observations = 0;
    EventReplay stops([&](const Delivery&, bool&) {
        ++stop_observations;
        during_stop->stop();
    });
    during_stop = &stops;
    stops.add(spec("stopper", {{2, 50}}, true));
    stops.add(spec("should_not_fire", {{2, 10}}, true));
    CHECK(!stops.dispatch(2));
    CHECK(stop_observations == 1 && stops.trace().size() == 1);
    CHECK(stops.handlers(2).empty());
    CHECK(!stops.request(0, false));

    // A notification observer can stop() while apply() is iterating the
    // pending batch; stable snapshots ensure no iterator invalidation.
    EventReplay* during_apply = nullptr;
    int notification_count = 0;
    EventReplay interrupted([&](const Delivery& delivery, bool&) {
        if (delivery.event != 5) return;
        ++notification_count;
        during_apply->stop();
    });
    during_apply = &interrupted;
    interrupted.add(spec("notify", {{5, 1}}, true));
    auto pending_one = interrupted.add(spec("pending_one", {{5, 0}}));
    auto pending_two = interrupted.add(spec("pending_two", {{5, -1}}));
    CHECK(interrupted.request(pending_one, true));
    CHECK(interrupted.request(pending_two, true));
    CHECK(!interrupted.apply(true, true));
    CHECK(notification_count == 1);
    CHECK(interrupted.pending_enables() == 0 && interrupted.pending_disables() == 0);
    CHECK(interrupted.handlers(5).empty());

    // Throwing dispatch observers release the admission fence; throwing
    // apply observers permanently stop a potentially half-applied batch.
    bool throw_once = true;
    EventReplay throwing_dispatch([&](const Delivery&, bool&) {
        if (throw_once) {
            throw_once = false;
            throw std::runtime_error("synthetic dispatch failure");
        }
    });
    throwing_dispatch.add(spec("throwing", {{2, 1}}, true));
    bool dispatch_threw = false;
    try { throwing_dispatch.dispatch(2); }
    catch (const std::runtime_error&) { dispatch_threw = true; }
    CHECK(dispatch_threw && throwing_dispatch.dispatch(2));

    EventReplay throwing_apply([&](const Delivery&, bool&) {
        throw std::runtime_error("synthetic apply failure");
    });
    throwing_apply.add(spec("notify", {{5, 1}}, true));
    auto apply_target = throwing_apply.add(spec("target", {{5, 0}}));
    CHECK(throwing_apply.request(apply_target, true));
    bool apply_threw = false;
    try { throwing_apply.apply(true, true); }
    catch (const std::runtime_error&) { apply_threw = true; }
    CHECK(apply_threw && !throwing_apply.dispatch(5));
    CHECK(throwing_apply.pending_enables() == 0);
    CHECK(throwing_apply.handlers(5).empty());

    std::cout << "R174/R191 event replay PASS: 66 modules, 27 event slots, "
                 "priority, cancellation, lifecycle and bounds\n";
}
