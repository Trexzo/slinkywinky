#pragma once
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace slinkywinky::reference_compat {
// Structured label components for synthetic snapshots; not a game renderer.
class NameTagsController {
public:
    struct Pos {double x,y,z;};
    struct Settings {float range;bool own_tag,display_name,health,hearts,distance,equipment;};
    struct Player {unsigned id,flags;bool self,dead;Pos pos,prev;float health,absorption;
        std::optional<std::string> plain,formatted;std::array<bool,5> slots;};
    struct Row {unsigned id;std::string name;char name_color;bool formatted;
        Pos pos,prev;float distance,previous_distance;std::optional<float> hp,absorption,shown_distance;
        std::array<bool,5> equipment;};
private:
    std::vector<Row> rows_;
    static bool finite(Pos p)noexcept{return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
    static double dist2(Pos a,Pos b)noexcept{double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
public:
    void collect(const Settings& s,bool world,bool font,bool third,Pos me,Pos prev,
                 const std::vector<Player>& players){
        rows_.clear();
        if(!world||!font)return;
        if(!std::isfinite(s.range)||s.range<0||!finite(me)||!finite(prev))
            throw std::invalid_argument("invalid tag scene");
        for(const auto& p:players){
            if((p.self&&(!s.own_tag||!third))||p.dead||(p.flags&2)||!finite(p.pos)||!finite(p.prev))continue;
            const double d=dist2(p.pos,me),prior=dist2(p.prev,prev);
            if(!std::isfinite(d)||!std::isfinite(prior)||d>static_cast<double>(s.range*s.range))continue;
            const auto& name=s.display_name?p.formatted:p.plain;
            if(!name)continue;
            Row row{p.id,*name,(p.flags&5)?'a':'c',s.display_name,p.pos,p.prev,
                static_cast<float>(std::sqrt(d)),static_cast<float>(std::sqrt(prior)),{},{},{},{}};
            if(!std::isfinite(row.distance)||!std::isfinite(row.previous_distance))continue;
            if(s.health){
                if(!std::isfinite(p.health)||!std::isfinite(p.absorption))continue;
                const float mult=s.hearts?.5f:1.f;
                row.hp=std::fmin(999.f,std::fmax(0.f,p.health))*mult;
                const float abs=std::fmin(99.f,std::fmax(0.f,p.absorption))*mult;
                if(abs>0)row.absorption=abs;
            }
            if(s.distance&&!p.self)row.shown_distance=row.distance;
            if(s.equipment)row.equipment=p.slots;
            rows_.push_back(std::move(row));
            if(rows_.size()==100)break;
        }
    }
    void disable(bool own,bool on)noexcept{if(own&&!on)rows_.clear();}
    const std::vector<Row>& rows()const noexcept{return rows_;}
};
}
