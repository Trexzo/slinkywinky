// R263: native source registration is not equivalent to implemented control.
#include "slinkywinky/module_control_capabilities.hpp"
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x)) throw std::runtime_error("R263: " #x);} while(false)

int main() {
    constexpr auto unknown=ModuleControlCapability::Unknown;
    constexpr auto descriptor=ModuleControlCapability::DescriptorOnly;
    constexpr auto preview=ModuleControlCapability::PreviewOnly;
    constexpr auto offline=ModuleControlCapability::OfflinePolicy;
    constexpr auto native=ModuleControlCapability::NativeOwnerProbe;
    CHECK(!module_control_request_supported(unknown));
    CHECK(!module_control_request_supported(descriptor));
    CHECK(!module_control_request_supported(preview));
    CHECK(module_control_request_supported(offline));
    CHECK(module_control_request_supported(native));
    CHECK(module_control_capability("")==unknown);
    CHECK(module_control_capability("Unknown Slinky")==unknown);
    CHECK(module_control_capability("sprint")==unknown); // canonical casing
    CHECK(module_control_capability("Chest Stealer")==descriptor);
    CHECK(module_control_capability("Ghost Hand")==preview);
    CHECK(module_control_capability("Reach")==preview);
    CHECK(module_control_capability("Strafe")==preview);
    const auto rows=module_control_catalog_capabilities();
    CHECK(rows.size()==67u);
    std::array<std::size_t,5> caps{};
    std::size_t registered=0, supported=0;
    for(std::size_t i=0;i<rows.size();++i) {
        const auto& row=rows[i];
        const auto& source=official_module_catalog()[i];
        CHECK(row.canonical_name==source.name);
        CHECK(row.tab==source.tab);
        CHECK(row.capability==module_control_capability(row.canonical_name));
        CHECK(row.capability!=unknown);
        CHECK(row.native_registered==
              candidate_native_manager_registered(row.canonical_name));
        if(row.native_registered)++registered;
        if(module_control_request_supported(row.capability)) {
            ++supported;
            CHECK(row.native_registered);
        }
        const auto index=static_cast<std::size_t>(row.capability);
        CHECK(index<caps.size());
        ++caps[index];
    }
    CHECK(registered==66u && supported==6u);
    CHECK(caps[static_cast<std::size_t>(unknown)]==0u);
    CHECK(caps[static_cast<std::size_t>(descriptor)]==1u);
    CHECK(caps[static_cast<std::size_t>(preview)]==60u);
    CHECK(caps[static_cast<std::size_t>(offline)]==3u);
    CHECK(caps[static_cast<std::size_t>(native)]==3u);
    // Original R211 JNI/native owner probes are not automatically usable
    // in the standalone production client; only source-authorized requests.
    for(const std::string_view name: {"No Jump Delay","No Slow","Sprint"})
        CHECK(module_control_capability(name)==native);
    for(const std::string_view name: {"No Hit Delay","Safe Walk","Fast Place"})
        CHECK(module_control_capability(name)==offline);
    for(const auto& row:rows)
        if(row.canonical_name=="Chest Stealer")
            CHECK(!row.native_registered &&
                  !module_control_request_supported(row.capability));
    std::cout<<"R263_GUI_67_DESCRIPTORS_66_REGISTRATIONS_CAPABILITY_PASS\\n";
    std::cout<<"R263_3_NATIVE_PROBES_3_OFFLINE_POLICIES_60_PREVIEW_ONLY_PASS\\n";
}
