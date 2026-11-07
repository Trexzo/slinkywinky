#include "slinkywinky/menu.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <string_view>

namespace slinkywinky {
namespace {
// R115: exact category/name descriptors from the supplied candidate decrypted
// binary archive. This does not authenticate an official Slinky build.
#include "r115_recovered_catalog.inc"

bool is_module_tab(MenuTab tab) noexcept {
    return tab <= MenuTab::Utility;
}
// Bounded, read-only parsing of the recovered fixture choice labels.
unsigned recovered_option_count(
        std::string_view options,unsigned maximum) noexcept {
    if(options.empty() || maximum<1u || maximum>16u)return 0u;
    unsigned count=1u;
    bool after_separator=true;
    for(const char c:options) {
        if(c=='|') {
            if(after_separator || count>=maximum)return 0u;
            ++count;
            after_separator=true;
        } else after_separator=false;
    }
    return after_separator?0u:count;
}
unsigned recovered_choice_count(std::string_view options) noexcept {
    return recovered_option_count(options,12u);
}
unsigned recovered_multi_count(std::string_view options) noexcept {
    return recovered_option_count(options,16u);
}
} // namespace

const std::array<MenuModule,67>& official_module_catalog() noexcept {return catalog;}

const char* menu_tab_name(MenuTab tab) noexcept {
    switch(tab){
        case MenuTab::Combat:return "Combat";
        case MenuTab::Move:return "Move";
        case MenuTab::Visual:return "Visual";
        case MenuTab::Block:return "Block";
        case MenuTab::Utility:return "Utility";
        case MenuTab::Profiles:return "Profiles";
        case MenuTab::Unload:return "Unload";
        default:return "Unknown";
    }
}
std::size_t MenuState::tab_count(MenuTab tab) const noexcept {
    if(!is_module_tab(tab))return 0;
    return static_cast<std::size_t>(std::count_if(catalog.begin(),catalog.end(),
        [tab](const MenuModule& item){return item.tab==tab;}));
}
bool MenuState::select(MenuTab tab) noexcept {
    if(tab==MenuTab::Count || tab==MenuTab::Unload)return false;
    dropdown_preview_.reset();
    unload_confirmation_preview_=false;
    selected_=tab;
    scroll_=0;
    return true;
}
bool MenuState::scroll_by(int delta) noexcept {
    const auto total=tab_count(selected_);
    if(total==0)return false;
    // Renderer has eight visible rows. Keep scrolling independent of module
    // enable flags, with no overflow from large wheel deltas.
    const auto maxScroll=total>8?total-8:0;
    const std::int64_t proposed=static_cast<std::int64_t>(scroll_)+delta;
    const std::int64_t capped=std::max<std::int64_t>(0,
        std::min<std::int64_t>(static_cast<std::int64_t>(maxScroll),proposed));
    const auto next=static_cast<std::size_t>(capped);
    if(next==scroll_)return false;
    dropdown_preview_.reset();
    scroll_=next;
    return true;
}
bool MenuState::toggle_preview(std::size_t index) noexcept {
    if(index>=catalog.size() || catalog[index].tab!=selected_)return false;
    enabled_[index]=!enabled_[index];
    return true;
}
bool MenuState::toggle_expanded(std::size_t index) noexcept {
    if(index>=catalog.size() || catalog[index].tab!=selected_)return false;
    dropdown_preview_.reset();
    expanded_[index]=!expanded_[index];
    if(!expanded_[index])settings_offsets_[index]=0;
    return true;
}
std::size_t MenuState::settings_offset(std::size_t index) const noexcept {
    return index<settings_offsets_.size()?settings_offsets_[index]:0;
}
bool MenuState::set_aim_preview_mode(unsigned mode) noexcept {
    if(selected_!=MenuTab::Combat || mode>3 || mode==aim_preview_mode_)
        return false;
    dropdown_preview_.reset();
    aim_preview_mode_=mode;
    settings_offsets_[0]=0;
    return true;
}
bool MenuState::set_target_preview_mode(unsigned mode) noexcept {
    if(selected_!=MenuTab::Combat || mode>1 || mode==target_preview_mode_)
        return false;
    dropdown_preview_.reset();
    target_preview_mode_=mode;
    settings_offsets_[0]=0;
    return true;
}
bool MenuState::change_settings_page(std::size_t index,int direction) noexcept {
    if((direction!=1&&direction!=-1) || index>=catalog.size() ||
       catalog[index].tab!=selected_ || !expanded_[index])return false;
    const std::size_t count=recovered_widget_visible_count(
        catalog[index].name,static_cast<int>(aim_preview_mode_),
        static_cast<int>(target_preview_mode_));
    if(count==0)return false;
    const std::size_t previous=settings_offsets_[index];
    dropdown_preview_.reset();
    // No arithmetic on untrusted or potentially overflowing offsets.
    if(direction<0) {
        if(previous<5)return false;
        settings_offsets_[index]-=5;
    } else {
        if(previous>=count || count-previous<=5)return false;
        settings_offsets_[index]+=5;
    }
    return true;
}
bool MenuState::move_piercing_selection(int direction) noexcept {
    if((direction!=1 && direction!=-1) ||
       selected_!=MenuTab::Combat || !expanded_[11])return false;
    const auto rows=page();
    const auto it=std::find_if(rows.begin(),rows.end(),
        [](const MenuItem& row){return row.catalog_index==11;});
    if(it==rows.end())return false;
    if(direction>0 && piercing_selected_>=7)return false;
    if(direction<0 && piercing_selected_==0)return false;
    piercing_selected_=static_cast<std::size_t>(
        static_cast<int>(piercing_selected_)+direction);
    // Keep the selected original callsite in the bounded 5-row viewport.
    settings_offsets_[11]=(piercing_selected_/5u)*5u;
    return true;
}
bool MenuState::select_piercing_control(std::size_t control) noexcept {
    if(control>7u || selected_!=MenuTab::Combat ||
       !expanded_[11] || control/5u!=settings_offsets_[11]/5u)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
        [](const MenuItem& row){return row.catalog_index==11;}))
        return false;
    piercing_selected_=control;
    return true;
}
bool MenuState::activate_piercing_selection() noexcept {
    if(selected_!=MenuTab::Combat || !expanded_[11])return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
        [](const MenuItem& row){return row.catalog_index==11;}))
        return false;
    if(piercing_selected_>7)return false;
    dropdown_preview_.reset();
    if(piercing_selected_==0)
        piercing_preview_.sort=1u-piercing_preview_.sort;
    else {
        auto& flag=piercing_preview_.enabled[piercing_selected_-1];
        flag=!flag;
    }
    return true;
}
// R127 only exposes the three reference-identified local preview
// dropdowns. Source-conditioned Aim widget indices must still be visible.
bool MenuState::open_preview_dropdown(
        MenuDropdownKind kind,std::size_t control_index) noexcept {
    if(unload_confirmation_preview_ || dropdown_preview_ ||
       selected_!=MenuTab::Combat ||
       (kind!=MenuDropdownKind::AimMode &&
        kind!=MenuDropdownKind::TargetMode &&
        kind!=MenuDropdownKind::PiercingSort))return false;
    const std::size_t index=kind==MenuDropdownKind::PiercingSort?11u:0u;
    if(!expanded_[index] || control_index<settings_offsets_[index] ||
       control_index-settings_offsets_[index]>=5u)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),[index](const MenuItem& row){
        return row.catalog_index==index;}))return false;
    std::array<const RecoveredWidget*,5> widgets{};
    const auto count=recovered_widget_visible_page(
        catalog[index].name,settings_offsets_[index],
        static_cast<int>(aim_preview_mode_),
        static_cast<int>(target_preview_mode_),widgets);
    const auto relative=control_index-settings_offsets_[index];
    if(count==0 || relative>=widgets.size() || !widgets[relative] ||
       widgets[relative]->kind!=RecoveredWidgetKind::Dropdown ||
       std::string_view(widgets[relative]->label)!="Mode" &&
       kind!=MenuDropdownKind::PiercingSort)
        return false;
    const auto group=std::string_view(widgets[relative]->group);
    if(kind==MenuDropdownKind::AimMode &&
       (index!=0u || control_index!=0u || group!="Aim"))
        return false;
    if(kind==MenuDropdownKind::TargetMode &&
       (index!=0u || group!="Target" ||
        std::string_view(widgets[relative]->label)!="Mode"))
        return false;
    if(kind==MenuDropdownKind::PiercingSort &&
       (index!=11u || control_index!=0u || group!="Always hit through" ||
        std::string_view(widgets[relative]->label)!="Sort targets by"))
        return false;
    dropdown_preview_=MenuDropdownPreview{
        kind,index,control_index,settings_offsets_[index]
    };
    return true;
}
bool MenuState::open_recovered_choice_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(dropdown_preview_ || unload_confirmation_preview_ ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       all[widget_index].kind!=RecoveredWidgetKind::Dropdown ||
       std::string_view(all[widget_index].module)!=
           catalog[catalog_index].name ||
       recovered_choice_count(all[widget_index].options)<2u)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [catalog_index](const MenuItem& item){
           return item.catalog_index==catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> pageWidgets{};
    if(!recovered_widget_visible_page(
       catalog[catalog_index].name,settings_offsets_[catalog_index],
       static_cast<int>(aim_preview_mode_),
       static_cast<int>(target_preview_mode_),pageWidgets))
        return false;
    for(std::size_t i=0;i<pageWidgets.size();++i) {
        if(pageWidgets[i]!=&all[widget_index])continue;
        const auto& widget=all[widget_index];
        // Dedicated source callsites retain their existing specialized
        // in-memory states and never alias the generic option store.
        if((catalog_index==0u &&
            std::string_view(widget.label)=="Mode" &&
            (std::string_view(widget.group)=="Aim" ||
             std::string_view(widget.group)=="Target")) ||
           (catalog_index==11u && i==0u &&
            std::string_view(widget.label)=="Sort targets by"))
            return false;
        dropdown_preview_=MenuDropdownPreview{
            MenuDropdownKind::RecoveredChoice,catalog_index,
            settings_offsets_[catalog_index]+i,
            settings_offsets_[catalog_index],widget_index
        };
        return true;
    }
    return false;
}
bool MenuState::open_recovered_multi_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(dropdown_preview_ || unload_confirmation_preview_ ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       all[widget_index].kind!=RecoveredWidgetKind::MultiSelect ||
       std::string_view(all[widget_index].module)!=
           catalog[catalog_index].name ||
       recovered_multi_count(all[widget_index].options)==0u)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [catalog_index](const MenuItem& item){
           return item.catalog_index==catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> pageWidgets{};
    if(!recovered_widget_visible_page(
       catalog[catalog_index].name,settings_offsets_[catalog_index],
       static_cast<int>(aim_preview_mode_),
       static_cast<int>(target_preview_mode_),pageWidgets))
        return false;
    for(std::size_t i=0;i<pageWidgets.size();++i) {
        if(pageWidgets[i]!=&all[widget_index])continue;
        dropdown_preview_=MenuDropdownPreview{
            MenuDropdownKind::RecoveredMulti,catalog_index,
            settings_offsets_[catalog_index]+i,
            settings_offsets_[catalog_index],widget_index,0u
        };
        return true;
    }
    return false;
}
bool MenuState::scroll_preview_popup(int direction) noexcept {
    if((direction!=1 && direction!=-1) || !dropdown_preview_ ||
       unload_confirmation_preview_)return false;
    auto& popup=*dropdown_preview_;
    if(popup.kind!=MenuDropdownKind::RecoveredChoice &&
       popup.kind!=MenuDropdownKind::RecoveredMulti)return false;
    const auto& all=recovered_widget_catalog();
    if(popup.widget_index>=all.size())return false;
    const auto& widget=all[popup.widget_index];
    const auto count=popup.kind==MenuDropdownKind::RecoveredMulti?
        recovered_multi_count(widget.options):
        recovered_choice_count(widget.options);
    constexpr unsigned pageSize=6u;
    if(count<=pageSize || popup.option_offset>count-pageSize)
        return false;
    if(direction<0) {
        if(popup.option_offset==0u)return false;
        --popup.option_offset;
    } else {
        if(popup.option_offset>=count-pageSize)return false;
        ++popup.option_offset;
    }
    return true;
}
std::optional<bool> MenuState::recovered_boolean_override(
        std::size_t widget_index) const noexcept {
    if(widget_index>=recovered_boolean_overrides_.size())return std::nullopt;
    return recovered_boolean_overrides_[widget_index];
}
bool MenuState::toggle_recovered_boolean_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept {
    // This is an independent, inert UI sandbox. Piercing's seven Boolean
    // controls already have their separate, source-indexed editor.
    const auto& catalog=official_module_catalog();
    const auto& widgets=recovered_widget_catalog();
    if(unload_confirmation_preview_ || dropdown_preview_ ||
       catalog_index>=catalog.size() || catalog_index==11u ||
       widget_index>=widgets.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       widgets[widget_index].kind!=RecoveredWidgetKind::Boolean ||
       std::string_view(widgets[widget_index].module)!=
           catalog[catalog_index].name)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [catalog_index](const MenuItem& item){
           return item.catalog_index==catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> visible{};
    if(!recovered_widget_visible_page(
       catalog[catalog_index].name,settings_offsets_[catalog_index],
       static_cast<int>(aim_preview_mode_),
       static_cast<int>(target_preview_mode_),visible))
        return false;
    // Only the five callsites actually drawn on this page are editable.
    bool on_page=false;
    for(const auto* ptr:visible) {
        if(ptr==&widgets[widget_index])on_page=true;
    }
    if(!on_page)return false;
    const bool previous=recovered_boolean_overrides_[widget_index].value_or(
        widgets[widget_index].example_enabled);
    recovered_boolean_overrides_[widget_index]=!previous;
    return true;
}

std::optional<float> MenuState::recovered_slider_override(
        std::size_t widget_index) const noexcept {
    if(widget_index>=recovered_slider_overrides_.size())return std::nullopt;
    return recovered_slider_overrides_[widget_index];
}
bool MenuState::set_recovered_slider_preview(
        std::size_t catalog_index,std::size_t widget_index,
        float requested_value) noexcept {
    // R132 only accepts a source-indexed slider on the rendered page.
    // These numeric fixture ranges are reconstructed, not authenticated
    // gameplay defaults; this function never writes ClientSettings.
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(!std::isfinite(requested_value) || dropdown_preview_ ||
       unload_confirmation_preview_ ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       all[widget_index].kind!=RecoveredWidgetKind::Slider ||
       std::string_view(all[widget_index].module)!=
           catalog[catalog_index].name)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
        [catalog_index](const MenuItem& row){
            return row.catalog_index==catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> pageWidgets{};
    if(!recovered_widget_visible_page(
         catalog[catalog_index].name,settings_offsets_[catalog_index],
         static_cast<int>(aim_preview_mode_),
         static_cast<int>(target_preview_mode_),pageWidgets))
        return false;
    if(std::none_of(pageWidgets.begin(),pageWidgets.end(),
        [&](const RecoveredWidget* widget){
            return widget==&all[widget_index];}))
        return false;
    const auto& widget=all[widget_index];
    const double min=widget.min_value;
    const double max=widget.max_value;
    const double step=widget.step;
    if(!std::isfinite(min) || !std::isfinite(max) ||
       !std::isfinite(step) || max<=min || step<=0.0 ||
       requested_value<min || requested_value>max)
        return false;
    const double steps=std::floor(
        (static_cast<double>(requested_value)-min)/step+0.5);
    const double snapped=std::clamp(min+steps*step,min,max);
    if(!std::isfinite(snapped))return false;
    recovered_slider_overrides_[widget_index]=static_cast<float>(snapped);
    return true;
}

std::optional<RecoveredRangePreview> MenuState::recovered_range_override(
        std::size_t widget_index) const noexcept {
    if(widget_index>=recovered_range_overrides_.size())return std::nullopt;
    return recovered_range_overrides_[widget_index];
}
bool MenuState::set_recovered_range_preview(
        std::size_t catalog_index,std::size_t widget_index,
        unsigned handle,float requested_value) noexcept {
    // The R133 range model has no link to original Minecraft packets,
    // client hooks, injected settings, profiles or native gameplay state.
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(handle>1u || !std::isfinite(requested_value) ||
       dropdown_preview_ || unload_confirmation_preview_ ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       all[widget_index].kind!=RecoveredWidgetKind::Range ||
       std::string_view(all[widget_index].module)!=
           catalog[catalog_index].name)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
        [catalog_index](const MenuItem& row){
            return row.catalog_index==catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> pageWidgets{};
    if(!recovered_widget_visible_page(
         catalog[catalog_index].name,settings_offsets_[catalog_index],
         static_cast<int>(aim_preview_mode_),
         static_cast<int>(target_preview_mode_),pageWidgets))
        return false;
    if(std::none_of(pageWidgets.begin(),pageWidgets.end(),
        [&](const RecoveredWidget* widget){
            return widget==&all[widget_index];}))
        return false;
    const auto& widget=all[widget_index];
    const double min=widget.min_value;
    const double max=widget.max_value;
    const double step=widget.step;
    if(!std::isfinite(min) || !std::isfinite(max) ||
       !std::isfinite(step) || !std::isfinite(widget.example_value) ||
       !std::isfinite(widget.example_other) ||
       max<=min || step<=0.0 ||
       widget.example_value<min || widget.example_value>max ||
       widget.example_other<min || widget.example_other>max ||
       widget.example_value>widget.example_other ||
       requested_value<min || requested_value>max)
        return false;
    const double units=std::floor(
        (static_cast<double>(requested_value)-min)/step+0.5);
    const double snapped=std::clamp(min+units*step,min,max);
    if(!std::isfinite(snapped))return false;
    RecoveredRangePreview next=recovered_range_overrides_[widget_index]
        .value_or(RecoveredRangePreview{
            widget.example_value,widget.example_other});
    if(!std::isfinite(next.lower) || !std::isfinite(next.upper) ||
       next.lower<min || next.upper>max ||
       next.lower>next.upper)return false;
    // The handle chosen at press remains the only editable handle
    // throughout the verified gesture. It cannot cross the other handle.
    if(handle==0u)next.lower=static_cast<float>(
        std::min(snapped,static_cast<double>(next.upper)));
    else next.upper=static_cast<float>(
        std::max(snapped,static_cast<double>(next.lower)));
    recovered_range_overrides_[widget_index]=next;
    return true;
}

bool MenuState::open_unload_confirmation_preview() noexcept {
    if(unload_confirmation_preview_)return false;
    dropdown_preview_.reset();
    unload_confirmation_preview_=true;
    return true;
}
bool MenuState::close_unload_confirmation_preview() noexcept {
    if(!unload_confirmation_preview_)return false;
    unload_confirmation_preview_=false;
    return true;
}
bool MenuState::close_preview_dropdown() noexcept {
    if(!dropdown_preview_)return false;
    dropdown_preview_.reset();
    return true;
}
bool MenuState::select_preview_dropdown_option(unsigned option) noexcept {
    if(!dropdown_preview_)return false;
    const auto popup=*dropdown_preview_;
    if(popup.kind!=MenuDropdownKind::RecoveredChoice &&
       popup.kind!=MenuDropdownKind::RecoveredMulti &&
       selected_!=MenuTab::Combat)return false;
    if((popup.kind==MenuDropdownKind::RecoveredChoice ||
        popup.kind==MenuDropdownKind::RecoveredMulti) &&
       (popup.catalog_index>=catalog.size() ||
        catalog[popup.catalog_index].tab!=selected_))return false;
    if(popup.catalog_index>=catalog.size() ||
       !expanded_[popup.catalog_index] ||
       settings_offsets_[popup.catalog_index]!=popup.page_offset)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),[&](const MenuItem& row){
        return row.catalog_index==popup.catalog_index;}))
        return false;
    std::array<const RecoveredWidget*,5> widgets{};
    const auto count=recovered_widget_visible_page(
        catalog[popup.catalog_index].name,popup.page_offset,
        static_cast<int>(aim_preview_mode_),
        static_cast<int>(target_preview_mode_),widgets);
    if(count==0 || popup.control_index<popup.page_offset ||
       popup.control_index-popup.page_offset>=widgets.size())
        return false;
    const auto* widget=widgets[popup.control_index-popup.page_offset];
    if(!widget)return false;
    if(popup.kind==MenuDropdownKind::RecoveredMulti) {
        if(widget->kind!=RecoveredWidgetKind::MultiSelect)
            return false;
    } else if(widget->kind!=RecoveredWidgetKind::Dropdown)
        return false;
    const unsigned total=popup.kind==MenuDropdownKind::RecoveredMulti?
        recovered_multi_count(widget->options):
        (popup.kind==MenuDropdownKind::RecoveredChoice?
         recovered_choice_count(widget->options):
         (popup.kind==MenuDropdownKind::AimMode?4u:2u));
    if(!total || option>=total ||
       option<popup.option_offset ||
       option>=popup.option_offset+std::min(6u,total))
        return false;
    switch(popup.kind) {
        case MenuDropdownKind::AimMode:
            if(option>=4u || popup.catalog_index!=0u ||
               popup.control_index!=0u ||
               std::string_view(widget->group)!="Aim" ||
               std::string_view(widget->label)!="Mode")
                return false;
            aim_preview_mode_=option;
            settings_offsets_[0]=0;
            break;
        case MenuDropdownKind::TargetMode:
            if(option>=2u || popup.catalog_index!=0u ||
               std::string_view(widget->group)!="Target" ||
               std::string_view(widget->label)!="Mode")
                return false;
            target_preview_mode_=option;
            settings_offsets_[0]=0;
            break;
        case MenuDropdownKind::PiercingSort:
            if(option>=2u || popup.catalog_index!=11u ||
               popup.control_index!=0u)
                return false;
            piercing_preview_.sort=option;
            piercing_selected_=0;
            break;
        case MenuDropdownKind::RecoveredChoice: {
            const auto& all=recovered_widget_catalog();
            if(popup.widget_index>=all.size() ||
               widget!=&all[popup.widget_index] ||
               option>=recovered_choice_count(widget->options))
                return false;
            recovered_choice_overrides_[popup.widget_index]=option;
            break;
        }
        case MenuDropdownKind::RecoveredMulti: {
            const auto& all=recovered_widget_catalog();
            if(popup.widget_index>=all.size() ||
               widget!=&all[popup.widget_index] ||
               option>=recovered_multi_count(widget->options))
                return false;
            const std::uint32_t current=
                recovered_multi_overrides_[popup.widget_index].value_or(0u);
            recovered_multi_overrides_[popup.widget_index]=
                current^(1u<<option);
            // This recovered multiselect stays open for another option.
            // Do not close, rebase, or change any other module's flags.
            return true;
        }
        default:return false;
    }
    dropdown_preview_.reset();
    return true;
}

bool MenuState::adopt_piercing_preview(
        const PiercingPreview& prefs) noexcept {
    if(prefs.sort>1u)return false;
    dropdown_preview_.reset();
    piercing_preview_=prefs;
    piercing_selected_=0;
    settings_offsets_[11]=0;
    return true;
}
bool MenuState::preview_enabled(std::size_t index) const noexcept {
    return index<enabled_.size() && enabled_[index];
}
bool MenuState::expanded(std::size_t index) const noexcept {
    return index<expanded_.size() && expanded_[index];
}
bool MenuState::set_scale(float value) noexcept {
    if(!std::isfinite(value)||value<0.65f||value>1.8f)return false;
    appearance_.scale=value;
    return true;
}
bool MenuState::set_accent(std::uint32_t rgb) noexcept {
    if(rgb>0xffffffu)return false;
    appearance_.accent_rgb=rgb;
    return true;
}
std::vector<MenuItem> MenuState::page(std::size_t maximum) const {
    std::vector<MenuItem> rows;
    if(!is_module_tab(selected_))return rows;
    const auto cap=std::min<std::size_t>(8,maximum);
    rows.reserve(cap);
    std::size_t skipped=0;
    for(std::size_t i=0;i<catalog.size();++i) {
        if(catalog[i].tab!=selected_)continue;
        if(skipped++<scroll_)continue;
        if(rows.size()==cap)break;
        rows.push_back({i,catalog[i].name,enabled_[i],
                        expanded_[i],settings_offsets_[i]});
    }
    return rows;
}
} // namespace slinkywinky
