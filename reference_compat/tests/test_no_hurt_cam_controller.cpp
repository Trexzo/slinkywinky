#include "slinkywinky/no_hurt_cam_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x)){\
 std::fprintf(stderr,"R257 NoHurtCam line %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(false)
static NoHurtCamObservation obs(int value=10,
                                std::uint64_t world=7,std::uint64_t player=11){
    return {true,world,player,value};
}
int main(){
    NoHurtCamController c;
    auto first=c.on_pre_render(obs());
    CHECK(first.admitted && first.apply_sentinel &&
          first.proposed_value==std::numeric_limits<int>::max() &&
          c.held());
    // Multiple frames observing the sentinel preserve initial value.
    auto second=c.on_pre_render(obs(NoHurtCamController::kSentinel));
    CHECK(second.admitted && second.apply_sentinel && c.held());
    auto restore=c.on_disable(obs(NoHurtCamController::kSentinel));
    CHECK(restore.admitted && restore.restore_original &&
          restore.proposed_value==10 && !c.held());
    CHECK(!c.on_disable(obs()).restore_original);
    // If another subsystem changed field from MAX to 6 while held,
    // source snapshot updates when it later re-applies its sentinel.
    c.on_pre_render(obs(10));
    c.on_pre_render(obs(6));
    restore=c.on_world_unload(obs(NoHurtCamController::kSentinel));
    CHECK(restore.restore_original && restore.proposed_value==6);
    // Disable must NOT overwrite a newer non-sentinel value.
    c.on_pre_render(obs(10));
    restore=c.on_disable(obs(8));
    CHECK(restore.admitted && !restore.restore_original && !c.held());
    // Unexpected owner changes reject old snapshots; no cross-world restore.
    c.on_pre_render(obs(10));
    auto changed=c.on_pre_render(obs(3,7,999));
    CHECK(!changed.admitted && !changed.apply_sentinel && !c.held());
    CHECK(!c.on_disable(obs(NoHurtCamController::kSentinel,7,999))
                 .restore_original);
    c.on_pre_render(obs(10));
    restore=c.on_disable(obs(NoHurtCamController::kSentinel,9,11));
    CHECK(!restore.admitted && !restore.restore_original && !c.held());
    // No game world, no speculative field changes.
    NoHurtCamObservation notReady{false,0,0,10};
    first=c.on_pre_render(notReady);
    CHECK(first.admitted && !first.apply_sentinel && !c.held());
    // A malformed field value is rejected without gaining ownership.
    first=c.on_pre_render(obs(-1));
    CHECK(!first.admitted && !c.held());
    // First observation is already sentinel: preserve exactly that
    // value rather than inventing an earlier field value.
    c.on_pre_render(obs(NoHurtCamController::kSentinel));
    restore=c.on_disable(obs(NoHurtCamController::kSentinel));
    CHECK(restore.restore_original &&
          restore.proposed_value==NoHurtCamController::kSentinel);
    c.reset();CHECK(!c.held());
    std::puts("R257_NO_HURT_CAM_FIELD_RESTORE_NEGATIVES_PASS");
}
