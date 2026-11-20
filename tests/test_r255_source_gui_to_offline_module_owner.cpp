// R255: source-focused GUI -> three real PR1 R254 owner-gated OFFLINE
// decisions -> authoritative ACK and ArrayList rows. No game/JNI hooks.
#include "slinkywinky/recovered_source_policy_gui_adapter.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
#define CHECK(expr) do {if(!(expr))throw std::runtime_error( \
    "R255 line "+std::to_string(__LINE__)+": " #expr);}while(false)

struct Case{
    MenuTab tab;
    std::size_t row;
    std::size_t index;
    const char* name;
    RecoveredSourceModule kind;
};
static void test_control(Case c){
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    CHECK(nav.select_tab(c.tab));
    for(std::size_t i=0;i<c.row;++i)
        CHECK(nav.apply(MenuNavAction::NextRow));
    CHECK(nav.visible_focus() && *nav.visible_focus()==c.index);
    CHECK(std::string(official_module_catalog()[c.index].name)==c.name);

    GuiModuleControlHandoff gui;
    ModuleArrayListFeed feed;
    std::mutex mutex;
    std::condition_variable cv;
    int state=0;
    bool enabled=false,worldless=false,disabled=false;
    std::exception_ptr owner_exception;
    std::thread native([&]{
        try{
            RecoveredSourcePolicyOwner source;
            RecoveredSourcePolicyGuiAdapter adapter;
            CHECK(source.transition(10u,true));
            CHECK(adapter.publish(source,feed,20u));
            {
                std::lock_guard<std::mutex> guard(mutex);state=1;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> guard(mutex);
                cv.wait(guard,[&]{return state>=2;});
            }
            enabled=adapter.apply(source,gui,feed,21u);
            CHECK(enabled && source.selected(c.kind));
            {
                std::lock_guard<std::mutex> guard(mutex);state=3;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> guard(mutex);
                cv.wait(guard,[&]{return state>=4;});
            }
            CHECK(source.transition(11u,false));
            worldless=adapter.publish(source,feed,22u);
            CHECK(worldless);
            {
                std::lock_guard<std::mutex> guard(mutex);state=5;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> guard(mutex);
                cv.wait(guard,[&]{return state>=6;});
            }
            disabled=adapter.apply(source,gui,feed,23u);
            CHECK(disabled && !source.selected(c.kind));
            CHECK(source.retire() && adapter.retire(feed));
            {
                std::lock_guard<std::mutex> guard(mutex);state=7;
            }
            cv.notify_all();
        }catch(...){
            owner_exception=std::current_exception();
            {
                std::lock_guard<std::mutex> guard(mutex);state=99;
            }
            cv.notify_all();
        }
    });
    auto wait=[&](int desired){
        std::unique_lock<std::mutex> guard(mutex);
        cv.wait(guard,[&]{return state>=desired;});
        CHECK(state!=99);
    };
    auto advance=[&](int next){
        {
            std::lock_guard<std::mutex> guard(mutex);
            state=next;
        }
        cv.notify_all();
    };
    wait(1);
    CHECK(feed.latest() && feed.latest()->source_sequence==20u);
    CHECK(gui.request_focused_toggle(menu,nav,feed,20u));
    CHECK(gui.view().request && gui.view().request->canonical_name==c.name &&
          gui.view().request->enable);
    CHECK(menu.toggle_preview(c.index));
    CHECK(menu.preview_enabled(c.index));
    CHECK(make_module_arraylist_display(*feed.latest())->active_names.empty());
    advance(2);
    wait(3);
    CHECK(gui.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(gui.view().last_confirmed_active);
    const auto active=feed.latest();
    CHECK(active && active->source_sequence==21u);
    const auto display=make_module_arraylist_display(*active);
    CHECK(display && display->active_names.size()==1u &&
          display->active_names[0]==c.name);
    CHECK(menu.preview_enabled(c.index));
    advance(4);
    wait(5);
    const auto lost=feed.latest();
    CHECK(lost && !lost->world_ready && lost->world_generation==11u);
    CHECK(lost->modules[static_cast<std::size_t>(c.kind)].requested_enabled);
    CHECK(make_module_arraylist_display(*lost)->active_names.empty());
    CHECK(gui.request_focused_toggle(menu,nav,feed,22u));
    CHECK(gui.view().request && !gui.view().request->enable);
    advance(6);
    wait(7);
    CHECK(enabled && worldless && disabled);
    CHECK(gui.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(!gui.view().last_confirmed_active);
    CHECK(feed.retired() && !feed.latest());
    CHECK(menu.preview_enabled(c.index));
    native.join();
    if(owner_exception)std::rethrow_exception(owner_exception);
    gui.retire();
    std::cout<<"R255_GUI_OFFLINE_SOURCE_ACK "<<c.name<<" PASS\n";
}
int main(){
    // R115 exact source catalog indexing, not hardcoded preview state.
    test_control({MenuTab::Utility,11u,60u,"No Hit Delay",
                  RecoveredSourceModule::NoHitDelay});
    test_control({MenuTab::Move,7u,24u,"Safe Walk",
                  RecoveredSourceModule::SafeWalk});
    test_control({MenuTab::Block,5u,47u,"Fast Place",
                  RecoveredSourceModule::FastPlace});
    std::cout<<"R255_THREE_SOURCE_FOCUSED_GUI_CONTROLS_OWNER_ACK_PASS\n";
    std::cout<<"R255_NO_JVM_NATIVE_INPUT_OR_ORIGINAL_CLIENT_GAME_WRITES\n";
}
