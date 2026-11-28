// R266: actual PR2 owner states projected to all 67 native GUI descriptors
// through PR1's source-authenticated adapter. Offline, no Minecraft writes.
#include "slinkywinky/native_module_snapshot_adapter.hpp"
#include "slinkywinky/module_runtime_roster.hpp"
#include "slinkywinky/sprint_observation_preview.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    std::string("R266 failed at ")+std::to_string(__LINE__)+": " #x);}while(false)

static SprintObservedTick sample(std::uint64_t gen,std::uint64_t time,bool player) {
    SprintObservedTick t{};
    t.observation.generation=gen;
    t.observation.time_ms=time;
    t.observation.flags=player?3u:0u;
    t.update.player_present=player;
    t.movement.player_present=player;
    if(player) {
        t.update.player_sprinting=true;
        t.movement.forward=-0.2f;
        t.movement.strafe=0.1f;
        t.movement.speed=0.1f;
    }
    return t;
}
static const ModuleRuntimeRosterRow& item(
        const ModuleRuntimeRoster& roster,std::string_view name) {
    for(const auto& row:roster.modules)
        if(row.canonical_name==name)return row;
    throw std::runtime_error("R266: absent catalog row");
}
static void verify_unobserved(const ModuleRuntimeRoster& roster) {
    CHECK(roster.native_registered==66u && roster.source_reported==3u);
    CHECK(roster.native_owner_reports==3u && roster.offline_policy_reports==0u);
    CHECK(item(roster,"Chest Stealer").authority==
          ModuleReportedAuthority::DescriptorOnly);
    CHECK(!item(roster,"Chest Stealer").native_registered);
    CHECK(item(roster,"Ghost Hand").native_registered);
    CHECK(item(roster,"Ghost Hand").tab==MenuTab::Block);
    CHECK(item(roster,"Ghost Hand").authority==
          ModuleReportedAuthority::Unobserved);
    CHECK(!item(roster,"Backtrack").source_reported);
    CHECK(!item(roster,"Safe Walk").source_reported);
    CHECK(item(roster,"Safe Walk").capability==
          ModuleControlCapability::OfflinePolicy);
}
int main() {
    // PR2's actual immutable native owner is NOT a synthetic mock.
    SprintObservationPreview native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(adapter.publish_on_owner(native,feed,1u,false));
    auto roster=make_module_runtime_roster(feed);
    CHECK(roster && roster->projection_version==2u);
    CHECK(!roster->world_ready && roster->world_generation==0u);
    verify_unobserved(*roster);
    CHECK(item(*roster,"Sprint").source_reported);
    CHECK(item(*roster,"Sprint").requested_enabled);
    CHECK(!item(*roster,"Sprint").owner_reports_active);
    std::cout<<"R266_ACTUAL_PR2_OUT_OF_WORLD_67_ROSTER_PASS\n";

    CHECK(native.observe(sample(70u,1000u,true)).admitted);
    CHECK(adapter.publish_on_owner(native,feed,2u,true));
    roster=make_module_runtime_roster(feed);
    CHECK(roster && roster->world_ready && roster->world_generation==70u);
    verify_unobserved(*roster);
    CHECK(item(*roster,"Sprint").owner_reports_active);
    CHECK(!item(*roster,"No Slow").owner_reports_active);
    CHECK(!item(*roster,"No Jump Delay").owner_reports_active);
    CHECK(native.request_module({"No Slow",true}));
    CHECK(adapter.publish_on_owner(native,feed,3u,true));
    roster=make_module_runtime_roster(feed);
    CHECK(roster && item(*roster,"No Slow").source_reported);
    CHECK(item(*roster,"No Slow").owner_reports_active);
    CHECK(item(*roster,"Sprint").owner_reports_active);
    verify_unobserved(*roster);
    std::cout<<"R266_ACTUAL_PR2_THREE_NATIVE_OWNERS_NO_UNREPORTED_ACTIVATION_PASS\n";

    CHECK(native.observe(sample(71u,1001u,false)).admitted);
    CHECK(adapter.publish_on_owner(native,feed,4u,false));
    roster=make_module_runtime_roster(feed);
    CHECK(roster && !roster->world_ready && roster->world_generation==71u);
    CHECK(item(*roster,"Sprint").requested_enabled);
    CHECK(item(*roster,"No Slow").requested_enabled);
    CHECK(!item(*roster,"Sprint").owner_reports_active);
    CHECK(!item(*roster,"No Slow").owner_reports_active);
    verify_unobserved(*roster);
    CHECK(native.shutdown());
    CHECK(adapter.retire_owner(feed));
    CHECK(!make_module_runtime_roster(feed));
    std::cout<<"R266_ACTUAL_PR2_WORLD_LOSS_AND_NATIVE_OWNER_RETIRE_PASS\n";
}
