#include "slinkywinky/bridge.h"
#include <jni.h>
#include <array>
#include <cstdint>
#include <cmath>
#include <limits>

namespace {
void error(JNIEnv* env,const char* cls,const char* message) {
    jclass e=env->FindClass(cls);
    if (e) env->ThrowNew(e,message);
}
SWContext* ptr(jlong value) {return reinterpret_cast<SWContext*>(static_cast<std::uintptr_t>(value));}
}
extern "C" JNIEXPORT jint JNICALL Java_dev_slinkywinky_NativeEngine_nativeVersion(
    JNIEnv*, jclass) {return static_cast<jint>(SW_ABI_VERSION);}
extern "C" JNIEXPORT jlong JNICALL Java_dev_slinkywinky_NativeEngine_nativeCreate(
    JNIEnv* env, jclass, jintArray values) {
    if(!values || env->GetArrayLength(values)!=4) {
        error(env,"java/lang/IllegalArgumentException","Four seed words required");return 0;
    }
    jint seed[4];env->GetIntArrayRegion(values,0,4,seed);
    const std::uint32_t words[4]={static_cast<std::uint32_t>(seed[0]),
        static_cast<std::uint32_t>(seed[1]),static_cast<std::uint32_t>(seed[2]),
        static_cast<std::uint32_t>(seed[3])};
    SWContext* context=nullptr;
    if(sw_create(words,&context)!=0){error(env,"java/lang/IllegalStateException","Native initialization rejected");return 0;}
    return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context));
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_dev_slinkywinky_NativeEngine_nativeStep(
    JNIEnv* env,jclass,jlong handle,jlong time,jlong generation,jboolean world,
    jboolean gui,jboolean mouse,jboolean weapon,jfloat yaw,jfloat pitch,
    jdoubleArray eye,jdoubleArray target,jfloatArray config) {
    if(!eye || !config || env->GetArrayLength(eye)!=3 ||
       (target && env->GetArrayLength(target)!=3) || env->GetArrayLength(config)!=7) {
        error(env,"java/lang/IllegalArgumentException","Incorrect snapshot array size");return nullptr;
    }
    jdouble p[3],t[3]={0,0,0};jfloat c[7];
    env->GetDoubleArrayRegion(eye,0,3,p);
    if(target) env->GetDoubleArrayRegion(target,0,3,t);
    env->GetFloatArrayRegion(config,0,7,c);
    SWFrame frame{};frame.abi_version=SW_ABI_VERSION;
    frame.time_ms=static_cast<std::uint64_t>(time);
    frame.generation=static_cast<std::uint64_t>(generation);
    frame.world_valid=world;frame.gui_open=gui;frame.mouse_left=mouse;frame.weapon=weapon;
    frame.eye_x=p[0];frame.eye_y=p[1];frame.eye_z=p[2];
    frame.target_present=target!=nullptr;frame.target_alive=target!=nullptr;
    frame.target_visible=target!=nullptr;
    frame.target_x=t[0];frame.target_y=t[1];frame.target_z=t[2];
    frame.yaw=yaw;frame.pitch=pitch;
    SWSettings settings{};settings.abi_version=SW_ABI_VERSION;
    settings.clicker=c[0]>0;settings.cps=c[1];
    settings.aim=c[2]>0;settings.aim_range=c[3];
    settings.aim_hspeed=c[4];settings.aim_vspeed=c[5];settings.aim_fov=c[6];
    SWOutput out{};
    const int status=sw_step(ptr(handle),&frame,&settings,&out);
    if(status!=0){error(env,"java/lang/IllegalStateException","Native tick rejected");return nullptr;}
    const jfloat result[5]={static_cast<float>(out.attempt_attack),
        static_cast<float>(out.left_held),static_cast<float>(out.has_rotation),out.yaw,out.pitch};
    jfloatArray array=env->NewFloatArray(5);
    if(array) env->SetFloatArrayRegion(array,0,5,result);
    return array;
}
extern "C" JNIEXPORT void JNICALL Java_dev_slinkywinky_NativeEngine_nativeClose(
    JNIEnv* env,jclass,jlong handle) {
    SWContext* context=ptr(handle);
    if(sw_destroy(&context)!=0)
        error(env,"java/lang/IllegalStateException","Native close rejected");
}

namespace {
jfloatArray write_detailed(JNIEnv* env,const SWOutput& out) {
    const jfloat values[10]={static_cast<jfloat>(out.left_edge),
        static_cast<jfloat>(out.right_edge), static_cast<jfloat>(out.left_held),
        static_cast<jfloat>(out.right_held),static_cast<jfloat>(out.attempt_attack),
        static_cast<jfloat>(out.has_rotation),out.yaw,out.pitch,
        static_cast<jfloat>(out.has_reach),out.reach_distance};
    jfloatArray result=env->NewFloatArray(10);
    if(result) env->SetFloatArrayRegion(result,0,10,values);
    return result;
}
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_dev_slinkywinky_NativeEngine_nativeStepDetailed(
    JNIEnv* env, jclass, jlong handle, jlongArray clocks, jintArray flags,
    jdoubleArray coords, jfloatArray angles, jfloatArray cfg) {
    if(!clocks || !flags || !coords || !angles || !cfg ||
       env->GetArrayLength(clocks)!=2 || env->GetArrayLength(flags)!=12 ||
       env->GetArrayLength(coords)!=6 || env->GetArrayLength(angles)!=2 ||
       env->GetArrayLength(cfg)!=19) {
        error(env,"java/lang/IllegalArgumentException","Incorrect detailed frame shape");return nullptr;
    }
    jlong time[2];jint f[12];jdouble p[6];jfloat a[2],c[19];
    env->GetLongArrayRegion(clocks,0,2,time);
    env->GetIntArrayRegion(flags,0,12,f);
    env->GetDoubleArrayRegion(coords,0,6,p);
    env->GetFloatArrayRegion(angles,0,2,a);
    env->GetFloatArrayRegion(cfg,0,19,c);
    if(env->ExceptionCheck()) return nullptr;
    if(time[0]<0 || time[1]<=0 || !std::isfinite(c[18]) ||
       c[18]<static_cast<jfloat>(std::numeric_limits<std::int32_t>::min()) ||
       static_cast<double>(c[18])>=2147483648.0) {
        error(env,"java/lang/IllegalArgumentException","Invalid detailed frame clock or hit-select value");return nullptr;
    }
    for(jdouble value:p) if(!std::isfinite(value)) {
        error(env,"java/lang/IllegalArgumentException","Invalid coordinates");return nullptr;
    }
    for(jfloat value:a) if(!std::isfinite(value)) {
        error(env,"java/lang/IllegalArgumentException","Invalid rotation");return nullptr;
    }
    for(jfloat value:c) if(!std::isfinite(value)) {
        error(env,"java/lang/IllegalArgumentException","Invalid settings");return nullptr;
    }
    SWFrame frame{};
    frame.abi_version=SW_ABI_VERSION;
    frame.time_ms=static_cast<std::uint64_t>(time[0]);
    frame.generation=static_cast<std::uint64_t>(time[1]);
    frame.world_valid=f[0];frame.gui_open=f[1];frame.mouse_left=f[2];
    frame.mouse_right=f[3];frame.weapon=f[4];frame.sprinting=f[5];
    frame.hurt_time=f[6];frame.last_attacker_time=f[7];
    frame.target_present=f[8];frame.target_visible=f[9];frame.target_alive=f[10];
    frame.target_id=f[11];
    frame.eye_x=p[0];frame.eye_y=p[1];frame.eye_z=p[2];
    frame.target_x=p[3];frame.target_y=p[4];frame.target_z=p[5];
    frame.yaw=a[0];frame.pitch=a[1];
    SWSettings settings{};
    settings.abi_version=SW_ABI_VERSION;
    settings.clicker=c[0]>0;settings.require_mouse=c[1]>0;
    settings.cps=c[2];settings.aim=c[3]>0;settings.aim_require_mouse=c[4]>0;
    settings.aim_range=c[5];settings.aim_hspeed=c[6];
    settings.aim_vspeed=c[7];settings.aim_fov=c[8];
    settings.auto_block=c[9]>0;settings.block_require_damage=c[10]>0;
    settings.block_range=c[11];settings.block_max_hurt=c[12];
    settings.reach=c[13]>0;settings.reach_distance=c[14];
    settings.reach_activation_ticks=c[15];settings.reach_require_sprint=c[16]>0;
    settings.hit_select=c[17]>0;settings.hit_select_min_hurt=static_cast<int>(c[18]);
    SWOutput output{};
    const int status=sw_step(ptr(handle),&frame,&settings,&output);
    if(status!=0) {error(env,"java/lang/IllegalStateException","Detailed native tick rejected");return nullptr;}
    return write_detailed(env,output);
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_dev_slinkywinky_NativeEngine_nativeDisable(
    JNIEnv* env,jclass,jlong handle,jboolean manualLeft,jboolean manualRight) {
    SWOutput out{};
    if(sw_disable(ptr(handle),manualLeft,manualRight,&out)!=0) {
        error(env,"java/lang/IllegalStateException","Native disable rejected");return nullptr;
    }
    return write_detailed(env,out);
}
