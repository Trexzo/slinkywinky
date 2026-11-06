#include "slinkywinky/bridge.h"
#include "slinkywinky/observation.hpp"
#include "slinkywinky/hud_snapshot.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_hud_ipc.hpp"
#include "slinkywinky/windows_stop_gate.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <windows.h>
#include <GL/gl.h>
#include <cstdlib>
#include <cstring>
#endif
#include <jni.h>
#include <array>
#include <cstdint>
#include <cmath>
#include <limits>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <thread>

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

/** Read-only native decision proof, dispatched on the exact Java callback thread. */
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeZeroActionTick(JNIEnv*,jclass) {
    const std::uint32_t seed[4] = {1u, 2u, 3u, 4u};
    SWContext* context = nullptr;
    if (sw_create(seed, &context) != 0 || !context) return 0;
    SWFrame frame{};
    frame.abi_version = SW_ABI_VERSION;
    frame.world_valid = 1;
    frame.generation = 1;
    frame.time_ms = 1000;
    SWSettings settings{};
    settings.abi_version = SW_ABI_VERSION;
    SWOutput output{};
    const int status = sw_step(context, &frame, &settings, &output);
    const int released = sw_destroy(&context);
    return status == 0 && released == 0 && output.abi_version == SW_ABI_VERSION &&
        !output.attempt_attack && !output.left_held && !output.right_held &&
        !output.has_rotation && !output.has_reach &&
        output.left_edge == 0 && output.right_edge == 0 ? 1 : 0;
}

namespace {
// Opaque monotonically increasing handles avoid raw pointers crossing the Java API.
// The registry is bounded and guards each C++ decision against concurrent close.
// sw_step/sw_destroy still independently enforce the creator-thread contract.
struct ObservationRuntime {
    SWContext* engine = nullptr;
    slinkywinky::ObservationLedger ledger;
    slinkywinky::HudSnapshotMailbox mailbox;
#ifdef _WIN32
    slinkywinky::HudIpcPublisher hud_ipc;
    bool hud_ipc_enabled = false;
    HANDLE stop_event = nullptr;
    const std::thread::id owner_thread = std::this_thread::get_id();

    bool enable_stop_signal() noexcept {
        if(stop_event || std::this_thread::get_id()!=owner_thread)return false;
        auto name=slinkywinky::continuous_stop_event_name(GetCurrentProcessId());
        HANDLE created=CreateEventW(nullptr,TRUE,FALSE,name.c_str());
        if(!created)return false;
        if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(created);return false;}
        stop_event=created;
        return true;
    }
    int poll_stop_signal() const noexcept {
        if(!stop_event || std::this_thread::get_id()!=owner_thread)return -1;
        DWORD state=WaitForSingleObject(stop_event,0); // No game-thread wait.
        return state==WAIT_OBJECT_0?1:(state==WAIT_TIMEOUT?0:-1);
    }
    ~ObservationRuntime() noexcept {
        if(stop_event)CloseHandle(stop_event);
    }
#endif
    explicit ObservationRuntime(SWContext* context) : engine(context) {}
};
std::mutex observationMutex;
std::unordered_map<jlong, std::unique_ptr<ObservationRuntime>> observationContexts;
std::atomic<std::uint64_t> observationNextId{1};
constexpr std::size_t maxObservationContexts = 4;
}
/** R61: a separate Win32 event can only ask the original client thread to stop.
 * Never require a second Java Attach operation while a game tick hook is active. */
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeEnableStopSignal(
    JNIEnv*,jclass,jlong handle) {
#ifdef _WIN32
    std::lock_guard<std::mutex> guard(observationMutex);
    const auto found=observationContexts.find(handle);
    if(found==observationContexts.end())return 0;
    return found->second->enable_stop_signal()?1:0;
#else
    (void)handle;
    return 0;
#endif
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativePollStopSignal(
    JNIEnv*,jclass,jlong handle) {
#ifdef _WIN32
    std::lock_guard<std::mutex> guard(observationMutex);
    const auto found=observationContexts.find(handle);
    if(found==observationContexts.end())return -1;
    return found->second->poll_stop_signal();
#else
    (void)handle;
    return -1;
#endif
}
/** R41-R45: real, read-only client observations; all gameplay settings remain OFF. */
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeObserveFrame(
    JNIEnv* env,jclass,jlong handle,jlong time,jlong generation,jint flags,
    jdoubleArray xyz,jfloatArray angles) {
    if(!xyz || !angles || env->GetArrayLength(xyz)!=6 ||
       env->GetArrayLength(angles)!=2 || time<0 || generation<=0 ||
       (flags&~7)!=0 || ((flags&2) && !(flags&1)) ||
       ((flags&4) && ((flags&3)!=3))) return 0;
    jdouble points[6];jfloat camera[2];
    env->GetDoubleArrayRegion(xyz,0,6,points);
    env->GetFloatArrayRegion(angles,0,2,camera);
    if(env->ExceptionCheck()) return 0;
    for(double value:points) if(!std::isfinite(value)) return 0;
    for(float value:camera) if(!std::isfinite(value)) return 0;
    std::lock_guard<std::mutex> guard(observationMutex);
    const auto found=observationContexts.find(handle);
    if(found==observationContexts.end()) return 0;
    ObservationRuntime& session=*found->second;
    slinkywinky::ObservationFrame sample{};
    sample.time_ms=static_cast<std::uint64_t>(time);
    sample.generation=static_cast<std::uint64_t>(generation);
    sample.flags=static_cast<std::uint32_t>(flags);
    for(std::size_t i=0;i<6;++i) sample.xyz[i]=points[i];
    sample.angles={camera[0],camera[1]};
    if(!session.ledger.can_accept(sample)) return 0;
    SWContext* context=session.engine;
    SWFrame frame{};
    frame.abi_version=SW_ABI_VERSION;
    frame.time_ms=static_cast<std::uint64_t>(time);
    frame.generation=static_cast<std::uint64_t>(generation);
    frame.world_valid=(flags&3)==3;
    frame.target_present=(flags&4)!=0;
    frame.target_visible=0; // No visibility certification from a ray hit alone.
    frame.target_alive=0; // No live entity health assumption.
    frame.eye_x=points[0];frame.eye_y=points[1];frame.eye_z=points[2];
    frame.target_x=points[3];frame.target_y=points[4];frame.target_z=points[5];
    frame.yaw=camera[0];frame.pitch=camera[1];
    SWSettings settings{};
    settings.abi_version=SW_ABI_VERSION;
    SWOutput output{};
    const int status=sw_step(context,&frame,&settings,&output);
    const bool safe=status==0 && output.abi_version==SW_ABI_VERSION &&
        !output.attempt_attack && !output.left_held && !output.right_held &&
        !output.has_rotation && !output.has_reach &&
        output.left_edge==0 && output.right_edge==0;
    if(!safe) return 0;
    session.ledger.commit(sample);
    const auto accepted=session.ledger.snapshot(); // Read only on client owner thread.
    // Never publish a frame that has not passed native zero-action admission.
    if(!accepted || !session.mailbox.publish(*accepted))return 0;
#ifdef _WIN32
    if(session.hud_ipc_enabled) {
        const auto published=session.mailbox.read();
        if(published) session.hud_ipc.publish(*published);
        // Local HUD IPC is best-effort and must never block the client tick.
    }
#endif
    return 1;
}

/** R43: long-lived context, created and destroyed on the exact game thread. */
extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeOpenObservation(JNIEnv*,jclass) {
    const std::uint32_t seed[4]={1u,2u,3u,4u};
    SWContext* context=nullptr;
    if(sw_create(seed,&context)!=0 || !context) return 0;
    std::lock_guard<std::mutex> guard(observationMutex);
    if(observationContexts.size()>=maxObservationContexts) {
        sw_destroy(&context); // Same thread that created the context.
        return 0;
    }
    const auto index=observationNextId.fetch_add(1);
    if(index==0 || index>static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())) {
        sw_destroy(&context);
        return 0;
    }
    const auto token=static_cast<jlong>(index);
    try {
        auto inserted=observationContexts.emplace(
            token,std::make_unique<ObservationRuntime>(context));
        if(!inserted.second) {
            sw_destroy(&context);
            return 0;
        }
#ifdef _WIN32
        const char* requested=std::getenv("SLINKYWINKY_HUD_IPC");
        if(requested && std::strcmp(requested,"1")==0) {
            auto& runtime=*inserted.first->second;
            if(!runtime.hud_ipc.open(GetCurrentProcessId())) {
                SWContext*& owned=runtime.engine;
                sw_destroy(&owned); // Invoked on creator thread.
                observationContexts.erase(inserted.first);
                return 0; // Opt-in could not be honored; fail admission.
            }
            runtime.hud_ipc_enabled=true;
        }
#endif
    } catch (...) {
        sw_destroy(&context);
        return 0;
    }
    return token;
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeCloseObservation(
    JNIEnv*,jclass,jlong handle) {
    std::lock_guard<std::mutex> guard(observationMutex);
    auto found=observationContexts.find(handle);
    if(found==observationContexts.end()) return 0;
    SWContext*& context=found->second->engine;
    if(sw_destroy(&context)!=0) return 0;
#ifdef _WIN32
    found->second->hud_ipc.revoke();
#endif
    const bool revoked=found->second->mailbox.revoke();
    observationContexts.erase(found);
    return revoked ? 1 : 0;
}


/** R53: owner-thread-only native status for a future read-only HUD. */
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeReadObservationStatus(
    JNIEnv* env,jclass,jlong handle) {
    std::lock_guard<std::mutex> guard(observationMutex);
    const auto found=observationContexts.find(handle);
    if(found==observationContexts.end()) return nullptr;
    const auto status=found->second->ledger.snapshot();
    if(!status) return nullptr;
    // No entity identities, pointers, packets or input commands are exposed.
    const jdouble values[9]={
        static_cast<jdouble>(status->valid_ticks),
        static_cast<jdouble>(status->session_transitions),
        static_cast<jdouble>(status->generation),
        static_cast<jdouble>(status->flags),
        static_cast<jdouble>(status->target_distance),
        static_cast<jdouble>(status->yaw),
        static_cast<jdouble>(status->pitch),
        static_cast<jdouble>(status->time_ms),
        static_cast<jdouble>(status->eye_y)
    };
    jdoubleArray result=env->NewDoubleArray(9);
    if(result) env->SetDoubleArrayRegion(result,0,9,values);
    return result;
}


/** R55: coherent, read-only published status for another JVM render/UI thread.
 * This never calls the owner-thread decision engine or exposes its pointers.
 * Format: existing nine status doubles followed by a publication sequence.
 */
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_instrument_InstrumentationProbe_nativeReadPublishedStatus(
    JNIEnv* env,jclass,jlong handle) {
    std::lock_guard<std::mutex> guard(observationMutex);
    const auto found=observationContexts.find(handle);
    if(found==observationContexts.end()) return nullptr;
    const auto published=found->second->mailbox.read();
    if(!published) return nullptr;
    const auto& s=published->status;
    const jdouble values[10]={
        static_cast<jdouble>(s.valid_ticks),
        static_cast<jdouble>(s.session_transitions),
        static_cast<jdouble>(s.generation),
        static_cast<jdouble>(s.flags),
        static_cast<jdouble>(s.target_distance),
        static_cast<jdouble>(s.yaw),
        static_cast<jdouble>(s.pitch),
        static_cast<jdouble>(s.time_ms),
        static_cast<jdouble>(s.eye_y),
        static_cast<jdouble>(published->sequence)
    };
    jdoubleArray result=env->NewDoubleArray(10);
    if(result) env->SetDoubleArrayRegion(result,0,10,values);
    return result;
}

#ifdef _WIN32
/**
 * R78: read-only snapshot of the caller's CURRENT WGL context.
 *
 * No wglMakeCurrent, hook, GL mutation, input interception, or framebuffer read.
 * Opaque native HGLRC/HDC values are used only for comparison inside Java and
 * never logged. Missing context produces an all-zero tuple, not a GL call.
 */
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCurrentWglSnapshot(
    JNIEnv* env,jclass) {
    const HGLRC context=wglGetCurrentContext();
    const HDC dc=wglGetCurrentDC();
    jlong sample[6]{};
    if(context!=nullptr && dc!=nullptr) {
        GLint viewport[4]{};
        glGetIntegerv(GL_VIEWPORT,viewport);
        sample[0]=static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context));
        sample[1]=static_cast<jlong>(reinterpret_cast<std::uintptr_t>(dc));
        sample[2]=static_cast<jlong>(viewport[0]);
        sample[3]=static_cast<jlong>(viewport[1]);
        sample[4]=static_cast<jlong>(viewport[2]);
        sample[5]=static_cast<jlong>(viewport[3]);
    }
    jlongArray result=env->NewLongArray(6);
    if(result)env->SetLongArrayRegion(result,0,6,sample);
    return result;
}
/**
 * R91 explicitly opted-in, ONE finite GL1.1 overlay frame on the admitted
 * original renderer thread after eight stable R78 context snapshots.
 *
 * No WGL context switching, framebuffer clear, input, networking or game
 * settings. A fresh owner-bound host is closed on the same callback; no
 * native renderer state is retained between frames.
 */
static jboolean drawValidatedMenuOverlay(
    jlong expectedContext,jlong expectedDc,jint width,jint height,
    slinkywinky::MenuGL11Mode mode) {
    if(expectedContext==0 || expectedDc==0 ||
       width<320 || width>8192 || height<240 || height>8192)
        return JNI_FALSE;
    const HGLRC context=reinterpret_cast<HGLRC>(
        static_cast<std::uintptr_t>(expectedContext));
    const HDC dc=reinterpret_cast<HDC>(
        static_cast<std::uintptr_t>(expectedDc));
    if(wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc)
        return JNI_FALSE;
    GLint viewportBefore[4]{},viewportAfter[4]{};
    GLint modeBefore=0,modeAfter=0;
    GLint attribBefore=0,attribAfter=0,clientBefore=0,clientAfter=0;
    GLfloat colorBefore[4]{},colorAfter[4]{};
    glGetIntegerv(GL_VIEWPORT,viewportBefore);
    if(viewportBefore[0]!=0 || viewportBefore[1]!=0 ||
       viewportBefore[2]!=width || viewportBefore[3]!=height)
        return JNI_FALSE;
    glGetIntegerv(GL_MATRIX_MODE,&modeBefore);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&attribBefore);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH,&clientBefore);
    glGetFloatv(GL_CURRENT_COLOR,colorBefore);
    slinkywinky::WglMenuOverlayHost host;
    slinkywinky::MenuState menu;
    slinkywinky::MenuProfileBridge profile(menu);
    if(!host.open(context,dc,static_cast<unsigned>(width),
                  static_cast<unsigned>(height)))
        return JNI_FALSE;
    const bool drawn=host.draw(menu,&profile,mode);
    const bool closed=host.close();
    glGetIntegerv(GL_VIEWPORT,viewportAfter);
    glGetIntegerv(GL_MATRIX_MODE,&modeAfter);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&attribAfter);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH,&clientAfter);
    glGetFloatv(GL_CURRENT_COLOR,colorAfter);
    if(!drawn || !closed || modeBefore!=modeAfter ||
       attribBefore!=attribAfter || clientBefore!=clientAfter ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc)
        return JNI_FALSE;
    for(int i=0;i<4;i++)
        if(viewportBefore[i]!=viewportAfter[i] ||
           std::fabs(colorBefore[i]-colorAfter[i])>0.00001f)
            return JNI_FALSE;
    return JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
    JNIEnv*,jclass,jlong context,jlong dc,jint width,jint height) {
    return drawValidatedMenuOverlay(context,dc,width,height,
        slinkywinky::MenuGL11Mode::BoundedOverlay);
}
// R93 only: the same GL ownership and state guards, a 70%-scale panel.
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
    JNIEnv*,jclass,jlong context,jlong dc,jint width,jint height) {
    return drawValidatedMenuOverlay(context,dc,width,height,
        slinkywinky::MenuGL11Mode::CompactOverlay);
}

#endif
