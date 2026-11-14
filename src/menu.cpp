#include "slinkywinky/menu.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_slider_aliases.hpp"
#include "slinkywinky/menu_numeric_formats.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>
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
// R200 ABI-preserving owner storage. MenuState and WglRetainedMenuPreview
// retain their exact compiled layout for the Java/native JNI agent.
// Every model destruction erases its popup; map accesses are synchronized.
std::mutex color_sidecar_mutex;
std::unordered_map<const MenuState*,MenuColorPopupPreview>
    color_sidecars;
} // namespace

MenuState::~MenuState() noexcept {
    std::lock_guard<std::mutex> lock(color_sidecar_mutex);
    color_sidecars.erase(this);
}
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
    if(color_popup_owner())close_color_popup(false);
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
    bind_hover_row_.reset();
    unload_confirmation_preview_=false;
    profile_action_preview_.reset();
    profile_icon_hover_.reset();
    profile_bind_capture_index_.reset();
    profile_name_focused_=false;
    profile_name_draft_.clear();
    profile_name_taken_=false;
    selected_=tab;
    scroll_=0;
    return true;
}
bool MenuState::scroll_by(int delta) noexcept {
    if(color_popup_owner())return false;
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
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
    bind_hover_row_.reset();
    scroll_=next;
    return true;
}
bool MenuState::scroll_by_visible(
        int delta,std::size_t visible_limit) noexcept {
    if(color_popup_owner())return false;
    const auto total=tab_count(selected_);
    if(total==0u || visible_limit==0u || visible_limit>8u)
        return false;
    const auto maxScroll=total>visible_limit?
        total-visible_limit:0u;
    const auto proposed=static_cast<std::int64_t>(scroll_)+delta;
    const auto capped=std::max<std::int64_t>(0,
        std::min<std::int64_t>(
            static_cast<std::int64_t>(maxScroll),proposed));
    const auto next=static_cast<std::size_t>(capped);
    if(next==scroll_)return false;
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
    bind_hover_row_.reset();
    scroll_=next;
    return true;
}
bool MenuState::begin_numeric_drag_preview(
        std::size_t module,std::size_t widget_index,
        unsigned handle) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(numeric_drag_preview_ || dropdown_preview_ ||
       unload_confirmation_preview_ || profile_action_preview_ ||
       selected_==MenuTab::Profiles ||
       module>=catalog.size() || widget_index>=all.size() ||
       catalog[module].tab!=selected_ || !expanded_[module] ||
       std::string_view(all[widget_index].module)!=catalog[module].name)
        return false;
    const auto& source=all[widget_index];
    const bool isSlider=source.kind==RecoveredWidgetKind::Slider;
    const bool isRange=source.kind==RecoveredWidgetKind::Range;
    if((!isSlider && !isRange) || (isSlider && handle!=0u) ||
       handle>1u || !std::isfinite(source.min_value) ||
       !std::isfinite(source.max_value) ||
       !std::isfinite(source.step) ||
       source.max_value<=source.min_value || source.step<=0.0f)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [module](const MenuItem& item) {
           return item.catalog_index==module;
       }))return false;
    std::array<const RecoveredWidget*,5> visible{};
    if(!recovered_widget_visible_page(
       catalog[module].name,settings_offsets_[module],
       static_cast<int>(aim_preview_mode_),
       static_cast<int>(target_preview_mode_),visible))
        return false;
    if(std::none_of(visible.begin(),visible.end(),
       [&](const RecoveredWidget* item) {
           return item==&source;
       }))return false;
    const auto current=isSlider?
       RecoveredRangePreview{
           recovered_slider_overrides_[widget_index].value_or(
               source.example_value),0.0f}:
       recovered_range_overrides_[widget_index].value_or(
           RecoveredRangePreview{source.example_value,source.example_other});
    if(!std::isfinite(current.lower) ||
       (isRange && !std::isfinite(current.upper)) ||
       current.lower<source.min_value ||
       current.lower>source.max_value ||
       (isRange && (current.upper<current.lower ||
                    current.upper>source.max_value)))
        return false;
    numeric_drag_preview_=MenuNumericDragPreview{
        module,widget_index,handle,current.lower,current.upper};
    return true;
}
bool MenuState::move_numeric_drag_preview(float requested) noexcept {
    if(!numeric_drag_preview_ || !std::isfinite(requested) ||
       dropdown_preview_ || unload_confirmation_preview_ ||
       profile_action_preview_)return false;
    auto& drag=*numeric_drag_preview_;
    const auto& all=recovered_widget_catalog();
    if(drag.widget_index>=all.size())return false;
    const auto& source=all[drag.widget_index];
    if(!std::isfinite(source.min_value) ||
       !std::isfinite(source.max_value) ||
       !std::isfinite(source.step) ||
       source.max_value<=source.min_value ||
       source.step<=0.0f ||
       requested<source.min_value ||
       requested>source.max_value)return false;
    const double minimum=source.min_value, maximum=source.max_value;
    const double units=std::floor(
        (static_cast<double>(requested)-minimum)/
        static_cast<double>(source.step)+0.5);
    const double snapped=std::clamp(
        minimum+units*static_cast<double>(source.step),
        minimum,maximum);
    if(!std::isfinite(snapped))return false;
    const float result=static_cast<float>(snapped);
    if(source.kind==RecoveredWidgetKind::Slider && drag.handle==0u) {
        if(drag.lower==result)return false;
        drag.lower=result;
        return true;
    }
    if(source.kind!=RecoveredWidgetKind::Range ||
       drag.handle>1u)return false;
    if(drag.handle==0u) {
        const float next=std::min(result,drag.upper);
        if(drag.lower==next)return false;
        drag.lower=next;
    } else {
        const float next=std::max(result,drag.lower);
        if(drag.upper==next)return false;
        drag.upper=next;
    }
    return true;
}
bool MenuState::cancel_numeric_drag_preview() noexcept {
    if(!numeric_drag_preview_)return false;
    numeric_drag_preview_.reset();
    return true;
}
bool MenuState::set_numeric_hover(
        std::optional<MenuNumericHover> hover) noexcept {
    if(hover) {
        const auto& catalog=official_module_catalog();
        const auto& widgets=recovered_widget_catalog();
        const auto module=hover->catalog_index;
        const auto index=hover->widget_index;
        if(dropdown_preview_ || unload_confirmation_preview_ ||
           profile_action_preview_ || selected_==MenuTab::Profiles ||
           module>=catalog.size() || index>=widgets.size() ||
           catalog[module].tab!=selected_ || !expanded_[module] ||
           (widgets[index].kind!=RecoveredWidgetKind::Slider &&
            widgets[index].kind!=RecoveredWidgetKind::Range) ||
           std::string_view(widgets[index].module)!=catalog[module].name ||
           menu_numeric_format_for_widget(index).empty())
            return false;
        const auto rows=page();
        if(std::none_of(rows.begin(),rows.end(),
            [module](const MenuItem& row) {
                return row.catalog_index==module;
            }))return false;
        std::array<const RecoveredWidget*,5> visible{};
        if(!recovered_widget_visible_page(
            catalog[module].name,settings_offsets_[module],
            static_cast<int>(aim_preview_mode_),
            static_cast<int>(target_preview_mode_),visible))
            return false;
        if(std::none_of(visible.begin(),visible.end(),
            [&](const RecoveredWidget* ptr) {
                return ptr==&widgets[index];
            }))return false;
    }
    if(numeric_hover_==hover)return false;
    numeric_hover_=hover;
    return true;
}
bool MenuState::set_bind_hover_row(
        std::optional<std::size_t> catalog_index) noexcept {
    if(catalog_index) {
        const auto& catalog=official_module_catalog();
        const std::size_t index=*catalog_index;
        if(dropdown_preview_ || unload_confirmation_preview_ ||
           index>=catalog.size() ||
           catalog[index].tab!=selected_ ||
           !menu_bind_supported_module(index))
            return false;
        const auto visible=page();
        if(std::none_of(visible.begin(),visible.end(),
            [index](const MenuItem& row) {
                return row.catalog_index==index;
            }))
            return false;
    }
    if(bind_hover_row_==catalog_index)return false;
    bind_hover_row_=catalog_index;
    return true;
}
namespace {
bool profile_name_byte_safe(char c) noexcept {
    const unsigned char value=static_cast<unsigned char>(c);
    return value>=32u && value<=126u && c!='/' && c!='\\';
}
std::string trim_profile_name(std::string_view name) {
    const auto first=name.find_first_not_of(' ');
    if(first==std::string_view::npos)return {};
    const auto last=name.find_last_not_of(' ');
    return std::string(name.substr(first,last-first+1u));
}
}
bool MenuState::focus_profile_name_preview() noexcept {
    if(selected_!=MenuTab::Profiles || unload_confirmation_preview_ ||
       dropdown_preview_ || profile_action_preview_ ||
       profile_bind_capture_index_ ||
       local_profiles_.size()>=6u)return false;
    if(profile_name_focused_)return false;
    profile_name_focused_=true;
    bind_capture_index_.reset();
    profile_bind_capture_index_.reset();
    return true;
}
bool MenuState::blur_profile_name_preview() noexcept {
    if(!profile_name_focused_)return false;
    profile_name_focused_=false;
    return true;
}
bool MenuState::append_profile_name_preview(std::string_view text) {
    if(!profile_name_focused_ || selected_!=MenuTab::Profiles ||
       profile_action_preview_ || unload_confirmation_preview_ ||
       text.size()!=1u || !profile_name_byte_safe(text.front()) ||
       profile_name_draft_.size()>=28u)return false;
    profile_name_draft_.append(text.data(),text.size());
    profile_name_taken_=false;
    return true;
}
bool MenuState::erase_profile_name_preview() noexcept {
    if(!profile_name_focused_ || selected_!=MenuTab::Profiles ||
       profile_action_preview_ || profile_name_draft_.empty())
        return false;
    profile_name_draft_.pop_back();
    profile_name_taken_=false;
    return true;
}
bool MenuState::create_profile_preview() {
    if(selected_!=MenuTab::Profiles || unload_confirmation_preview_ ||
       dropdown_preview_ || profile_action_preview_ ||
       profile_bind_capture_index_ ||
       local_profiles_.size()>=6u)return false;
    const auto name=trim_profile_name(profile_name_draft_);
    if(name.empty() || name.size()>28u)return false;
    for(const char c:name)
        if(!profile_name_byte_safe(c))return false;
    if(std::any_of(local_profiles_.begin(),local_profiles_.end(),
       [&](const MenuLocalProfile& profile) {
           return profile.name==name;
       })) {
        profile_name_draft_.clear();
        profile_name_taken_=true;
        profile_name_focused_=true;
        return false;
    }
    local_profiles_.push_back({name,"",""});
    profile_name_draft_.clear();
    profile_name_focused_=false;
    profile_name_taken_=false;
    return true;
}
bool MenuState::set_profile_icon_hover(
        std::optional<MenuProfileIconHover> hover) noexcept {
    if(hover) {
        if(selected_!=MenuTab::Profiles || dropdown_preview_ ||
           unload_confirmation_preview_ || profile_action_preview_ ||
           hover->profile_index>=local_profiles_.size() ||
           hover->action>3u)return false;
    }
    if(profile_icon_hover_==hover)return false;
    profile_icon_hover_=hover;
    return true;
}
bool MenuState::begin_profile_bind_capture(
        std::size_t index) noexcept {
    if(selected_!=MenuTab::Profiles || unload_confirmation_preview_ ||
       dropdown_preview_ || profile_action_preview_ ||
       profile_name_focused_ || profile_bind_capture_index_ ||
       index>=local_profiles_.size())return false;
    profile_bind_capture_index_=index;
    return true;
}
bool MenuState::commit_profile_bind_capture(std::uint16_t code) {
    if(selected_!=MenuTab::Profiles || unload_confirmation_preview_ ||
       dropdown_preview_ || profile_action_preview_ ||
       profile_name_focused_ || !profile_bind_capture_index_ ||
       !menu_valid_bind_code(code))return false;
    const auto index=*profile_bind_capture_index_;
    if(index>=local_profiles_.size())return false;
    const auto display=menu_bind_code_label(code);
    if(code && display.size()>8u)return false;
    local_profiles_[index].bind=code==0u?std::string{}:display;
    profile_bind_capture_index_.reset();
    return true;
}
bool MenuState::cancel_profile_bind_capture() noexcept {
    if(!profile_bind_capture_index_)return false;
    profile_bind_capture_index_.reset();
    return true;
}
bool MenuState::begin_profile_action_preview(
        std::size_t profile_index,MenuProfileAction action) noexcept {
    if(selected_!=MenuTab::Profiles || unload_confirmation_preview_ ||
       dropdown_preview_ || profile_action_preview_ ||
       profile_bind_capture_index_ ||
       profile_index>=local_profiles_.size() ||
       static_cast<unsigned>(action)>
           static_cast<unsigned>(MenuProfileAction::Delete) ||
       (profile_index==0u &&
           (action==MenuProfileAction::Save ||
            action==MenuProfileAction::Delete)))
        return false;
    profile_name_focused_=false;
    profile_bind_capture_index_.reset();
    profile_icon_hover_.reset();
    profile_action_preview_=MenuProfileActionPreview{profile_index,action};
    return true;
}
bool MenuState::cancel_profile_action_preview() noexcept {
    if(!profile_action_preview_)return false;
    profile_action_preview_.reset();
    return true;
}
bool MenuState::confirm_profile_action_preview() noexcept {
    if(selected_!=MenuTab::Profiles || !profile_action_preview_)return false;
    const auto state=*profile_action_preview_;
    if(state.profile_index>=local_profiles_.size() ||
       (state.profile_index==0u &&
        (state.action==MenuProfileAction::Save ||
         state.action==MenuProfileAction::Delete)))return false;
    profile_action_preview_.reset();
    if(state.action==MenuProfileAction::Delete) {
        // A locally created, unsaved preview card is removed only after
        // confirmation. The original game/OS filesystem is untouched.
        profile_icon_hover_.reset();
        local_profiles_.erase(local_profiles_.begin()+
                              static_cast<std::ptrdiff_t>(state.profile_index));
    }
    // Load, Save and OpenFolder deliberately do no filesystem/runtime work:
    // the supplied HTML reference uses empty confirmation callbacks.
    return true;
}

bool MenuState::begin_bind_capture(
        std::size_t catalog_index) noexcept {
    const auto& catalog=official_module_catalog();
    if(bind_capture_index_ || dropdown_preview_ ||
       unload_confirmation_preview_ || focused_text_widget_ ||
       catalog_index>=catalog.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !menu_bind_supported_module(catalog_index))
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [catalog_index](const MenuItem& row) {
           return row.catalog_index==catalog_index;
       }))
        return false;
    bind_capture_index_=catalog_index;
    return true;
}
bool MenuState::commit_bind_capture(std::uint16_t code) noexcept {
    if(!bind_capture_index_ || !menu_valid_bind_code(code) ||
       dropdown_preview_ || unload_confirmation_preview_ ||
       focused_text_widget_)return false;
    const std::size_t index=*bind_capture_index_;
    const auto& catalog=official_module_catalog();
    if(index>=catalog.size() ||
       catalog[index].tab!=selected_ ||
       !menu_bind_supported_module(index))return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
        [index](const MenuItem& row){
            return row.catalog_index==index;
        }))
        return false;
    bind_previews_[index].code=code;
    if(code==0u)bind_previews_[index].hold=false;
    bind_capture_index_.reset();
    return true;
}
bool MenuState::cancel_bind_capture() noexcept {
    if(!bind_capture_index_)return false;
    bind_capture_index_.reset();
    return true;
}
bool MenuState::toggle_bind_hold(
        std::size_t catalog_index) noexcept {
    const auto& catalog=official_module_catalog();
    if(bind_capture_index_ || dropdown_preview_ ||
       unload_confirmation_preview_ || focused_text_widget_ ||
       catalog_index>=catalog.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !menu_bind_supported_module(catalog_index) ||
       bind_previews_[catalog_index].code==0u)
        return false;
    const auto rows=page();
    if(std::none_of(rows.begin(),rows.end(),
       [catalog_index](const MenuItem& row){
           return row.catalog_index==catalog_index;
       }))
        return false;
    bind_previews_[catalog_index].hold=
        !bind_previews_[catalog_index].hold;
    return true;
}

bool MenuState::toggle_preview(std::size_t index) noexcept {
    if(color_popup_owner() || index>=catalog.size() || catalog[index].tab!=selected_)return false;
    enabled_[index]=!enabled_[index];
    return true;
}
bool MenuState::toggle_expanded(std::size_t index) noexcept {
    if(color_popup_owner() || index>=catalog.size() || catalog[index].tab!=selected_)return false;
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
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
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    aim_preview_mode_=mode;
    settings_offsets_[0]=0;
    return true;
}
bool MenuState::set_target_preview_mode(unsigned mode) noexcept {
    if(selected_!=MenuTab::Combat || mode>1 || mode==target_preview_mode_)
        return false;
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    target_preview_mode_=mode;
    settings_offsets_[0]=0;
    return true;
}
bool MenuState::change_settings_page(std::size_t index,int direction) noexcept {
    if(color_popup_owner() || (direction!=1&&direction!=-1) || index>=catalog.size() ||
       catalog[index].tab!=selected_ || !expanded_[index])return false;
    const std::size_t count=recovered_widget_visible_count(
        catalog[index].name,static_cast<int>(aim_preview_mode_),
        static_cast<int>(target_preview_mode_));
    if(count==0)return false;
    const std::size_t previous=settings_offsets_[index];
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
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
       (std::string_view(widgets[relative]->label)!="Mode" &&
        kind!=MenuDropdownKind::PiercingSort))
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
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
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
        focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
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
        focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
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
bool MenuState::set_recovered_boolean_preview(
        std::size_t catalog_index,std::size_t widget_index,
        bool desired) noexcept {
    // This setter is local GUI state only. Piercing has its own editor.
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
    recovered_boolean_overrides_[widget_index]=desired;
    return true;
}
bool MenuState::toggle_recovered_boolean_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept {
    const auto& source=recovered_widget_catalog();
    if(widget_index>=source.size())return false;
    const bool previous=recovered_boolean_overrides_[widget_index].value_or(
        source[widget_index].example_enabled);
    return set_recovered_boolean_preview(catalog_index,widget_index,!previous);
}

bool MenuState::focus_recovered_text_preview(
        std::size_t catalog_index,std::size_t widget_index) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(dropdown_preview_ || unload_confirmation_preview_ ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       all[widget_index].kind!=RecoveredWidgetKind::Text ||
       std::string_view(all[widget_index].module)!=
           catalog[catalog_index].name)
        return false;
    const auto visible=page();
    if(std::none_of(visible.begin(),visible.end(),
        [catalog_index](const MenuItem& item){
            return item.catalog_index==catalog_index;}))
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
    bind_capture_index_.reset();
    focused_text_widget_=widget_index;
    return true;
}
bool MenuState::blur_recovered_text_preview() noexcept {
    if(!focused_text_widget_)return false;
    focused_text_widget_.reset();
    return true;
}
bool MenuState::append_recovered_text_preview(
        std::string_view unicode_scalar) {
    if(!focused_text_widget_ ||
       !menu_one_printable_utf8(unicode_scalar))return false;
    const auto widgetIndex=*focused_text_widget_;
    const auto& widget=recovered_widget_catalog()[widgetIndex];
    const auto& catalog=official_module_catalog();
    std::size_t moduleIndex=catalog.size();
    for(std::size_t i=0;i<catalog.size();++i)
        if(std::string_view(widget.module)==catalog[i].name)
            {moduleIndex=i;break;}
    if(moduleIndex>=catalog.size() ||
       !focus_recovered_text_preview(moduleIndex,widgetIndex))
        return false;
    auto& value=recovered_text_overrides_[widgetIndex];
    const std::size_t used=value?value->size():0u;
    if(used+unicode_scalar.size()>128u)return false;
    if(!value)value.emplace();
    value->append(unicode_scalar.data(),unicode_scalar.size());
    return true;
}
bool MenuState::erase_recovered_text_preview() noexcept {
    if(!focused_text_widget_)return false;
    const auto widgetIndex=*focused_text_widget_;
    const auto& widget=recovered_widget_catalog()[widgetIndex];
    const auto& catalog=official_module_catalog();
    std::size_t moduleIndex=catalog.size();
    for(std::size_t i=0;i<catalog.size();++i)
        if(std::string_view(widget.module)==catalog[i].name)
            {moduleIndex=i;break;}
    if(moduleIndex>=catalog.size() ||
       !focus_recovered_text_preview(moduleIndex,widgetIndex))
        return false;
    auto& value=recovered_text_overrides_[widgetIndex];
    if(!value || value->empty())return false;
    std::size_t cut=value->size()-1u;
    while(cut>0u &&
         (static_cast<unsigned char>((*value)[cut])&0xC0u)==0x80u)
        --cut;
    value->resize(cut);
    return true;
}

namespace {
// Standalone read-only admission for the same source-visible color swatch
// that the existing R136 setter already validates. Never mutates a setting.
bool color_popup_target_visible(const MenuState& menu,
                                std::size_t module,std::size_t widget) noexcept {
    const auto& modules=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(module>=modules.size() || widget>=all.size() ||
       modules[module].tab!=menu.tab() || !menu.expanded(module) ||
       all[widget].kind!=RecoveredWidgetKind::Color ||
       std::string_view(all[widget].module)!=modules[module].name ||
       !menu_source_widget_color(widget))return false;
    const auto rows=menu.page();
    if(std::none_of(rows.begin(),rows.end(),[module](const MenuItem& row){
           return row.catalog_index==module;}))return false;
    std::array<const RecoveredWidget*,5> current{};
    if(!recovered_widget_visible_page(
        modules[module].name,menu.settings_offset(module),
        static_cast<int>(menu.aim_preview_mode()),
        static_cast<int>(menu.target_preview_mode()),current))
        return false;
    return std::find(current.begin(),current.end(),&all[widget])!=
        current.end();
}
}
std::optional<MenuColorPopupOwner> MenuState::color_popup_owner()
        const noexcept {
    if(!dropdown_preview_ ||
       dropdown_preview_->kind!=MenuDropdownKind::ColorPicker)
        return std::nullopt;
    return MenuColorPopupOwner{dropdown_preview_->catalog_index,
                               dropdown_preview_->widget_index};
}
std::optional<MenuColorPopupPreview> MenuState::color_popup_model() const {
    if(!color_popup_owner())return std::nullopt;
    std::lock_guard<std::mutex> lock(color_sidecar_mutex);
    const auto it=color_sidecars.find(this);
    if(it==color_sidecars.end() || !it->second.active())
        return std::nullopt;
    return it->second;
}
bool MenuState::open_recovered_color_popup(
        std::size_t module,std::size_t widget) noexcept {
    if(dropdown_preview_ || unload_confirmation_preview_ ||
       profile_action_preview_ || bind_capture_index_ ||
       numeric_drag_preview_ ||
       !color_popup_target_visible(*this,module,widget))
        return false;
    const auto value=recovered_color_overrides_[widget].value_or(
        *menu_source_widget_color(widget));
    {
        std::lock_guard<std::mutex> lock(color_sidecar_mutex);
        auto& picker=color_sidecars[this];
        if(!picker.open(value))return false;
    }
    focused_text_widget_.reset();
    numeric_hover_.reset();
    bind_hover_row_.reset();
    // Reuse existing modal descriptor; no MenuState, MenuRenderFrame or
    // WglRetainedMenuPreview member/ABI changes are permitted.
    dropdown_preview_=MenuDropdownPreview{
        MenuDropdownKind::ColorPicker,module,0u,
        settings_offsets_[module],widget,0u};
    return true;
}
bool MenuState::set_color_popup_hsl_channel(
        unsigned channel,float ratio) noexcept {
    if(!color_popup_owner() || channel>=3u ||
       !std::isfinite(ratio) || ratio<0.0f || ratio>1.0f)
        return false;
    std::lock_guard<std::mutex> lock(color_sidecar_mutex);
    const auto it=color_sidecars.find(this);
    if(it==color_sidecars.end() || !it->second.active())return false;
    auto values=it->second.hsl();
    if(!values)return false;
    if(channel==0u)values->hue=ratio;
    else if(channel==1u)values->saturation=ratio;
    else values->lightness=ratio;
    return it->second.set_hsl(*values);
}
bool MenuState::select_color_popup_recent(unsigned index) noexcept {
    if(!color_popup_owner())return false;
    std::lock_guard<std::mutex> lock(color_sidecar_mutex);
    const auto it=color_sidecars.find(this);
    return it!=color_sidecars.end() &&
           it->second.select_recent(index);
}
bool MenuState::close_color_popup(bool apply) noexcept {
    const auto owner=color_popup_owner();
    if(!owner)return false;
    std::optional<std::uint32_t> result;
    {
        std::lock_guard<std::mutex> lock(color_sidecar_mutex);
        const auto it=color_sidecars.find(this);
        if(it==color_sidecars.end() || !it->second.active())return false;
        result=it->second.close(apply);
    }
    dropdown_preview_.reset();
    // Cancelling remembers the color but never commits a module setting.
    if(!apply)return true;
    return result && set_recovered_color_preview(
        owner->catalog_index,owner->widget_index,*result);
}

std::optional<std::uint32_t> MenuState::recovered_color_override(
        std::size_t widget_index) const noexcept {
    if(widget_index>=recovered_color_overrides_.size())return std::nullopt;
    return recovered_color_overrides_[widget_index];
}
bool MenuState::set_recovered_color_preview(
        std::size_t catalog_index,std::size_t widget_index,
        std::uint32_t rgb) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& widgets=recovered_widget_catalog();
    if(rgb>0xFFFFFFu || dropdown_preview_ ||
       unload_confirmation_preview_ ||
       catalog_index>=catalog.size() ||
       widget_index>=widgets.size() ||
       catalog[catalog_index].tab!=selected_ ||
       !expanded_[catalog_index] ||
       widgets[widget_index].kind!=RecoveredWidgetKind::Color ||
       std::string_view(widgets[widget_index].module)!=
           catalog[catalog_index].name ||
       !menu_source_widget_color(widget_index))
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
        [&](const RecoveredWidget* ptr){
            return ptr==&widgets[widget_index];}))
        return false;
    // Independent GUI fixture state, never overwrites MenuAppearance's
    // global accent or a gameplay module's ClientSettings.
    recovered_color_overrides_[widget_index]=rgb;
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
    // The native candidate's Aim Assist registration contains one
    // Horizontal speed and one Vertical speed parameter. The two
    // conditional display branches for each are aliases, not independent
    // values. Admit the visible widget first, then atomically mirror
    // only into its verified opposite-branch partner.
    const bool aliasCandidate=menu_is_aim_speed_branch(widget_index);
    const auto peer=menu_aim_speed_alias_peer(widget_index);
    if(aliasCandidate && !peer)return false;
    recovered_slider_overrides_[widget_index]=static_cast<float>(snapped);
    if(peer)
        recovered_slider_overrides_[*peer]=static_cast<float>(snapped);
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
    if(unload_confirmation_preview_ || color_popup_owner())return false;
    dropdown_preview_.reset();
    focused_text_widget_.reset();
    numeric_hover_.reset();
    numeric_drag_preview_.reset();
    bind_capture_index_.reset();
    bind_hover_row_.reset();
    profile_action_preview_.reset();
    profile_icon_hover_.reset();
    profile_bind_capture_index_.reset();
    profile_name_focused_=false;
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
    if(dropdown_preview_->kind==MenuDropdownKind::ColorPicker)
        return close_color_popup(false);
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
