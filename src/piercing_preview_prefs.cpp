#include "slinkywinky/menu.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace slinkywinky {
bool operator==(const PiercingPreview& a,const PiercingPreview& b) noexcept {
    return a.sort==b.sort && a.enabled==b.enabled;
}
std::string serialize_piercing_preview(const PiercingPreview& p) {
    if(p.sort>1u)return {};
    std::string s="# Local sandbox Piercing GUI only; NOT game profile\n";
    s+="version=1\nsort=";
    s+=static_cast<char>('0'+p.sort);
    s+="\nflags=";
    for(bool flag:p.enabled)s+=flag?'1':'0';
    s+='\n';
    return s;
}
bool parse_piercing_preview(const std::string& data,
                           PiercingPreview& out) noexcept {
    // Strict, fixed, bounded first-party format: reject duplicate/extra
    // lines, whitespace ambiguity, oversized input and partial imports.
    if(data.empty() || data.size()>512u)return false;
    try {
        PiercingPreview parsed{};
        bool version=false,sort=false,flags=false;
        std::size_t start=0;
        while(start<data.size()) {
            auto stop=data.find('\n',start);
            if(stop==std::string::npos)return false;
            const auto line=data.substr(start,stop-start);
            start=stop+1;
            if(line.empty() || line[0]=='#')continue;
            if(line=="version=1") {
                if(version)return false;
                version=true;
            } else if(line.size()==6 && line.compare(0,5,"sort=")==0 &&
                      (line[5]=='0' || line[5]=='1')) {
                if(sort)return false;
                parsed.sort=static_cast<unsigned>(line[5]-'0');
                sort=true;
            } else if(line.size()==13 && line.compare(0,6,"flags=")==0) {
                if(flags)return false;
                for(std::size_t i=0;i<7u;++i) {
                    const char c=line[6+i];
                    if(c!='0' && c!='1')return false;
                    parsed.enabled[i]=c=='1';
                }
                flags=true;
            } else return false;
        }
        if(!version || !sort || !flags)return false;
        out=parsed; // Transactional; never import partial/unknown fields.
        return true;
    }catch(...){return false;}
}
bool load_piercing_preview(const std::filesystem::path& path,
                           PiercingPreview& out) noexcept {
    try {
        std::ifstream input(path,std::ios::binary);
        if(!input)return false;
        std::string contents;
        char c;
        while(input.get(c)) {
            if(contents.size()>=512u)return false;
            contents+=c;
        }
        if(input.bad())return false;
        return parse_piercing_preview(contents,out);
    }catch(...){return false;}
}
bool save_piercing_preview(const std::filesystem::path& path,
                           const PiercingPreview& p) noexcept {
    const auto content=serialize_piercing_preview(p);
    if(content.empty() || path.empty())return false;
    try {
        if(!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        const auto tmp=std::filesystem::path(path.string()+".tmp");
        {
            std::ofstream out(tmp,std::ios::binary|std::ios::trunc);
            if(!out)return false;
            out<<content;
            out.flush();
            if(!out)return false;
        }
#ifdef _WIN32
        if(!MoveFileExW(tmp.c_str(),path.c_str(),
            MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            std::error_code ignored;
            std::filesystem::remove(tmp,ignored);
            return false;
        }
#else
        std::filesystem::rename(tmp,path);
#endif
        return true;
    }catch(...){return false;}
}
} // namespace slinkywinky
