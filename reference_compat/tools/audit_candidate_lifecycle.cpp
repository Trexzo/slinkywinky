// Independent, offline event-lifecycle oracle driven by original-derived metadata.
// Contains no game code, native hooks, injection or proprietary module data.
#include "slinkywinky/reference_event_model.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using slinkywinky::reference_compat::EventReplay;
using slinkywinky::reference_compat::ModuleSpec;
using slinkywinky::reference_compat::Subscription;
using slinkywinky::reference_compat::kEventSlots;

namespace {
void require(bool check, const std::string& error) {
    if (!check) throw std::runtime_error(error);
}

int integer(const std::string& value) {
    int number = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
        throw std::invalid_argument("invalid integer in event fixture");
    return number;
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (true) {
        const auto index = value.find(delimiter, start);
        result.push_back(value.substr(start, index == std::string::npos ? index : index - start));
        if (index == std::string::npos) break;
        start = index + 1;
    }
    return result;
}

std::vector<ModuleSpec> load_fixture(const std::string& path, bool expect_candidate) {
    std::ifstream file(path);
    require(file.good(), "missing TSV fixture");
    std::string line;
    require(static_cast<bool>(std::getline(file, line)), "empty TSV fixture");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    require(line == "id\tname\tcategory\tinitially_enabled\tevent_priorities", "invalid TSV header");
    std::vector<ModuleSpec> result;
    std::size_t callback_count = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto fields = split(line, '\t');
        require(fields.size() == 5, "invalid TSV field count");
        const int id = integer(fields[0]);
        const int category = integer(fields[2]);
        const int initial = integer(fields[3]);
        require(id >= 0 && static_cast<std::size_t>(id) == result.size(), "invalid descriptor order");
        require(category >= 0 && category < 5 && (initial == 0 || initial == 1), "invalid category/state");
        ModuleSpec mod;
        mod.name = fields[1];
        mod.category = static_cast<unsigned>(category);
        mod.initially_enabled = initial == 1;
        if (!fields[4].empty()) {
            for (const auto& pair_text : split(fields[4], ',')) {
                const auto parts = split(pair_text, ':');
                require(parts.size() == 2, "malformed event/priority entry");
                const int event = integer(parts[0]);
                require(event >= 0 && event < static_cast<int>(kEventSlots), "invalid event slot");
                mod.subscriptions.push_back(Subscription{static_cast<unsigned>(event), integer(parts[1])});
            }
        }
        callback_count += mod.subscriptions.size();
        result.push_back(std::move(mod));
    }
    require(!result.empty() && result.size() <= 66, "module count outside bounds");
    if (expect_candidate) require(result.size() == 66 && callback_count == 171,
                                  "private reference count mismatch");
    return result;
}

int priority_for(const ModuleSpec& mod, unsigned event) {
    for (auto item : mod.subscriptions) if (item.event == event) return item.priority;
    throw std::runtime_error("handler not present in fixture");
}

bool subscribes(const ModuleSpec& mod, unsigned event) {
    for (auto item : mod.subscriptions) if (item.event == event) return true;
    return false;
}

std::array<std::size_t, kEventSlots> verify_state(
        EventReplay& replay, const std::vector<ModuleSpec>& specs,
        const std::vector<bool>& expected, const std::string& stage) {
    require(expected.size() == specs.size(), stage + ": size mismatch");
    std::array<std::size_t, kEventSlots> histogram{};
    for (std::size_t id = 0; id < specs.size(); ++id)
        require(replay.enabled(id) == expected[id], stage + ": module state mismatch");
    for (unsigned event = 0; event < kEventSlots; ++event) {
        const auto& handlers = replay.handlers(event);
        std::vector<bool> seen(specs.size(), false);
        bool have_previous = false;
        int last_priority = 0;
        for (const auto id : handlers) {
            require(id < specs.size() && !seen[id], stage + ": duplicate/invalid callback recipient");
            require(expected[id] && subscribes(specs[id], event), stage + ": unexpected callback");
            seen[id] = true;
            const int current_priority = priority_for(specs[id], event);
            if (have_previous) require(last_priority >= current_priority,
                                        stage + ": callback priority inversion");
            last_priority = current_priority;
            have_previous = true;
        }
        for (std::size_t id = 0; id < specs.size(); ++id)
            require(seen[id] == (expected[id] && subscribes(specs[id], event)),
                    stage + ": missing/extra handler");
        histogram[event] = handlers.size();
        replay.clear_trace();
        require(replay.dispatch(event), stage + ": valid event rejected");
        require(replay.trace().size() == handlers.size(), stage + ": trace length mismatch");
        for (std::size_t index = 0; index < handlers.size(); ++index)
            require(replay.trace()[index].recipient == handlers[index] &&
                    replay.trace()[index].event == event,
                    stage + ": callback trace ordering mismatch");
    }
    replay.clear_trace();
    return histogram;
}

std::size_t count_event(const std::vector<ModuleSpec>& specs, const std::vector<bool>& active,
                        unsigned event) {
    std::size_t count = 0;
    for (std::size_t id = 0; id < specs.size(); ++id)
        if (active[id] && subscribes(specs[id], event)) ++count;
    return count;
}

void verify_toggle_notifications(const EventReplay& replay,
                                 std::size_t expected_targets,
                                 std::size_t expected_handlers,
                                 bool expected_new_state) {
    require(replay.trace().size() == expected_targets * expected_handlers,
            "event-5 notification count differs");
    for (const auto& receipt : replay.trace())
        require(receipt.event == 5 && receipt.new_state == expected_new_state,
                "unexpected lifecycle notification");
}
}  // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && std::string(argv[2]) == "--expect-candidate"),
                "usage: audit_candidate_lifecycle FILE.tsv [--expect-candidate]");
        const bool expect_candidate = argc == 3;
        const auto specs = load_fixture(argv[1], expect_candidate);
        EventReplay replay;
        std::vector<bool> active;
        std::size_t originally_off = 0;
        for (const auto& mod : specs) {
            const auto id = replay.add(mod);
            require(id == active.size(), "registration order changed");
            active.push_back(mod.initially_enabled);
            if (!mod.initially_enabled) ++originally_off;
        }
        verify_state(replay, specs, active, "initial");
        for (std::size_t id = 0; id < specs.size(); ++id)
            require(replay.request(id, true), "could not queue module enable");
        require(replay.pending_enables() == originally_off, "enable queue length mismatch");
        require(!replay.apply(false, false) && !replay.apply(true, false),
                "world/player admission guard absent");
        require(replay.pending_enables() == originally_off, "pending enables lost before admission");
        replay.clear_trace();
        require(replay.apply(true, true), "could not activate all modules");
        const std::size_t event5_count = count_event(specs, std::vector<bool>(specs.size(), true), 5);
        verify_toggle_notifications(replay, originally_off, event5_count, true);
        std::fill(active.begin(), active.end(), true);
        const auto histogram = verify_state(replay, specs, active, "all-enabled");
        if (expect_candidate) {
            constexpr std::array<std::size_t, kEventSlots> native_histogram{
                24, 5, 6, 10, 3, 25, 7, 4, 4, 3, 3, 2, 1, 5,
                10, 6, 3, 9, 20, 1, 1, 3, 2, 1, 1, 5, 7};
            require(histogram == native_histogram, "original-derived histogram mismatch");
        }
        // Deactivate the odd-indexed modules; the notifications are emitted
        // before the old event-5 handler list is rebuilt.
        std::size_t disabled = 0;
        for (std::size_t id = 1; id < specs.size(); id += 2) {
            require(replay.request(id, false), "disable request refused");
            ++disabled;
        }
        replay.clear_trace();  // pre-disable event-4 receipts are tested elsewhere
        require(!replay.apply(false, true), "disable incorrectly applied outside world");
        require(replay.apply(true, true), "disable batch failed");
        verify_toggle_notifications(replay, disabled, event5_count, false);
        for (std::size_t id = 1; id < specs.size(); id += 2) active[id] = false;
        verify_state(replay, specs, active, "even-only");

        const std::size_t remaining5 = count_event(specs, active, 5);
        std::size_t remaining = 0;
        for (std::size_t id = 0; id < specs.size(); id += 2) {
            require(replay.request(id, false), "second disable request refused");
            ++remaining;
        }
        replay.clear_trace();
        require(replay.apply(true, true), "second disable batch failed");
        verify_toggle_notifications(replay, remaining, remaining5, false);
        std::fill(active.begin(), active.end(), false);
        const auto empty_histogram = verify_state(replay, specs, active, "all-disabled");
        for (const auto count : empty_histogram) require(count == 0, "handler survived disable");

        // Reenable a noncontiguous subset, verifying rebuilt membership again.
        for (std::size_t id = 0; id < specs.size(); id += 3)
            require(replay.request(id, true), "selective reenable request refused");
        replay.clear_trace();
        require(replay.apply(true, true), "selective reenable failed");
        for (std::size_t id = 0; id < specs.size(); id += 3) active[id] = true;
        verify_state(replay, specs, active, "every-third");
        replay.stop();
        require(!replay.dispatch(0) && !replay.request(0, true) && !replay.apply(true, true),
                "post-shutdown callback admitted");
        for (unsigned event = 0; event < kEventSlots; ++event)
            require(replay.handlers(event).empty(), "handler survived teardown");
        std::cout << "R176 lifecycle PASS modules=" << specs.size()
                  << " event_slots=" << kEventSlots
                  << " stages=5 priority-order=PASS toggle-notices=PASS teardown=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "R176 lifecycle FAIL: " << e.what() << '\n';
        return 1;
    }
}
