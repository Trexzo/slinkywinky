// R265: registration metadata and authoritative reported state are separate.
// All tests run against copied data; no Java, Minecraft, or packet actions.
#include "slinkywinky/module_runtime_roster.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    std::string("R265 failed at ")+std::to_string(__LINE__)+": " #x);}while(false)

static const ModuleRuntimeRosterRow& row(
    const ModuleRuntimeRoster& roster,std::string_view name) {
    for(const auto& item:roster.modules)
        if(item.canonical_name==name)return item;
    throw std::runtime_error("R265 row absent");
}
static ModuleArrayListSnapshot original() {
    ModuleArrayListSnapshot source;
    source.source_sequence=100;
    source.world_generation=17;
    source.world_ready=true;
    source.modules={
        {"No Jump Delay",false,false,17},
        {"No Slow",true,false,17},
        {"Sprint",true,true,17}
    };
    return source;
}
int main() {
    auto source=original();
    const auto full=make_module_runtime_roster(source);
    CHECK(full && full->projection_version==2u);
    CHECK(full->native_registered==66u && full->source_reported==3u);
    CHECK(full->native_owner_reports==3u && full->offline_policy_reports==0u);
    CHECK(full->source_sequence==100u && full->world_generation==17u);
    CHECK(full->world_ready);
    const auto catalog=official_module_catalog();
    for(std::size_t i=0;i<catalog.size();++i) {
        CHECK(full->modules[i].canonical_name==catalog[i].name);
        CHECK(full->modules[i].tab==catalog[i].tab);
    }
    const auto& sprint=row(*full,"Sprint");
    CHECK(sprint.native_registered && sprint.source_reported);
    CHECK(sprint.authority==ModuleReportedAuthority::NativeOwnerProbe);
    CHECK(sprint.requested_enabled && sprint.owner_reports_active);
    const auto& slow=row(*full,"No Slow");
    CHECK(slow.source_reported && slow.requested_enabled && !slow.owner_reports_active);
    const auto& reach=row(*full,"Reach");
    CHECK(reach.native_registered && !reach.source_reported &&
          reach.authority==ModuleReportedAuthority::Unobserved &&
          !reach.requested_enabled && !reach.owner_reports_active);
    const auto& offline=row(*full,"Safe Walk");
    CHECK(offline.capability==ModuleControlCapability::OfflinePolicy &&
          offline.authority==ModuleReportedAuthority::Unobserved &&
          !offline.source_reported);
    const auto& ghost=row(*full,"Ghost Hand");
    CHECK(ghost.native_registered && ghost.tab==MenuTab::Block &&
          !ghost.source_reported);
    const auto& chest=row(*full,"Chest Stealer");
    CHECK(!chest.native_registered && !chest.source_reported &&
          chest.capability==ModuleControlCapability::DescriptorOnly &&
          chest.authority==ModuleReportedAuthority::DescriptorOnly);
    CHECK(std::count_if(full->modules.begin(),full->modules.end(),
          [](const ModuleRuntimeRosterRow& x){return x.source_reported;})==3);
    std::cout<<"R265_67_ROWS_66_REGISTERED_3_REPORTED_NO_FALSE_ACTIVATIONS_PASS\n";

    // A real source can report an offline controller, but its provenance
    // remains OfflinePolicy and cannot masquerade as an injected module.
    source.modules.push_back({"Safe Walk",true,true,17});
    const auto four=make_module_runtime_roster(source);
    CHECK(four && four->source_reported==4u &&
          four->offline_policy_reports==1u);
    CHECK(row(*four,"Safe Walk").authority==ModuleReportedAuthority::OfflinePolicy);
    CHECK(row(*four,"Safe Walk").owner_reports_active);
    // An unimplemented but registered module cannot be smuggled in as
    // authoritative even with both flags OFF.
    source=original();
    source.modules.push_back({"Ghost Hand",false,false,17});
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.modules.push_back({"Aim Assist",true,true,17});
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.modules.push_back({"Chest Stealer",false,false,17});
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.modules.push_back(source.modules.front()); // duplicates
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.contract_version=2;
    CHECK(!make_module_runtime_roster(source)); // v1 input, v2 projection
    source=original();
    source.source_sequence=0;
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.modules[0].world_generation=18;
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.modules[0].active_in_world=true; // enabled=false
    CHECK(!make_module_runtime_roster(source));
    source=original();
    source.world_ready=false;
    CHECK(!make_module_runtime_roster(source)); // active report cannot be out of world
    std::cout<<"R265_UNSUPPORTED_PREVIEW_AND_MALFORMED_PROVENANCE_REFUSED_PASS\n";

    source=original();
    source.source_sequence=102;
    source.world_generation=18;
    source.world_ready=false;
    for(auto& x:source.modules) {
        x.world_generation=18;
        x.active_in_world=false;
    }
    const auto detached=make_module_runtime_roster(source);
    CHECK(detached && !detached->world_ready);
    CHECK(row(*detached,"Sprint").requested_enabled &&
          !row(*detached,"Sprint").owner_reports_active);
    ModuleArrayListFeed feed;
    CHECK(!make_module_runtime_roster(feed));
    CHECK(feed.publish(original()));
    CHECK(make_module_runtime_roster(feed).has_value());
    CHECK(feed.publish(source));
    CHECK(make_module_runtime_roster(feed)->world_generation==18u);
    feed.retire();
    CHECK(!make_module_runtime_roster(feed));
    std::cout<<"R265_WORLD_LOSS_SELECTION_AND_FEED_RETIRE_PASS\n";
}
