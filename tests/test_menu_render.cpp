#include "slinkywinky/menu_render.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <thread>
#define CHECK(c) do {if(!(c)) throw std::runtime_error("render session: " #c);}while(false)

using namespace slinkywinky;
int main() {
    MenuState menu;
    MenuRenderSession render;
    CHECK(!render.attached());
    CHECK(!render.next(menu));
    CHECK(!render.detach());
    CHECK(!render.attach(0,676,620));
    CHECK(!render.attach(1,0,620));
    CHECK(!render.attach(1,676,0));
    CHECK(!render.attach(1,319,620));
    CHECK(!render.attach(1,676,239));
    CHECK(!render.attach(1,8193,620));
    CHECK(!render.attach(1,676,8193));
    CHECK(render.attach(1,676,620));
    CHECK(render.attached() && render.sequence()==0);
    CHECK(!render.attach(2,676,620));
    auto a=render.next(menu);
    CHECK(a && a->epoch==1 && a->sequence==1);
    CHECK(a->viewport_width==676 && a->viewport_height==620);
    CHECK(a->tab==MenuTab::Combat && a->visible.size()==8);
    CHECK(a->visible[0].title=="Aim Assist");
    CHECK(!a->visible[0].preview_enabled);
    CHECK(menu.toggle_preview(0));
    auto b=render.next(menu);
    CHECK(b && b->sequence==2 && b->visible[0].preview_enabled);
    CHECK(!a->visible[0].preview_enabled); // snapshot is independent
    CHECK(render.sequence()==2);
    CHECK(!render.resize(0,620));
    CHECK(!render.resize(9000,620));
    CHECK(render.resize(844,620));
    CHECK(menu.select(MenuTab::Profiles));
    auto profiles=render.next(menu);
    CHECK(profiles && profiles->is_placeholder && profiles->visible.empty());
    CHECK(profiles->viewport_width==844 && profiles->sequence==3);
    std::atomic<bool> wrongFrame{false},wrongResize{false},wrongDetach{false};
    std::thread wrong([&] {
        wrongFrame.store(!render.next(menu));
        wrongResize.store(!render.resize(676,620));
        wrongDetach.store(!render.detach());
    });
    wrong.join();
    CHECK(wrongFrame && wrongResize && wrongDetach);
    CHECK(render.attached() && render.sequence()==3);
    CHECK(menu.select(MenuTab::Block));
    auto blocks=render.next(menu);
    CHECK(blocks && blocks->visible.size()==7);
    CHECK(render.detach());
    CHECK(!render.attached() && render.sequence()==0);
    CHECK(!render.next(menu));
    CHECK(!render.resize(676,620));
    CHECK(!render.attach(1,676,620)); // never resurrect stale context epoch
    CHECK(render.attach(2,676,620));
    CHECK(render.next(menu)->epoch==2);
    CHECK(render.detach());
    // The session is owner bound: a nonowner can't shut it down or render.
    std::atomic<bool> workerAttached{false},workerStopped{false};
    std::thread worker([&] {
        workerAttached=render.attach(3,676,620);
        workerStopped=render.detach();
    });
    worker.join();
    CHECK(workerAttached && workerStopped);
    CHECK(!render.attached());
    CHECK(!render.attach(3,676,620));
    CHECK(render.attach(4,676,620));
    CHECK(render.detach());
    return 0;
}
