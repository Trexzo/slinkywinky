// Offline-only consumer of private metadata fixture. No game access or JNI.
#include "slinkywinky/reference_event_model.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static int parse_int(const std::string& raw) {
    int n = 0;
    const auto r = std::from_chars(raw.data(), raw.data() + raw.size(), n);
    if (r.ec != std::errc{} || r.ptr != raw.data() + raw.size())
        throw std::invalid_argument("non-integer fixture field");
    return n;
}
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const auto end = s.find(delim, start);
        fields.push_back(s.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return fields;
}
int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--expect-candidate"))
            throw std::invalid_argument("Usage: event_fixture_runner FILE.tsv [--expect-candidate]");
        const bool expect_candidate = argc == 3;
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("fixture TSV could not be opened");
        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("empty fixture TSV");
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line != "id\tname\tcategory\tinitially_enabled\tevent_priorities")
            throw std::runtime_error("bad event-fixture TSV header");
        EventReplay replay;
        std::size_t subscriptions = 0;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto fields = split(line, '\t');
            if (fields.size() != 5) throw std::runtime_error("fixture row must have five columns");
            const int id = parse_int(fields[0]);
            const int cat = parse_int(fields[2]);
            const int initial = parse_int(fields[3]);
            if (id < 0 || cat < 0 || cat >= 5 || (initial != 0 && initial != 1))
                throw std::runtime_error("invalid fixture descriptor");
            ModuleSpec spec;
            spec.name = fields[1];
            spec.category = static_cast<unsigned>(cat);
            spec.initially_enabled = initial != 0;
            if (!fields[4].empty()) {
                for (const auto& token : split(fields[4], ',')) {
                    const auto pair = split(token, ':');
                    if (pair.size() != 2) throw std::runtime_error("invalid event:priority pair");
                    const int slot = parse_int(pair[0]);
                    if (slot < 0 || slot >= static_cast<int>(kEventSlots))
                        throw std::runtime_error("event slot out of bounds");
                    spec.subscriptions.push_back({static_cast<unsigned>(slot), parse_int(pair[1])});
                }
            }
            subscriptions += spec.subscriptions.size();
            const auto assigned = replay.add(std::move(spec));
            if (static_cast<std::size_t>(id) != assigned)
                throw std::runtime_error("descriptor registration order differs");
        }
        if (replay.module_count() == 0) throw std::runtime_error("empty module fixture");
        if (expect_candidate && (replay.module_count() != 66 || subscriptions != 171))
            throw std::runtime_error("candidate module or callback count mismatch");
        for (std::size_t id = 0; id < replay.module_count(); ++id)
            if (!replay.request(id, true)) throw std::runtime_error("enable queue rejected module");
        if (replay.pending_enables() && replay.apply(false, false))
            throw std::runtime_error("module activated before world admission");
        if (!replay.apply(true, true)) throw std::runtime_error("world admission rejected");
        std::array<unsigned, kEventSlots> handlers{};
        unsigned sum = 0;
        for (unsigned slot = 0; slot < kEventSlots; ++slot) {
            handlers[slot] = static_cast<unsigned>(replay.handlers(slot).size());
            sum += handlers[slot];
            if (!replay.dispatch(slot)) throw std::runtime_error("valid event rejected");
        }
        if (sum != subscriptions) throw std::runtime_error("subscription/handler inventory drift");
        if (expect_candidate) {
            constexpr std::array<unsigned, kEventSlots> original = {
                24,5,6,10,3,25,7,4,4,3,3,2,1,5,10,6,3,9,20,1,1,3,2,1,1,5,7};
            if (handlers != original) throw std::runtime_error("original event histogram mismatch");
        }
        replay.stop();
        if (replay.dispatch(0)) throw std::runtime_error("post-stop callback admitted");
        std::cout << "R175 event fixture PASS: modules=" << replay.module_count()
                  << " callbacks=" << sum << " event_slots=" << kEventSlots << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "R175 fixture runner failed: " << e.what() << '\n';
        return 1;
    }
}
