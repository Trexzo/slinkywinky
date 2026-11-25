#include "slinkywinky/menu_preview_document.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
using namespace slinkywinky;
namespace fs=std::filesystem;
#define CHECK(x) do {if(!(x))throw std::runtime_error("R324 line "+std::to_string(__LINE__)+": " #x);}while(false)
namespace {
struct Scratch {
    fs::path path;
    Scratch() {
        std::random_device random;
        for(unsigned i=0;i<32;++i) {
            auto candidate=fs::temp_directory_path()/("slinkywinky-r324-"+std::to_string(random()));
            if(fs::create_directory(candidate)){path=std::move(candidate);return;}
        }
        throw std::runtime_error("test directory");
    }
    ~Scratch(){std::error_code ignored;fs::remove_all(path,ignored);}
};
MenuPreviewDocument full_fixture() {
    MenuPreviewDocument d;
    d.compact=d.wide=true;d.accent_rgb=0x123456;d.scale=1.25f;
    d.aim_mode=3;d.target_mode=1;d.piercing.sort=1;d.piercing.enabled[0]=true;
    d.profiles.push_back({"Preview","Local test card","F6"});
    d.profiles.push_back({"Second","",""});
    const auto& all=recovered_widget_catalog();
    for(std::size_t i=0;i<all.size();++i) {
        const auto& w=all[i];
        const std::string_view module=w.module,group=w.group,label=w.label;
        if(module=="Piercing" || w.kind==RecoveredWidgetKind::Button ||
           (module=="Aim Assist" && w.kind==RecoveredWidgetKind::Dropdown &&
             label=="Mode" && (group=="Aim" || group=="Target")))continue;
        switch(w.kind) {
        case RecoveredWidgetKind::Boolean:d.values.push_back({i,false});break;
        case RecoveredWidgetKind::Slider:d.values.push_back({i,w.min_value});break;
        case RecoveredWidgetKind::Dropdown:d.values.push_back({i,PreviewChoiceValue{0}});break;
        case RecoveredWidgetKind::Range:d.values.push_back({i,RecoveredRangePreview{w.min_value,w.max_value}});break;
        case RecoveredWidgetKind::Color:d.values.push_back({i,PreviewColorValue{0xabcdef}});break;
        case RecoveredWidgetKind::MultiSelect:d.values.push_back({i,PreviewMultiValue{0}});break;
        case RecoveredWidgetKind::Text:d.values.push_back({i,std::string(u8"Test \u00e5 \U0001f600")});break;
        default:throw std::runtime_error("unexpected fixture kind");
        }
    }
    return d;
}
std::uint64_t checksum(std::string_view data) {
    std::uint64_t h=14695981039346656037ull;
    for(unsigned char c:data){h^=c;h*=1099511628211ull;}return h;
}
void recalculate(std::string& data) {
    const auto value=checksum(std::string_view(data).substr(0,data.size()-8));
    for(unsigned i=0;i<8;++i)data[data.size()-8+i]=static_cast<char>(value>>(8*i));
}
}
int main(){
 try {
    MenuState menu;
    const auto initial=encode_menu_preview_document(capture_local_preview(menu));
    CHECK(initial && decode_menu_preview_document(*initial));
    CHECK(menu_preview_catalog_fingerprint()!=0u);
    auto d=full_fixture();
    CHECK(d.values.size()==373u && menu_preview_document_valid(d));
    const auto encoded=encode_menu_preview_document(d);
    CHECK(encoded && encoded->size()<menu_preview_document_limit);
    auto parsed=decode_menu_preview_document(*encoded);
    CHECK(parsed && parsed->values.size()==373u && parsed->profiles.size()==3u);
    CHECK(encode_menu_preview_document(*parsed)==encoded);
    std::reverse(d.values.begin(),d.values.end());
    CHECK(encode_menu_preview_document(d)==encoded);
    // Import never changes runtime-model profile state, module preview enable,
    // allow-input policy or existing module-bind display values.
    MenuProfileBridge profile(menu);
    CHECK(menu.toggle_preview(0));
    menu.set_allow_input(true);
    const auto binds=menu.bind_previews();
    CHECK(menu.toggle_expanded(0));
    CHECK(menu.open_preview_dropdown(MenuDropdownKind::AimMode,0));
    CHECK(menu.restore_local_preview(*parsed));
    CHECK(!menu.dropdown_preview() && !menu.numeric_drag_preview());
    CHECK(menu.preview_enabled(0) && !profile.enabled(0));
    CHECK(menu.appearance().allow_input_while_open && menu.bind_previews()==binds);
    CHECK(menu.aim_preview_mode()==3 && menu.target_preview_mode()==1);
    CHECK(encode_menu_preview_document(capture_local_preview(menu))==encoded);
    CHECK(menu.select(MenuTab::Profiles));
    CHECK(menu.local_profiles().size()==3 && menu.focus_profile_name_preview());
    CHECK(menu.append_profile_name_preview("X"));
    CHECK(menu.restore_local_preview(*parsed));
    CHECK(!menu.profile_name_focused() && menu.profile_name_draft().empty());
    // A malformed import never partially changes the visible state.
    auto reject=[&](const MenuPreviewDocument& invalid){
        CHECK(!menu_preview_document_valid(invalid));
        CHECK(!encode_menu_preview_document(invalid));
        CHECK(!menu.restore_local_preview(invalid));
        CHECK(encode_menu_preview_document(capture_local_preview(menu))==encoded);
    };
    {auto x=*parsed;x.scale=std::numeric_limits<float>::quiet_NaN();reject(x);}
    {auto x=*parsed;x.accent_rgb=0x1000000u;reject(x);}
    {auto x=*parsed;x.target_mode=2;reject(x);}
    {auto x=*parsed;x.profiles[1].name="Default";reject(x);}
    {auto x=*parsed;x.profiles[1].name=" trailing ";reject(x);}
    {auto x=*parsed;x.profiles[0].description="wrong";reject(x);}
    {auto x=*parsed;x.values.push_back(x.values.front());reject(x);}
    {auto x=*parsed;x.values[0].widget_index=385;reject(x);}
    {auto x=*parsed;x.values[0].value=PreviewColorValue{0};reject(x);}
    {auto x=*parsed;x.values.push_back({0,PreviewChoiceValue{0}});reject(x);}
    const auto reject_type=[&](std::size_t index,PreviewWidgetValue value){
        auto x=*parsed;bool found=false;
        for(auto& r:x.values)if(r.value.index()==index){r.value=value;found=true;break;}
        CHECK(found);reject(x);
    };
    reject_type(1,std::numeric_limits<float>::infinity());
    reject_type(2,PreviewChoiceValue{99});
    reject_type(3,RecoveredRangePreview{10,-10});
    reject_type(4,PreviewColorValue{0x1000000u});
    reject_type(5,PreviewMultiValue{0xffffffffu});
    reject_type(6,std::string(129,'x'));
    reject_type(6,std::string("\xc0\xaf",2));
    reject_type(6,std::string("a\0b",3));
    {auto x=*parsed;x.values.erase(std::remove_if(x.values.begin(),x.values.end(),
        [](const auto& r){return r.widget_index==4u;}),x.values.end());reject(x);}
    // Every persistable callsite must reject an out-of-range ID, a
    // duplicate record, and a deliberately mismatched value type.
    for(std::size_t i=0;i<parsed->values.size();++i) {
        auto x=*parsed;
        x.values[i].widget_index=385;
        CHECK(!menu_preview_document_valid(x));
        x=*parsed;
        x.values[i].value=x.values[i].value.index()==0?
            PreviewWidgetValue{std::string("wrong type")}:PreviewWidgetValue{false};
        CHECK(!menu_preview_document_valid(x));
        x=*parsed;
        x.values[i]=x.values[(i+1)%x.values.size()];
        CHECK(!menu_preview_document_valid(x));
    }
    // All eight dedicated mode combinations must preserve the entire
    // offline document across capture/restore and round-trip.
    for(unsigned a=0;a<4;++a)for(unsigned t=0;t<2;++t) {
        auto x=*parsed;x.aim_mode=a;x.target_mode=t;
        const auto data=encode_menu_preview_document(x);
        CHECK(data && menu.restore_local_preview(x));
        CHECK(encode_menu_preview_document(capture_local_preview(menu))==data);
        CHECK(decode_menu_preview_document(*data).has_value());
    }
    CHECK(menu.restore_local_preview(*parsed));
    // Exhaustively reject all truncated prefixes and single-byte flips
    // at representative positions. Re-sign forged version/catalog data
    // to exercise semantic guards rather than only the checksum.
    for(std::size_t n=0;n<encoded->size();++n)
        CHECK(!decode_menu_preview_document(std::string_view(*encoded).substr(0,n)));
    for(std::size_t i=0;i<encoded->size();i+=17) {
        auto x=*encoded;x[i]^=1;CHECK(!decode_menu_preview_document(x));
    }
    {auto x=*encoded;x[9]^=1;recalculate(x);CHECK(!decode_menu_preview_document(x));}
    {auto x=*encoded;x[13]^=1;recalculate(x);CHECK(!decode_menu_preview_document(x));}
    {auto x=*encoded;x[21]=2;recalculate(x);CHECK(!decode_menu_preview_document(x));}
    CHECK(!decode_menu_preview_document(std::string(menu_preview_document_limit+1,'x')));
    // Recalculate the non-cryptographic checksum after mutations, so
    // the decoder's structural checks are reached instead of bypassed.
    // Some mutations are legitimate other values; any admitted document
    // must round-trip canonically and remain valid under restoration.
    std::mt19937 rng(325);
    unsigned accepted=0,rejected=0;
    for(unsigned trial=0;trial<2048;++trial) {
        auto x=*encoded;
        const std::size_t pos=21u+(rng()%(x.size()-29u));
        x[pos]=static_cast<char>(static_cast<unsigned char>(x[pos])^
            static_cast<unsigned char>(1u+(rng()%255u)));
        recalculate(x);
        const auto decoded=decode_menu_preview_document(x);
        if(!decoded){++rejected;continue;}
        ++accepted;
        CHECK(menu_preview_document_valid(*decoded));
        const auto canonical=encode_menu_preview_document(*decoded);
        CHECK(canonical && decode_menu_preview_document(*canonical));
        CHECK(menu.restore_local_preview(*decoded));
        CHECK(encode_menu_preview_document(capture_local_preview(menu))==canonical);
    }
    CHECK(accepted>0 && rejected>0 && accepted+rejected==2048u);
    CHECK(menu.restore_local_preview(*parsed));
    std::cout<<"R325_RECHECKSUMMED_MUTATIONS_ACCEPTED="<<accepted
             <<" REJECTED="<<rejected<<'\n';
    Scratch scratch;
    const auto file=scratch.path/fs::u8path(u8"preview-\u00e5.swp");
    CHECK(save_menu_preview_document(file,*parsed));
    CHECK(load_menu_preview_document(file));
    auto bad=*parsed;bad.scale=0;
    CHECK(!save_menu_preview_document(file,bad));
    const auto still=load_menu_preview_document(file);
    CHECK(still && encode_menu_preview_document(*still)==encoded);
    CHECK(save_menu_preview_document(file,MenuPreviewDocument{}));
    CHECK(encode_menu_preview_document(*load_menu_preview_document(file))==initial);
    {std::ofstream out(file,std::ios::binary|std::ios::trunc);
     out<<std::string(menu_preview_document_limit+1,'x');}
    CHECK(!load_menu_preview_document(file));
    CHECK(!load_menu_preview_document(scratch.path/"missing"));
    CHECK(!save_menu_preview_document({},*parsed));
    CHECK(menu.restore_local_preview(MenuPreviewDocument{}));
    CHECK(encode_menu_preview_document(capture_local_preview(menu))==initial);
    CHECK(menu.preview_enabled(0) && menu.appearance().allow_input_while_open);
    std::cout<<"R324_ALL_385_WIDGET_CALLSITES_ACCOUNTED_373_GENERIC_10_DEDICATED_2_ACTIONS_EXCLUDED_PASS\n";
    std::cout<<"R324_OFFLINE_PREVIEW_DOCUMENT_TRANSACTION_AND_ROUNDTRIP_PASS\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
