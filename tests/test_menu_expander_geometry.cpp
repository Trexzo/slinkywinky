#include "slinkywinky/menu_expander_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x))throw std::runtime_error(\
    "R154 expander source geometry "+std::to_string(__LINE__));}while(false)
using namespace slinkywinky;
int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuRenderSession session;
    CHECK(session.attach(154,900,630));
    auto frame=session.next(menu,&profile);
    CHECK(frame && !frame->visible.empty());
    const auto card=menu_module_card_rect(*frame,900.0f,0);
    const auto exp=menu_expander_geometry(*frame,900.0f,0);
    CHECK(exp && !exp->compact);
    CHECK(exp->x==card.x+8.0f && exp->y==card.y+40.0f);
    CHECK(exp->height==20.0f);
    CHECK(exp->plus_x==exp->x+exp->text_width+8.0f);
    CHECK(exp->width==exp->text_width+28.0f);
    CHECK(exp->x+exp->width<=card.x+card.width-8.0f+0.01f);
    CHECK(menu_expander_contains(*exp,exp->x+1,exp->y+10));
    CHECK(menu_expander_contains(*exp,exp->plus_x+10,exp->y+10));
    CHECK(!menu_expander_contains(*exp,exp->x-1,exp->y+10));
    CHECK(!menu_expander_contains(*exp,exp->x+exp->width,exp->y+10));
    CHECK(!menu_expander_contains(*exp,exp->x+20,exp->y+20));
    const auto textHit=hit_test_menu(*frame,MenuPanelMode::StandalonePreview,
        static_cast<int>(exp->x+4),static_cast<int>(exp->y+10));
    CHECK(textHit && textHit->kind==MenuHitKind::Expand &&
          textHit->catalog_index==0);
    const auto iconHit=hit_test_menu(*frame,MenuPanelMode::StandalonePreview,
        static_cast<int>(exp->plus_x+10),static_cast<int>(exp->y+10));
    CHECK(iconHit && iconHit->kind==MenuHitKind::Expand);
    CHECK(!menu_expander_geometry(*frame,900.0f,100));
    auto compact=*frame;
    compact.appearance.compact=true;
    const auto c=menu_expander_geometry(compact,900.0f,0);
    CHECK(c && c->compact && c->y==card.y+8.0f);
    CHECK(c->plus_x>c->x+50.0f);
    CHECK(session.detach());
    return 0;
}
