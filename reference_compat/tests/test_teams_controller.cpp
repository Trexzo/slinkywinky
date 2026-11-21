#include "slinkywinky/teams_controller.hpp"
#include <cstdio>
#include <cstdlib>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do { if(!(x)) {std::fprintf(stderr,"R266 Teams fail %d: %s\n",__LINE__,#x);std::exit(1);} } while(0)
int main(){
    CHECK(TeamsController::color_code(u'0'));
    CHECK(TeamsController::color_code(u'f'));
    CHECK(TeamsController::color_code(u'r'));
    CHECK(!TeamsController::color_code(u'F'));
    CHECK(TeamsController::name_character(u'_'));
    CHECK(!TeamsController::name_character(u'-'));
    CHECK(!TeamsController::name_character(u'Á'));
    CHECK(TeamsController::prefix_color(u"§aUser")=='a');
    CHECK(TeamsController::prefix_color(u"§l§6Name")=='6');
    CHECK(TeamsController::prefix_color(u"§4One§aTwo")=='4');
    CHECK(TeamsController::prefix_color(u"Name§a")=='\0');
    CHECK(TeamsController::prefix_color(u"§FName")=='\0');
    CHECK(TeamsController::prefix_color(u"§r_Name")=='r');
    CHECK(TeamsController::trailing_color(u"§cTag")=='c');
    CHECK(TeamsController::trailing_color(u"NoColor")=='f');
    CHECK(TeamsController::trailing_color(u"§l§4Tag")=='4');
    CHECK(TeamsController::trailing_color(u"Tag§F")=='f');
    CHECK(TeamsController::trailing_color(u"§rReset")=='r');
    for(int i=0;i<10;++i)CHECK(TeamsController::color_index(static_cast<char>('0'+i))==i);
    for(int i=0;i<6;++i)CHECK(TeamsController::color_index(static_cast<char>('a'+i))==10+i);
    CHECK(TeamsController::color_index('r')==-1);
    CHECK(TeamsController::color_index('A')==-1);
    CHECK(TeamsController::color_index('\0')==-1);
    TeamsEntityName self{u"§cMe",std::nullopt};
    TeamsOwnerName owner{self,false,false,u"",std::nullopt};
    TeamsEntityName remote{u"§cOther",std::nullopt};
    TeamsSettings settings;
    auto d=TeamsController::on_event0(false,owner,remote,settings);
    CHECK(!d.admitted && !d.proposed_team_member);
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.admitted && !d.proposed_team_member && d.owner_color=='c' && d.target_color_index==12 && d.relationship_flag==1);
    settings.match_self_color=true;
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.proposed_team_member);
    remote.raw_name=u"§9Enemy";
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(!d.proposed_team_member);
    settings.named_colors[9]=true;
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.proposed_team_member);
    owner.player_info_present=true;
    owner.scoreboard_team_present=true;
    owner.scoreboard_prefix=u"§9Team";
    settings.match_self_color=true;
    settings.named_colors[9]=false;
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.proposed_team_member && d.owner_color=='9');
    owner.scoreboard_prefix=u"NoColor";
    remote.raw_name=u"§rOther";
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.proposed_team_member && d.owner_color=='r' && d.target_color_index==-1);
    owner.scoreboard_team_present=false;
    owner.plain_player_info_display=u"§6Info";
    remote.raw_name=u"Other";
    remote.plain_display_name=u"§6Other";
    d=TeamsController::on_event0(true,owner,remote,settings);
    CHECK(d.proposed_team_member && d.owner_color=='6' && d.target_color=='6');
    CHECK(TeamsController::on_disable_clear_mask()==1u);
    CHECK(TeamsController::on_unload_clear_mask()==1u);
    std::puts("R266_TEAMS_SOURCE_COLOR_CLASSIFICATION_PASS");
}
