#include "slinkywinky/bridge.h"
#include <jni.h>
#include <array>
#include <cstdint>

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
