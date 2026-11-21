#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool yes,const char* what,int line) {
    if(!yes)throw std::runtime_error("R254 FreeLook "+std::to_string(line)+": "+what);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static FreeLookFrame observed(std::uint64_t world=101,std::uint64_t player=202){
    FreeLookFrame f;f.game_ready=true;f.world_token=world;f.player_token=player;
    f.current_perspective=0;f.target_perspective=2;
    f.stored_camera_valid=true;f.stored_camera_yaw=141.0f;
    f.stored_camera_pitch=-27.0f;f.player_yaw=11.0f;f.player_pitch=5.0f;
    return f;
}
static ExecutionFrame frame13(const FreeLookFrame& f) {
    ExecutionFrame f13;f13.event=13;
    f13.world_present=f13.player_present=true;
    f13.free_look_rotation=FreeLookRotationPayload{f,{},0};
    return f13;
}
int main(){
    ModuleExecutionBridge b;
    const auto fl=b.add({"Free Look",2,{{13,0}},false});
    const auto clutch=b.add({"Clutch",3,{{0,0},{13,-100},{18,-1000}},false});
    const auto block=b.add({"Block In",3,{{0,0},{5,0},{13,-2000},
                                          {15,-1000},{16,-500}},false});
    CHECK(b.bind_free_look_from_catalog() && b.bind_clutch_from_catalog() &&
          b.bind_block_in_from_catalog());
    CHECK(!b.bind_free_look(fl));
    CHECK(b.coverage().implemented_per_event[13]==3);
    CHECK(!b.preview_free_look_enable(observed()).admitted);
    CHECK(b.request(fl,true) && b.request(clutch,true) && b.request(block,true));
    CHECK(!b.apply(false,true));CHECK(b.apply(true,true));
    auto e=frame13(observed());
    e.clutch_rotation=ClutchRotationPayload{};
    e.block_in_rotation=BlockInRotationPayload{};
    CHECK(b.dispatch(e));
    CHECK((e.executed_modules==std::vector<std::size_t>{fl,clutch,block}));
    CHECK(e.free_look_rotation->executions==1);
    auto d=e.free_look_rotation->decision;
    CHECK(d.admitted && d.rotation_active && d.set_perspective &&
          d.perspective==2 && d.yaw==141.0f && d.pitch==-27.0f);
    CHECK(e.clutch_rotation->executions==1 && e.block_in_rotation->executions==1);
    e=frame13(observed());CHECK(b.dispatch(e));
    CHECK(e.free_look_rotation->executions==1 &&
          !e.free_look_rotation->decision.set_perspective);
    e=frame13(observed());e.event=14;CHECK(!b.dispatch(e));
    CHECK(e.free_look_rotation->executions==0);
    auto restored=b.preview_free_look_disable(101,202);
    CHECK(restored.admitted && restored.restore_perspective &&
          restored.restore_camera && restored.perspective==0);
    CHECK(!b.preview_free_look_disable(101,202).restore_camera);
    e=frame13(observed());CHECK(b.dispatch(e));
    restored=b.preview_free_look_disable(101,999);
    CHECK(restored.admitted && !restored.restore_camera);
    e=frame13(observed());CHECK(b.dispatch(e) &&
          e.free_look_rotation->decision.set_perspective);
    restored=b.preview_free_look_world_unload(101,202);
    CHECK(restored.restore_camera && restored.restore_perspective);
    e=frame13(observed());CHECK(b.dispatch(e));
    CHECK(b.request(fl,false) && b.apply(true,true));
    CHECK(!b.preview_free_look_disable(101,202).admitted);
    CHECK(b.request(fl,true) && b.apply(true,true));
    e=frame13(observed());CHECK(b.dispatch(e) &&
          e.free_look_rotation->decision.set_perspective);
    e=frame13(observed(303,404));CHECK(b.dispatch(e));
    CHECK(!e.free_look_rotation->decision.admitted);
    e=frame13(observed(303,404));CHECK(b.dispatch(e) &&
          e.free_look_rotation->decision.set_perspective);
    ModuleExecutionBridge wrong;
    auto wrongCategory=wrong.add({"Free Look",1,{{13,0}},true});
    auto wrongName=wrong.add({"Other",2,{{13,0}},true});
    CHECK(!wrong.bind_free_look(wrongCategory));
    CHECK(!wrong.bind_free_look(wrongName));
    ModuleExecutionBridge invalid;
    auto wrongEvents=invalid.add({"Free Look",2,{{13,0},{14,0}},true});
    CHECK(!invalid.bind_free_look(wrongEvents));
    ModuleExecutionBridge wrongSlot;
    auto wrongEvent=wrongSlot.add({"Free Look",2,{{14,0}},true});
    CHECK(!wrongSlot.bind_free_look(wrongEvent));
    b.stop();
    CHECK(!b.preview_free_look_disable(303,404).admitted);
    e=frame13(observed(303,404));CHECK(!b.dispatch(e));
    std::cout<<"R254_OFFLINE_FREE_LOOK_EVENT13_BRIDGE_PASS\n";
}
