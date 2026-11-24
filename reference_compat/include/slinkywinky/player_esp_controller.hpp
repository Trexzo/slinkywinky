#pragma once
// R299: source-ordered Java PlayerEspModule event-18 row collection.
// No GL, Minecraft ownership, projection or world graphics hooks.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace slinkywinky::reference_compat {
struct PlayerEspPosition { double x=0.0,y=0.0,z=0.0; };
struct PlayerEspSettings {
    float range=0.0f;
    bool hide_friendlies=false,override_target_color=false;
    int color_mode=0,mode=0;
    std::string color_target="target",color_friendlies="friendly",color_enemies="enemy";
    // R308: explicit opt-in recovered RGB selection; no guessed settings.
    bool source_style=false;
    std::array<float,3> target_rgb{{1.0f,1.0f,0.0f}};
    std::array<float,3> friendly_rgb{{0.0f,1.0f,0.0f}};
    std::array<float,3> enemy_rgb{{1.0f,0.0f,0.0f}};
};
struct PlayerEspEntity {
    bool is_self=false,dead=false,is_target=false;
    std::uint32_t relationship_flags=0;
    PlayerEspPosition previous{},position{};
    std::string name_color="name";
    std::array<float,3> name_rgb{{1.0f,1.0f,1.0f}};
    bool name_rgb_known=false;
    float absorption=0.0f, max_health=20.0f, health=20.0f;
    int hurt_time=0;
};
struct PlayerEspRow {
    PlayerEspPosition previous{},position{};
    std::string color;
    float distance=0.0f,health=0.0f,absorption=0.0f;
    int hurt=0;
    bool target=false;
    // Preserves exact source float channels, not 8-bit palette approximations.
    std::array<float,3> source_rgb{{0.0f,0.0f,0.0f}};
    bool source_rgb_known=false;
};
class PlayerEspController final {
public:
    static constexpr std::size_t kSourceRowCap=100;
    static constexpr float kSourceHealthMax=1.0f; // RenderConstants.f(1378848)
    static constexpr bool listens(int type) noexcept {
        return type==6 || type==7 || type==18;
    }
    static constexpr std::uint32_t relationship_mask(bool hide) noexcept {
        return (hide?5u:0u)+2u; // Java source operator precedence
    }
    static float source_max(float a,float b) noexcept {return a>b?a:b;}
    static float source_min(float a,float b) noexcept {return a<b?a:b;}
    static float source_clamp(float lo,float hi,float n) noexcept {
        return n<lo?lo:source_min(hi,n);
    }
    // Exact Java Float.compare handling, including NaN and signed zeros.
    static int java_float_compare(float a,float b) noexcept {
        if(a<b)return -1;
        if(a>b)return 1;
        std::uint32_t a_bits=0,b_bits=0;
        std::memcpy(&a_bits,&a,sizeof(a));
        std::memcpy(&b_bits,&b,sizeof(b));
        if(std::isnan(a))a_bits=0x7fc00000u;
        if(std::isnan(b))b_bits=0x7fc00000u;
        const std::int32_t ai=static_cast<std::int32_t>(a_bits);
        const std::int32_t bi=static_cast<std::int32_t>(b_bits);
        return ai==bi?0:(ai<bi?-1:1);
    }
    // Original compareDistance(a,b) delegates to Float.compare(b,a),
    // so collection is sorted FARTHER first, after the first 100 are read.
    void update(bool ready,PlayerEspPosition self,
                const PlayerEspSettings& settings,
                const std::vector<PlayerEspEntity>& players) {
        if(!ready)return; // The original update() preserves stale rows here.
        rows_.clear();
        const double radius_squared=static_cast<double>(settings.range*settings.range);
        for(const auto& p:players) {
            if(p.is_self||p.dead||
               (p.relationship_flags&relationship_mask(settings.hide_friendlies))!=0u)
                continue;
            const double dx=p.position.x-self.x;
            const double dy=p.position.y-self.y;
            const double dz=p.position.z-self.z;
            const double distance=dz*dz+(dx*dx+dy*dy);
            if(distance>radius_squared)continue;
            const bool target=settings.override_target_color&&p.is_target;
            std::string color=target?settings.color_target:
                settings.color_mode==0?p.name_color:
                (p.relationship_flags&5u)!=0u?
                  settings.color_friendlies:settings.color_enemies;
            const float extra=source_max(0.0f,p.absorption);
            const float total=p.max_health+extra;
            const float health=total==0.0f?0.0f:
                source_clamp(0.0f,kSourceHealthMax,(p.health+extra)/total);
            const float absorb=p.health==0.0f?0.0f:
                source_clamp(0.0f,kSourceHealthMax,extra/p.health);
            PlayerEspRow row{p.previous,p.position,color,
                static_cast<float>(distance),health,absorb,p.hurt_time,target};
            if(settings.source_style){
                // Original priority: target > name (automatic) >
                // friend/enemy (manual). Flags 2/7 are already filtered.
                if(target){
                    row.source_rgb=settings.target_rgb;
                    row.source_rgb_known=true;
                }else if(settings.color_mode==0){
                    row.source_rgb=p.name_rgb;
                    row.source_rgb_known=p.name_rgb_known;
                }else{
                    row.source_rgb=(p.relationship_flags&5u)!=0u ?
                        settings.friendly_rgb:settings.enemy_rgb;
                    row.source_rgb_known=true;
                }
            }
            rows_.push_back(row);
            if(rows_.size()==kSourceRowCap)break;
        }
        std::stable_sort(rows_.begin(),rows_.end(),[](const auto& a,const auto& b){
            return java_float_compare(b.distance,a.distance)<0;
        });
    }
    bool world_mode(const PlayerEspSettings& settings,bool ready) const noexcept {
        return settings.mode==1&&ready;
    }
    bool screen_mode(const PlayerEspSettings& settings,bool frame_present) const noexcept {
        return settings.mode==0&&frame_present;
    }
    void world_unload() noexcept {rows_.clear();}
    const std::vector<PlayerEspRow>& rows() const noexcept {return rows_;}
private:
    std::vector<PlayerEspRow> rows_;
};
} // namespace slinkywinky::reference_compat
