#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace slinkywinky::reference_compat {
// Offline entry lifecycle only. No font measurement, original easing or drawing.
class ModOverlayController {
public:
    struct Settings{std::array<bool,5> categories;bool require_bound,lowercase,details;};
    struct Module{unsigned id,category,flags;bool enabled,bound;std::string name,suffix;};
    struct Entry{unsigned id;bool alive;float animation;std::string name,suffix;};
private:
    std::vector<Entry> rows_;
    float fade_=0;
    bool dirty_=true;
    static bool visible(const Settings& s,const Module& m)noexcept{
        return !(m.flags&4u)&&(!s.require_bound||m.bound)&&
            (m.category>=s.categories.size()||s.categories[m.category]);
    }
    static std::string text(std::string s,bool lower,std::size_t limit){
        auto nul=s.find('\0');if(nul!=std::string::npos)s.resize(nul);
        if(s.size()>limit)s.resize(limit);
        if(lower)for(char& c:s)if(c>='A'&&c<='Z')c=static_cast<char>(c+'a'-'A');
        return s;
    }
    auto find(unsigned id){
        return std::find_if(rows_.begin(),rows_.end(),[&](const Entry& e){return e.id==id;});
    }
    static void refresh(Entry& e,const Settings& s,const Module& m){
        e.name=text(m.name,s.lowercase,19);
        e.suffix=s.details?text(m.suffix,s.lowercase,39):std::string{};
    }
    void append(const Settings& s,const Module& m){
        if(rows_.size()>=50)return;
        Entry e{m.id,true,0.f,{},{}};
        refresh(e,s,m);rows_.push_back(std::move(e));
    }
public:
    void rebuild(const Settings& s,const std::vector<Module>& roster){
        std::set<unsigned> seen;
        for(const auto& m:roster)
            if(!seen.insert(m.id).second)throw std::invalid_argument("duplicate overlay id");
        for(auto& e:rows_)e.alive=false;
        for(const auto& m:roster){
            if(!m.enabled||!visible(s,m))continue;
            auto it=find(m.id);
            if(it==rows_.end())append(s,m);
            else{it->alive=true;refresh(*it,s,m);}
        }
        dirty_=false;
    }
    void toggle(const Settings& s,const Module& m){
        fade_=.2f;
        auto it=find(m.id);
        if(it!=rows_.end()){
            if(!m.enabled)it->alive=false;
            else if(visible(s,m)){it->alive=true;refresh(*it,s,m);}
        }else if(m.enabled&&visible(s,m))append(s,m);
    }
    void profile_event(unsigned flags)noexcept{if(!flags)dirty_=true;}
    bool dirty()const noexcept{return dirty_;}
    void frame(float dt){
        if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("invalid overlay delta");
        fade_=std::max(0.f,fade_-dt);
        for(auto& e:rows_){
            double step=static_cast<double>(dt)*5.0;
            e.animation=static_cast<float>(std::clamp(static_cast<double>(e.animation)+(e.alive?step:-step),0.,1.));
        }
        rows_.erase(std::remove_if(rows_.begin(),rows_.end(),[](const Entry& e){
            return !e.alive&&e.animation==0.f;
        }),rows_.end());
    }
    unsigned hud_flags(unsigned incoming)const noexcept{
        bool active=dirty_||fade_>0||std::any_of(rows_.begin(),rows_.end(),[](const Entry& e){
            return !e.alive||e.animation<1.f;
        });
        return incoming|(active?3u:1u);
    }
    const std::vector<Entry>& rows()const noexcept{return rows_;}
    void reset()noexcept{rows_.clear();fade_=0;dirty_=true;}
};
}
