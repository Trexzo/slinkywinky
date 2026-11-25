#pragma once
#include <array>
namespace slinkywinky::reference_compat {
// Value-only fixture data; no game objects or input interface.
struct InventorySampleItem {
    unsigned id=0, metadata=0;
    bool occupied=false;
    bool has_item() const noexcept {return occupied&&id!=0;}
};
using HotbarSample=std::array<InventorySampleItem,9>;
using MainInventorySample=std::array<InventorySampleItem,27>;
constexpr bool splash_potion(const InventorySampleItem& item) noexcept {
    return item.occupied&&item.id==373&&(item.metadata&0x4000u)!=0;
}
constexpr bool healing_potion(const InventorySampleItem& item) noexcept {
    return splash_potion(item)&&(item.metadata&15u)==5u;
}
}
