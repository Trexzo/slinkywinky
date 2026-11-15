#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_settings_vertical.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R131 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& all=recovered_widget_catalog();
    unsigned dropCount=0;
    for(const auto& widget:all)
        if(widget.kind==RecoveredWidgetKind::Dropdown)++dropCount;
    CHECK(dropCount==32u);
    CHECK(menu_source_choice_count(all.size())==0u);
    std::size_t pattern=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        const auto& row=all[i];
        if(std::string_view(row.module)=="Auto Clicker" &&
           std::string_view(row.label)=="Click pattern")
            pattern=i;
    }
    CHECK(pattern<all.size());
    CHECK(menu_source_choice_count(pattern)==2u);
    CHECK(menu_source_choice_label(pattern,0u)=="Jitter");
    CHECK(menu_source_choice_label(pattern,1u)=="Butterfly");
    CHECK(menu_source_choice_label(pattern,2u).empty());

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.focus_visible_row(2u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession frames;
        CHECK(frames.attach(131,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y){
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[2].catalog_index==2u &&
              frame->visible[2].expanded);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),2u);
        CHECK(card.y+card.height<
              panel->logical_height()-menu_panel_padding);
        const auto control=menu_setting_column(card.x,card.width);
        const auto target=xy(control.x+control.width*0.5f,card.y+
            menu_row_body_height(*frame)+
            menu_settings_first_control_offset+10.0f);
        const auto hit=hit_test_menu(
            *frame,mode,target.first,target.second);
        CHECK(hit && hit->kind==MenuHitKind::RecoveredChoiceControl &&
              hit->catalog_index==2u && hit->control_index==pattern);
        MenuMouseGesture gesture;
        CHECK(gesture.press(*frame,mode,target.first,target.second,
                            100,true));
        const auto opened=gesture.release(
            *frame,mode,target.first,target.second,120,true);
        CHECK(opened && opened->control_index==pattern);
        CHECK(menu.open_recovered_choice_preview(
            opened->catalog_index,opened->control_index));
        CHECK(menu.dropdown_preview() &&
              menu.dropdown_preview()->widget_index==pattern &&
              menu.dropdown_preview()->kind==
                  MenuDropdownKind::RecoveredChoice);
        const auto snap=frames.next(menu,&profile,&nav);
        const auto popup=menu_dropdown_popup_geometry(
            *snap,panel->logical_width(),panel->logical_height());
        CHECK(popup && popup->option_count==2u &&
              popup->selected==0u);
        CHECK(!menu.select_preview_dropdown_option(2u));
        CHECK(menu.dropdown_preview());
        const auto p=xy(popup->x+popup->width*0.5f,
                        popup->y+popup->row_height*1.5f);
        const auto popupHit=hit_test_menu(
            *snap,mode,p.first,p.second);
        CHECK(popupHit && popupHit->kind==MenuHitKind::DropdownOption &&
              popupHit->control_index==1u);
        // A popup cannot leak a click to the underlying card.
        const auto blocked=hit_test_menu(
            *snap,mode,target.first,target.second);
        CHECK(blocked && blocked->kind==MenuHitKind::DropdownOption &&
              blocked->control_index==0u);
        CHECK(gesture.press(*snap,mode,p.first,p.second,1000,true));
        const auto chosen=gesture.release(
            *snap,mode,p.first,p.second,1020,true);
        CHECK(chosen && chosen->kind==MenuHitKind::DropdownOption);
        CHECK(menu.select_preview_dropdown_option(
            static_cast<unsigned>(chosen->control_index)));
        CHECK(!menu.dropdown_preview());
        const auto updated=frames.next(menu,&profile,&nav);
        CHECK(updated && updated->recovered_choice_overrides[pattern] &&
              *updated->recovered_choice_overrides[pattern]==1u);
        CHECK(!menu.preview_enabled(2u) && !profile.enabled(2u));
        // One UI preview choice does not affect another module's settings.
        CHECK(menu.open_recovered_choice_preview(2u,pattern));
        const auto pressFrame=frames.next(menu,&profile,&nav);
        const auto openAgain=menu_dropdown_popup_geometry(
            *pressFrame,panel->logical_width(),panel->logical_height());
        CHECK(openAgain && openAgain->selected==1u);
        const auto firstChoice=xy(openAgain->x+openAgain->width*0.5f,
            openAgain->y+openAgain->row_height*0.5f);
        CHECK(gesture.press(*pressFrame,mode,
              firstChoice.first,firstChoice.second,2000,true));
        CHECK(menu.close_preview_dropdown());
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(!gesture.release(*stale,mode,
              firstChoice.first,firstChoice.second,2020,true));
        CHECK(stale->recovered_choice_overrides[pattern] &&
              *stale->recovered_choice_overrides[pattern]==1u);
        // The original Aim mode is still owned by its specialized path.
        CHECK(!menu.open_recovered_choice_preview(0u,pattern));
        CHECK(frames.detach());

        // R226: original large optional selection is not a normal
        // "None" popup row. Its 20px clear X is a distinct source hit.
        std::size_t invModule=official_module_catalog().size();
        std::size_t invWidget=all.size();
        for(std::size_t i=0;i<official_module_catalog().size();++i)
            if(std::string_view(official_module_catalog()[i].name)==
               "Inv Manager")invModule=i;
        for(std::size_t i=0;i<all.size();++i)
            if(std::string_view(all[i].module)=="Inv Manager" &&
               std::string_view(all[i].label)=="Slot 1" &&
               all[i].kind==RecoveredWidgetKind::Dropdown)invWidget=i;
        CHECK(invModule<official_module_catalog().size() &&
              invWidget<all.size() &&
              menu_source_large_optional_choice(invWidget) &&
              !menu_source_large_optional_choice(pattern));
        MenuState inventory;
        MenuProfileBridge inventoryProfile(inventory);
        MenuNavigation inventoryNav(inventory,inventoryProfile);
        CHECK(inventoryNav.select_tab(MenuTab::Utility));
        for(unsigned n=0;n<20u;++n) {
            const auto rows=inventory.page();
            if(!rows.empty() && rows.front().catalog_index==invModule)
                break;
            CHECK(inventoryProfile.scroll_by_visible(1,8));
        }
        CHECK(!inventory.page().empty() &&
              inventory.page().front().catalog_index==invModule);
        CHECK(inventoryNav.focus_visible_row(invModule));
        CHECK(inventoryNav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession invFrames;
        CHECK(invFrames.attach(226,900,630));
        for(unsigned page=0;page<10u;++page) {
            const auto candidate=invFrames.next(
                inventory,&inventoryProfile,&inventoryNav);
            CHECK(candidate);
            std::array<const RecoveredWidget*,5> widgets{};
            CHECK(recovered_widget_visible_page("Inv Manager",
                candidate->visible.front().settings_offset,
                static_cast<int>(candidate->aim_preview_mode),
                static_cast<int>(candidate->target_preview_mode),
                widgets));
            if(std::find(widgets.begin(),widgets.end(),&all[invWidget])!=
               widgets.end())break;
            CHECK(inventory.change_settings_page(invModule,1));
        }
        auto invFrame=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        CHECK(invFrame);
        const auto clear=menu_dropdown_clear_geometry(
            *invFrame,panel->logical_width(),panel->logical_height(),
            invModule,invWidget);
        CHECK(clear);
        const auto clearAt=xy(clear->x+10.0f,clear->y+10.0f);
        const auto clearHit=hit_test_menu(
            *invFrame,mode,clearAt.first,clearAt.second);
        CHECK(clearHit &&
              clearHit->kind==MenuHitKind::RecoveredChoiceClear &&
              clearHit->catalog_index==invModule &&
              clearHit->control_index==invWidget);
        CHECK(!inventory.clear_recovered_choice_preview(2u,invWidget));
        CHECK(inventory.clear_recovered_choice_preview(
            invModule,invWidget));
        CHECK(!inventory.clear_recovered_choice_preview(
            invModule,invWidget));
        invFrame=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        CHECK(invFrame && invFrame->recovered_choice_overrides[invWidget] &&
              *invFrame->recovered_choice_overrides[invWidget]==0u);
        CHECK(!menu_dropdown_clear_geometry(
            *invFrame,panel->logical_width(),panel->logical_height(),
            invModule,invWidget));
        const auto clearedHit=hit_test_menu(
            *invFrame,mode,clearAt.first,clearAt.second);
        CHECK(clearedHit &&
              clearedHit->kind==MenuHitKind::RecoveredChoiceControl);
        CHECK(inventory.open_recovered_choice_preview(
            invModule,invWidget));
        const auto openFrame=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        const auto modal=menu_search_popup_geometry(
            *openFrame,panel->logical_width(),panel->logical_height());
        CHECK(modal && modal->matches.count==9u &&
              modal->matches.indices[0]==1u &&
              modal->matches.indices[8]==9u);
        CHECK(!inventory.select_preview_dropdown_option(0u));
        CHECK(!inventory.scroll_preview_popup(-1));
        CHECK(!inventory.scroll_preview_popup(1));
        CHECK(menu_source_search_matches(invWidget,"GOLDEN").count==1u);
        CHECK(menu_source_search_matches(invWidget,"oW").count==1u);
        CHECK(!menu_search_ascii_valid("gold!"));
        const auto close=xy(modal->close.x+10.0f,modal->close.y+10.0f);
        const auto closeHit=hit_test_menu(
            *openFrame,mode,close.first,close.second);
        CHECK(closeHit && closeHit->kind==MenuHitKind::SearchPopupClose);
        const auto field=xy(modal->field.x+10.0f,modal->field.y+10.0f);
        const auto fieldHit=hit_test_menu(
            *openFrame,mode,field.first,field.second);
        CHECK(fieldHit && fieldHit->kind==MenuHitKind::SearchPopupField);
        const auto behind=xy(25.0f,20.0f);
        CHECK(!hit_test_menu(*openFrame,mode,
            behind.first,behind.second));
        const auto first=modal->result_cell(0u);
        const auto choicePoint=xy(first.x+10.0f,first.y+10.0f);
        MenuMouseGesture searchGesture;
        CHECK(searchGesture.press(*openFrame,mode,
              choicePoint.first,choicePoint.second,2000u,true));
        CHECK(inventory.search_preview_popup_key(34u)); // G
        const auto changed=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        CHECK(!searchGesture.release(*changed,mode,
              choicePoint.first,choicePoint.second,2040u,true));
        CHECK(inventory.search_preview_popup_key(24u)); // O
        CHECK(inventory.search_preview_popup_key(38u)); // L
        const auto searchFrame=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        const auto filtered=menu_search_popup_geometry(
            *searchFrame,panel->logical_width(),panel->logical_height());
        CHECK(filtered && filtered->matches.count==1u &&
              filtered->matches.indices[0]==7u &&
              searchFrame->dropdown_search_query=="gol");
        CHECK(!inventory.select_preview_dropdown_option(9u));
        const auto cell=filtered->result_cell(0u);
        const auto point=xy(cell.x+8.0f,cell.y+12.0f);
        const auto filteredHit=hit_test_menu(
            *searchFrame,mode,point.first,point.second);
        CHECK(filteredHit &&
              filteredHit->kind==MenuHitKind::SearchPopupOption &&
              filteredHit->control_index==7u);
        CHECK(inventory.search_preview_popup_key(14u)); // Backspace
        CHECK(inventory.search_preview_popup_query()=="go");
        CHECK(inventory.search_preview_popup_key(38u)); // L
        CHECK(inventory.select_preview_dropdown_option(7u));
        const auto picked=invFrames.next(
            inventory,&inventoryProfile,&inventoryNav);
        CHECK(picked && !picked->dropdown_preview &&
              picked->dropdown_search_query.empty() &&
              picked->recovered_choice_overrides[invWidget] &&
              *picked->recovered_choice_overrides[invWidget]==7u);
        CHECK(inventory.open_recovered_choice_preview(invModule,invWidget));
        CHECK(inventory.search_preview_popup_query().empty());
        CHECK(inventory.search_preview_popup_key(34u));
        CHECK(inventory.search_preview_popup_key(1u)); // Esc cancels
        CHECK(!inventory.dropdown_preview() &&
              !inventory.search_preview_popup_key(34u));
        CHECK(menu_dropdown_clear_geometry(
            *picked,panel->logical_width(),panel->logical_height(),
            invModule,invWidget).has_value());
        CHECK(invFrames.detach());
    }
    return 0;
}
