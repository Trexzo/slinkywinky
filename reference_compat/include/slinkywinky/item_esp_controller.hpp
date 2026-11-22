#pragma once
// R298: Java ItemEspModule source-faithful event-18 collection and mergeCounts.
// No live world, renderer, hotbar or OpenGL access.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {
struct ItemEspPosition { double x=0.0,y=0.0,z=0.0; };
struct ItemEspEntity {
    bool is_item=true,dead=false,stack_present=true,item_present=true;
    ItemEspPosition previous{},position{};
    int item_id=0, stack_size=0;
};
struct ItemEspPolicy {int resolved_item_id=-1;std::string color;};
struct ItemEspSettings {
    float range=0.0f;
    bool highlight_all=false,show_count=false;
    std::string default_color="default_color";
};
struct ItemEspRow {
    ItemEspPosition previous{},position{};
    std::string color;
    int item_id=0,quantity=0;
    std::string label;
    int width=0;
};
class ItemEspController final {
public:
    static constexpr std::size_t kSourceRowCap=100;
    static constexpr double kSourceMergeRadiusSquared=0.0625;
    static constexpr bool listens(int t) noexcept {return t==6 || t==18 || t==26;}
    // Mirrors first resolved ID wins in TreeMap and rejects ID -1.
    void refresh_policy(const std::vector<ItemEspPolicy>& allowed) {
        policy_.clear();
        for(const auto& entry:allowed)if(entry.resolved_item_id!=-1)
            policy_.emplace(entry.resolved_item_id,entry.color);
    }
    void clear_event26() noexcept {policy_.clear();rows_.clear();}
    void world_unload() noexcept {rows_.clear();}
    const std::vector<ItemEspRow>& rows() const noexcept {return rows_;}
    std::size_t policy_size() const noexcept {return policy_.size();}
    static double distance_squared(ItemEspPosition a,ItemEspPosition b) noexcept {
        const double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
        return x*x+y*y+z*z;
    }
    // Java source merges in list order, including zeroing all qualifying
    // other rows; the anchor retains up to 99 and label receives clamped N.
    static void merge_counts(std::vector<ItemEspRow>& rows,double radius_squared) {
        for(std::size_t i=0;i<rows.size();++i) {
            ItemEspRow& row=rows[i];
            if(row.quantity==0)continue;
            std::int64_t total=row.quantity;
            for(std::size_t j=0;j<rows.size();++j) {
                if(j==i)continue;
                ItemEspRow& other=rows[j];
                if(row.item_id==other.item_id &&
                   distance_squared(row.position,other.position)<radius_squared) {
                    total+=other.quantity;
                    other.quantity=0;
                }
            }
            row.quantity=static_cast<int>(total<99?total:99);
            row.label=total>1?std::to_string(row.quantity):std::string{};
        }
    }
    // Mirrors event18: clear rows, refresh policy even when world not ready,
    // then collect at most 100 eligible EntityItem stacks in encounter order.
    // Width calculation is host-provided to avoid inventing Minecraft fonts.
    void sample(bool ready,ItemEspPosition self,
                const ItemEspSettings& settings,
                const std::vector<ItemEspPolicy>& allowed,
                const std::vector<ItemEspEntity>& entities,
                const std::function<int(const std::string&)>& text_width={}) {
        rows_.clear();
        refresh_policy(allowed);
        if(!ready)return;
        const double max_distance=static_cast<double>(settings.range*settings.range);
        for(const auto& e:entities) {
            if(!e.is_item||e.dead)continue;
            if(distance_squared(e.position,self)>max_distance)continue;
            if(!e.stack_present||e.stack_size<=0||!e.item_present)continue;
            auto found=policy_.find(e.item_id);
            if(found==policy_.end()&&!settings.highlight_all)continue;
            const std::string color=found!=policy_.end()?found->second:settings.default_color;
            rows_.push_back({e.previous,e.position,color,e.item_id,e.stack_size&0xff,{},{}});
            if(rows_.size()==kSourceRowCap)break;
        }
        if(settings.show_count) {
            merge_counts(rows_,kSourceMergeRadiusSquared);
            for(auto& row:rows_)if(!row.label.empty()&&text_width)
                row.width=text_width(row.label);
        }
    }
private:
    std::vector<ItemEspRow> rows_;
    std::map<int,std::string> policy_;
};
} // namespace slinkywinky::reference_compat
