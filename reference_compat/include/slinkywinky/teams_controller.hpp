#pragma once
// R266: first-party, offline-only reconstruction of Teams color evidence.
// Source semantics derived from recovered TeamsModule.java; this header
// does NOT alter Minecraft RelationshipService, chat, entities or teams.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace slinkywinky::reference_compat {
struct TeamsEntityName {
    std::u16string raw_name{};
    std::optional<std::u16string> plain_display_name{};
};
struct TeamsOwnerName {
    TeamsEntityName entity{};
    bool player_info_present=false;
    bool scoreboard_team_present=false;
    std::u16string scoreboard_prefix{};
    std::optional<std::u16string> plain_player_info_display{};
};
struct TeamsSettings {
    bool match_self_color=false;
    std::array<bool,16> named_colors{};
};
struct TeamsDecision {
    bool admitted=false;
    bool proposed_team_member=false;
    char owner_color='\0',target_color='\0';
    int target_color_index=-1;
    unsigned relationship_flag=1;
};
class TeamsController final {
public:
    // Java char comparisons: only lowercase a-f and r count. Unicode
    // formatting introducer is U+00A7, not the byte 0xA7.
    static constexpr bool color_code(char16_t c) noexcept {
        return (c>=u'0' && c<=u'9') || (c>=u'a' && c<=u'f') || c==u'r';
    }
    static constexpr bool name_character(char16_t c) noexcept {
        return c==u'_' || (c>=u'0' && c<=u'9') ||
               ((c>=u'A' && c<=u'Z') || (c>=u'a' && c<=u'z'));
    }
    static char prefix_color(std::u16string_view s) noexcept {
        char result='\0';
        bool section=false;
        for(char16_t c:s) {
            const bool escaped=section;
            section=(c==u'\u00a7');
            if(c!=u'\u00a7') {
                if(escaped) {
                    if(color_code(c))result=static_cast<char>(c);
                }else if(name_character(c)) {
                    return result;
                }
            }
        }
        return '\0';
    }
    static char trailing_color(std::u16string_view s) noexcept {
        char selected='\0';
        bool valid=false;
        for(std::size_t i=s.size();i>0;--i) {
            const char16_t c=s[i-1];
            if(c==u'\u00a7') {
                if(valid)return selected;
                valid=false;
            }else{
                valid=color_code(c);
                if(valid)selected=static_cast<char>(c);
            }
        }
        return 'f';
    }
    static char entity_color(const TeamsEntityName& e) noexcept {
        const char prefix=prefix_color(e.raw_name);
        if(prefix!='\0')return prefix;
        return e.plain_display_name?trailing_color(*e.plain_display_name):'f';
    }
    static char owner_color(const TeamsOwnerName& o) noexcept {
        char result=entity_color(o.entity);
        if(o.player_info_present) {
            if(o.scoreboard_team_present) {
                result=prefix_color(o.scoreboard_prefix);
                if(result=='\0')result='r';
            }else if(o.plain_player_info_display) {
                result=trailing_color(*o.plain_player_info_display);
            }
        }
        return result;
    }
    static int color_index(char c) noexcept {
        const std::uint8_t low=static_cast<std::uint8_t>(c);
        const auto signed_low=static_cast<std::int8_t>(low);
        const unsigned result=static_cast<unsigned>(
            (signed_low>=97?169:208)+static_cast<int>(low))&255u;
        return result<16u?static_cast<int>(result):-1;
    }
    static TeamsDecision on_event0(bool world_ready,
                                   const TeamsOwnerName& owner,
                                   const TeamsEntityName& remote,
                                   const TeamsSettings& settings) noexcept {
        TeamsDecision out{};
        if(!world_ready)return out;
        out.admitted=true;
        out.owner_color=owner_color(owner);
        out.target_color=entity_color(remote);
        out.target_color_index=color_index(out.target_color);
        out.proposed_team_member=settings.match_self_color &&
                                 out.target_color==out.owner_color;
        if(!out.proposed_team_member && out.target_color_index>=0) {
            out.proposed_team_member=settings.named_colors[
                static_cast<std::size_t>(out.target_color_index)];
        }
        return out;
    }
    // Recovered onDisable/onWorldUnload calls RelationshipService.clear(1).
    // These value-only signals cannot touch the actual relationship table.
    static unsigned on_disable_clear_mask() noexcept { return 1u; }
    static unsigned on_unload_clear_mask() noexcept { return 1u; }
};
} // namespace slinkywinky::reference_compat
