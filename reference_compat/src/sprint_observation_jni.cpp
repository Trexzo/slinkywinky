// R204 Java 8 => real first-party C++ ObservationFrame => Sprint policy.
// Read-only JNI surface: NO key presses, game memory writes, hooks, packets,
// player setters, or native library injection.
#include "slinkywinky/sprint_observation_preview.hpp"
#include <jni.h>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace {
using slinkywinky::reference_compat::SprintObservationPreview;
std::mutex g_sessions_mutex;
std::map<jlong, std::unique_ptr<SprintObservationPreview>> g_sessions;
jlong g_next_id = 1;

void throw_java(JNIEnv* env, const char* type, const char* reason) {
    jclass exception = env->FindClass(type);
    if (exception) env->ThrowNew(exception, reason);
}
bool boolean_flag(jint n) noexcept { return n == 0 || n == 1; }
} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeCreate(JNIEnv* env, jclass) {
    try {
        std::lock_guard<std::mutex> lock(g_sessions_mutex);
        if (g_next_id <= 0) {
            throw_java(env, "java/lang/IllegalStateException", "Sprint preview session IDs exhausted");
            return 0;
        }
        const jlong id = g_next_id++;
        g_sessions.emplace(id, std::make_unique<SprintObservationPreview>());
        return id;
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "Could not create read-only Sprint preview");
        return 0;
    }
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeObserve(
    JNIEnv* env, jclass, jlong id,
    jlongArray clocks, jintArray flags,
    jdoubleArray coords, jfloatArray angles, jfloatArray movement) {
    if (!clocks || !flags || !coords || !angles || !movement ||
        env->GetArrayLength(clocks) != 2 ||
        env->GetArrayLength(flags) != 8 ||
        env->GetArrayLength(coords) != 6 ||
        env->GetArrayLength(angles) != 2 ||
        env->GetArrayLength(movement) != 3) {
        throw_java(env, "java/lang/IllegalArgumentException", "Invalid read-only Sprint frame dimensions");
        return nullptr;
    }
    jlong clock[2] = {};
    jint flag[8] = {};
    jdouble xyz[6] = {};
    jfloat rot[2] = {}, move[3] = {};
    env->GetLongArrayRegion(clocks, 0, 2, clock);
    env->GetIntArrayRegion(flags, 0, 8, flag);
    env->GetDoubleArrayRegion(coords, 0, 6, xyz);
    env->GetFloatArrayRegion(angles, 0, 2, rot);
    env->GetFloatArrayRegion(movement, 0, 3, move);
    if (env->ExceptionCheck()) return nullptr;
    if (clock[0] < 0 || clock[1] <= 0 || flag[0] < 0 || flag[0] > 7) {
        throw_java(env, "java/lang/IllegalArgumentException", "Invalid Sprint observation clock or flags");
        return nullptr;
    }
    for (unsigned i = 1; i < 8; ++i)
        if (!boolean_flag(flag[i])) {
            throw_java(env, "java/lang/IllegalArgumentException", "Sprint boolean flag not 0 or 1");
            return nullptr;
        }

    try {
        std::lock_guard<std::mutex> lock(g_sessions_mutex);
        const auto it = g_sessions.find(id);
        if (it == g_sessions.end()) {
            throw_java(env, "java/lang/IllegalStateException", "Unknown or closed Sprint session");
            return nullptr;
        }
        slinkywinky::reference_compat::SprintObservedTick tick;
        tick.observation.time_ms = static_cast<std::uint64_t>(clock[0]);
        tick.observation.generation = static_cast<std::uint64_t>(clock[1]);
        tick.observation.flags = static_cast<std::uint32_t>(flag[0]);
        for (unsigned i = 0; i < 6; ++i) tick.observation.xyz[i] = xyz[i];
        for (unsigned i = 0; i < 2; ++i) tick.observation.angles[i] = rot[i];
        const bool player = (flag[0] & 3) == 3;
        tick.update.player_present = player;
        tick.update.gui_open = flag[1] != 0;
        tick.update.player_sprinting = flag[2] != 0;
        tick.update.settings.while_in_gui = flag[4] != 0;
        tick.movement.player_present = player;
        tick.movement.using_item = flag[3] != 0;
        tick.movement.settings.while_in_gui = flag[4] != 0;
        tick.movement.settings.while_using_item = flag[5] != 0;
        tick.movement.settings.while_backwards = flag[6] != 0;
        tick.movement.settings.while_sideways = flag[7] != 0;
        tick.movement.forward = move[0];
        tick.movement.strafe = move[1];
        tick.movement.speed = move[2];

        const auto result = it->second->observe(tick);
        // All values are finite (admission checks and policy bounds);
        // no game actions are performed by this native call.
        const jfloat values[9] = {
            result.admitted ? 1.0f : 0.0f,
            result.world_ready ? 1.0f : 0.0f,
            result.session_transition ? 1.0f : 0.0f,
            result.update.proposed_key_pressed ? 1.0f : 0.0f,
            result.update.proposed_player_sprinting.has_value() ? 1.0f : 0.0f,
            result.update.proposed_player_sprinting.value_or(false) ? 1.0f : 0.0f,
            result.movement.proposed_speed.has_value() ? 1.0f : 0.0f,
            result.movement.proposed_speed.value_or(0.0f),
            result.movement.propose_set_sprinting ? 1.0f : 0.0f
        };
        jfloatArray output = env->NewFloatArray(9);
        if (output) env->SetFloatArrayRegion(output, 0, 9, values);
        return output;
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "Read-only Sprint preview evaluation failed");
        return nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeClose(JNIEnv* env, jclass, jlong id) {
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() || !it->second->shutdown()) {
        throw_java(env, "java/lang/IllegalStateException", "Unknown or foreign-thread Sprint session");
        return;
    }
    g_sessions.erase(it);
}
