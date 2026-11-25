#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
namespace slinkywinky::reference_compat {
// Pure data snapshot: no JVM handles, persistence or game writes.
class FriendsController {
public:
    using UUID=std::pair<std::uint64_t,std::uint64_t>;
    enum class Change { none, added, removed };
    struct Player { UUID uuid; unsigned flags; bool self; };
    using Roster=std::vector<Player>;
private:
    std::map<UUID,std::int64_t> friends_;
public:
    Change toggle(bool setting,bool other_player,UUID uuid,unsigned flags,std::int64_t now){
        if(!setting||!other_player)return Change::none;
        auto it=friends_.find(uuid);
        if(it!=friends_.end()){friends_.erase(it);return Change::removed;}
        if(flags&2u)return Change::none;
        friends_.emplace(uuid,now);
        return Change::added;
    }
    Roster snapshot(bool world,bool player,const Roster& input,std::int64_t now){
        Roster out=input;
        if(!world||!player)return out;
        for(auto& p:out){
            if(p.self)continue;
            auto it=friends_.find(p.uuid);
            if(it==friends_.end())p.flags&=~4u;
            else{p.flags|=4u;it->second=now;}
        }
        return out;
    }
    Roster disabled(bool own_descriptor,bool on,bool world,const Roster& input)const{
        Roster out=input;
        if(own_descriptor&&!on&&world)
            for(auto& p:out)if(!p.self)p.flags&=~4u;
        return out;
    }
    const std::map<UUID,std::int64_t>& entries()const noexcept{return friends_;}
    void shutdown()noexcept{friends_.clear();}
};
}
