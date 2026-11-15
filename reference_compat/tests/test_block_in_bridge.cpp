#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void check(bool yes,const char* what,int line) {
    if(!yes)throw std::runtime_error(
        "R230 Block In line "+std::to_string(line)+": "+what);
}
#define CHECK(x) check(!!(x),#x,__LINE__)

static BlockInWorldInput simple() {
    BlockInWorldInput w;
    w.in_game=true;w.on_ground=true;
    w.rotation_priority_granted=true;w.held_block_item=true;
    w.player_x=0.5;w.player_y=64.0;w.player_z=0.5;
    w.diff_yaw=0;w.diff_pitch=0;
    for(auto& roof:w.roof_block_ids)roof=0u;
    for(unsigned i=0;i<4;++i) {
        w.wall_intersects_player[i]=false;
        w.wall_block_ids[i]={1u,0u,1u,1u};
    }
    return w;
}
static void model_tests() {
    CHECK(kBlockInRoofOffsets[0].y==3 &&
          kBlockInRoofOffsets[9].y==4);
    CHECK(kBlockInRoofOffsets[5].x==-1 &&
          kBlockInRoofOffsets[5].z==-1);
    CHECK(kBlockInWalls[0].z==1 &&
          kBlockInWalls[1].x==-1 &&
          kBlockInWalls[2].z==-1 &&
          kBlockInWalls[3].x==1);
    for(const unsigned i:{0u,8u,9u,31u})
        CHECK(block_in_is_open_id(i));
    for(const unsigned i:{1u,7u,10u,30u,32u,255u})
        CHECK(!block_in_is_open_id(i));
    CHECK(block_in_middle(.31,.69) && block_in_middle(.5,.5));
    CHECK(!block_in_middle(.30,.5) &&
          !block_in_middle(.70,.5));
    BlockInController m;
    CHECK(!m.on_rotation().override_yaw);
    CHECK(!m.on_movement().proposed_override_keys);
    CHECK(!m.on_toggle({false,true,90.0f,{}}).admitted);
    auto enabled=m.on_toggle({true,true,90.0f,{}});
    CHECK(enabled.admitted && enabled.remembered_quadrant==1);
    auto w=simple();
    const auto placement=m.on_tick(w);
    CHECK(placement.admitted && placement.active && !placement.proposed_disable);
    CHECK(placement.proposed_placement_bit==3u &&
          placement.proposed_target_offset);
    CHECK(placement.proposed_target_offset->x==-1 &&
          placement.proposed_click);
    CHECK((placement.placed_mask&(1ull<<3))!=0);
    CHECK(placement.movement_mode==2);
    CHECK(m.on_rotation().override_yaw && m.on_rotation().override_pitch);
    auto stop=m.on_movement();
    CHECK(stop.proposed_override_keys && !stop.proposed_keys[0] &&
          !stop.proposed_keys[1] && !stop.proposed_keys[2] &&
          !stop.proposed_keys[3]);

    // Source: preexisting roof solid at index1 (when slot0 is open)
    // wins before any wall. Bit33 is set only on a successful click.
    w.roof_block_ids[1]=1u;
    const auto roof=m.on_tick(w);
    CHECK(roof.proposed_placement_bit==33u &&
          roof.proposed_click &&
          (m.placed_mask()&(1ull<<33))!=0);
    // Y difference >.01 resets all placed marks.
    w.player_y+=0.02;
    const auto reset_height=m.on_tick(w);
    CHECK(reset_height.proposed_placement_bit==33u &&
          reset_height.proposed_click);
    CHECK((reset_height.placed_mask&(1ull<<3))==0);
    // If roof slot0 is occupied, source falls back to wall search.
    w.roof_block_ids[0]=1u;
    const auto roof_zero=m.on_tick(w);
    CHECK(roof_zero.proposed_placement_bit==3u &&
          roof_zero.proposed_click);

    // Different camera angle: too far to click this tick, yet walking
    // mode must orient toward center and stop alternate movement keys.
    m.reset();
    CHECK(m.on_toggle({true,true,0.0f,{}}).admitted);
    w=simple();w.player_x=0.2;w.player_z=0.2;
    w.diff_yaw=160;w.diff_pitch=90;w.aim_speed=0.05f;
    const auto walk=m.on_tick(w);
    CHECK(walk.admitted && walk.active && !walk.proposed_click &&
          walk.movement_mode==1 &&
          walk.proposed_yaw_delta && walk.proposed_pitch_delta);
    m.set_center_rotation_for_preview(55.0f,80.0f);
    const auto rot=m.on_rotation();
    CHECK(rot.override_yaw && rot.override_pitch &&
          rot.proposed_yaw==55.0f && rot.proposed_pitch==80.0f);
    const auto move=m.on_movement();
    CHECK(move.proposed_override_keys && move.proposed_keys[0] &&
          !move.proposed_keys[1] && !move.proposed_keys[2] &&
          !move.proposed_keys[3]);

    // Missing actual world block id cannot be treated as air.
    m.reset();w=simple();w.roof_block_ids[1].reset();
    CHECK(!m.on_tick(w).admitted);
    m.reset();w=simple();w.roof_block_ids[0]=1u;
    w.wall_block_ids[0][0].reset();
    CHECK(!m.on_tick(w).admitted);
    m.reset();w=simple();
    w.wall_intersects_player[0].reset();
    CHECK(!m.on_tick(w).admitted);
    m.reset();w=simple();
    w.roof_block_ids[0]=4096u;
    CHECK(!m.on_tick(w).admitted);
    m.reset();w=simple();
    w.player_x=std::numeric_limits<double>::quiet_NaN();
    CHECK(!m.on_tick(w).admitted);
    m.reset();w=simple();
    w.aim_speed=std::numeric_limits<float>::infinity();
    CHECK(!m.on_tick(w).admitted);

    // Source requires ground if setting enabled, otherwise allows air.
    m.reset();w=simple();w.require_ground=true;w.on_ground=false;
    const auto denied=m.on_tick(w);
    CHECK(denied.admitted && denied.proposed_disable && !denied.active);
    m.reset();w=simple();w.require_ground=false;w.on_ground=false;
    CHECK(m.on_tick(w).active);
    m.reset();w=simple();w.rotation_priority_granted=false;
    CHECK(m.on_tick(w).proposed_disable && !m.on_rotation().override_yaw);
    m.reset();w=simple();w.held_block_item=false;
    CHECK(m.on_tick(w).proposed_disable);

    // No valid world block placement: disable only when grounded &
    // centered. Source needs source-backed empties with no walls.
    m.reset();w=simple();
    for(auto& c:w.wall_intersects_player)c=true;
    CHECK(m.on_tick(w).proposed_disable);
    w.on_ground=false;
    CHECK(!m.on_tick(w).proposed_disable);
    w.on_ground=true;w.player_x=0.1;
    CHECK(!m.on_tick(w).proposed_disable);
    const std::array<bool,4> physical={true,false,true,false};
    const auto off=m.on_toggle({true,false,0.0f,physical});
    CHECK(off.admitted && off.proposed_restore_keys &&
          off.restored_keys==physical);
    CHECK(!m.on_rotation().override_yaw &&
          !m.on_movement().proposed_override_keys);
    CHECK(m.on_empty_tick());
}
static void bridge_tests() {
    ModuleExecutionBridge bridge;
    const auto safe=bridge.add({"Safe Walk",1,{{0,0}},false});
    const auto place=bridge.add({"Fast Place",3,{{0,-1000}},false});
    const auto mine=bridge.add({"Fast Mine",3,{{0,0},{18,0}},false});
    const auto clutch=bridge.add({"Clutch",3,{{0,0},{13,-100},{18,-1000}},false});
    const auto block=bridge.add({"Block In",3,
        {{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false});
    CHECK(bridge.bind_safe_walk_from_catalog() &&
          bridge.bind_fast_place_from_catalog() &&
          bridge.bind_fast_mine_from_catalog() &&
          bridge.bind_clutch_from_catalog() &&
          bridge.bind_block_in_from_catalog());
    CHECK(!bridge.bind_block_in(block));
    const auto coverage=bridge.coverage();
    CHECK(coverage.executable_callback_bindings==12);
    CHECK(coverage.implemented_per_event[0]==5 &&
          coverage.implemented_per_event[5]==1 &&
          coverage.implemented_per_event[13]==2 &&
          coverage.implemented_per_event[15]==1 &&
          coverage.implemented_per_event[16]==1);
    for(auto id:{safe,place,mine,clutch,block}) CHECK(bridge.request(id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));
    ExecutionFrame f;
    f.event=5;f.world_present=f.player_present=true;
    f.notification_target=block;f.notification_new_state=true;
    f.block_in_toggle=BlockInTogglePayload{{true,true,90.0f,{}},{},0};
    CHECK(bridge.dispatch(f) && f.block_in_toggle->executions==1 &&
          f.block_in_toggle->decision.remembered_quadrant==1);
    f=ExecutionFrame{};
    f.event=16;f.world_present=f.player_present=true;
    f.block_in_tick=BlockInTickPayload{simple(),{},0};
    CHECK(bridge.dispatch(f) && f.block_in_tick->executions==1 &&
          f.block_in_tick->decision.proposed_click);
    f=ExecutionFrame{};
    f.event=13;f.world_present=f.player_present=true;
    f.clutch_rotation=ClutchRotationPayload{};
    f.block_in_rotation=BlockInRotationPayload{};
    CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{clutch,block}));
    CHECK(f.clutch_rotation->executions==1 &&
          f.block_in_rotation->executions==1 &&
          f.block_in_rotation->decision.override_yaw);
    f=ExecutionFrame{};
    f.event=15;f.world_present=f.player_present=true;
    f.block_in_movement=BlockInMovementPayload{};
    CHECK(bridge.dispatch(f) && f.block_in_movement->executions==1);
    f=ExecutionFrame{};
    f.event=0;f.world_present=f.player_present=true;
    f.block_in_empty=BlockInEmptyPayload{};
    f.safe_walk=SafeWalkTickPayload{};
    f.safe_walk->input.in_game=true;
    f.fast_mine_early=FastMineEarlyPayload{};
    f.fast_mine_early->input.in_game=true;
    f.clutch_click=ClutchClickPayload{};
    f.clutch_click->input.in_game=true;
    f.fast_place=FastPlaceTickPayload{};
    f.fast_place->input.in_game=true;
    CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{
        safe,mine,clutch,block,place}));
    CHECK(f.block_in_empty->executions==1);
    // Wrong source event, missing world and foreign toggle are denied.
    f.event=16;CHECK(!bridge.dispatch(f));
    f=ExecutionFrame{};
    f.event=16;f.world_present=false;f.player_present=true;
    f.block_in_tick=BlockInTickPayload{simple(),{},0};
    CHECK(!bridge.dispatch(f));
    f=ExecutionFrame{};
    f.event=5;f.world_present=f.player_present=true;
    f.notification_target=safe;f.notification_new_state=false;
    f.block_in_toggle=BlockInTogglePayload{};
    CHECK(bridge.dispatch(f) && f.block_in_toggle->executions==1 &&
          !f.block_in_toggle->decision.admitted);
    CHECK(bridge.request(block,false));
    CHECK(bridge.apply(true,true));
    f=ExecutionFrame{};
    f.event=16;f.world_present=f.player_present=true;
    f.block_in_tick=BlockInTickPayload{simple(),{},0};
    CHECK(bridge.dispatch(f) && f.block_in_tick->executions==0);
    bridge.reset_block_in_world_state();
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    for(const auto& wrong:std::vector<ModuleSpec>{
        {"Block In",1,{{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false},
        {"Block In",3,{{0,1},{5,0},{13,-2000},{15,-1000},{16,-500}},false},
        {"Block In",3,{{0,0},{5,10},{13,-2000},{15,-1000},{16,-500}},false},
        {"Block In",3,{{0,0},{5,0},{13,-1999},{15,-1000},{16,-500}},false},
        {"Block In",3,{{0,0},{5,0},{13,-2000},{15,-999},{16,-500}},false},
        {"Block In",3,{{0,0},{5,0},{13,-2000},{15,-1000},{16,-501}},false},
        {"Block In",3,{{0,0},{5,0},{13,-2000},{15,-1000}},false},
        {"Block In",3,{{0,0},{5,0},{13,-2000},{15,-1000},{16,-500},{18,0}},false},
        {"Other",3,{{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false},
    }) {
        ModuleExecutionBridge bad;
        const auto id=bad.add(wrong);
        CHECK(!bad.bind_block_in(id));
        CHECK(!bad.bind_block_in_from_catalog());
    }
    std::cout<<"R230_BLOCK_IN_FIVE_SOURCE_CALLBACKS_PASS "
                "roofFirst=1 wallMask=1 playerCenter=1 "
                "metadataGuards=1 gameWrites=0\n";
}
int main(){
    try { model_tests();bridge_tests();return 0; }
    catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";return 1;
    }
}
