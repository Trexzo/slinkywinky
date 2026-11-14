#pragma once
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace slinkywinky {

// R207 Chat 1: copy-only view of Chat 2's version-1 ModuleControlState
// contract. No game pointers cross it. A ClickGUI preview toggle is NOT
// a ModuleArrayListState and must never supply this snapshot by default.
struct ModuleArrayListState {
    std::string canonical_name;
    bool requested_enabled=false;
    bool active_in_world=false;
    std::uint64_t world_generation=0;
};
struct ModuleArrayListSnapshot {
    std::uint32_t contract_version=1u;
    std::uint64_t source_sequence=0u;
    std::uint64_t world_generation=0u;
    bool world_ready=false;
    std::vector<ModuleArrayListState> modules;
};
struct ModuleArrayListDisplay {
    std::uint64_t source_sequence=0u;
    std::uint64_t world_generation=0u;
    std::vector<std::string> active_names;
};

// Validates a copy of the owner-thread runtime state. "Requested" alone
// does not show the row. All names are checked against the pinned candidate
// catalog; a duplicate, unknown or stale-generation record fails closed.
// Returns no display on invalid input, an empty display for a valid but
// inactive/out-of-world snapshot. No module behavior is executed here.
std::optional<ModuleArrayListDisplay> make_module_arraylist_display(
    const ModuleArrayListSnapshot& snapshot);

// R208: bounded, value-only handoff from a module-owner producer to a
// renderer consumer. Publishes whole snapshots, never Java/game pointers.
// A violation quarantines the feed; a new attachment must use a new feed.
// The render owner must quiesce its drawings before calling retire() at unload.
class ModuleArrayListFeed final {
public:
    bool publish(const ModuleArrayListSnapshot& snapshot);
    std::optional<ModuleArrayListSnapshot> latest() const;
    void retire();
    bool retired() const;
    ModuleArrayListFeed()=default;
    ModuleArrayListFeed(const ModuleArrayListFeed&)=delete;
    ModuleArrayListFeed& operator=(const ModuleArrayListFeed&)=delete;
private:
    mutable std::mutex mutex_;
    std::optional<ModuleArrayListSnapshot> latest_;
    std::uint64_t sequence_=0;
    std::uint64_t last_world_generation_=0;
    bool retired_=false;
};

} // namespace slinkywinky
