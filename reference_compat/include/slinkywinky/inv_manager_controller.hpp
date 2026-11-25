#pragma once
#include <cstdint>
#include <optional>
namespace slinkywinky::reference_compat {
// Rank supplied category scores; the original enchantment/item scorer is not recovered here.
class InvManagerController {
public:
    static std::optional<unsigned> paired_kind(unsigned kind)noexcept{
        switch(kind){case 22:return 25;case 17:return 7;case 7:return 17;default:return {};}
    }
    static std::uint32_t rank(bool present,bool swap_sides,unsigned kind,
                               std::uint16_t direct,std::uint16_t paired)noexcept{
        if(!present)return 0;
        if(!swap_sides)return direct;
        if(direct)return static_cast<std::uint32_t>(direct)<<16;
        return paired_kind(kind)?paired:0;
    }
};
}
