#include "slinkywinky/menu.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <set>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <limits>
#define CHECK(x) do {if(!(x))throw std::runtime_error("Menu test failure: " #x);}while(false)

using namespace slinkywinky;
int main() {
    const auto& catalog=official_module_catalog();
    CHECK(catalog.size()==67);
    CHECK(std::string(catalog[54].name)=="Chest Stealer");
    CHECK(std::string(catalog[55].name)=="Debug");
    CHECK(std::string(catalog[35].name)=="Mod Overlay");
    std::set<std::string> unique;
    std::size_t counts[5]{};
    for(const auto& entry:catalog){
        CHECK(entry.name!=nullptr && std::string(entry.name).size()>1);
        CHECK(unique.insert(entry.name).second);
        auto category=static_cast<std::size_t>(entry.tab);
        CHECK(category<5);
        ++counts[category];
    }
    CHECK(counts[0]==17 && counts[1]==10 && counts[2]==15 &&
          counts[3]==7 && counts[4]==18);
    CHECK(std::string(menu_tab_name(MenuTab::Combat))=="Combat");
    CHECK(std::string(menu_tab_name(MenuTab::Profiles))=="Profiles");
    CHECK(std::string(menu_tab_name(MenuTab::Count))=="Unknown");

    MenuState state;
    CHECK(state.tab()==MenuTab::Combat && state.scroll()==0);
    CHECK(state.tab_count(MenuTab::Combat)==17);
    CHECK(state.tab_count(MenuTab::Block)==7);
    CHECK(state.tab_count(MenuTab::Profiles)==0);
    CHECK(state.page().size()==8);
    CHECK(state.page(200).size()==8); // bounded presentation
    CHECK(state.page(2).size()==2);
    CHECK(state.page(0).empty());
    CHECK(!state.preview_enabled(0) && !state.expanded(0));
    CHECK(!state.toggle_preview(67) && !state.toggle_expanded(67));
    CHECK(!state.toggle_preview(17)); // cannot mutate off-category entries
    CHECK(!state.preview_enabled(17));
    CHECK(state.toggle_preview(0) && state.preview_enabled(0));
    CHECK(state.toggle_expanded(0) && state.expanded(0));
    CHECK(state.page()[0].preview_enabled && state.page()[0].expanded);
    CHECK(state.toggle_preview(0) && !state.preview_enabled(0));
    CHECK(!state.scroll_by(-20));
    CHECK(state.scroll_by(9999));
    CHECK(state.scroll()==9 && state.page().size()==8);
    CHECK(std::string(state.page().front().title)=="Knockback Displace");
    CHECK(!state.scroll_by(2));
    CHECK(state.scroll_by(-99999) && state.scroll()==0);
    CHECK(state.select(MenuTab::Move) && state.scroll()==0);
    CHECK(state.page().size()==8);
    CHECK(state.page().front().title=="Fast Accel");
    CHECK(!state.toggle_preview(0)); // deny cross-tab modification
    CHECK(state.select(MenuTab::Block) && state.page().size()==7);
    CHECK(!state.scroll_by(100));
    CHECK(state.select(MenuTab::Profiles) && state.page().empty());
    CHECK(!state.select(MenuTab::Unload) && state.tab()==MenuTab::Profiles);
    CHECK(!state.select(MenuTab::Count) && state.tab()==MenuTab::Profiles);
    CHECK(state.select(MenuTab::Combat) && state.expanded(0));
    CHECK(!state.appearance().compact && !state.appearance().wide);
    CHECK(!state.appearance().allow_input_while_open);
    CHECK(state.appearance().accent_rgb==0xF35D12u);
    state.set_compact(true);
    state.set_wide(true);
    state.set_allow_input(true);
    CHECK(state.appearance().compact && state.appearance().wide &&
          state.appearance().allow_input_while_open);
    CHECK(state.set_scale(0.65f) && state.set_scale(1.8f));
    CHECK(!state.set_scale(0.64f) && !state.set_scale(1.81f));
    CHECK(!state.set_scale(std::numeric_limits<float>::quiet_NaN()));
    CHECK(!state.set_scale(std::numeric_limits<float>::infinity()));
    CHECK(std::abs(state.appearance().scale-1.8f)<0.001f);
    CHECK(state.set_accent(0xFF7D30u));
    CHECK(!state.set_accent(0x1000000u));
    CHECK(state.appearance().accent_rgb==0xFF7D30u);
    // Strict round trip, independent of game profiles and no module toggles.
    MenuAppearance stored=state.appearance(), parsed{};
    const auto serialized=serialize_menu_appearance(stored);
    CHECK(serialized.find("version=1")!=std::string::npos);
    CHECK(serialized.find("accent=ff7d30")!=std::string::npos);
    CHECK(serialized.find("auto_clicker")==std::string::npos);
    CHECK(parse_menu_appearance(serialized,parsed));
    CHECK(parsed.compact && parsed.wide && parsed.allow_input_while_open);
    CHECK(parsed.accent_rgb==0xFF7D30u);
    CHECK(std::abs(parsed.scale-1.8f)<0.001f);
    auto before=parsed;
    for(const auto& invalid:std::vector<std::string>{
        "", "version=2\n", "version=1\ncompact=maybe\n",
        "version=1\ncompact=true\ncompact=false\n",
        "version=1\ncompact=true\nwide=true\nallow_input=true\naccent=xyz123\nscale=1.0\n",
        "version=1\ncompact=true\nwide=true\nallow_input=true\naccent=ffffff\nscale=inf\n",
        "version=1\ncompact=true\nwide=true\nallow_input=true\naccent=ffffff\nscale=0.2\n",
        "version=1\ncompact=true\nwide=true\nallow_input=true\naccent=ffffff\nscale=1.0\nextra=3\n"
    }) {
        CHECK(!parse_menu_appearance(invalid,parsed));
        CHECK(parsed.accent_rgb==before.accent_rgb && parsed.scale==before.scale);
    }
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r62-test-menu-pref.ini";
    std::error_code ignored;
    std::filesystem::remove(file,ignored);
    CHECK(save_menu_appearance(file,stored));
    MenuAppearance fileCopy{};
    CHECK(load_menu_appearance(file,fileCopy));
    CHECK(fileCopy.accent_rgb==stored.accent_rgb &&
          fileCopy.compact==stored.compact && fileCopy.wide==stored.wide);
    {
        std::ofstream invalidFile(file,std::ios::binary|std::ios::trunc);
        invalidFile<<"not an appearance document";
    }
    CHECK(!load_menu_appearance(file,fileCopy));
    CHECK(fileCopy.accent_rgb==stored.accent_rgb);
    std::filesystem::remove(file,ignored);
    // R235: one native source text event is all-or-nothing, including
    // mixed UTF-8 scalars, malformed suffixes, NUL and field capacity.
    {
        const auto& widgets=recovered_widget_catalog();
        std::size_t textIndex=widgets.size();
        for(std::size_t i=0;i<widgets.size();++i)
            if(widgets[i].kind==RecoveredWidgetKind::Text &&
               std::string_view(widgets[i].module)=="Auto Tool" &&
               std::string_view(widgets[i].auxiliary)=="Search item")
                {textIndex=i;break;}
        CHECK(textIndex<widgets.size());
        MenuState editor;
        CHECK(editor.select(MenuTab::Block));
        CHECK(editor.toggle_expanded(42u)); // Auto Tool, exact catalog
        bool focused=false;
        for(unsigned page=0u;page<10u && !focused;++page) {
            focused=editor.focus_recovered_text_preview(42u,textIndex);
            if(!focused)CHECK(editor.change_settings_page(42u,1));
        }
        CHECK(focused && editor.modal_input_owned());
        CHECK(editor.append_recovered_text_event_preview("ab"));
        CHECK(editor.append_recovered_text_event_preview("\xC3\xA4"));
        CHECK(!editor.append_recovered_text_event_preview(
            std::string_view("A\xC3",2u)));
        CHECK(!editor.append_recovered_text_event_preview("\xC0\xAF"));
        CHECK(!editor.append_recovered_text_event_preview("12345"));
        CHECK(!editor.append_recovered_text_event_preview({}));
        CHECK(editor.recovered_text_overrides()[textIndex] &&
              *editor.recovered_text_overrides()[textIndex]=="ab\xC3\xA4");
        for(unsigned n=0u;n<30u;++n)
            CHECK(editor.append_recovered_text_event_preview("xxxx"));
        CHECK(editor.append_recovered_text_event_preview("xxx"));
        const std::string beforeLimit=
            std::string("ab\xC3\xA4")+std::string(123u,'x');
        CHECK(beforeLimit.size()==127u &&
              *editor.recovered_text_overrides()[textIndex]==beforeLimit);
        CHECK(!editor.append_recovered_text_event_preview("YZ"));
        CHECK(*editor.recovered_text_overrides()[textIndex]==beforeLimit);
        CHECK(editor.append_recovered_text_event_preview("Q"));
        CHECK(editor.recovered_text_overrides()[textIndex]->size()==128u);
        CHECK(!editor.append_recovered_text_event_preview("!"));
        CHECK(*editor.recovered_text_overrides()[textIndex]==beforeLimit+"Q");
        CHECK(editor.erase_recovered_text_preview() &&
              *editor.recovered_text_overrides()[textIndex]==beforeLimit);
        CHECK(editor.blur_recovered_text_preview());
        CHECK(!editor.append_recovered_text_event_preview("K"));
        CHECK(editor.select(MenuTab::Combat));
    }
    // R236: a keybind/text editor is the exclusive owner. Direct
    // low-level GUI calls cannot silently replace it with a popup,
    // drag or setting write, even if a caller bypasses navigation.
    {
        const auto& widgets=recovered_widget_catalog();
        std::size_t textIndex=widgets.size(),multiIndex=widgets.size();
        std::size_t sliderIndex=widgets.size(),booleanIndex=widgets.size();
        for(std::size_t i=0u;i<widgets.size();++i) {
            if(std::string_view(widgets[i].module)!="Auto Tool")continue;
            if(widgets[i].kind==RecoveredWidgetKind::Text &&
               std::string_view(widgets[i].auxiliary)=="Search item")
                textIndex=i;
            if(widgets[i].kind==RecoveredWidgetKind::MultiSelect)
                multiIndex=i;
            if(widgets[i].kind==RecoveredWidgetKind::Slider)
                sliderIndex=i;
            if(widgets[i].kind==RecoveredWidgetKind::Boolean &&
               std::string_view(widgets[i].label)=="Switch back when done")
                booleanIndex=i;
        }
        CHECK(textIndex<widgets.size() && multiIndex<widgets.size() &&
              sliderIndex<widgets.size() && booleanIndex<widgets.size());
        MenuState owned;
        CHECK(owned.select(MenuTab::Block));
        CHECK(owned.toggle_expanded(42u)); // source Auto Tool
        CHECK(owned.begin_bind_capture(42u));
        CHECK(!owned.focus_recovered_text_preview(42u,textIndex));
        CHECK(!owned.open_recovered_multi_preview(42u,multiIndex));
        CHECK(!owned.set_recovered_slider_preview(42u,sliderIndex,20.0f));
        CHECK(!owned.set_recovered_boolean_preview(
            42u,booleanIndex,false));
        CHECK(!owned.begin_numeric_drag_preview(42u,sliderIndex,0u));
        CHECK(owned.bind_capture_index()==42u &&
              !owned.focused_text_widget() && !owned.dropdown_preview());
        CHECK(!owned.recovered_slider_override(sliderIndex) &&
              !owned.recovered_boolean_override(booleanIndex));
        CHECK(owned.cancel_bind_capture());

        CHECK(owned.focus_recovered_text_preview(42u,textIndex));
        CHECK(owned.focus_recovered_text_preview(42u,textIndex));
        CHECK(owned.append_recovered_text_event_preview("ok"));
        CHECK(!owned.begin_bind_capture(42u));
        CHECK(!owned.open_recovered_multi_preview(42u,multiIndex));
        CHECK(!owned.set_recovered_slider_preview(42u,sliderIndex,20.0f));
        CHECK(!owned.set_recovered_boolean_preview(
            42u,booleanIndex,false));
        CHECK(!owned.begin_numeric_drag_preview(42u,sliderIndex,0u));
        CHECK(!owned.open_unload_confirmation_preview());
        CHECK(owned.focused_text_widget()==textIndex &&
              !owned.dropdown_preview() &&
              owned.recovered_text_overrides()[textIndex] &&
              *owned.recovered_text_overrides()[textIndex]=="ok");
        CHECK(owned.blur_recovered_text_preview());
        CHECK(owned.open_recovered_multi_preview(42u,multiIndex));
        CHECK(!owned.begin_bind_capture(42u));
        CHECK(owned.close_preview_dropdown());
        CHECK(owned.set_recovered_slider_preview(42u,sliderIndex,20.0f));
        CHECK(owned.set_recovered_boolean_preview(
            42u,booleanIndex,false));
        CHECK(owned.recovered_slider_override(sliderIndex)==20.0f &&
              owned.recovered_boolean_override(booleanIndex)==false);
        CHECK(owned.begin_numeric_drag_preview(42u,sliderIndex,0u));
        CHECK(!owned.open_recovered_multi_preview(42u,multiIndex));
        CHECK(owned.cancel_numeric_drag_preview());
        CHECK(owned.begin_bind_capture(42u));
        CHECK(owned.cancel_bind_capture());

        MenuState combat;
        CHECK(combat.toggle_expanded(0u)); // source Aim Assist
        CHECK(combat.begin_bind_capture(0u));
        CHECK(!combat.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        CHECK(combat.bind_capture_index()==0u &&
              !combat.dropdown_preview());
        CHECK(combat.cancel_bind_capture());
        CHECK(combat.open_preview_dropdown(MenuDropdownKind::AimMode,0u));
        CHECK(combat.close_preview_dropdown());
    }
    // UI is strictly an inert model: no game/JVM or network dependencies.
    return 0;
}
