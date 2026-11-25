#include "slinkywinky/menu.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <iostream>
using namespace slinkywinky;
namespace fs=std::filesystem;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct TestDirectory{
    fs::path root;
    TestDirectory(){
        std::random_device r;
        for(unsigned i=0;i<32;++i){
            auto p=fs::temp_directory_path()/("slinkywinky-r317-"+std::to_string(r()));
            if(fs::create_directory(p)){root=std::move(p);return;}
        }
        throw std::runtime_error("test directory");
    }
    ~TestDirectory(){std::error_code error;fs::remove_all(root,error);}
};
static void write(const fs::path& path,const std::string& data){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(data.data(),static_cast<std::streamsize>(data.size()));
    out.close();check(bool(out),"fixture write");
}
static std::string read(const fs::path& path){
    std::ifstream in(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
int main(){
 try{
    TestDirectory temp;
    const auto appearance=temp.root/fs::u8path(u8"appearance-\u00e5.ini");
    auto legacy=appearance;legacy+=".tmp";
    write(legacy,"do not overwrite");
    MenuAppearance one{},two{};
    one.compact=true;one.accent_rgb=0x102030;
    two.wide=true;two.accent_rgb=0xabcdef;
    check(save_menu_appearance(appearance,one),"initial save");
    check(read(legacy)=="do not overwrite","legacy temporary file unchanged");
    MenuAppearance loaded{};
    check(load_menu_appearance(appearance,loaded)&&loaded.accent_rgb==one.accent_rgb,"unicode path");
    check(save_menu_appearance(appearance,two),"replace");
    check(load_menu_appearance(appearance,loaded)&&loaded.accent_rgb==two.accent_rgb&&loaded.wide,"replace content");
    auto invalid=one;invalid.scale=0.f;
    check(!save_menu_appearance(appearance,invalid)&&read(appearance)==serialize_menu_appearance(two),"invalid save unchanged");
    const auto valid=serialize_menu_appearance(one);
    const auto oversized=valid+"#"+std::string(4096,'a');
    check(!parse_menu_appearance(oversized,loaded)&&loaded.accent_rgb==two.accent_rgb,"oversized parser");
    write(appearance,oversized);
    check(!load_menu_appearance(appearance,loaded)&&loaded.accent_rgb==two.accent_rgb,"oversized file");
    check(!parse_menu_appearance(valid+"#"+std::string(1,'\0'),loaded),"NUL appearance");
    const auto boundary=valid+"#"+std::string(4096-valid.size()-1,'x');
    write(appearance,boundary);
    check(load_menu_appearance(appearance,loaded)&&loaded.accent_rgb==one.accent_rgb,"exact bound");
    const auto preview=temp.root/"preview.ini";
    PiercingPreview first{},second{};
    second.sort=1;second.enabled[0]=true;
    check(save_piercing_preview(preview,first),"preview initial");
    check(save_piercing_preview(preview,second),"preview replace");
    PiercingPreview copy{};
    check(load_piercing_preview(preview,copy)&&copy==second,"preview load");
    const auto data=serialize_piercing_preview(first);
    write(preview,data+"#"+std::string(512,'x')+"\n");
    check(!load_piercing_preview(preview,copy)&&copy==second,"preview oversized");
    check(!parse_piercing_preview(data+"#"+std::string(1,'\0')+"\n",copy),"NUL preview");
    const auto blocked=temp.root/"directory.ini";
    fs::create_directory(blocked);
    check(!save_menu_appearance(blocked,one)&&fs::is_directory(blocked),"failed rename preserves directory");
    check(!save_menu_appearance({},one),"empty path");
    check(!save_piercing_preview({},first),"empty preview path");
    check(save_menu_appearance(appearance,one),"concurrency baseline");
    std::atomic<unsigned> successes{0};
    auto save=[&](const MenuAppearance& prefs){
        for(unsigned i=0;i<20;++i)
            if(save_menu_appearance(appearance,prefs))++successes;
    };
    std::thread a(save,std::cref(one)),b(save,std::cref(two));a.join();b.join();
    check(successes.load()>0,"at least one concurrent save");
    check(load_menu_appearance(appearance,loaded),"complete concurrent result");
    check(read(appearance)==serialize_menu_appearance(one)||read(appearance)==serialize_menu_appearance(two),"no mixed document");
    check(read(legacy)=="do not overwrite","legacy temporary preserved");
    for(const auto& entry:fs::directory_iterator(temp.root))
        check(entry.path().filename().string().find(".slinkywinky-prefs-")!=0,"scratch cleanup");
    std::cout<<"R317_GUI_PREFERENCE_PERSISTENCE_PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
