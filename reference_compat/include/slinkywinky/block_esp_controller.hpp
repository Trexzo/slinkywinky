#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>

namespace slinkywinky::reference_compat {
// Offline resource accounting for an externally supplied synthetic scene.
// No block discovery, visibility override, geometry or game integration.
class BlockEspController {
public:
    using Key=std::pair<int,int>;
    struct Resource {std::size_t vertices,indices;};
    struct Tick {bool rebuild_existing=false, resupply_scene=false;};
private:
    using Resources=std::map<Key,Resource>;
    Resources resources_;
    std::uint64_t owner_=0;
    bool enabled_=false, settings_dirty_=false, resupply_=false;
    static constexpr std::size_t max_resources=4096;
public:
    bool enable(std::uint64_t owner){
        if(owner==0)throw std::invalid_argument("zero owner");
        if(enabled_&&owner_==owner)return false;
        clear();
        owner_=owner;enabled_=true;resupply_=true;
        return true;
    }
    void clear() noexcept {
        resources_.clear();owner_=0;enabled_=false;
        settings_dirty_=false;resupply_=false;
    }
    bool update(std::uint64_t owner,Key key,Resource r){
        if(!enabled_||owner!=owner_)return false;
        if(!r.vertices||!r.indices||r.indices%2!=0)
            throw std::invalid_argument("invalid mesh resource counts");
        auto it=resources_.find(key);
        if(it==resources_.end()&&resources_.size()==max_resources)
            return false;
        resources_.insert_or_assign(key,r);
        return true;
    }
    bool remove(std::uint64_t owner,Key key){
        return enabled_&&owner==owner_&&resources_.erase(key)!=0;
    }
    void settings_changed() noexcept {if(enabled_)settings_dirty_=true;}
    void invalidate_scene() noexcept {if(enabled_)resupply_=true;}
    Tick tick(std::uint64_t owner){
        if(!enabled_)return {};
        if(owner!=owner_||!owner){clear();return {};}
        Tick t{settings_dirty_&&!resupply_,resupply_};
        if(resupply_)resources_.clear();
        settings_dirty_=resupply_=false;
        return t;
    }
    std::size_t resource_count()const noexcept{return resources_.size();}
};
}
