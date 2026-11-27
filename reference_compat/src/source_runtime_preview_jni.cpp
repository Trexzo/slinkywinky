// R335: actual Java8 -> JNI -> existing four source C++ controllers.
// Read-only observations. No packet interception, timer writes or Java refs.
#include "slinkywinky/module_execution_bridge.hpp"
#include <jni.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace slinkywinky::reference_compat;
void require(bool ok,const char* msg){if(!ok)throw std::invalid_argument(msg);}
void java_error(JNIEnv* env,const char* klass,const char* msg){
    jclass c=env->FindClass(klass);if(c)env->ThrowNew(c,msg);
}
bool flag(const std::vector<jint>& a,int i){
    require(a.at(static_cast<std::size_t>(i))==0||
            a.at(static_cast<std::size_t>(i))==1,"Invalid JNI bool");
    return a[static_cast<std::size_t>(i)]!=0;
}
int enum_value(const std::vector<jint>& a,int i,int max){
    const int n=a.at(static_cast<std::size_t>(i));
    require(n>=0&&n<=max,"Invalid native opcode enum");
    return n;
}
float finite_float(const std::vector<jdouble>& a,int i){
    const double n=a.at(static_cast<std::size_t>(i));
    require(std::isfinite(n)&&n>=-1000000&&n<=1000000,"Invalid native float");
    return static_cast<float>(n);
}
std::int64_t millis(const std::vector<jdouble>& a,int i){
    const double n=a.at(static_cast<std::size_t>(i));
    require(n>=0&&n<=1000000000&&std::floor(n)==n,"Invalid packet age");
    return static_cast<std::int64_t>(n);
}
struct Session {
    std::thread::id owner=std::this_thread::get_id();
    ModuleExecutionBridge bridge{};
    jlong world=0,player=0,generation=0,tick=-1;
    std::array<std::size_t,4> module{};
    Session(){
        module[0]=bridge.add({"Backtrack",0,{{2,0},{6,0},{18,0}},false});
        module[1]=bridge.add({"Lag Range",0,{{3,-100},{6,0},{15,0},{18,0}},false});
        module[2]=bridge.add({"Timer Range",0,{{0,-100},{1,0},{3,0},{5,0}},false});
        module[3]=bridge.add({"Inv Walk",1,{{5,0},{17,0},{23,0},{25,0}},false});
        if(!bridge.bind_backtrack_source_tick(module[0])||
           !bridge.bind_backtrack_source_packet(module[0])||
           !bridge.bind_backtrack_source_render(module[0])||
           !bridge.bind_lag_range_source_tick(module[1])||
           !bridge.bind_lag_range_source_render(module[1])||
           !bridge.bind_lag_range_source_attack(module[1])||
           !bridge.bind_lag_range_source_packet(module[1])||
           !bridge.bind_timer_range_source_tick(module[2])||
           !bridge.bind_timer_range_source_packet(module[2])||
           !bridge.bind_timer_range_source_frame(module[2])||
           !bridge.bind_inv_walk_source_tick(module[3])||
           !bridge.bind_inv_walk_source_packet(module[3])||
           !bridge.bind_inv_walk_source_flush(module[3])||
           !bridge.bind_timer_range_source_toggle(module[2])||
           !bridge.bind_inv_walk_source_toggle(module[3]))
            throw std::logic_error("Source callback signature mismatch");
        for(auto m:module)
            if(!bridge.request(m,true))
                throw std::logic_error("Source module request failed");
        if(!bridge.apply(true,true))
            throw std::logic_error("Source module apply failed");
    }
    void observe_identity(const jlong id[4]){
        require(id[0]>0&&id[1]>0&&id[0]!=id[1]&&id[2]>0&&id[3]>=0,
                "Invalid world/player/generation/tick");
        if(generation!=0){
            require(id[2]>=generation,"Stale world generation");
            if(id[2]==generation)
                require(id[0]==world&&id[1]==player&&id[3]>=tick,
                        "Invalid owner switch or stale tick");
            else{
                bridge.reset_backtrack_source_world_state();
                bridge.reset_lag_range_source_world_state();
                bridge.reset_timer_range_source_world_state();
                bridge.reset_inv_walk_source_world_state();
            }
        }
        world=id[0];player=id[1];generation=id[2];tick=id[3];
    }
};
std::mutex mutex;
std::map<jlong,std::unique_ptr<Session>> sessions;
jlong next_id=1;
static constexpr int bits_by_op[11]={12,4,12,4,15,1,12,1,2,0,2};
static constexpr int vals_by_op[11]={7,1,6,4,7,20,1,0,0,0,0};
static constexpr unsigned event_by_op[11]={18,2,18,3,0,3,25,23,17,1,15};
std::array<double,8> dispatch(Session& s,int op,
                               const std::vector<jint>& bits,
                               const std::vector<jdouble>& vals){
    ExecutionFrame f{};
    f.event=event_by_op[op];f.world_present=f.player_present=true;
    std::array<double,8> result{{1,static_cast<double>(f.event),0,0,0,0,0,0}};
    switch(op){
    case 0:{
        BacktrackSourceTickPayload p{};
        p.input.in_game=flag(bits,0);p.input.weapon=flag(bits,1);
        p.input.target=flag(bits,2);p.input.player_ticks=bits[3];
        p.input.last_attack_tick=bits[4];p.input.target_id=bits[5];
        p.input.own_id=bits[6];p.input.target_hurt=bits[7];
        p.input.source_reach_eligible=flag(bits,8);
        p.settings.require_weapon=flag(bits,9);
        p.settings.render_remote_pos=flag(bits,10);
        p.settings.disable_on_hit=flag(bits,11);
        p.input.distance_sq=vals[0];
        p.input.real_position_minus_shown_distance=vals[1];
        p.settings.delay_ms=finite_float(vals,2);
        p.settings.cooldown_ticks=finite_float(vals,3);
        p.settings.min_range=finite_float(vals,4);
        p.settings.max_range=finite_float(vals,5);
        p.settings.max_target_hurt=finite_float(vals,6);
        f.backtrack_source_tick=p;
        require(s.bridge.dispatch(f),"Backtrack tick denied");
        const auto& d=*f.backtrack_source_tick;
        result[2]=d.executions;result[3]=d.active;result[4]=d.show_box;
        break;
    }
    case 1:{
        BacktrackSourcePacketPayload p{};
        p.input.kind=static_cast<BacktrackPacketType>(enum_value(bits,0,5));
        p.input.sequence=bits[1];p.input.entity_id=bits[2];
        p.input.prior_verdict_zero=flag(bits,3);
        p.input.held_ms=millis(vals,0);
        f.backtrack_source_packet=p;
        require(s.bridge.dispatch(f),"Backtrack packet denied");
        const auto& d=*f.backtrack_source_packet;
        result[2]=d.executions;
        result[3]=static_cast<int>(d.decision.verdict);
        result[4]=d.decision.stopped;
        result[5]=d.decision.sample_held_movement;
        result[6]=d.decision.sample_released_movement;
        break;
    }
    case 2:{
        LagRangeSourceTickPayload p{};
        p.input.in_game=flag(bits,0);p.input.weapon=flag(bits,1);
        p.input.target=flag(bits,2);p.input.sprinting=flag(bits,3);
        p.input.splash_potion=flag(bits,4);
        p.input.hit_this_tick=flag(bits,5);
        p.input.third_person=flag(bits,6);p.input.target_hurt=bits[7];
        p.settings.require_weapon=flag(bits,8);
        p.settings.flush_splash_potion=flag(bits,9);
        p.settings.flush_sprint_reset=flag(bits,10);
        p.settings.render_remote_pos=flag(bits,11);
        p.input.target_distance_sq=vals[0];
        p.input.sampled_max_range=finite_float(vals,1);
        p.input.target_server_position={vals[2],vals[3],vals[4]};
        p.settings.delay_ms=finite_float(vals,5);
        f.lag_range_source_tick=p;
        require(s.bridge.dispatch(f),"Lag Range tick denied");
        const auto& d=*f.lag_range_source_tick;
        result[2]=d.executions;result[3]=d.active;result[4]=d.show_box;
        break;
    }
    case 3:{
        LagRangeSourcePacketPayload p{};
        p.input.kind=static_cast<LagRangePacketType>(enum_value(bits,0,3));
        p.input.sequence=bits[1];p.input.movement_has_position=flag(bits,2);
        p.input.prior_verdict_zero=flag(bits,3);
        p.input.held_ms=millis(vals,0);
        p.input.movement_position={vals[1],vals[2],vals[3]};
        f.lag_range_source_packet=p;
        require(s.bridge.dispatch(f),"Lag Range packet denied");
        const auto& d=*f.lag_range_source_packet;
        result[2]=d.executions;result[3]=d.decision.hold;
        result[4]=d.decision.released_on_attack;
        result[5]=d.decision.released_on_sprint;
        result[6]=d.decision.position_recorded;
        break;
    }
    case 4:{
        TimerRangeSourceTickPayload p{};
        p.input.in_game=flag(bits,0);p.input.target=flag(bits,1);
        p.input.player_on_ground=flag(bits,2);
        p.input.mouse=flag(bits,3);p.input.weapon=flag(bits,4);
        p.input.using_item=flag(bits,5);p.input.jumping=flag(bits,6);
        p.input.line_of_sight=flag(bits,7);
        p.input.entity_under_crosshair=flag(bits,8);
        p.input.sword_held=flag(bits,9);p.input.target_hurt_time=bits[10];
        p.settings.require_mouse=flag(bits,11);
        p.settings.require_weapon=flag(bits,12);
        p.settings.require_not_using_item=flag(bits,13);
        p.settings.mode=static_cast<TimerRangeMode>(enum_value(bits,14,1));
        p.input.target_distance_sq=vals[0];p.input.motion_y=vals[1];
        p.input.forward_input=finite_float(vals,2);
        p.settings.duration_ticks=finite_float(vals,3);
        p.settings.duration_legit_ticks=finite_float(vals,4);
        p.settings.min_dist=finite_float(vals,5);
        p.settings.cooldown_seconds=finite_float(vals,6);
        f.timer_range_source_tick=p;
        require(s.bridge.dispatch(f),"Timer Range tick denied");
        const auto& d=*f.timer_range_source_tick;
        result[2]=d.executions;
        result[3]=d.decision.attack_click;
        result[4]=d.decision.write_speed;
        result[5]=d.decision.speed;
        break;
    }
    case 5:{
        TimerRangeSourceFramePayload p{};
        p.sword_held=flag(bits,0);
        p.delta_seconds=vals[0];p.partial_ticks=finite_float(vals,1);
        TimerRangeSourceObservation o{};
        o.player_eye={vals[2],vals[3],vals[4]};
        o.initial_motion={vals[5],vals[6],vals[7]};
        o.target_client_pos={vals[8],vals[9],vals[10]};
        o.target_server_pos={vals[11],vals[12],vals[13]};
        o.yaw_degrees=vals[14];o.pitch_degrees=vals[15];
        o.forward_input=finite_float(vals,16);
        o.strafe_input=finite_float(vals,17);
        o.movement_factor=vals[18];
        require(vals[19]>=0&&vals[19]<=10000&&std::floor(vals[19])==vals[19],
                "Invalid target interpolation steps");
        o.target_interpolation_steps=static_cast<int>(vals[19]);
        p.source_snapshot=o;
        f.timer_range_source_frame=p;
        require(s.bridge.dispatch(f),"Timer Range frame denied");
        const auto& d=*f.timer_range_source_frame;
        result[2]=d.executions;
        result[3]=d.decision.burst_planned;
        result[4]=d.decision.write_speed;
        result[5]=d.decision.speed;
        result[6]=d.decision.write_elapsed_ticks;
        result[7]=d.decision.elapsed_ticks;
        break;
    }
    case 6:{
        InvWalkSourceTickPayload p{};
        p.input.in_game=flag(bits,0);
        p.input.screen=static_cast<InvWalkScreen>(enum_value(bits,1,2));
        for(int i=0;i<5;++i)p.input.physically_held[i]=flag(bits,2+i);
        p.input.cursor_busy=flag(bits,7);
        p.input.crafting_busy=flag(bits,8);
        p.settings.mode=static_cast<InvWalkMode>(enum_value(bits,9,1));
        p.settings.stop_on_click=static_cast<InvWalkStop>(enum_value(bits,10,2));
        p.settings.stop_non_inventory=static_cast<InvWalkStop>(enum_value(bits,11,2));
        p.settings.stop_duration_ticks=finite_float(vals,0);
        f.inv_walk_source_tick=p;
        require(s.bridge.dispatch(f),"Inv Walk tick denied");
        const auto& d=*f.inv_walk_source_tick;
        result[2]=d.executions;
        result[3]=d.decision.update_keys;
        result[4]=d.decision.cancel_sprint;
        result[5]=d.decision.close_on_server;
        result[6]=d.decision.open_on_server;
        break;
    }
    case 7:{
        InvWalkSourcePacketPayload p{};
        p.kind=static_cast<InvWalkPacketType>(enum_value(bits,0,3));
        f.inv_walk_source_packet=p;
        require(s.bridge.dispatch(f),"Inv Walk packet denied");
        const auto& d=*f.inv_walk_source_packet;
        result[2]=d.executions;
        result[3]=static_cast<int>(d.decision.action);
        result[4]=d.decision.send_open_inventory;
        break;
    }
    case 8:{
        InvWalkSourceFlushPayload p{};
        p.flush_confirmed=flag(bits,0);p.in_game=flag(bits,1);
        f.inv_walk_source_flush=p;
        require(s.bridge.dispatch(f),"Inv Walk flush denied");
        const auto& d=*f.inv_walk_source_flush;
        result[2]=d.executions;
        result[3]=d.decision.drain_stale_keyboard_events;
        break;
    }
    case 10:{
        // R353: original Lag Range event15 phase callback. This is native
        // ModuleExecutionBridge's real source controller, not a Java model.
        LagRangeSourceAttackPayload p{};
        p.phase_one=flag(bits,0);
        p.hit_at_current_tick=flag(bits,1);
        f.lag_range_source_attack=p;
        require(s.bridge.dispatch(f),"Lag Range source attack event15 denied");
        const auto& d=*f.lag_range_source_attack;
        result[2]=d.executions;
        result[3]=d.phase_one&&d.hit_at_current_tick;
        break;
    }
    case 9:{
        // Original Timer Range source event1 is outgoing packet gating.
        // This JNI opcode merely observes the C++ source hold flag;
        // no actual on-wire packet is delayed, retained or cancelled.
        TimerRangeSourcePacketPayload p{};
        f.timer_range_source_packet=p;
        require(s.bridge.dispatch(f),"Timer Range event1 denied");
        const auto& d=*f.timer_range_source_packet;
        result[2]=d.executions;
        result[3]=d.hold_packets;
        break;
    }
    }
    for(double x:result)require(std::isfinite(x),"Nonfinite source JNI output");
    return result;
}
}
extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_SourceRuntimePreview_nativeOpen(JNIEnv* env,jclass){
    try {
        std::unique_ptr<Session> p(new Session);
        std::lock_guard<std::mutex> lk(mutex);
        if(next_id<=0||next_id==std::numeric_limits<jlong>::max())
            throw std::logic_error("Source session IDs exhausted");
        jlong id=next_id++;
        sessions.emplace(id,std::move(p));
        return id;
    }catch(const std::exception& e){
        java_error(env,"java/lang/IllegalStateException",e.what());
        return 0;
    }
}
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_SourceRuntimePreview_nativeObserve(
        JNIEnv* env,jclass,jlong handle,jlongArray identity,jint op,
        jintArray flags,jdoubleArray values){
    if(!identity||!flags||!values||env->GetArrayLength(identity)!=4||
       env->GetArrayLength(flags)>40||env->GetArrayLength(values)>64){
        java_error(env,"java/lang/IllegalArgumentException",
                   "Source JNI dimensions invalid");return nullptr;
    }
    jlong id[4]{};
    env->GetLongArrayRegion(identity,0,4,id);
    const std::size_t nb=static_cast<std::size_t>(env->GetArrayLength(flags));
    const std::size_t nv=static_cast<std::size_t>(env->GetArrayLength(values));
    std::vector<jint> b(nb);
    std::vector<jdouble> v(nv);
    if(nb)env->GetIntArrayRegion(flags,0,static_cast<jsize>(nb),b.data());
    if(nv)env->GetDoubleArrayRegion(values,0,static_cast<jsize>(nv),v.data());
    if(env->ExceptionCheck())return nullptr;
    try {
        require(op>=0&&op<11,"Unknown source event opcode");
        require(nb==static_cast<std::size_t>(bits_by_op[op])&&
                nv==static_cast<std::size_t>(vals_by_op[op]),
                "Source event ABI shape mismatch");
        for(double x:v)require(std::isfinite(x),"Nonfinite source JNI input");
        std::lock_guard<std::mutex> lk(mutex);
        auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->owner!=std::this_thread::get_id())
            throw std::logic_error("Closed or foreign native source session");
        it->second->observe_identity(id);
        const auto out=dispatch(*it->second,op,b,v);
        jdoubleArray result=env->NewDoubleArray(8);
        if(result)env->SetDoubleArrayRegion(result,0,8,out.data());
        return result;
    }catch(const std::invalid_argument& e){
        java_error(env,"java/lang/IllegalArgumentException",e.what());
        return nullptr;
    }catch(const std::exception& e){
        java_error(env,"java/lang/IllegalStateException",e.what());
        return nullptr;
    }
}
extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SourceRuntimePreview_nativeClose(JNIEnv* env,jclass,jlong handle){
    try {
        std::lock_guard<std::mutex> lk(mutex);
        auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->owner!=std::this_thread::get_id())
            throw std::logic_error("Closed or foreign source session");
        it->second->bridge.stop();
        sessions.erase(it);
    }catch(const std::exception& e){
        java_error(env,"java/lang/IllegalStateException",e.what());
    }
}


// R336: explicit source-module enable/disable replay over the SAME owner.
// This produces cleanup receipts only; it is not authorized game I/O.
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_SourceRuntimePreview_nativeControl(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,jint moduleId,
    jboolean enabled){
    if(!identity||env->GetArrayLength(identity)!=4){
        java_error(env,"java/lang/IllegalArgumentException","Invalid source control identity");
        return nullptr;
    }
    jlong id[4]{};
    env->GetLongArrayRegion(identity,0,4,id);
    if(env->ExceptionCheck())return nullptr;
    try {
        require(moduleId>=0&&moduleId<4,"Unknown source module control");
        require(id[0]>0&&id[1]>0&&id[0]!=id[1]&&id[2]>0&&id[3]>=0,
                "Invalid source control world identity");
        // Reserve result before changing module state; an allocation failure
        // cannot leave an unreported successful disable behind.
        jlongArray output=env->NewLongArray(7);
        if(!output)return nullptr;
        std::lock_guard<std::mutex> lk(mutex);
        auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->owner!=std::this_thread::get_id())
            throw std::logic_error("Closed or foreign source control session");
        Session& s=*it->second;
        s.observe_identity(id);
        const bool requested=enabled==JNI_TRUE;
        const auto module=s.module[static_cast<std::size_t>(moduleId)];
        if(!s.bridge.request(module,requested)||!s.bridge.apply(true,true))
            throw std::logic_error("Original source module lifecycle rejected change");
        const auto timer=s.bridge.take_timer_range_source_cleanup();
        const auto inv=s.bridge.take_inv_walk_source_cleanup();
        const jlong receipt[7]={
            static_cast<jlong>(moduleId),requested?1:0,
            s.bridge.enabled(module)?1:0,
            timer && timer->write_speed && timer->speed==1 ? 1:0,
            timer && timer->write_use_key && !timer->use_key ? 1:0,
            inv ? static_cast<jlong>(inv->queued_clicks_before_reset):0,
            id[2]
        };
        env->SetLongArrayRegion(output,0,7,receipt);
        return output;
    }catch(const std::invalid_argument& e){
        java_error(env,"java/lang/IllegalArgumentException",e.what());
        return nullptr;
    }catch(const std::exception& e){
        java_error(env,"java/lang/IllegalStateException",e.what());
        return nullptr;
    }
}


// R341: source event6 passes BOTH remote box geometry callbacks through the
// original native ModuleExecutionBridge. Observation only; JNI never invokes GL.
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_SourceRuntimePreview_nativeSourceBoxes(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,jdoubleArray coordinates){
    if(!identity||!coordinates||env->GetArrayLength(identity)!=4||
       env->GetArrayLength(coordinates)!=12){
        java_error(env,"java/lang/IllegalArgumentException","Invalid source box JNI shape");
        return nullptr;
    }
    jlong id[4]{};
    jdouble v[12]{};
    env->GetLongArrayRegion(identity,0,4,id);
    env->GetDoubleArrayRegion(coordinates,0,12,v);
    if(env->ExceptionCheck())return nullptr;
    try {
        for(double d:v)require(std::isfinite(d)&&std::abs(d)<=10000000,
                                "Nonfinite or out-of-range native source box");
        require(v[9]>=0&&v[9]<=1,"Invalid source box partial tick");
        jdoubleArray result=env->NewDoubleArray(30);
        if(!result)return nullptr;
        std::lock_guard<std::mutex> lk(mutex);
        const auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->owner!=std::this_thread::get_id())
            throw std::logic_error("Closed or foreign source box JNI owner");
        Session& session=*it->second;
        session.observe_identity(id);
        ExecutionFrame frame{};
        frame.event=6;frame.world_present=frame.player_present=true;
        SourceBoxObservation o{};
        o.previous={v[0],v[1],v[2]};
        o.current={v[3],v[4],v[5]};
        o.camera={v[6],v[7],v[8]};
        o.partial_ticks=static_cast<float>(v[9]);
        o.yaw_previous=static_cast<float>(v[10]);
        o.yaw_current=static_cast<float>(v[11]);
        BacktrackSourceRenderPayload back{};
        back.observation=o;
        back.style.color={{0.75f,0.45f,0.90f}}; // debug palette, NOT original
        back.style.line_width=1.5f;
        back.style.rotate=true;
        LagRangeSourceRenderPayload lag{};
        lag.observation=o;
        lag.style.color={{0.25f,0.65f,1.0f}}; // debug palette, NOT original
        lag.style.line_width=1.5f;
        frame.backtrack_source_render=back;
        frame.lag_range_source_render=lag;
        require(session.bridge.dispatch(frame),"Native source e6 geometry rejected");
        const auto& b=*frame.backtrack_source_render;
        const auto& l=*frame.lag_range_source_render;
        require(b.executions<=1&&l.executions<=1,
                "Duplicate native source e6 geometry callback");
        // A disabled module produces an empty but structurally valid
        // draw proposal; it must NOT crash the other active module.

        std::array<jdouble,30> output{};
        const auto pack=[&](int offset,const SourceBoxProposal& p){
            output[offset+0]=p.draw?1:0;
            output[offset+1]=p.translate.x;
            output[offset+2]=p.translate.y;
            output[offset+3]=p.translate.z;
            output[offset+4]=p.scale.x;
            output[offset+5]=p.scale.y;
            output[offset+6]=p.scale.z;
            output[offset+7]=p.rotate?1:0;
            output[offset+8]=p.rotation_y_degrees;
            output[offset+9]=p.filled?1:0;
            output[offset+10]=p.line_width;
            output[offset+11]=p.color[0];
            output[offset+12]=p.color[1];
            output[offset+13]=p.color[2];
            output[offset+14]=p.fill_alpha;
        };
        pack(0,b.decision);
        pack(15,l.decision);
        for(double d:output)require(std::isfinite(d),"Nonfinite source e6 geometry");
        env->SetDoubleArrayRegion(result,0,30,output.data());
        return result;
    }catch(const std::invalid_argument& e){
        java_error(env,"java/lang/IllegalArgumentException",e.what());return nullptr;
    }catch(const std::exception& e){
        java_error(env,"java/lang/IllegalStateException",e.what());return nullptr;
    }
}
