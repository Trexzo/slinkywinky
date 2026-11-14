#include "slinkywinky/module_arraylist.hpp"
#include "slinkywinky/menu.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace slinkywinky;
#define CHECK(x) do{ if(!(x))throw std::runtime_error("R207 " #x); }while(false)

int main() {
    ModuleArrayListSnapshot fixture;
    fixture.source_sequence=10u;
    fixture.world_generation=7u;
    fixture.world_ready=true;
    fixture.modules={
        {"Sprint",true,true,7u},
        {"Timer",true,true,7u},
        {"Keep Sprint",true,false,7u}, // requested, not ACTIVE
        {"Aim Assist",false,false,7u}
    };
    const auto shown=make_module_arraylist_display(fixture);
    CHECK(shown && shown->source_sequence==10u &&
          shown->world_generation==7u);
    CHECK(shown->active_names.size()==2u);
    CHECK(shown->active_names[0]=="Sprint" &&
          shown->active_names[1]=="Timer");
    const auto original=fixture;
    fixture.modules[0].active_in_world=false;
    fixture.modules[1].active_in_world=false;
    CHECK(make_module_arraylist_display(fixture)->active_names.empty());
    fixture.world_ready=false;
    CHECK(make_module_arraylist_display(fixture)->active_names.empty());
    fixture.world_generation=0u;
    for(auto& row:fixture.modules)row.world_generation=0u;
    CHECK(make_module_arraylist_display(fixture)->active_names.empty());
    fixture=original;
    const auto rejects=[](const ModuleArrayListSnapshot& value) {
        CHECK(!make_module_arraylist_display(value));
    };
    fixture.contract_version=2u;rejects(fixture);
    fixture=original;fixture.source_sequence=0u;rejects(fixture);
    fixture=original;fixture.world_generation=0u;rejects(fixture);
    fixture=original;fixture.modules[0].world_generation=6u;rejects(fixture);
    fixture=original;fixture.modules[0].requested_enabled=false;rejects(fixture);
    fixture=original;fixture.world_ready=false;rejects(fixture);
    fixture=original;fixture.modules.push_back(fixture.modules[0]);rejects(fixture);
    fixture=original;fixture.modules[0].canonical_name="Not Slinky";rejects(fixture);
    fixture=original;fixture.modules[0].canonical_name="";rejects(fixture);
    fixture=original;fixture.modules[0].canonical_name=std::string(49,'a');rejects(fixture);
    fixture=original;fixture.modules.clear();
    for(const auto& item:official_module_catalog())
        fixture.modules.push_back({item.name,false,false,7u});
    CHECK(fixture.modules.size()==67u);
    rejects(fixture);
    fixture=original;fixture.modules.clear();
    CHECK(make_module_arraylist_display(fixture)->active_names.empty());


    // R208 owner-published feed: copies, monotonic sequence and world loss.
    ModuleArrayListFeed feed;
    CHECK(!feed.latest() && !feed.retired());
    CHECK(feed.publish(original));
    CHECK(feed.latest() && feed.latest()->modules.size()==4u);
    auto detached=feed.latest();
    detached->modules[0].canonical_name="Tampered";
    CHECK(feed.latest()->modules[0].canonical_name=="Sprint");
    auto next=original;
    next.source_sequence=11u;
    next.modules[0].active_in_world=false;
    next.modules[1].active_in_world=false;
    CHECK(feed.publish(next));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    next.source_sequence=12u;
    next.world_ready=false; // world retired; requested bits remain only requests
    CHECK(feed.publish(next));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    next.source_sequence=13u;
    next.world_generation=8u;
    next.world_ready=true;
    for(auto& row:next.modules)row.world_generation=8u;
    next.modules[0].active_in_world=true;
    CHECK(feed.publish(next));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.size()==1u);
    feed.retire();
    CHECK(feed.retired() && !feed.latest());
    next.source_sequence=14u;
    CHECK(!feed.publish(next) && !feed.latest());
    ModuleArrayListFeed replay;
    CHECK(replay.publish(original));
    CHECK(!replay.publish(original)); // duplicate sequence quarantines
    CHECK(replay.retired() && !replay.latest());
    ModuleArrayListFeed oldWorld;
    CHECK(oldWorld.publish(next));
    next.source_sequence=15u;
    next.world_generation=7u;
    for(auto& row:next.modules)row.world_generation=7u;
    CHECK(!oldWorld.publish(next)); // world-generation regression
    CHECK(oldWorld.retired() && !oldWorld.latest());
    ModuleArrayListFeed malformed;
    auto bad=original;
    bad.contract_version=2u;
    CHECK(!malformed.publish(bad));
    CHECK(malformed.retired() && !malformed.latest());
    std::cout<<"R208_COPY_ONLY_MONOTONIC_WORLD_RETIRE_PASS\n";
    std::cout<<"R208_STALE_REPLAY_QUARANTINE_PASS\n";

    // Source-faithful GUI preview toggles are NOT runtime active status.
    // This model exposes no MenuState pointer or preview_enabled field.
    MenuState previews;
    CHECK(previews.toggle_preview(0u));
    CHECK(make_module_arraylist_display(original)->active_names.size()==2u);
    std::cout<<"R207_MODULE_ARRAYLIST_SOURCE_ONLY_ACTIVE_STATE_PASS\n";
    std::cout<<"R207_PREVIEW_TOGGLE_CANNOT_CREATE_ACTIVE_ROW_PASS\n";
    std::cout<<"R207_WORLD_GENERATION_AND_DUPLICATES_FAIL_CLOSED_PASS\n";
}
