// R204 Java 8 => real first-party C++ ObservationFrame => Sprint policy.
// Read-only JNI surface: NO key presses, game memory writes, hooks, packets,
// player setters, or native library injection.
#include "slinkywinky/sprint_observation_preview.hpp"
#include "slinkywinky/strafe_controller.hpp"
#include <jni.h>
#include <cstdint>
#include <cmath>
#include <string>
#include <map>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

namespace {
using slinkywinky::reference_compat::SprintObservationPreview;
std::mutex g_sessions_mutex;
std::map<jlong, std::unique_ptr<SprintObservationPreview>> g_sessions;
// R224: a read-only preview scoped to the SAME native module owner/session.
// Not an original event-21/22 hook or a future gameplay action gate.
struct StrafeReadOnlySession {
    std::thread::id owner=std::this_thread::get_id();
    std::uint64_t generation=0;
    std::uint64_t last_tick=0;
    slinkywinky::reference_compat::StrafeController controller{};
};
std::map<jlong,StrafeReadOnlySession> g_strafe_observers;
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
        if (g_next_id <= 0 || g_next_id == std::numeric_limits<jlong>::max()) {
            throw_java(env, "java/lang/IllegalStateException", "Sprint preview session IDs exhausted");
            return 0;
        }
        const jlong id = g_next_id++;
        g_sessions.emplace(id, std::make_unique<SprintObservationPreview>());
        try {
            g_strafe_observers.emplace(id,StrafeReadOnlySession{});
        } catch(...) {
            g_sessions.erase(id);
            throw;
        }
        return id;
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "Could not create read-only Sprint preview");
        return 0;
    }
}

namespace {
// Both the legacy 9-float Sprint JNI result and the new R211 15-float
// three-module result use exactly one admission/dispatch path.
jfloatArray observe_modules(
    JNIEnv* env, jlong id, jlongArray clocks, jintArray flags,
    jdoubleArray coords, jfloatArray angles, jfloatArray movement,
    jintArray module_context, jfloatArray passive_no_slow_settings) {
    const bool extended = module_context || passive_no_slow_settings;
    if (!clocks || !flags || !coords || !angles || !movement ||
        env->GetArrayLength(clocks) != 2 ||
        env->GetArrayLength(flags) != 8 ||
        env->GetArrayLength(coords) != 6 ||
        env->GetArrayLength(angles) != 2 ||
        env->GetArrayLength(movement) != 3 ||
        (extended && (!module_context || !passive_no_slow_settings ||
          env->GetArrayLength(module_context) != 5 ||
          env->GetArrayLength(passive_no_slow_settings) != 3))) {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "Invalid module observation array dimensions");
        return nullptr;
    }
    jlong clock[2] = {};
    jint flag[8] = {}, extra[5] = {};
    jdouble xyz[6] = {};
    jfloat rot[2] = {}, move[3] = {}, passive[3] = {};
    env->GetLongArrayRegion(clocks, 0, 2, clock);
    env->GetIntArrayRegion(flags, 0, 8, flag);
    env->GetDoubleArrayRegion(coords, 0, 6, xyz);
    env->GetFloatArrayRegion(angles, 0, 2, rot);
    env->GetFloatArrayRegion(movement, 0, 3, move);
    if (extended) {
        env->GetIntArrayRegion(module_context, 0, 5, extra);
        env->GetFloatArrayRegion(passive_no_slow_settings, 0, 3, passive);
    }
    if (env->ExceptionCheck()) return nullptr;
    if (clock[0] < 0 || clock[1] <= 0 ||
        flag[0] < 0 || flag[0] > 7) {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "Invalid module observation clock or flags");
        return nullptr;
    }
    for (unsigned i = 1; i < 8; ++i)
        if (!boolean_flag(flag[i])) {
            throw_java(env, "java/lang/IllegalArgumentException",
                       "Module boolean flag not 0 or 1");
            return nullptr;
        }
    const bool player = (flag[0] & 3) == 3;
    if (extended) {
        if (!boolean_flag(extra[0]) || !boolean_flag(extra[2]) ||
            !boolean_flag(extra[3]) || (extra[0] != 0) != player ||
            (extra[2] != 0) != (flag[3] != 0) ||
            extra[1] < 0 || extra[1] > 10000 ||
            extra[4] < -1 || extra[4] > 32767 ||
            !std::isfinite(move[0]) || !std::isfinite(move[1]) ||
            !std::isfinite(move[2]) ||
            !std::isfinite(passive[0]) || !std::isfinite(passive[1]) ||
            !std::isfinite(passive[2]) ||
            passive[0] < 0 || passive[1] < 0 || passive[2] < 0) {
            throw_java(env, "java/lang/IllegalArgumentException",
                       "Inconsistent No Jump Delay / No Slow player inputs");
            return nullptr;
        }
    }

    try {
        std::lock_guard<std::mutex> lock(g_sessions_mutex);
        const auto it = g_sessions.find(id);
        if (it == g_sessions.end()) {
            throw_java(env, "java/lang/IllegalStateException",
                       "Unknown or closed module-control session");
            return nullptr;
        }
        slinkywinky::reference_compat::SprintObservedTick tick;
        tick.observation.time_ms = static_cast<std::uint64_t>(clock[0]);
        tick.observation.generation = static_cast<std::uint64_t>(clock[1]);
        tick.observation.flags = static_cast<std::uint32_t>(flag[0]);
        for (unsigned i = 0; i < 6; ++i) tick.observation.xyz[i] = xyz[i];
        for (unsigned i = 0; i < 2; ++i) tick.observation.angles[i] = rot[i];
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
        if (extended && player) {
            tick.jump_delay =
                slinkywinky::reference_compat::JumpDelayInput{true, extra[1]};
            slinkywinky::reference_compat::NoSlowInput no_slow;
            no_slow.using_item = extra[2] != 0;
            no_slow.held_item_present = extra[3] != 0;
            no_slow.item_id = extra[4];
            no_slow.forward = move[0];
            no_slow.strafe = move[1];
            no_slow.settings = {passive[0],passive[1],passive[2]};
            tick.no_slow = no_slow;
        }
        const auto result = it->second->observe(tick);
        // R211 appends six fields after the exact original nine:
        // jumpProposalPresent, jumpTicks, noSlowPolicyEvaluated,
        // noSlowReplaceInput, noSlowStrafe, noSlowForward.
        const jfloat values[15] = {
            result.admitted ? 1.0f : 0.0f,
            result.world_ready ? 1.0f : 0.0f,
            result.session_transition ? 1.0f : 0.0f,
            result.update.proposed_key_pressed ? 1.0f : 0.0f,
            result.update.proposed_player_sprinting.has_value() ? 1.0f : 0.0f,
            result.update.proposed_player_sprinting.value_or(false) ? 1.0f : 0.0f,
            result.movement.proposed_speed.has_value() ? 1.0f : 0.0f,
            result.movement.proposed_speed.value_or(0.0f),
            result.movement.propose_set_sprinting ? 1.0f : 0.0f,
            result.proposed_jump_ticks.has_value() ? 1.0f : 0.0f,
            static_cast<jfloat>(result.proposed_jump_ticks.value_or(0)),
            result.no_slow.has_value() ? 1.0f : 0.0f,
            result.no_slow && result.no_slow->replace_input ? 1.0f : 0.0f,
            result.no_slow ? result.no_slow->strafe : 0.0f,
            result.no_slow ? result.no_slow->forward : 0.0f
        };
        const jsize count = extended ? 15 : 9;
        jfloatArray output = env->NewFloatArray(count);
        if (output) env->SetFloatArrayRegion(output, 0, count, values);
        return output;
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Read-only native module evaluation failed");
        return nullptr;
    }
}
} // namespace

extern "C" JNIEXPORT jfloatArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeObserve(
    JNIEnv* env, jclass, jlong id,
    jlongArray clocks, jintArray flags,
    jdoubleArray coords, jfloatArray angles, jfloatArray movement) {
    return observe_modules(env, id, clocks, flags, coords, angles, movement,
                           nullptr, nullptr);
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeObserveModules(
    JNIEnv* env, jclass, jlong id,
    jlongArray clocks, jintArray flags, jdoubleArray coords,
    jfloatArray angles, jfloatArray movement,
    jintArray module_context, jfloatArray passive_no_slow_settings) {
    return observe_modules(env, id, clocks, flags, coords, angles, movement,
                           module_context, passive_no_slow_settings);
}

// R210: stable first-party module-control JNI ABI; no Minecraft actions.
// All control requests are enforced by the session's original owner thread.
extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeSetEnabled(
    JNIEnv* env, jclass, jlong id, jboolean enable) {
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() ||
        !it->second->request_module({"Sprint", enable == JNI_TRUE})) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Sprint module state request rejected or foreign thread");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeSetSettings(
    JNIEnv* env, jclass, jlong id, jint mask) {
    if (mask < 0 || mask > 15) {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "Unsupported Sprint settings bitmask");
        return;
    }
    slinkywinky::reference_compat::SprintSettings settings;
    settings.while_in_gui = (mask & 1) != 0;
    settings.while_using_item = (mask & 2) != 0;
    settings.while_backwards = (mask & 4) != 0;
    settings.while_sideways = (mask & 8) != 0;
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() || !it->second->configure_sprint(settings)) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Sprint settings rejected or foreign thread");
    }
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeControlState(
    JNIEnv* env, jclass, jlong id) {
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end()) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Unknown Sprint module-control session");
        return nullptr;
    }
    const auto state = it->second->control_state();
    if (!state) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Sprint state read refused from foreign or stopped thread");
        return nullptr;
    }
    const jlong values[5] = {
        static_cast<jlong>(slinkywinky::kModuleControlContractVersion),
        state->requested_enabled ? 1 : 0,
        state->active_in_world ? 1 : 0,
        state->settings_override ? 1 : 0,
        static_cast<jlong>(state->world_generation)
    };
    jlongArray out = env->NewLongArray(5);
    if (out) env->SetLongArrayRegion(out, 0, 5, values);
    return out;
}

namespace {
bool extract_module_name(JNIEnv* env, jstring name, std::string& out) {
    if (!name || env->GetStringLength(name) > 32) {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "Null or oversized module name");
        return false;
    }
    const char* utf8 = env->GetStringUTFChars(name, nullptr);
    if (!utf8) return false;
    out.assign(utf8);
    env->ReleaseStringUTFChars(name, utf8);
    if (out != "Sprint" && out != "No Jump Delay" && out != "No Slow") {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "Unknown canonical module name");
        return false;
    }
    return true;
}
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeSetModuleEnabled(
    JNIEnv* env, jclass, jlong id, jstring name, jboolean enable) {
    std::string module;
    if (!extract_module_name(env, name, module)) return;
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() ||
        !it->second->request_module({module, enable == JNI_TRUE})) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Native module toggle refused or foreign owner thread");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeSetNoSlowSettings(
    JNIEnv* env, jclass, jlong id,
    jfloat bow, jfloat sword, jfloat consumable) {
    if (!std::isfinite(bow) || !std::isfinite(sword) ||
        !std::isfinite(consumable) ||
        bow < 0.0f || sword < 0.0f || consumable < 0.0f) {
        throw_java(env, "java/lang/IllegalArgumentException",
                   "No Slow multipliers must be finite and nonnegative");
        return;
    }
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() ||
        !it->second->configure_no_slow({bow,sword,consumable})) {
        throw_java(env, "java/lang/IllegalStateException",
                   "No Slow settings refused or foreign owner thread");
    }
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeModuleControlState(
    JNIEnv* env, jclass, jlong id, jstring name) {
    std::string module;
    if (!extract_module_name(env, name, module)) return nullptr;
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end()) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Unknown native module-control session");
        return nullptr;
    }
    const auto state = it->second->control_state(module);
    if (!state) {
        throw_java(env, "java/lang/IllegalStateException",
                   "Module state refused or foreign owner thread");
        return nullptr;
    }
    const jlong data[5] = {
        static_cast<jlong>(slinkywinky::kModuleControlContractVersion),
        state->requested_enabled ? 1 : 0,
        state->active_in_world ? 1 : 0,
        state->settings_override ? 1 : 0,
        static_cast<jlong>(state->world_generation)
    };
    jlongArray out = env->NewLongArray(5);
    if (out) env->SetLongArrayRegion(out, 0, 5, data);
    return out;
}

extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeClose(JNIEnv* env, jclass, jlong id) {
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it = g_sessions.find(id);
    if (it == g_sessions.end() || !it->second->shutdown()) {
        throw_java(env, "java/lang/IllegalStateException", "Unknown or foreign-thread Sprint session");
        return;
    }
    g_strafe_observers.erase(id);
    g_sessions.erase(it);
}


// R224: attached to the existing, owner-pinned SprintObservationProbe native
// session, but invokes only the independent first-party read-only Strafe
// decision. Event21 sprint snapshot and event22 movement observation are
// delivered from the SAME Forge player-tick callback, not asserted to be the
// original binary's callback timing. No Minecraft field, key or packet writes.
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_SprintObservationProbe_nativeObserveStrafe(
    JNIEnv* env,jclass,jlong handle,jlong generation,jlong tick,
    jboolean world,jboolean player,jboolean sprinting,
    jfloat forward,jfloat strafe,jfloat yaw,
    jboolean on_ground,jboolean jump_phase,
    jdouble motion_x,jdouble motion_y,jdouble motion_z) {
    if (generation<=0 || tick<=0 ||
        (world!=JNI_FALSE && world!=JNI_TRUE) ||
        (player!=JNI_FALSE && player!=JNI_TRUE) ||
        (sprinting!=JNI_FALSE && sprinting!=JNI_TRUE) ||
        (on_ground!=JNI_FALSE && on_ground!=JNI_TRUE) ||
        (jump_phase!=JNI_FALSE && jump_phase!=JNI_TRUE) ||
        (player==JNI_TRUE && world==JNI_FALSE) ||
        !std::isfinite(forward) || !std::isfinite(strafe) ||
        !std::isfinite(yaw) || !std::isfinite(motion_x) ||
        !std::isfinite(motion_y) || !std::isfinite(motion_z)) {
        throw_java(env,"java/lang/IllegalArgumentException",
                   "R224 malformed read-only movement observation");
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    const auto it=g_sessions.find(handle);
    const auto live=g_strafe_observers.find(handle);
    if (it==g_sessions.end() || live==g_strafe_observers.end() ||
        live->second.owner!=std::this_thread::get_id()) {
        throw_java(env,"java/lang/IllegalStateException",
                   "R224 closed or foreign native owner");
        return nullptr;
    }
    auto& session=live->second;
    if (world==JNI_FALSE || player==JNI_FALSE) {
        session.controller.reset();
        session.generation=0;
        session.last_tick=0;
    } else {
        // The same native Sprint session MUST have admitted an actual
        // world/player observation for this exact world generation.
        const auto state=it->second->control_state();
        if (!state || state->world_generation!=
                        static_cast<std::uint64_t>(generation) ||
            static_cast<std::uint64_t>(generation)<session.generation ||
            (session.generation==static_cast<std::uint64_t>(generation) &&
             static_cast<std::uint64_t>(tick)<=session.last_tick)) {
            throw_java(env,"java/lang/IllegalStateException",
                       "R224 stale or unadmitted world/tick");
            return nullptr;
        }
        if (session.generation!=static_cast<std::uint64_t>(generation)) {
            session.controller.reset();
            session.generation=static_cast<std::uint64_t>(generation);
            session.last_tick=0;
        }
        session.last_tick=static_cast<std::uint64_t>(tick);
        session.controller.observe_sprinting(sprinting==JNI_TRUE);
    }
    const bool admitted=world==JNI_TRUE && player==JNI_TRUE;
    slinkywinky::reference_compat::StrafeMotionDecision decision{};
    if (admitted) {
        slinkywinky::reference_compat::StrafeMotionInput input{};
        input.forward=forward;
        input.strafe=strafe;
        input.yaw_degrees=yaw;
        input.on_ground=on_ground==JNI_TRUE;
        input.jump_phase=jump_phase==JNI_TRUE;
        input.motion_x=motion_x;
        input.motion_y=motion_y;
        input.motion_z=motion_z;
        decision=session.controller.evaluate(input);
    }
    const auto& value=decision.proposed_motion;
    const jdouble fields[5]{
        admitted?1.0:0.0,value?1.0:0.0,
        value?value->x:0.0,value?value->y:0.0,value?value->z:0.0};
    jdoubleArray out=env->NewDoubleArray(5);
    if(out)env->SetDoubleArrayRegion(out,0,5,fields);
    return out;
}
