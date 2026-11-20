// R239 independent Java8 -> native Clutch planner transport.
// No game writes, original hooks, clicks, rotations, injected modules,
// recovered commercial binary code or pointer scanning.
#include "slinkywinky/clutch_observation_bridge.hpp"
#include <jni.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <vector>

namespace {
using namespace slinkywinky::reference_compat;
struct ClutchPreviewSession {
    std::thread::id owner=std::this_thread::get_id();
    ClutchObservationOwner gate{};
    SourceWalkBank<8> rng{};
    ClutchPreviewSession() {
        if(!rng.configure(std::array<std::uint32_t,4>{{1,2,3,4}}))
            throw std::runtime_error("Clutch preview seed refused");
    }
};
std::mutex g_clutch_mutex;
std::map<jlong,std::unique_ptr<ClutchPreviewSession>> g_clutch;
jlong g_next_clutch=1;

void fail(JNIEnv* env,const char* kind,const char* msg) {
    jclass type=env->FindClass(kind);
    if(type)env->ThrowNew(type,msg);
}
bool valid_integer(jlong n,jlong lo,jlong hi) noexcept {
    return n>=lo && n<=hi;
}
bool valid_float(double x) noexcept {
    return std::isfinite(x) &&
        std::fabs(x)<=static_cast<double>(std::numeric_limits<float>::max());
}
} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_ClutchNativePreview_nativeCreate(
    JNIEnv* env,jclass) {
    try {
        std::lock_guard<std::mutex> lock(g_clutch_mutex);
        if(g_next_clutch<=0 ||
           g_next_clutch==std::numeric_limits<jlong>::max()) {
            fail(env,"java/lang/IllegalStateException",
                "Clutch preview handles exhausted");
            return 0;
        }
        const jlong handle=g_next_clutch++;
        g_clutch.emplace(handle,std::make_unique<ClutchPreviewSession>());
        return handle;
    } catch(...) {
        fail(env,"java/lang/IllegalStateException",
            "Clutch preview native owner creation failed");
        return 0;
    }
}

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_ClutchNativePreview_nativeEvaluate(
    JNIEnv* env,jclass,jlong handle,jlongArray scope,
    jdoubleArray geometry,jintArray candidates,jintArray ground) {
    if(!scope || !geometry || !candidates || !ground ||
       env->GetArrayLength(scope)!=10 ||
       env->GetArrayLength(geometry)!=15 ||
       env->GetArrayLength(ground)!=8) {
        fail(env,"java/lang/IllegalArgumentException",
            "Malformed Clutch JNI snapshot arrays");
        return nullptr;
    }
    jlong s[10]{};
    jdouble g[15]{};
    jint ground_ids[8]{};
    env->GetLongArrayRegion(scope,0,10,s);
    env->GetDoubleArrayRegion(geometry,0,15,g);
    env->GetIntArrayRegion(ground,0,8,ground_ids);
    if(env->ExceptionCheck())return nullptr;
    if(!valid_integer(s[0],1,std::numeric_limits<jlong>::max()) ||
       !valid_integer(s[1],1,std::numeric_limits<jlong>::max()) ||
       !valid_integer(s[2],1,std::numeric_limits<jlong>::max()) ||
       !valid_integer(s[3],1,std::numeric_limits<jlong>::max()) ||
       !valid_integer(s[4],-999999,999999) ||
       !valid_integer(s[5],-999999,999999) ||
       !valid_integer(s[6],-999999,999999) ||
       !valid_integer(s[7],0,7) ||
       !valid_integer(s[8],0,1) ||
       s[9]!=-1 ||
       env->GetArrayLength(candidates)!=
           static_cast<jsize>(kClutchDistanceCounts[static_cast<std::size_t>(s[7])])) {
        fail(env,"java/lang/IllegalArgumentException",
            "Malformed Clutch JNI source scope");
        return nullptr;
    }
    for(double v:g)if(!valid_float(v)) {
        fail(env,"java/lang/IllegalArgumentException",
            "Nonfinite or out-of-range Clutch geometry");
        return nullptr;
    }
    std::vector<jint> observed(static_cast<std::size_t>(
        env->GetArrayLength(candidates)));
    env->GetIntArrayRegion(candidates,0,
        static_cast<jsize>(observed.size()),observed.data());
    if(env->ExceptionCheck())return nullptr;
    try {
        std::lock_guard<std::mutex> lock(g_clutch_mutex);
        const auto it=g_clutch.find(handle);
        if(it==g_clutch.end() || it->second->owner!=std::this_thread::get_id()) {
            fail(env,"java/lang/IllegalStateException",
                "Unknown, stale or foreign native Clutch session");
            return nullptr;
        }
        ClutchObservedFrame f{};
        f.generation=static_cast<std::uint64_t>(s[0]);
        f.tick=static_cast<std::uint64_t>(s[1]);
        f.world_token=static_cast<std::uint64_t>(s[2]);
        f.player_token=static_cast<std::uint64_t>(s[3]);
        f.base_x=static_cast<int>(s[4]);
        f.base_y=static_cast<int>(s[5]);
        f.base_z=static_cast<int>(s[6]);
        f.range_index=static_cast<unsigned>(s[7]);
        f.ground_blocked=s[8]==1;
        f.previous_block_id=-1;
        f.box_lo={g[0],g[1],g[2]};
        f.box_hi={g[3],g[4],g[5]};
        f.eye={g[6],g[7],g[8]};
        f.current_yaw=static_cast<float>(g[9]);
        f.current_pitch=static_cast<float>(g[10]);
        f.min_height=static_cast<float>(g[11]);
        f.max_aim_degrees=static_cast<float>(g[12]);
        f.randomisation=static_cast<float>(g[13]);
        f.clock_seconds=g[14];
        f.candidate_block_ids.assign(observed.begin(),observed.end());
        for(unsigned i=0;i<8;++i)f.ground_block_ids[i]=ground_ids[i];

        auto& session=*it->second;
        const auto world=session.gate.consume(f);
        if(!world) {
            fail(env,"java/lang/IllegalStateException",
                "Native Clutch snapshot ownership or evidence refused");
            return nullptr;
        }
        // R235 source planner operates on already validated block evidence.
        // A copy prevents invalid source sampling from advancing shared RNG.
        const auto rng_before=session.rng;
        const auto plan=clutch_plan_original(*world,session.rng);
        if(!plan.admitted) {
            session.rng=rng_before;
            session.gate.close();
            fail(env,"java/lang/IllegalStateException",
                "Native Clutch planner refused supplied world evidence");
            return nullptr;
        }
        // R241: preserve native source-computed aim and signed rotation
        // deltas as read-only values. This is NOT a camera write.
        const jdouble value[15]{
            1.0,
            plan.found?1.0:0.0,
            plan.face_up?1.0:0.0,
            static_cast<jdouble>(plan.source_index),
            static_cast<jdouble>(plan.selected.x),
            static_cast<jdouble>(plan.selected.y),
            static_cast<jdouble>(plan.selected.z),
            static_cast<jdouble>(plan.visited),
            static_cast<jdouble>(plan.angular_cost),
            plan.aim.x,plan.aim.y,plan.aim.z,
            static_cast<jdouble>(plan.aim_yaw_diff),
            static_cast<jdouble>(plan.aim_pitch_diff),
            plan.preserved_prior?1.0:0.0
        };
        jdoubleArray result=env->NewDoubleArray(15);
        if(result)env->SetDoubleArrayRegion(result,0,15,value);
        return result;
    } catch(...) {
        fail(env,"java/lang/IllegalStateException",
            "Native Clutch read-only evaluation failed");
        return nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_ClutchNativePreview_nativeClose(
    JNIEnv* env,jclass,jlong handle) {
    std::lock_guard<std::mutex> lock(g_clutch_mutex);
    const auto it=g_clutch.find(handle);
    if(it==g_clutch.end() || it->second->owner!=std::this_thread::get_id()) {
        fail(env,"java/lang/IllegalStateException",
            "Native Clutch close refused stale or foreign owner");
        return;
    }
    it->second->gate.close();
    g_clutch.erase(it);
}
