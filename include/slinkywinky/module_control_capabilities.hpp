#pragma once
// R263: authoritative *availability* metadata for the candidate 67-name GUI.
// A name in the native 66-module registration roster is NOT proof that a
// gameplay module or an injectable native owner exists. Availability is a
// distinct, fail-closed admission boundary for reconstructed GUI requests.
#include "slinkywinky/menu.hpp"
#include "slinkywinky/module_arraylist.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace slinkywinky {

// No level in this enum means the original Minecraft gameplay is functional.
enum class ModuleControlCapability : std::uint8_t {
    Unknown = 0,          // not in the authenticated candidate GUI roster
    DescriptorOnly = 1,   // candidate descriptor but no native registration
    PreviewOnly = 2,      // registered, but no GUI owner request implementation
    OfflinePolicy = 3,    // R254, independent offline-only owner model
    NativeOwnerProbe = 4  // R211 three native owner/JNI decision probes
};

inline ModuleControlCapability module_control_capability(
        std::string_view canonical_name) noexcept {
    if(canonical_name == "Chest Stealer")
        return ModuleControlCapability::DescriptorOnly;
    // Critically distinguish native registration from implemented controls.
    // Unknown names, including invented aliases, must not become requests.
    if(!candidate_native_manager_registered(canonical_name))
        return ModuleControlCapability::Unknown;
    if(canonical_name == "No Jump Delay" ||
       canonical_name == "No Slow" ||
       canonical_name == "Sprint")
        return ModuleControlCapability::NativeOwnerProbe;
    if(canonical_name == "No Hit Delay" ||
       canonical_name == "Safe Walk" ||
       canonical_name == "Fast Place")
        return ModuleControlCapability::OfflinePolicy;
    return ModuleControlCapability::PreviewOnly;
}

inline bool module_control_request_supported(
        ModuleControlCapability capability) noexcept {
    return capability == ModuleControlCapability::NativeOwnerProbe ||
           capability == ModuleControlCapability::OfflinePolicy;
}

struct ModuleControlCatalogRow {
    std::string_view canonical_name;
    MenuTab tab = MenuTab::Combat;
    ModuleControlCapability capability = ModuleControlCapability::Unknown;
    bool native_registered = false;
};

// Source order and category come from the same 67-item native GUI catalog
// used by menu navigation. No owner state, requested bit or active flag is
// inferred from these rows. An actual runtime snapshot is separately needed.
inline std::array<ModuleControlCatalogRow,67>
module_control_catalog_capabilities() noexcept {
    std::array<ModuleControlCatalogRow,67> rows{};
    const auto& catalog = official_module_catalog();
    for(std::size_t i=0;i<rows.size();++i) {
        const std::string_view name = catalog[i].name;
        rows[i] = {name,catalog[i].tab,
                   module_control_capability(name),
                   candidate_native_manager_registered(name)};
    }
    return rows;
}

} // namespace slinkywinky
