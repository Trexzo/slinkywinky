#pragma once
// R295: recovered Java ChamsModule begin/end policy, source-only.
// No OpenGL invocation, game rendering hook or entity mutation.
#include <cstdint>
#include <utility>

namespace slinkywinky::reference_compat {
struct ChamsInput {
    bool module_enabled=false,hide_friendlies=false;
    std::uint32_t relationship_flags=0;
};
class ChamsController final {
public:
    static constexpr int kPolygonOffsetFill=32823; // GL_POLYGON_OFFSET_FILL
    // The recovered Java reads:
    // (RelationshipService.flags(entity) &
    //   (flag("hide_friendlies") ? 5 : 0) + 2) == 0
    // Java addition has higher precedence than bitwise &, yielding
    // mask 2 (bots) normally, mask 7 when hide_friendlies enabled.
    static constexpr std::uint32_t relationship_mask(bool hide) noexcept {
        return (hide?5u:0u)+2u;
    }
    static constexpr bool eligible(const ChamsInput& in) noexcept {
        return in.module_enabled &&
               (in.relationship_flags &
                relationship_mask(in.hide_friendlies))==0u;
    }
    // Testable source-like graphics port. Caller explicitly supplies
    // recovered RenderConstants.f(1392420) and f(1392424) values.
    // Unknown original native float values are never invented.
    template<class Graphics>
    static bool begin(const ChamsInput& in,Graphics& graphics,
                      float factor,float units) {
        const bool selected=eligible(in);
        if(selected) {
            graphics.enable(kPolygonOffsetFill);
            graphics.offset(factor,units);
        }
        return selected;
    }
    template<class Graphics>
    static void end(bool selected,Graphics& graphics) {
        if(selected)graphics.disable(kPolygonOffsetFill);
    }
    // Matches original Java's "selected=begin(entity)" before try.
    // If begin throws, no finally is entered in the ORIGINAL source.
    // If original rendering throws, disable runs before propagation.
    template<class Graphics,class Original>
    static bool render(const ChamsInput& in,Graphics& graphics,
                       float factor,float units,Original&& original) {
        const bool selected=begin(in,graphics,factor,units);
        try {
            std::forward<Original>(original)();
        } catch(...) {
            end(selected,graphics);
            throw;
        }
        end(selected,graphics);
        return selected;
    }
};
} // namespace slinkywinky::reference_compat
