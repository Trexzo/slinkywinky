#include "slinkywinky/windows_gui_module_click_fixture.hpp"
#ifdef _WIN32
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu.hpp"
#include <algorithm>
#include <string_view>

namespace slinkywinky {
GuiModuleClickTransaction::GuiModuleClickTransaction() noexcept
    :owner_(std::this_thread::get_id()) {}
bool GuiModuleClickTransaction::owner() const noexcept {
    return owner_==std::this_thread::get_id();
}
bool GuiModuleClickTransaction::pending() const noexcept {
    return owner() && pending_;
}
void GuiModuleClickTransaction::revoke() noexcept {
    if(!owner())return;
    gesture_.cancel();
    pending_=false;
    epoch_=painted_frame_=source_sequence_=source_generation_=down_ms_=0;
    context_=nullptr;
    dc_=nullptr;
}
bool GuiModuleClickTransaction::fixture_event(
        RetainedPointerPhase phase,WglRetainedMenuPreview& menu,
        MenuInputHandoffLedger& ledger,MenuKeyboardRouter& router,
        GuiModuleControlHandoff& handoff,const ModuleArrayListFeed& source,
        HGLRC context,HDC dc,unsigned width,unsigned height,
        std::uint64_t epoch,int x,int y,std::uint64_t ms,
        bool foreground_verified) {
    // This model handles only separately admitted SYNTHETIC fixture edges;
    // never call it from real Minecraft event dispatch.
    if(!owner())return false;
    const auto fail=[&]() noexcept {
        if(pending_)ledger.abort();
        revoke();
        return false;
    };
    if(phase==RetainedPointerPhase::Cancel)return fail();
    if(!menu.owner_thread() || !menu.active() ||
       !router.fixture_routes(menu.fixture_navigation()) ||
       !router.active() ||
       ledger.phase()!=InputHandoffPhase::MenuFixture ||
       ledger.epoch()!=epoch || router.epoch()!=epoch ||
       !epoch || !ms || !foreground_verified ||
       !context || !dc)return fail();
    const auto* frame=menu.fixture_painted_frame(
        context,dc,width,height);
    if(!frame || frame->appearance.wide)return fail();
    const auto latest=source.latest();
    if(!latest || !make_module_arraylist_display(*latest))return fail();
    const auto row=std::find_if(latest->modules.begin(),
        latest->modules.end(),[](const ModuleArrayListState& entry){
            return entry.canonical_name=="Sprint";
        });
    if(row==latest->modules.end())return fail();
    const auto hit=hit_test_menu(
        *frame,MenuPanelMode::CompactOverlay,x,y);
    const auto focus=menu.focused_row();
    // A click on the toggle itself must belong to the focused, currently
    // painted exact source row, not merely another on-screen control.
    const bool target=hit && hit->kind==MenuHitKind::Toggle &&
        hit->catalog_index<official_module_catalog().size() &&
        std::string_view(official_module_catalog()[hit->catalog_index].name)=="Sprint" &&
        focus && *focus==hit->catalog_index;
    if(phase==RetainedPointerPhase::Down) {
        if(pending_ || !ledger.tracked_released() || !target)return fail();
        if(ledger.simulate(
             {InputHandoffDevice::Mouse,0,true},true,true,true,true)
           !=InputHandoffRoute::MenuFixtureOnly)return fail();
        if(!gesture_.press(*frame,MenuPanelMode::CompactOverlay,x,y,ms,true)){
            ledger.abort();
            return fail();
        }
        pending_=true;
        epoch_=epoch;
        painted_frame_=menu.drawn_frames();
        source_sequence_=latest->source_sequence;
        source_generation_=latest->world_generation;
        down_ms_=ms;
        context_=context;dc_=dc;
        return true;
    }
    if(!pending_ || epoch_!=epoch || context_!=context ||
       dc_!=dc || menu.drawn_frames()!=painted_frame_ ||
       latest->source_sequence!=source_sequence_ ||
       latest->world_generation!=source_generation_ ||
       !ledger.tracked_down(InputHandoffDevice::Mouse,0) ||
       ms<=down_ms_)return fail();
    if(phase==RetainedPointerPhase::Move)
        return gesture_.move(*frame,MenuPanelMode::CompactOverlay,x,y,true)
            ?true:fail();
    if(phase!=RetainedPointerPhase::Up || !target)return fail();
    if(ledger.simulate(
        {InputHandoffDevice::Mouse,0,false},true,true,true,true)
        !=InputHandoffRoute::MenuFixtureOnly)return fail();
    const auto release=gesture_.release(
        *frame,MenuPanelMode::CompactOverlay,x,y,ms,true);
    if(!release || release->kind!=MenuHitKind::Toggle ||
       release->catalog_index!=*focus)return fail();
    const auto exact_source_sequence=source_sequence_;
    revoke();
    // This is a REQUEST ONLY. Re-check the source sequence inside the
    // handoff lock, so concurrent publication cannot retarget an old click.
    return menu.queue_focused_runtime_toggle(
        handoff,source,context,dc,width,height,exact_source_sequence);
}
} // namespace slinkywinky
#endif
