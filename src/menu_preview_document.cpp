#include "slinkywinky/menu_preview_document.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_slider_aliases.hpp"
#include "prefs_file_io.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace slinkywinky {
namespace {
constexpr std::string_view magic="SWPREVIEW";
constexpr std::uint32_t version=1;
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,
              "preview format requires binary32");
static_assert(static_cast<unsigned>(RecoveredWidgetKind::Button)==7u,
              "update wire schema when widget kinds change");
struct Writer {
    std::string data;
    void u8(unsigned value) {data.push_back(static_cast<char>(value&255u));}
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)u8(value>>(8*i));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)u8(static_cast<unsigned>(value>>(8*i)));
    }
    void real(float v) {
        std::uint32_t bits=0;std::memcpy(&bits,&v,4);u32(bits);
    }
    void text(std::string_view value) {
        u32(static_cast<std::uint32_t>(value.size()));
        data.append(value.data(),value.size());
    }
};
struct Reader {
    std::string_view data;
    std::size_t pos=0;
    unsigned u8() {
        if(pos>=data.size())throw std::runtime_error("truncated byte");
        return static_cast<unsigned char>(data[pos++]);
    }
    bool flag() {
        const auto v=u8();
        if(v>1u)throw std::runtime_error("invalid flag");
        return v!=0;
    }
    std::uint32_t u32() {
        std::uint32_t v=0;
        for(unsigned i=0;i<4;++i)v|=static_cast<std::uint32_t>(u8())<<(8*i);
        return v;
    }
    std::uint64_t u64() {
        std::uint64_t v=0;
        for(unsigned i=0;i<8;++i)v|=static_cast<std::uint64_t>(u8())<<(8*i);
        return v;
    }
    float real() {
        const auto bits=u32();
        float v=0;std::memcpy(&v,&bits,4);return v;
    }
    std::string text(std::size_t max) {
        const auto count=u32();
        if(count>max || count>data.size()-pos)
            throw std::runtime_error("invalid string length");
        std::string value(data.substr(pos,count));
        pos+=count;return value;
    }
};
std::uint64_t fingerprint(std::string_view value) noexcept {
    std::uint64_t h=14695981039346656037ull;
    for(unsigned char c:value) {h^=c;h*=1099511628211ull;}
    return h;
}
bool valid_text(std::string_view value) noexcept {
    if(value.size()>128u)return false;
    while(!value.empty()) {
        const auto c=static_cast<unsigned char>(value.front());
        const std::size_t length=c<128u?1u:c<224u?2u:c<240u?3u:4u;
        if(length>value.size() ||
           !menu_one_printable_utf8(value.substr(0,length)))return false;
        value.remove_prefix(length);
    }
    return true;
}
bool numeric_bounds(const RecoveredWidget& w,float value) noexcept {
    return std::isfinite(w.min_value) && std::isfinite(w.max_value) &&
        std::isfinite(w.step) && w.min_value<w.max_value && w.step>0 &&
        std::isfinite(value) && value>=w.min_value && value<=w.max_value;
}
bool dedicated(const RecoveredWidget& w) noexcept {
    const std::string_view module=w.module,group=w.group,label=w.label;
    return module=="Piercing" || (module=="Aim Assist" &&
        w.kind==RecoveredWidgetKind::Dropdown && label=="Mode" &&
        (group=="Aim" || group=="Target"));
}
bool record_valid(const PreviewWidgetRecord& r) noexcept {
    const auto& catalog=recovered_widget_catalog();
    if(r.widget_index>=catalog.size())return false;
    const auto& w=catalog[r.widget_index];
    if(dedicated(w) || w.kind==RecoveredWidgetKind::Button ||
       r.value.index()!=static_cast<unsigned>(w.kind))return false;
    switch(w.kind) {
    case RecoveredWidgetKind::Boolean:return true;
    case RecoveredWidgetKind::Slider:{
        const float v=std::get<float>(r.value);
        if(!numeric_bounds(w,v))return false;
        const double lo=w.min_value,hi=w.max_value,step=w.step;
        const double units=std::floor((static_cast<double>(v)-lo)/step+0.5);
        return v==static_cast<float>(std::clamp(lo+units*step,lo,hi));
    }
    case RecoveredWidgetKind::Dropdown:{
        const auto count=menu_source_choice_count(r.widget_index);
        return count>=2u && std::get<PreviewChoiceValue>(r.value).ordinal<count;
    }
    case RecoveredWidgetKind::Range:{
        const auto v=std::get<RecoveredRangePreview>(r.value);
        return numeric_bounds(w,v.lower)&&numeric_bounds(w,v.upper)&&v.lower<=v.upper;
    }
    case RecoveredWidgetKind::Color:
        return menu_source_widget_color(r.widget_index).has_value() &&
            std::get<PreviewColorValue>(r.value).rgb<=0xffffffu;
    case RecoveredWidgetKind::MultiSelect:{
        const auto count=menu_source_multi_count(r.widget_index);
        return count>0 && (std::get<PreviewMultiValue>(r.value).mask>>count)==0u;
    }
    case RecoveredWidgetKind::Text:return valid_text(std::get<std::string>(r.value));
    default:return false;
    }
}
void write_value(Writer& w,const PreviewWidgetRecord& r) {
    w.u32(static_cast<std::uint32_t>(r.widget_index));
    w.u8(static_cast<unsigned>(r.value.index()));
    switch(r.value.index()) {
    case 0:w.u8(std::get<bool>(r.value));break;
    case 1:w.real(std::get<float>(r.value));break;
    case 2:w.u32(std::get<PreviewChoiceValue>(r.value).ordinal);break;
    case 3:{
        const auto v=std::get<RecoveredRangePreview>(r.value);
        w.real(v.lower);w.real(v.upper);break;
    }
    case 4:w.u32(std::get<PreviewColorValue>(r.value).rgb);break;
    case 5:w.u32(std::get<PreviewMultiValue>(r.value).mask);break;
    case 6:w.text(std::get<std::string>(r.value));break;
    default:throw std::runtime_error("unknown widget type");
    }
}
PreviewWidgetRecord read_value(Reader& r) {
    PreviewWidgetRecord value;
    value.widget_index=r.u32();
    switch(r.u8()) {
    case 0:value.value=r.flag();break;
    case 1:value.value=r.real();break;
    case 2:value.value=PreviewChoiceValue{r.u32()};break;
    case 3:{
        const float lower=r.real(),upper=r.real();
        value.value=RecoveredRangePreview{lower,upper};break;
    }
    case 4:value.value=PreviewColorValue{r.u32()};break;
    case 5:value.value=PreviewMultiValue{r.u32()};break;
    case 6:value.value=r.text(128);break;
    default:throw std::runtime_error("unknown widget type");
    }
    return value;
}
} // namespace

std::uint64_t menu_preview_catalog_fingerprint() {
    // Fixed ordered metadata, not a cryptographic signature. Includes
    // bounds, examples and conditional visibility, not just display names.
    static const auto result=[] {
        Writer w;
        for(const auto& m:official_module_catalog()) {
            w.text(m.name);w.u8(static_cast<unsigned>(m.tab));
        }
        for(const auto& item:recovered_widget_catalog()) {
            w.text(item.module);w.text(item.group);w.text(item.label);
            w.u8(static_cast<unsigned>(item.kind));
            w.text(item.options);w.text(item.auxiliary);
            w.real(item.min_value);w.real(item.max_value);w.real(item.step);
            w.real(item.example_value);w.real(item.example_other);
            w.u8(item.example_enabled);w.u8(static_cast<unsigned>(item.visibility));
        }
        return fingerprint(w.data);
    }();
    return result;
}
bool menu_preview_document_valid(const MenuPreviewDocument& doc) noexcept {
    if(doc.accent_rgb>0xffffffu || !std::isfinite(doc.scale) ||
       doc.scale<0.65f || doc.scale>1.8f || doc.aim_mode>3 ||
       doc.target_mode>1 || doc.piercing.sort>1 ||
       doc.profiles.empty() || doc.profiles.size()>6 ||
       doc.values.size()>recovered_widget_catalog().size())return false;
    for(std::size_t i=0;i<doc.profiles.size();++i) {
        const auto& p=doc.profiles[i];
        if(!menu_profile_text_valid(p.name,28) || p.name.front()==' ' ||
           p.name.back()==' ' ||
           (i==0 && (p.name!="Default" ||
               p.description!="Contains the default configuration.")) ||
           (!p.description.empty() && !menu_profile_text_valid(p.description,62)) ||
           (!p.bind.empty() && !menu_profile_text_valid(p.bind,8)))return false;
        for(std::size_t j=0;j<i;++j)if(doc.profiles[j].name==p.name)return false;
    }
    std::array<const PreviewWidgetRecord*,385> indexed{};
    for(const auto& r:doc.values) {
        if(!record_valid(r) || indexed[r.widget_index])return false;
        indexed[r.widget_index]=&r;
    }
    // Alias settings must never disagree between their two visual callsites.
    for(const auto& r:doc.values) {
        if(!menu_is_aim_speed_branch(r.widget_index))continue;
        const auto peer=menu_aim_speed_alias_peer(r.widget_index);
        if(!peer || !indexed[*peer] ||
           std::get<float>(r.value)!=std::get<float>(indexed[*peer]->value))return false;
    }
    return true;
}
MenuPreviewDocument capture_local_preview(const MenuState& menu) {
    MenuPreviewDocument d;
    const auto& a=menu.appearance();
    d.compact=a.compact;d.wide=a.wide;d.scale=a.scale;d.accent_rgb=a.accent_rgb;
    d.aim_mode=menu.aim_preview_mode();d.target_mode=menu.target_preview_mode();
    d.piercing=menu.piercing_preview();d.profiles=menu.local_profiles();
    for(std::size_t i=0;i<recovered_widget_catalog().size();++i) {
        if(const auto& v=menu.recovered_boolean_overrides()[i])d.values.push_back({i,*v});
        if(const auto& v=menu.recovered_slider_overrides()[i])d.values.push_back({i,*v});
        if(const auto& v=menu.recovered_choice_overrides()[i])d.values.push_back({i,PreviewChoiceValue{*v}});
        if(const auto& v=menu.recovered_range_overrides()[i])d.values.push_back({i,*v});
        if(const auto& v=menu.recovered_color_overrides()[i])d.values.push_back({i,PreviewColorValue{*v}});
        if(const auto& v=menu.recovered_multi_overrides()[i])d.values.push_back({i,PreviewMultiValue{*v}});
        if(const auto& v=menu.recovered_text_overrides()[i])d.values.push_back({i,*v});
    }
    return d;
}
std::optional<std::string> encode_menu_preview_document(const MenuPreviewDocument& doc) {
    try {
        if(!menu_preview_document_valid(doc))return std::nullopt;
        Writer w;w.data.append(magic.data(),magic.size());w.u32(version);
        w.u64(menu_preview_catalog_fingerprint());
        w.u8(doc.compact);w.u8(doc.wide);w.u32(doc.accent_rgb);w.real(doc.scale);
        w.u8(doc.aim_mode);w.u8(doc.target_mode);w.u8(doc.piercing.sort);
        for(bool flag:doc.piercing.enabled)w.u8(flag);
        w.u32(static_cast<std::uint32_t>(doc.profiles.size()));
        for(const auto& p:doc.profiles) {w.text(p.name);w.text(p.description);w.text(p.bind);}
        // Canonical sparse record order independent of caller order.
        auto records=doc.values;
        std::sort(records.begin(),records.end(),[](const auto& a,const auto& b){
            return a.widget_index<b.widget_index;
        });
        w.u32(static_cast<std::uint32_t>(records.size()));
        for(const auto& r:records)write_value(w,r);
        w.u64(fingerprint(w.data));
        if(w.data.size()>menu_preview_document_limit)return std::nullopt;
        return std::move(w.data);
    }catch(...){return std::nullopt;}
}
std::optional<MenuPreviewDocument> decode_menu_preview_document(std::string_view data) {
    try {
        if(data.size()>menu_preview_document_limit || data.size()<magic.size()+20u ||
           data.substr(0,magic.size())!=magic)return std::nullopt;
        const auto payload=data.substr(0,data.size()-8);
        Reader checksum{data.substr(data.size()-8)};
        if(checksum.u64()!=fingerprint(payload))return std::nullopt;
        Reader r{payload,magic.size()};
        if(r.u32()!=version || r.u64()!=menu_preview_catalog_fingerprint())return std::nullopt;
        MenuPreviewDocument doc;
        doc.compact=r.flag();doc.wide=r.flag();doc.accent_rgb=r.u32();doc.scale=r.real();
        doc.aim_mode=r.u8();doc.target_mode=r.u8();doc.piercing.sort=r.u8();
        for(auto& flag:doc.piercing.enabled)flag=r.flag();
        const auto profiles=r.u32();
        if(!profiles || profiles>6)return std::nullopt;
        doc.profiles.clear();doc.profiles.reserve(profiles);
        for(unsigned i=0;i<profiles;++i)
            doc.profiles.push_back({r.text(28),r.text(62),r.text(8)});
        const auto records=r.u32();
        if(records>recovered_widget_catalog().size())return std::nullopt;
        doc.values.reserve(records);
        for(unsigned i=0;i<records;++i) {
            auto value=read_value(r);
            if(!doc.values.empty() && value.widget_index<=doc.values.back().widget_index)
                return std::nullopt;
            doc.values.push_back(std::move(value));
        }
        if(r.pos!=payload.size() || !menu_preview_document_valid(doc))return std::nullopt;
        return doc;
    }catch(...){return std::nullopt;}
}
bool save_menu_preview_document(const std::filesystem::path& path,
                                const MenuPreviewDocument& doc) noexcept {
    try {
        const auto data=encode_menu_preview_document(doc);
        return data && prefs_file_io::replace(path,*data);
    }catch(...){return false;}
}
std::optional<MenuPreviewDocument> load_menu_preview_document(
    const std::filesystem::path& path) noexcept {
    try {
        std::string data;
        if(!prefs_file_io::read(path,menu_preview_document_limit,data))return std::nullopt;
        return decode_menu_preview_document(data);
    }catch(...){return std::nullopt;}
}
bool MenuState::restore_local_preview(const MenuPreviewDocument& doc) noexcept {
    try {
        if(!menu_preview_document_valid(doc))return false;
        // Stage all allocation and validation before committing. Preserve
        // non-document state: actual profile bridge, enabled flags, active
        // module-bind display values and allow-input policy are not imported.
        MenuState staged=*this;
        staged.appearance_.compact=doc.compact;staged.appearance_.wide=doc.wide;
        staged.appearance_.accent_rgb=doc.accent_rgb;staged.appearance_.scale=doc.scale;
        staged.aim_preview_mode_=doc.aim_mode;staged.target_preview_mode_=doc.target_mode;
        staged.piercing_preview_=doc.piercing;staged.piercing_selected_=0;
        staged.local_profiles_=doc.profiles;
        staged.recovered_boolean_overrides_={};staged.recovered_slider_overrides_={};
        staged.recovered_choice_overrides_={};staged.recovered_range_overrides_={};
        staged.recovered_color_overrides_={};staged.recovered_multi_overrides_={};
        staged.recovered_text_overrides_={};
        for(const auto& r:doc.values) {
            const auto i=r.widget_index;
            switch(r.value.index()) {
            case 0:staged.recovered_boolean_overrides_[i]=std::get<bool>(r.value);break;
            case 1:staged.recovered_slider_overrides_[i]=std::get<float>(r.value);break;
            case 2:staged.recovered_choice_overrides_[i]=std::get<PreviewChoiceValue>(r.value).ordinal;break;
            case 3:staged.recovered_range_overrides_[i]=std::get<RecoveredRangePreview>(r.value);break;
            case 4:staged.recovered_color_overrides_[i]=std::get<PreviewColorValue>(r.value).rgb;break;
            case 5:staged.recovered_multi_overrides_[i]=std::get<PreviewMultiValue>(r.value).mask;break;
            case 6:staged.recovered_text_overrides_[i]=std::get<std::string>(r.value);break;
            default:return false;
            }
        }
        // Cancel transient captures/modal state and reset stale pagination.
        if(!staged.select(staged.selected_))return false;
        staged.settings_offsets_.fill(0);
        static_assert(std::is_nothrow_move_assignable<MenuState>::value,
                      "preview publication must be nonthrowing");
        *this=std::move(staged);
        return true;
    }catch(...){return false;}
}
} // namespace slinkywinky
