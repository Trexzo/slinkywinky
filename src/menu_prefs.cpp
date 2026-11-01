#include "slinkywinky/menu.hpp"
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace slinkywinky {
namespace {
std::string trim(const std::string& s) {
    auto left=s.find_first_not_of(" \t\n\r");
    if(left==std::string::npos)return {};
    auto right=s.find_last_not_of(" \t\n\r");
    return s.substr(left,right-left+1);
}
bool boolean(const std::string& text,bool& out) {
    if(text=="true"){out=true;return true;}
    if(text=="false"){out=false;return true;}
    return false;
}
bool plausible(const MenuAppearance& a) noexcept {
    return std::isfinite(a.scale) && a.scale>=0.65f &&
           a.scale<=1.8f && a.accent_rgb<=0xffffffu;
}
} // namespace

std::string serialize_menu_appearance(const MenuAppearance& a) {
    if(!plausible(a))return {};
    std::ostringstream o;
    o.imbue(std::locale::classic());
    o<<"# First-party menu appearance, separate from gameplay profiles\n";
    o<<"version=1\n";
    o<<"compact="<<(a.compact?"true":"false")<<'\n';
    o<<"wide="<<(a.wide?"true":"false")<<'\n';
    o<<"allow_input="<<(a.allow_input_while_open?"true":"false")<<'\n';
    o<<"accent="<<std::hex<<std::nouppercase<<std::setfill('0')
     <<std::setw(6)<<a.accent_rgb<<std::dec<<'\n';
    o<<"scale="<<std::fixed<<std::setprecision(3)<<a.scale<<'\n';
    return o.str();
}
bool parse_menu_appearance(const std::string& text,
                           MenuAppearance& appearance) noexcept {
    try {
        MenuAppearance parsed;
        std::set<std::string> seen;
        std::istringstream stream(text);
        std::string line;
        while(std::getline(stream,line)) {
            line=trim(line);
            if(line.empty()||line[0]=='#')continue;
            const auto eq=line.find('=');
            if(eq==std::string::npos || eq==0)return false;
            const auto key=trim(line.substr(0,eq));
            const auto val=trim(line.substr(eq+1));
            if(!seen.insert(key).second)return false;
            if(key=="version"){
                if(val!="1")return false;
            }else if(key=="compact"){
                if(!boolean(val,parsed.compact))return false;
            }else if(key=="wide"){
                if(!boolean(val,parsed.wide))return false;
            }else if(key=="allow_input"){
                if(!boolean(val,parsed.allow_input_while_open))return false;
            }else if(key=="accent"){
                if(val.size()!=6)return false;
                for(unsigned char c:val)if(!std::isxdigit(c))return false;
                std::size_t used=0;
                parsed.accent_rgb=static_cast<std::uint32_t>(
                    std::stoul(val,&used,16));
                if(used!=6)return false;
            }else if(key=="scale"){
                std::size_t used=0;
                std::istringstream scale(val);
                scale.imbue(std::locale::classic());
                float n=0;
                if(!(scale>>n))return false;
                scale>>std::ws;
                if(!scale.eof())return false;
                parsed.scale=n;
            }else return false;
        }
        if(seen.size()!=6 || !plausible(parsed))return false;
        appearance=parsed; // Publish only a fully validated document.
        return true;
    }catch(...){return false;}
}
bool load_menu_appearance(const std::filesystem::path& path,
                          MenuAppearance& output) noexcept {
    try {
        std::ifstream in(path,std::ios::binary);
        if(!in)return false;
        std::ostringstream contents;contents<<in.rdbuf();
        if(in.bad())return false;
        return parse_menu_appearance(contents.str(),output);
    }catch(...){return false;}
}
bool save_menu_appearance(const std::filesystem::path& path,
                          const MenuAppearance& prefs) noexcept {
    if(!plausible(prefs))return false;
    const auto contents=serialize_menu_appearance(prefs);
    if(contents.empty())return false;
    try {
        if(!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        const auto tmp=std::filesystem::path(path.string()+".tmp");
        {
            std::ofstream out(tmp,std::ios::binary|std::ios::trunc);
            if(!out)return false;
            out<<contents;out.flush();
            if(!out)return false;
        }
#ifdef _WIN32
        if(!MoveFileExW(tmp.c_str(),path.c_str(),
                        MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            std::error_code ignored;std::filesystem::remove(tmp,ignored);
            return false;
        }
#else
        std::filesystem::rename(tmp,path);
#endif
        return true;
    }catch(...){return false;}
}
} // namespace slinkywinky
