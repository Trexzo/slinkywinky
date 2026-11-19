#include "slinkywinky/native_module_snapshot_adapter.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <array>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error("R246 " #x);} while(false)

// Only a test ABI shape, deliberately NOT a copy of PR #2's implementation.
// Matches PR #2 R211 ModuleControlRequest/State exposed field signatures.
struct R246NativeRequest {std::string_view canonical_name;bool enable;};
struct R246NativeState {
    std::string canonical_name;
    bool requested_enabled=false;
    bool active_in_world=false;
    bool settings_override=false;
    std::uint64_t world_generation=0;
};
class R246NativeFixture {
public:
    explicit R246NativeFixture(bool ready=true)
        : owner_(std::this_thread::get_id()), ready_(ready) {}
    std::uint64_t generation() const noexcept {
        return std::this_thread::get_id()==owner_ ? generation_:0u;
    }
    std::optional<std::vector<R246NativeState>> control_states() const {
        if(std::this_thread::get_id()!=owner_ || unavailable_)return std::nullopt;
        std::vector<R246NativeState> out;
        const char* names[]={"No Jump Delay","No Slow","Sprint"};
        for(std::size_t i=0;i<3;++i) {
            out.push_back({names[(i+shift_)%3],selected_[i],
                           selected_[i]&&ready_,false,generation_});
        }
        if(duplicate_)out[1].canonical_name=out[0].canonical_name;
        if(wrongGeneration_)out[1].world_generation=100u;
        if(activeWithoutRequest_) {
            out[1].requested_enabled=false;
            out[1].active_in_world=true;
        }
        return out;
    }
    bool request_module(R246NativeRequest request) {
        if(std::this_thread::get_id()!=owner_)return false;
        if(reject_)return false;
        const char* names[]={"No Jump Delay","No Slow","Sprint"};
        for(std::size_t i=0;i<3;++i) {
            if(request.canonical_name==names[i]){
                if(!ignore_)selected_[i]=request.enable;
                return true;
            }
        }
        return false;
    }
    void set_ready(bool ready){CHECK(std::this_thread::get_id()==owner_);ready_=ready;}
    void set_generation(std::uint64_t gen){CHECK(std::this_thread::get_id()==owner_);generation_=gen;}
    void shift_names(){shift_=1;}
    void duplicate_names(){duplicate_=true;}
    void invalid_generation(){wrongGeneration_=true;}
    void invalid_activation(){activeWithoutRequest_=true;}
    void unavailable(){unavailable_=true;}
    void deny(){reject_=true;}
    void ignore(){ignore_=true;}
private:
    const std::thread::id owner_;
    bool ready_;
    std::uint64_t generation_=7u;
    std::array<bool,3> selected_{{false,false,false}};
    std::size_t shift_=0;
    bool duplicate_=false,wrongGeneration_=false,activeWithoutRequest_=false;
    bool unavailable_=false,reject_=false,ignore_=false;
};

static void test_snapshot_negatives() {
    for(int variation=0;variation<5;++variation) {
        R246NativeFixture native;
        NativeModuleSnapshotAdapter adapter;
        ModuleArrayListFeed feed;
        CHECK(adapter.publish_on_owner(native,feed,10u,true));
        switch(variation) {
          case 0:native.shift_names();break;
          case 1:native.duplicate_names();break;
          case 2:native.invalid_generation();break;
          case 3:native.invalid_activation();break;
          case 4:native.unavailable();break;
        }
        CHECK(!adapter.publish_on_owner(native,feed,11u,true));
        CHECK(adapter.retired() && feed.retired() && !feed.latest());
    }
    R246NativeFixture native;
    NativeModuleSnapshotAdapter adapter;
    ModuleArrayListFeed feed;
    CHECK(adapter.publish_on_owner(native,feed,1u,true));
    CHECK(!adapter.publish_on_owner(native,feed,1u,true));
    CHECK(adapter.retired() && feed.retired());
    R246NativeFixture detached;
    NativeModuleSnapshotAdapter foreignAdapter;
    ModuleArrayListFeed foreignFeed;
    bool accepted=true;
    std::thread outsider([&]{
        accepted=foreignAdapter.publish_on_owner(detached,foreignFeed,5u,true);
    });
    outsider.join();
    CHECK(!accepted && !foreignAdapter.retired() && !foreignFeed.retired());
    std::cout<<"R246_NATIVE_OWNER_SNAPSHOT_SHAPE_AND_THREAD_NEGATIVES_PASS\n";
}

static void test_request_roundtrip() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation navigation(menu,profile);
    CHECK(navigation.select_tab(MenuTab::Move));
    for(int i=0;i<5;++i)CHECK(navigation.apply(MenuNavAction::NextRow));
    CHECK(navigation.visible_focus() && *navigation.visible_focus()==22u);
    ModuleArrayListFeed feed;
    GuiModuleControlHandoff handoff;
    std::mutex mu;
    std::condition_variable cv;
    int stage=0;
    bool published=false,activated=false,worldRetired=false;

    std::thread owner([&]{
        R246NativeFixture native;
        NativeModuleSnapshotAdapter adapter;
        published=adapter.publish_on_owner(native,feed,10u,true);
        {
            std::lock_guard<std::mutex> lock(mu);
            stage=1;
        }
        cv.notify_all();
        {
            std::unique_lock<std::mutex> lock(mu);
            cv.wait(lock,[&]{return stage>=2;});
        }
        activated=adapter.apply_request_on_owner(native,handoff,feed,11u,true);
        // Release the active row on world exit while retaining selection.
        native.set_ready(false);
        native.set_generation(8u);
        worldRetired=adapter.publish_on_owner(native,feed,12u,false);
        {
            std::lock_guard<std::mutex> lock(mu);
            stage=3;
        }
        cv.notify_all();
    });
    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock,[&]{return stage>=1;});
    }
    CHECK(published && feed.latest() && feed.latest()->modules.size()==3u);
    CHECK(handoff.request_focused_toggle(menu,navigation,feed,10u));
    CHECK(handoff.view().request &&
          handoff.view().request->canonical_name=="No Slow" &&
          handoff.view().request->enable);
    CHECK(menu.toggle_preview(22u)); // inert UI preview
    CHECK(menu.preview_enabled(22u));
    {
        std::lock_guard<std::mutex> lock(mu);
        stage=2;
    }
    cv.notify_all();
    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock,[&]{return stage>=3;});
    }
    owner.join();
    CHECK(activated && worldRetired);
    CHECK(handoff.view().phase==GuiModuleControlPhase::Acknowledged);
    CHECK(handoff.view().last_confirmed_active);
    const auto latest=feed.latest();
    CHECK(latest && latest->source_sequence==12u &&
          latest->world_generation==8u && !latest->world_ready);
    CHECK(latest->modules[1].requested_enabled &&
          !latest->modules[1].active_in_world);
    CHECK(make_module_arraylist_display(*latest)->active_names.empty());
    CHECK(menu.preview_enabled(22u));
    std::cout<<"R246_REAL_CONTRACT_SHAPE_NO_SLOW_REQUEST_ACK_WORLD_LOSS_PASS\n";
}

static void test_denial_and_mismatch() {
    // A manager denial leaves the original feed stable, unlike a
    // malformed/stale source that must quarantine the full display.
    for(int variation=0;variation<3;++variation){
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Move));
        for(int i=0;i<4;++i)CHECK(nav.apply(MenuNavAction::NextRow));
        ModuleArrayListFeed feed;
        GuiModuleControlHandoff handoff;
        std::mutex mu;
        std::condition_variable cv;
        int stage=0;
        bool published=false,accepted=true,retired=false;
        std::thread owner([&]{
            R246NativeFixture native;
            NativeModuleSnapshotAdapter adapter;
            published=adapter.publish_on_owner(native,feed,30u,true);
            {
                std::lock_guard<std::mutex> lock(mu);stage=1;
            }
            cv.notify_all();
            {
                std::unique_lock<std::mutex> lock(mu);
                cv.wait(lock,[&]{return stage>=2;});
            }
            if(variation==0)native.deny();
            if(variation==1)native.set_generation(9u);
            if(variation==2)native.ignore();
            accepted=adapter.apply_request_on_owner(native,handoff,feed,31u,true);
            retired=adapter.retired();
            {
                std::lock_guard<std::mutex> lock(mu);stage=3;
            }
            cv.notify_all();
        });
        {
            std::unique_lock<std::mutex> lock(mu);
            cv.wait(lock,[&]{return stage>=1;});
        }
        CHECK(published && feed.latest());
        CHECK(handoff.request_focused_toggle(menu,nav,feed,30u));
        {
            std::lock_guard<std::mutex> lock(mu);stage=2;
        }
        cv.notify_all();
        {
            std::unique_lock<std::mutex> lock(mu);
            cv.wait(lock,[&]{return stage>=3;});
        }
        owner.join();
        CHECK(!accepted);
        if(variation==0) {
            CHECK(!retired && !feed.retired() &&
                  handoff.view().phase==GuiModuleControlPhase::Denied);
            CHECK(feed.latest() && feed.latest()->source_sequence==30u);
        } else {
            CHECK(retired && feed.retired() && !feed.latest() &&
                  handoff.view().phase==GuiModuleControlPhase::Retired);
        }
    }
    std::cout<<"R246_NATIVE_DENIAL_STALE_GENERATION_AND_FALSE_ACK_QUARANTINE_PASS\n";
}
int main(){
    test_snapshot_negatives();
    test_request_roundtrip();
    test_denial_and_mismatch();
    std::cout<<"R246_NO_MINECRAFT_INPUT_OR_GAME_ACTIONS_PASS\n";
}
