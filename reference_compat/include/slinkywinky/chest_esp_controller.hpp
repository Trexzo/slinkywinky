#pragma once
// R297: source-ordered, read-only Chest ESP collection and draw-plan model.
// Recovered Java 8 ChestEspModule; no Minecraft, JNI or OpenGL operations.
#include <array>
#include <cstddef>
#include <vector>

namespace slinkywinky::reference_compat {

struct ChestEspPosition {
    double x=0.0, y=0.0, z=0.0;
};
struct ChestEspTile {
    ChestEspPosition position{};
    int block_id=0;
};
struct ChestEspSettings {
    float range=0.0f;
    bool render_chest=false, render_ender_chest=false;
    bool render_trapped_chest=false, render_beacon=false;
    bool filled=false;
    float line_width=1.0f;
};
struct ChestEspRow {
    ChestEspPosition position{};
    int block_id=0;
};
struct ChestEspDraw {
    std::array<float,3> translation{};
    int block_id=0;
    const char* color_key=nullptr;
    bool filled=false;
};
struct ChestEspPlan {
    float line_width=1.0f;
    float scale=0.5f;
    float filled_alpha=0.4000000059604645f;
    int filled_primitive=168960;
    int outline_primitive=169008;
    std::vector<ChestEspDraw> draws;
};
class ChestEspController final {
public:
    static constexpr std::size_t kSourceRowCap=100;
    static constexpr float kSourceScale=0.5f;
    static constexpr float kSourceFilledAlpha=0.4f;
    static constexpr int kSourceFilledPrimitive=168960;
    static constexpr int kSourceOutlinePrimitive=169008;

    static constexpr bool listens(int type) noexcept { return type==0 || type==6; }
    static constexpr const char* kind(int id) noexcept {
        return id==54 ? "chest" : id==130 ? "ender_chest" :
               id==146 ? "trapped_chest" : id==138 ? "beacon" : nullptr;
    }
    static constexpr bool configured(int id,const ChestEspSettings& settings) noexcept {
        return id==54 ? settings.render_chest :
               id==130 ? settings.render_ender_chest :
               id==146 ? settings.render_trapped_chest :
               id==138 ? settings.render_beacon : false;
    }
    // Matches event 0: clear first, scan loaded tile entities in encounter
    // order, use BLOCK identifiers (not tile types), inclusive float-range
    // comparison, and stop immediately after collecting 100 entries.
    void tick(bool ready,ChestEspPosition self,float range,
              const ChestEspSettings& settings,
              const std::vector<ChestEspTile>& loaded_tiles) {
        rows_.clear();
        if(!ready)return;
        const double radius_squared=static_cast<double>(range*range);
        for(const auto& tile:loaded_tiles) {
            if(!configured(tile.block_id,settings))continue;
            const double dx=tile.position.x-self.x;
            const double dy=tile.position.y-self.y;
            const double dz=tile.position.z-self.z;
            const double distance_squared=dx*dx+dy*dy+dz*dz;
            // Source: !(distanceSquared > (double)(range * range)).
            if(!(distance_squared>radius_squared)) {
                rows_.push_back({tile.position,tile.block_id});
                if(rows_.size()==kSourceRowCap)break;
            }
        }
    }
    void unload() noexcept {rows_.clear();}
    const std::vector<ChestEspRow>& rows() const noexcept{return rows_;}
    // Event 6 describes plan output only. It never calls OpenGL or
    // accesses a player/world. Recovered float and double operands.
    ChestEspPlan plan(ChestEspPosition camera,const ChestEspSettings& settings) const {
        ChestEspPlan plan;
        plan.line_width=settings.line_width;
        plan.draws.reserve(rows_.size());
        for(const auto& row:rows_) {
            const auto& p=row.position;
            plan.draws.push_back({{
                static_cast<float>((p.x-camera.x)+0.5),
                static_cast<float>((p.y-camera.y)+0.5),
                static_cast<float>((p.z-camera.z)+0.5)
            },row.block_id,kind(row.block_id),settings.filled});
        }
        return plan;
    }
private:
    std::vector<ChestEspRow> rows_;
};
} // namespace slinkywinky::reference_compat
