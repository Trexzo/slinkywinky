#pragma once
// R265: versioned, read-only view joining the 67-name menu catalog to
// authoritative *reported* native module states. Unreported modules stay
// unknown, never falsely displayed as disabled or active. This does not
// implement a module, JVM hook, packet owner or Minecraft input action.
#include "slinkywinky/module_control_capabilities.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace slinkywinky {

// The source ModuleArrayListSnapshot remains contract v1. This is a new
// display/provenance *projection* rather than a wire ABI migration.
inline constexpr std::uint32_t kRuntimeRosterProjectionVersion = 2u;

enum class ModuleReportedAuthority : std::uint8_t {
    DescriptorOnly = 0, // metadata, not one of native 66
    Unobserved = 1,     // registered, but no current owner row received
    NativeOwnerProbe = 2,
    OfflinePolicy = 3
};

struct ModuleRuntimeRosterRow {
    std::string_view canonical_name{};
    MenuTab tab = MenuTab::Combat;
    ModuleControlCapability capability = ModuleControlCapability::Unknown;
    ModuleReportedAuthority authority = ModuleReportedAuthority::Unobserved;
    bool native_registered = false;
    bool source_reported = false;
    bool requested_enabled = false;
    bool owner_reports_active = false;
    // owner_reports_active is NOT a gameplay-action or live-client receipt.
};

struct ModuleRuntimeRoster {
    std::uint32_t projection_version = kRuntimeRosterProjectionVersion;
    std::uint64_t source_sequence = 0;
    std::uint64_t world_generation = 0;
    bool world_ready = false;
    std::size_t native_registered = 0;
    std::size_t source_reported = 0;
    std::size_t native_owner_reports = 0;
    std::size_t offline_policy_reports = 0;
    std::array<ModuleRuntimeRosterRow,67> modules{};
};

// No unreported item is presumed OFF. If a caller needs authoritative
// selection state for a module, that module's own native controller must
// publish it first. Contract-v1's native 66-name identity check is reused.
inline std::optional<ModuleRuntimeRoster>
make_module_runtime_roster(const ModuleArrayListSnapshot& snapshot) {
    if(!make_module_arraylist_display(snapshot)) return std::nullopt;
    ModuleRuntimeRoster result;
    result.source_sequence = snapshot.source_sequence;
    result.world_generation = snapshot.world_generation;
    result.world_ready = snapshot.world_ready;
    const auto catalog = module_control_catalog_capabilities();
    for(std::size_t i=0;i<catalog.size();++i) {
        const auto& row = catalog[i];
        auto& dst = result.modules[i];
        dst.canonical_name = row.canonical_name;
        dst.tab = row.tab;
        dst.capability = row.capability;
        dst.native_registered = row.native_registered;
        dst.authority = row.capability==ModuleControlCapability::DescriptorOnly ?
            ModuleReportedAuthority::DescriptorOnly :
            ModuleReportedAuthority::Unobserved;
        if(dst.native_registered) ++result.native_registered;
    }
    if(result.native_registered!=66u) return std::nullopt;

    // The existing source contract validates duplicates, sequence, world
    // generation and active=>requested, but it does not yet know which
    // registered module has an implemented owner. Enforce that here.
    for(const auto& source : snapshot.modules) {
        const auto found = std::find_if(
            result.modules.begin(),result.modules.end(),
            [&source](const ModuleRuntimeRosterRow& row){
                return row.canonical_name==source.canonical_name;
            });
        if(found==result.modules.end() ||
           !found->native_registered || found->source_reported)
            return std::nullopt;
        if(!module_control_request_supported(found->capability))
            return std::nullopt;
        auto& dst=*found;
        dst.source_reported = true;
        dst.requested_enabled = source.requested_enabled;
        dst.owner_reports_active = source.active_in_world;
        if(dst.capability==ModuleControlCapability::NativeOwnerProbe) {
            dst.authority=ModuleReportedAuthority::NativeOwnerProbe;
            ++result.native_owner_reports;
        } else {
            dst.authority=ModuleReportedAuthority::OfflinePolicy;
            ++result.offline_policy_reports;
        }
        ++result.source_reported;
    }
    return result;
}

inline std::optional<ModuleRuntimeRoster>
make_module_runtime_roster(const ModuleArrayListFeed& feed) {
    const auto source = feed.latest();
    return source ? make_module_runtime_roster(*source) : std::nullopt;
}

} // namespace slinkywinky
