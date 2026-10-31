#include "slinkywinky/settings.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace slinkywinky {
namespace {
std::string trim(std::string s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end-start+1);
}
bool parse_bool(std::string s) {
    if (s == "true") return true;
    if (s == "false") return false;
    throw std::invalid_argument("expected true or false");
}
float parse_float(const std::string& s,float lo,float hi) {
    std::size_t consumed = 0;
    const float value = std::stof(s, &consumed);
    if (consumed != s.size() || !std::isfinite(value) || value < lo || value > hi)
        throw std::out_of_range("invalid numeric setting");
    return value;
}
unsigned parse_uint(const std::string& s, unsigned lo, unsigned hi) {
    if (s.empty() || s[0] == '-') throw std::invalid_argument("negative setting");
    std::size_t consumed = 0;
    const auto value = std::stoul(s, &consumed);
    if (consumed != s.size() || value < lo || value > hi)
        throw std::out_of_range("invalid integral setting");
    return static_cast<unsigned>(value);
}
void put_bool(std::ostringstream& out, const char* name,bool b) { out<<name<<'='<<(b?"true":"false")<<'\n'; }
void put_float(std::ostringstream& out,const char* name,float v) {out<<name<<'='<<v<<'\n';}
}
SettingsResult parse_settings(const std::string& text) {
    ClientSettings parsed;
    std::set<std::string> seen;
    std::istringstream stream(text);
    std::string line;
    std::size_t line_no = 0;
    try {
        while (std::getline(stream,line)) {
            ++line_no;
            line=trim(line);
            if (line.empty() || line[0]=='#') continue;
            const auto split=line.find('=');
            if (split==std::string::npos) throw std::invalid_argument("missing =");
            const std::string name=trim(line.substr(0,split));
            const std::string value=trim(line.substr(split+1));
            if (name.empty() || !seen.insert(name).second) throw std::invalid_argument("empty/duplicate setting");
            auto& c=parsed.combat;
            if(name=="version") parsed.version=parse_uint(value,1,1);
            else if(name=="auto_clicker_enabled") c.auto_clicker_enabled=parse_bool(value);
            else if(name=="clicks_per_second") c.clicks_per_second=parse_float(value,1,30);
            else if(name=="require_mouse_down") c.require_mouse_down=parse_bool(value);
            else if(name=="aim_enabled") c.aim_enabled=parse_bool(value);
            else if(name=="aim_range") c.aim_range=parse_float(value,0,6);
            else if(name=="aim_horizontal_speed") c.aim_horizontal_speed=parse_float(value,0,100);
            else if(name=="aim_vertical_speed") c.aim_vertical_speed=parse_float(value,0,100);
            else if(name=="aim_fov") c.aim_fov=parse_float(value,0,180);
            else if(name=="aim_requires_mouse") c.aim_requires_mouse=parse_bool(value);
            else if(name=="auto_block_enabled") c.auto_block_enabled=parse_bool(value);
            else if(name=="block_range") c.block_range=parse_float(value,0,6);
            else if(name=="block_requires_damage") c.block_requires_damage=parse_bool(value);
            else if(name=="block_max_hurt_time") c.block_max_hurt_time=parse_float(value,0,10);
            else if(name=="reach_enabled") c.reach_enabled=parse_bool(value);
            else if(name=="reach_distance") c.reach.distance=parse_float(value,0,6);
            else if(name=="reach_activation_ticks") c.reach.activation_ticks=parse_float(value,0,9);
            else if(name=="reach_require_sprint") c.reach.require_sprint=parse_bool(value);
            else if(name=="hit_select_enabled") c.hit_select_enabled=parse_bool(value);
            else if(name=="hit_select_min_hurt") c.hit_select_min_hurt=static_cast<int>(parse_uint(value,0,10));
            else if(name=="velocity_enabled") parsed.velocity.enabled=parse_bool(value);
            else if(name=="velocity_horizontal") parsed.velocity.horizontal=parse_float(value,0,1);
            else if(name=="velocity_vertical") parsed.velocity.vertical=parse_float(value,0,1);
            else if(name=="velocity_chance_percent") parsed.velocity.chance_percent=parse_float(value,0,100);
            else if(name=="backtrack_enabled") parsed.backtrack_enabled=parse_bool(value);
            else if(name=="backtrack_ms") parsed.backtrack_ms=parse_uint(value,0,1000);
            // Unknown keys deliberately ignored and never silently materialized.
        }
        if(!seen.count("version")) throw std::invalid_argument("missing version");
        return {parsed,true,{}};
    } catch(const std::exception& e) {
        return {ClientSettings{},false,"line "+std::to_string(line_no)+": "+e.what()};
    }
}
std::string serialize_settings(const ClientSettings& s) {
    std::ostringstream out;
    out<<std::setprecision(9);
    out<<"# SlinkyWinky standalone profile; no secrets or account data\n";
    out<<"version=1\n";
    const auto& c=s.combat;
    put_bool(out,"auto_clicker_enabled",c.auto_clicker_enabled);
    put_float(out,"clicks_per_second",c.clicks_per_second);
    put_bool(out,"require_mouse_down",c.require_mouse_down);
    put_bool(out,"aim_enabled",c.aim_enabled);
    put_float(out,"aim_range",c.aim_range);
    put_float(out,"aim_horizontal_speed",c.aim_horizontal_speed);
    put_float(out,"aim_vertical_speed",c.aim_vertical_speed);
    put_float(out,"aim_fov",c.aim_fov);
    put_bool(out,"aim_requires_mouse",c.aim_requires_mouse);
    put_bool(out,"auto_block_enabled",c.auto_block_enabled);
    put_float(out,"block_range",c.block_range);
    put_bool(out,"block_requires_damage",c.block_requires_damage);
    put_float(out,"block_max_hurt_time",c.block_max_hurt_time);
    put_bool(out,"reach_enabled",c.reach_enabled);
    put_float(out,"reach_distance",c.reach.distance);
    put_float(out,"reach_activation_ticks",c.reach.activation_ticks);
    put_bool(out,"reach_require_sprint",c.reach.require_sprint);
    put_bool(out,"hit_select_enabled",c.hit_select_enabled);
    out<<"hit_select_min_hurt="<<c.hit_select_min_hurt<<'\n';
    put_bool(out,"velocity_enabled",s.velocity.enabled);
    put_float(out,"velocity_horizontal",s.velocity.horizontal);
    put_float(out,"velocity_vertical",s.velocity.vertical);
    put_float(out,"velocity_chance_percent",s.velocity.chance_percent);
    put_bool(out,"backtrack_enabled",s.backtrack_enabled);
    out<<"backtrack_ms="<<s.backtrack_ms<<'\n';
    return out.str();
}
SettingsResult load_settings(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file) return {ClientSettings{},false,"could not open settings file"};
    std::ostringstream buffer; buffer<<file.rdbuf();
    if(!file.eof() && file.fail()) return {ClientSettings{},false,"could not read settings file"};
    return parse_settings(buffer.str());
}
bool save_settings(const std::filesystem::path& path, const ClientSettings& settings,
                   std::string* error) {
    const auto serialized=serialize_settings(settings);
    const auto validated=parse_settings(serialized);
    if (!validated.valid) {
        if(error) *error="refusing to persist invalid settings: "+validated.error;
        return false;
    }
    const auto temporary=std::filesystem::path(path.string()+".tmp");
    try {
        if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        {
            std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
            if(!out) throw std::runtime_error("unable to create temporary file");
            out<<serialized;
            out.flush();
            if(!out) throw std::runtime_error("temporary file write failed");
        }
        // Write to temporary location first; rename can fail without touching the original.
        // Windows cannot atomically replace an existing target via standard rename.
        const auto backup=std::filesystem::path(path.string()+".bak");
        if (std::filesystem::exists(path)) {
            std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing);
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(),path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("atomic Windows profile replace failed");
#else
        std::filesystem::rename(temporary,path);
#endif
        return true;
    } catch (const std::exception& e) {
        if(error) *error=e.what();
        std::error_code ignored;
        std::filesystem::remove(temporary,ignored);
        const auto backup=std::filesystem::path(path.string()+".bak");
        if(!std::filesystem::exists(path) && std::filesystem::exists(backup))
            std::filesystem::copy_file(backup,path,std::filesystem::copy_options::overwrite_existing,ignored);
        return false;
    }
}
} // namespace slinkywinky
