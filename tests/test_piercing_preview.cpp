#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <atomic>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R124 line "+std::to_string(__LINE__)+": " #x);}while(false)

using namespace slinkywinky;

int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    CHECK(menu.piercing_selected()==0);
    CHECK(menu.piercing_preview().sort==0);
    CHECK(menu.piercing_preview().enabled[6]); // source example default only
    for(std::size_t i=0;i<6;i++)CHECK(!menu.piercing_preview().enabled[i]);
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl)); // wrong focus
    CHECK(!menu.activate_piercing_selection());
    CHECK(!menu.move_piercing_selection(0));
    CHECK(!menu.move_piercing_selection(2));
    CHECK(!nav.focus_visible_row(11)); // initially outside eight-row page
    CHECK(profile.scroll_by(4)); // show catalog items 4..11
    CHECK(nav.focus_visible_row(11));
    CHECK(!nav.apply(MenuNavAction::ActivatePiercingControl)); // collapsed
    CHECK(nav.apply(MenuNavAction::ExpandFocused));
    CHECK(menu.expanded(11));
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(menu.piercing_preview().sort==1); // Health
    CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(menu.piercing_selected()==1);
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(menu.piercing_preview().enabled[0]); // Non-player entities
    CHECK(nav.apply(MenuNavAction::PreviousPiercingControl));
    CHECK(menu.piercing_selected()==0);
    CHECK(!nav.apply(MenuNavAction::PreviousPiercingControl));
    CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    for(std::size_t i=2;i<=7;i++)
        CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(menu.piercing_selected()==7 && menu.settings_offset(11)==5);
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl)); // upper bound
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(!menu.piercing_preview().enabled[6]);
    const PiercingPreview saved=menu.piercing_preview();
    CHECK(saved.sort==1 && saved.enabled[0] && !saved.enabled[6]);
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r124-piercing-sandbox-only.conf";
    std::error_code ignored;
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(file.string()+".tmp",ignored);
    const auto text=serialize_piercing_preview(saved);
    CHECK(text.find("version=1\n")!=std::string::npos);
    CHECK(text.find("sort=1\n")!=std::string::npos);
    CHECK(text.find("flags=1000000\n")!=std::string::npos);
    CHECK(text.find("aim_enabled")==std::string::npos);
    CHECK(text.find("Minecraft")==std::string::npos);
    CHECK(save_piercing_preview(file,saved));
    PiercingPreview loaded{};
    CHECK(load_piercing_preview(file,loaded) && loaded==saved);
    CHECK(parse_piercing_preview(text,loaded));
    CHECK(loaded==saved);
    auto prior=loaded;
    for(const auto& malformed: {
        "", "version=2\nsort=1\nflags=1000000\n",
        "version=1\nsort=2\nflags=1000000\n",
        "version=1\nsort=1\nflags=100000\n",
        "version=1\nsort=1\nflags=10000x0\n",
        "version=1\nsort=1\nflags=1000000\nextra=yes\n",
        "version=1\nsort=1\nsort=0\nflags=1000000\n",
        "version=1\nsort=1\nflags=1000000",
        "version=1\nsort=1\nflags=1000000\nflags=1000000\n"
    }) {
        CHECK(!parse_piercing_preview(malformed,loaded));
        CHECK(loaded==prior); // no partial import
    }
    PiercingPreview invalid=saved;
    invalid.sort=2;
    CHECK(serialize_piercing_preview(invalid).empty());
    CHECK(!menu.adopt_piercing_preview(invalid));
    CHECK(menu.piercing_preview()==saved);
    CHECK(!save_piercing_preview(file,invalid));
    CHECK(menu.adopt_piercing_preview(loaded));
    CHECK(menu.piercing_selected()==0 && menu.settings_offset(11)==0);
    CHECK(menu.piercing_preview()==saved);

    // Snapshot values, selected callsite and mode are immutable after edits.
    MenuRenderSession frames;
    CHECK(frames.attach(124,900,630));
    auto oldFrame=frames.next(menu,&profile,&nav);
    CHECK(oldFrame && oldFrame->piercing_preview==saved);
    CHECK(oldFrame->piercing_selected==0);
    MenuMouseGesture gate;
    CHECK(gate.press(*oldFrame,MenuPanelMode::StandalonePreview,
                     40,30,100,true));
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    auto newFrame=frames.next(menu,&profile,&nav);
    CHECK(newFrame && newFrame->piercing_preview.sort==0);
    CHECK(oldFrame->piercing_preview.sort==1);
    CHECK(!gate.release(*newFrame,MenuPanelMode::StandalonePreview,
                        40,30,120,true));
    CHECK(!gate.pending());
    CHECK(frames.detach());
    CHECK(nav.apply(MenuNavAction::ExpandFocused)); // collapse
    CHECK(!nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(nav.select_tab(MenuTab::Move));
    CHECK(!menu.activate_piercing_selection());
    CHECK(!menu.move_piercing_selection(1));

    std::atomic<bool> foreignRejected{false};
    std::thread foreign([&]{
        foreignRejected=!nav.apply(MenuNavAction::NextPiercingControl) &&
                        !nav.apply(MenuNavAction::ActivatePiercingControl);
    });
    foreign.join();
    CHECK(foreignRejected);
    {
        std::ofstream broken(file,std::ios::binary|std::ios::trunc);
        broken<<"version=1\nsort=1\nflags=1111111\ninvalid=1\n";
    }
    CHECK(!load_piercing_preview(file,loaded) && loaded==prior);
    {
        std::ofstream huge(file,std::ios::binary|std::ios::trunc);
        huge<<std::string(513,'x');
    }
    CHECK(!load_piercing_preview(file,loaded) && loaded==prior);
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(file.string()+".tmp",ignored);
    return 0;
}
