#include "slinkywinky/gui_module_control_handoff.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error("R210 " #x);}while(false)

static bool admit_on_worker(GuiModuleControlHandoff& handoff,
    ModuleArrayListFeed& feed,const ModuleArrayListSnapshot& ack,
    bool should_accept) {
    bool result=false;
    std::thread module([&] {
        const auto request=handoff.take_on_module_owner();
        if(!request)return;
        result=should_accept ?
            handoff.acknowledge_on_module_owner(request->id,ack,feed) :
            handoff.deny_on_module_owner(request->id);
    });
    module.join();
    return result;
}

static void r245_three_module_request_fixture(const char* name,
        std::size_t row,std::size_t catalog_index,bool world_ready) {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    CHECK(nav.select_tab(MenuTab::Move));
    for(std::size_t i=0;i<row;i++)CHECK(nav.apply(MenuNavAction::NextRow));
    CHECK(nav.visible_focus() && *nav.visible_focus()==catalog_index);
    ModuleArrayListSnapshot source;
    source.source_sequence=31u;
    source.world_generation=4u;
    source.world_ready=world_ready;
    source.modules={{name,false,false,4u}};
    ModuleArrayListFeed feed;
    CHECK(feed.publish(source));
    GuiModuleControlHandoff enable;
    CHECK(enable.request_focused_toggle(menu,nav,feed,31u));
    CHECK(enable.view().request &&
          enable.view().request->canonical_name==name &&
          enable.view().request->enable);
    CHECK(menu.toggle_preview(catalog_index));
    CHECK(menu.preview_enabled(catalog_index));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    source.source_sequence=32u;
    source.modules[0].requested_enabled=true;
    source.modules[0].active_in_world=world_ready;
    CHECK(admit_on_worker(enable,feed,source,true));
    CHECK(enable.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(enable.view().last_confirmed_active==world_ready);
    const auto display=make_module_arraylist_display(*feed.latest());
    CHECK(display && display->active_names.size()==(world_ready?1u:0u));
    if(world_ready)CHECK(display->active_names[0]==name);
    CHECK(menu.preview_enabled(catalog_index));
    GuiModuleControlHandoff disable;
    CHECK(disable.request_focused_toggle(menu,nav,feed,32u));
    CHECK(disable.view().request &&
          disable.view().request->canonical_name==name &&
          !disable.view().request->enable);
    source.source_sequence=33u;
    source.modules[0].requested_enabled=false;
    source.modules[0].active_in_world=false;
    CHECK(admit_on_worker(disable,feed,source,true));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    CHECK(menu.preview_enabled(catalog_index)); // preview stays inert
}

int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation navigation(menu,profile);
    ModuleArrayListFeed feed;
    ModuleArrayListSnapshot source;
    source.source_sequence=10u;
    source.world_generation=9u;
    source.world_ready=true;
    source.modules={{"Sprint",false,false,9u}};
    CHECK(feed.publish(source));

    GuiModuleControlHandoff first;
    CHECK(!first.request_focused_toggle(menu,navigation,feed));
    CHECK(navigation.select_tab(MenuTab::Move));
    // Source catalog: Sprint is the ninth Move row (catalog index 25).
    for(int i=0;i<8;i++)CHECK(navigation.apply(MenuNavAction::NextRow));
    CHECK(navigation.visible_focus() && *navigation.visible_focus()==25u);
    MenuState unrelated;
    CHECK(unrelated.select(MenuTab::Move));
    CHECK(!first.request_focused_toggle(unrelated,navigation,feed));
    CHECK(menu.toggle_preview(25u)); // inert GUI preview, not an ack
    CHECK(menu.preview_enabled(25u));
    CHECK(!first.request_focused_toggle(menu,navigation,feed,999u));
    // R264: a real source keybind capture owns both keyboard and
    // background menu; no hidden module request may bypass it.
    CHECK(menu.begin_bind_capture(25u));
    CHECK(menu.modal_input_owned());
    CHECK(!first.request_focused_toggle(menu,navigation,feed,10u));
    CHECK(first.view().phase==GuiModuleControlPhase::Idle);
    CHECK(menu.cancel_bind_capture());
    CHECK(!menu.modal_input_owned());
    CHECK(first.request_focused_toggle(menu,navigation,feed,10u));
    CHECK(!first.request_focused_toggle(menu,navigation,feed));
    const auto pending=first.view();
    CHECK(pending.phase==GuiModuleControlPhase::Pending &&
        pending.request && pending.request->canonical_name=="Sprint" &&
        pending.request->enable && pending.request->observed_source_sequence==10u);
    CHECK(!make_module_arraylist_display(*feed.latest())->active_names.size());
    source.source_sequence=11u;
    source.modules[0].requested_enabled=true;
    source.modules[0].active_in_world=true;
    CHECK(admit_on_worker(first,feed,source,true));
    CHECK(first.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(first.view().last_confirmed_active);
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.size()==1u);
    CHECK(menu.preview_enabled(25u)); // still only UI preview
    CHECK(first.request_focused_toggle(menu,navigation,feed));
    CHECK(first.view().request && !first.view().request->enable);
    // The former module worker owns this channel. GUI thread cannot
    // take/ack requests itself even with the right request id.
    CHECK(!first.take_on_module_owner());
    CHECK(!first.acknowledge_on_module_owner(first.view().request->id,source,feed));
    first.retire();
    CHECK(first.view().phase==GuiModuleControlPhase::Retired);
    CHECK(!first.request_focused_toggle(menu,navigation,feed));
    std::cout<<"R210_GUI_SOURCE_FOCUS_REQUEST_ONLY_NO_PREVIEW_ACK_PASS\n";

    GuiModuleControlHandoff disable;
    CHECK(disable.request_focused_toggle(menu,navigation,feed));
    CHECK(disable.view().request && !disable.view().request->enable);
    source.source_sequence=12u;
    source.modules[0].requested_enabled=false;
    source.modules[0].active_in_world=false;
    CHECK(admit_on_worker(disable,feed,source,true));
    CHECK(!disable.view().last_confirmed_active);
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());

    source.source_sequence=13u;
    source.world_ready=false;
    CHECK(feed.publish(source));
    GuiModuleControlHandoff worldless;
    CHECK(worldless.request_focused_toggle(menu,navigation,feed));
    source.source_sequence=14u;
    source.modules[0].requested_enabled=true;
    CHECK(admit_on_worker(worldless,feed,source,true));
    CHECK(worldless.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(!worldless.view().last_confirmed_active);
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    std::cout<<"R210_REQUESTED_VS_ACTIVE_WORLDLESS_ACK_PASS\n";

    GuiModuleControlHandoff replay;
    CHECK(replay.request_focused_toggle(menu,navigation,feed));
    CHECK(!admit_on_worker(replay,feed,source,true)); // stale ack sequence
    CHECK(replay.view().phase==GuiModuleControlPhase::Retired);
    CHECK(feed.retired() && !feed.latest());
    std::cout<<"R210_REPLAY_QUARANTINES_RUNTIME_FEED_PASS\n";

    ModuleArrayListFeed deniedFeed;
    ModuleArrayListSnapshot ready;
    ready.source_sequence=21u;ready.world_generation=11u;
    ready.world_ready=true;
    ready.modules={{"Sprint",false,false,11u}};
    CHECK(deniedFeed.publish(ready));
    GuiModuleControlHandoff denied;
    CHECK(denied.request_focused_toggle(menu,navigation,deniedFeed));
    CHECK(admit_on_worker(denied,deniedFeed,ready,false));
    CHECK(denied.view().phase==GuiModuleControlPhase::Denied);
    CHECK(deniedFeed.latest()->source_sequence==21u);
    // The next GUI request may be retried with real unchanged state.
    CHECK(denied.request_focused_toggle(menu,navigation,deniedFeed));
    denied.retire();
    deniedFeed.retire();

    ModuleArrayListFeed foreignFeed;
    CHECK(foreignFeed.publish(ready));
    GuiModuleControlHandoff foreign;
    bool acceptedElsewhere=true;
    std::thread other([&]{
        acceptedElsewhere=foreign.request_focused_toggle(
            menu,navigation,foreignFeed);
    });
    other.join();
    CHECK(!acceptedElsewhere);
    CHECK(foreign.view().phase==GuiModuleControlPhase::Idle);
    CHECK(navigation.select_tab(MenuTab::Combat));
    CHECK(!foreign.request_focused_toggle(menu,navigation,foreignFeed));
    foreign.retire();
    foreignFeed.retire();
    std::cout<<"R210_DENIAL_OWNER_THREAD_AND_UNSUPPORTED_FOCUS_PASS\n";

    // R245 original Move source order: No Jump Delay 21, No Slow 22,
    // Sprint 25. PR #2 R211 admits those three native controls only.
    r245_three_module_request_fixture("No Jump Delay",4u,21u,true);
    r245_three_module_request_fixture("No Slow",5u,22u,false);

    MenuState unsupported;
    MenuProfileBridge unsupportedProfile(unsupported);
    MenuNavigation unsupportedNav(unsupported,unsupportedProfile);
    CHECK(unsupportedNav.select_tab(MenuTab::Move));
    for(int i=0;i<9;i++)CHECK(unsupportedNav.apply(MenuNavAction::NextRow));
    CHECK(unsupportedNav.visible_focus() &&
          *unsupportedNav.visible_focus()==26u); // Strafe is not R211
    ModuleArrayListSnapshot unsupportedSource;
    unsupportedSource.source_sequence=41u;
    unsupportedSource.world_generation=5u;
    unsupportedSource.world_ready=true;
    unsupportedSource.modules={{"Strafe",false,false,5u}};
    ModuleArrayListFeed unsupportedFeed;
    CHECK(unsupportedFeed.publish(unsupportedSource));
    GuiModuleControlHandoff unsupportedControl;
    CHECK(!unsupportedControl.request_focused_toggle(
        unsupported,unsupportedNav,unsupportedFeed));
    CHECK(unsupportedControl.view().phase==GuiModuleControlPhase::Idle);
    std::cout<<"R245_THREE_MODULE_SOURCE_GUI_REQUEST_AND_ACK_PASS\n";
    std::cout<<"R245_OTHER_MODULES_REMAIN_PREVIEW_ONLY_PASS\n";
}
