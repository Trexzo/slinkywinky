#include "slinkywinky/free_look_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do { if(!(x)){\
    std::fprintf(stderr,"R253 FreeLook FAILED line %d: %s\n",__LINE__,#x);\
    std::exit(1);} } while(0)

static FreeLookFrame frame() {
    FreeLookFrame f;
    f.game_ready=true;f.world_token=101;f.player_token=202;
    f.current_perspective=0;f.target_perspective=2;
    f.stored_camera_valid=true;
    f.stored_camera_yaw=141.0f;f.stored_camera_pitch=-27.0f;
    f.player_yaw=11.0f;f.player_pitch=5.0f;
    return f;
}
int main() {
    FreeLookController c;
    auto f=frame();
    auto e=c.on_enable(f);
    CHECK(e.admitted && e.set_perspective && e.perspective==2 &&
          !e.rotation_active && c.saved());
    auto r=c.on_event13(f);
    CHECK(r.admitted && !r.set_perspective && r.rotation_active &&
          r.yaw==141.0f && r.pitch==-27.0f);
    r=c.on_disable(101,202);
    CHECK(r.admitted && r.restore_perspective && r.restore_camera &&
          r.perspective==0 && r.yaw==141.0f && r.pitch==-27.0f &&
          !c.saved());
    CHECK(!c.on_disable(101,202).restore_camera);
    // No source camera cache: use observed player yaw/pitch.
    f.stored_camera_valid=false;
    r=c.on_event13(f);
    CHECK(r.set_perspective && r.rotation_active &&
          r.yaw==11.0f && r.pitch==5.0f);
    c.reset();
    // Explicit not-ready enable must not capture or change perspective.
    f.game_ready=false;
    r=c.on_enable(f);
    CHECK(r.admitted && !r.set_perspective && !c.saved());
    f.game_ready=true;
    r=c.on_event13(f);
    CHECK(r.rotation_active && r.set_perspective && c.saved());
    r=c.on_world_unload(101,202);
    CHECK(r.restore_camera && !c.saved());
    // Ownership swap cannot apply a stale camera state to new player/world.
    f=frame();c.on_event13(f);
    auto changed=f;changed.world_token=909;
    r=c.on_event13(changed);
    CHECK(!r.admitted && !r.rotation_active && !r.restore_camera &&
          !c.saved());
    r=c.on_event13(changed);
    CHECK(r.admitted && r.rotation_active && r.set_perspective);
    r=c.on_disable(101,202);
    CHECK(r.admitted && !r.restore_perspective && !r.restore_camera &&
          !c.saved());
    // Fail closed on malformed camera readings or invalid perspectives.
    f=frame();f.stored_camera_yaw=std::numeric_limits<float>::quiet_NaN();
    CHECK(!c.on_event13(f).admitted && !c.saved());
    f=frame();f.target_perspective=3;
    CHECK(!c.on_event13(f).admitted && !c.saved());
    f=frame();f.world_token=0;
    CHECK(!c.on_event13(f).admitted && !c.saved());
    // A stored perspective must be preserved even if caller changes it
    // after the first admitted event-13 activation.
    f=frame();f.current_perspective=1;
    c.on_event13(f);f.current_perspective=2;
    r=c.on_event13(f);
    CHECK(!r.set_perspective && r.rotation_active);
    r=c.on_disable(101,202);
    CHECK(r.restore_perspective && r.perspective==1);
    std::puts("R253_OFFLINE_FREE_LOOK_LIFECYCLE_PASS");
    return 0;
}
