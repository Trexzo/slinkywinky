#pragma once
// R178: parse metadata-only candidate event manifest without private binaries.
// This is an independently written, read-only offline input adapter.
#include "slinkywinky/reference_event_model.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {

struct CandidateCatalog {
    std::vector<ModuleSpec> modules;
    std::array<std::size_t, kEventSlots> subscriptions_per_event{};
    std::size_t native_subscriptions = 0;
    std::size_t modules_without_callbacks = 0;
};

inline int parse_catalog_integer(const std::string& value) {
    if (value.empty()) throw std::invalid_argument("empty integer in candidate catalog");
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument("invalid integer in candidate catalog");
    return result;
}
inline std::vector<std::string> split_catalog_field(const std::string& value, char sep) {
    std::vector<std::string> fields;
    std::size_t cursor = 0;
    while (true) {
        const auto end = value.find(sep, cursor);
        fields.push_back(value.substr(cursor,
                        end == std::string::npos ? end : end - cursor));
        if (end == std::string::npos) break;
        cursor = end + 1;
    }
    return fields;
}
inline CandidateCatalog read_candidate_catalog(const std::string& path,
                                               bool expect_private_candidate = false) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("candidate catalog cannot be opened");
    std::string line;
    if (!std::getline(file, line)) throw std::runtime_error("candidate catalog is empty");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "id\tname\tcategory\tinitially_enabled\tevent_priorities")
        throw std::runtime_error("candidate catalog header is invalid");

    CandidateCatalog catalog;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto fields = split_catalog_field(line, '\t');
        if (fields.size() != 5) throw std::runtime_error("candidate catalog requires five fields");
        if (catalog.modules.size() >= kCandidateModules)
            throw std::runtime_error("more than 66 candidate module records");
        const int id = parse_catalog_integer(fields[0]);
        const int category = parse_catalog_integer(fields[2]);
        const int initially_enabled = parse_catalog_integer(fields[3]);
        if (id < 0 || static_cast<std::size_t>(id) != catalog.modules.size())
            throw std::runtime_error("candidate descriptor ID/order mismatch");
        if (fields[1].empty() || category < 0 || category > 4 ||
            (initially_enabled != 0 && initially_enabled != 1))
            throw std::runtime_error("invalid candidate descriptor metadata");
        for (const auto& registered : catalog.modules)
            if (registered.name == fields[1])
                throw std::runtime_error("duplicate candidate module name");

        ModuleSpec spec;
        spec.name = fields[1];
        spec.category = static_cast<unsigned>(category);
        spec.initially_enabled = initially_enabled == 1;
        std::array<bool, kEventSlots> seen{};
        if (!fields[4].empty()) {
            for (const auto& pair : split_catalog_field(fields[4], ',')) {
                const auto parts = split_catalog_field(pair, ':');
                if (parts.size() != 2)
                    throw std::runtime_error("malformed candidate event:priority pair");
                const int event = parse_catalog_integer(parts[0]);
                const int priority = parse_catalog_integer(parts[1]);
                if (event < 0 || event >= static_cast<int>(kEventSlots) ||
                    seen[static_cast<unsigned>(event)])
                    throw std::runtime_error("duplicate/out-of-range candidate event");
                seen[static_cast<unsigned>(event)] = true;
                spec.subscriptions.push_back({static_cast<unsigned>(event), priority});
                ++catalog.native_subscriptions;
                ++catalog.subscriptions_per_event[static_cast<unsigned>(event)];
            }
        }
        if (spec.subscriptions.empty()) ++catalog.modules_without_callbacks;
        catalog.modules.push_back(std::move(spec));
    }
    if (catalog.modules.empty()) throw std::runtime_error("candidate catalog has no modules");
    if (expect_private_candidate) {
        constexpr std::array<std::size_t, kEventSlots> histogram{
            24, 5, 6, 10, 3, 25, 7, 4, 4, 3, 3, 2, 1, 5,
            10, 6, 3, 9, 20, 1, 1, 3, 2, 1, 1, 5, 7};
        if (catalog.modules.size() != 66 || catalog.native_subscriptions != 171 ||
            catalog.modules_without_callbacks != 2 ||
            catalog.subscriptions_per_event != histogram)
            throw std::runtime_error("private candidate manifest fingerprint mismatch");
    }
    return catalog;
}
} // namespace slinkywinky::reference_compat
