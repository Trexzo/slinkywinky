#include "slinkywinky/hitboxes_controller.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x)){\
 std::fprintf(stderr,"R264 Hitboxes line %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(false)
int main(){
    auto r=HitboxesController::on_event8({true,0.15f,true});
    CHECK(r.admitted && r.proposed_enable_event &&
          r.proposed_expansion==static_cast<double>(0.15f) &&
          r.proposed_status_visible);
    r=HitboxesController::on_event8({true,0.0f,false});
    CHECK(r.admitted && r.proposed_enable_event &&
          r.proposed_expansion==0.0 && !r.proposed_status_visible);
    r=HitboxesController::on_event8({false,2.5f,true});
    CHECK(r.admitted && !r.proposed_enable_event &&
          r.proposed_expansion==0.0 && r.proposed_status_visible);
    // A status-setting flag never authorizes an input event itself.
    r=HitboxesController::on_event8({false,1.0f,false});
    CHECK(r.admitted && !r.proposed_enable_event);
    // Preserve exact original float->double promotion (not a rounded GUI
    // string), including a negative setting when policy supplied it.
    r=HitboxesController::on_event8({true,-0.25f,false});
    CHECK(r.admitted && r.proposed_enable_event &&
          r.proposed_expansion==-0.25);
    for(float bad:{std::numeric_limits<float>::quiet_NaN(),
                   std::numeric_limits<float>::infinity(),
                   -std::numeric_limits<float>::infinity(),
                   1000001.0f}) {
        r=HitboxesController::on_event8({true,bad,true});
        CHECK(!r.admitted && !r.proposed_enable_event &&
              !r.proposed_status_visible);
    }
    std::puts("R264_HITBOXES_EVENT8_EXPANSION_READONLY_PASS");
}
