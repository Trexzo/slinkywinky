#include "slinkywinky/bridge.h"
#include <jni.h>
#include <jvmti.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <atomic>

namespace {
bool known_minecraft_class(const char* signature) {
    return signature &&
        (std::strcmp(signature, "Lnet/minecraft/client/Minecraft;") == 0 ||
         std::strcmp(signature, "Lave;") == 0); // Vanilla 1.8.9 mapped class
}
bool inspect_vm(JavaVM* vm, bool require_game) {
    jvmtiEnv* ti = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&ti), JVMTI_VERSION_1_0) != JNI_OK || !ti)
        return false;
    jint count = 0;
    jclass* classes = nullptr;
    if (ti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return false;
    bool found_game = false;
    for (jint i = 0; i < count; ++i) {
        char* signature = nullptr;
        char* generic = nullptr;
        if (ti->GetClassSignature(classes[i], &signature, &generic) == JVMTI_ERROR_NONE) {
            found_game = found_game || known_minecraft_class(signature);
        }
        if (signature) ti->Deallocate(reinterpret_cast<unsigned char*>(signature));
        if (generic) ti->Deallocate(reinterpret_cast<unsigned char*>(generic));
    }
    if (classes) ti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    std::printf("[SlinkyWinkyAgent] loadedClasses=%d minecraftClass=%d\n",
                static_cast<int>(count), found_game ? 1 : 0);
    return !require_game || found_game;
}
// Opt-in diagnostic only. JVMTI breakpoint runs on the target's actual game
// thread and is automatically cleared after 20 callbacks. No game mutations.
std::atomic<int> g_tick_count{0};
std::atomic<bool> g_tick_enabled{false};
jmethodID g_tick_method = nullptr;
jlocation g_tick_location = 0;
void JNICALL on_breakpoint(jvmtiEnv* ti, JNIEnv*, jthread,
                           jmethodID method, jlocation location) {
    if (!g_tick_enabled.load() || method != g_tick_method || location != g_tick_location)
        return;
    const int n = g_tick_count.fetch_add(1) + 1;
    if (n == 1) {
        // Test the C++ engine on the exact admitted callback thread, with zero actions.
        constexpr std::uint32_t seed[4] = {1, 2, 3, 4};
        SWContext* context = nullptr;
        bool good = sw_create(seed, &context) == 0 && context;
        if (good) {
            SWFrame frame{}; frame.abi_version = SW_ABI_VERSION;
            frame.time_ms = 1000; frame.generation = 1; frame.world_valid = 1;
            SWSettings settings{}; settings.abi_version = SW_ABI_VERSION;
            SWOutput output{};
            good = sw_step(context, &frame, &settings, &output) == 0 &&
                   !output.attempt_attack && !output.has_rotation &&
                   !output.left_held && !output.right_held;
            sw_destroy(&context);
        }
        std::printf("[SlinkyWinkyAgent] GAME_THREAD_NATIVE_PROBE_%s\n",
                    good ? "PASS" : "FAIL");
        std::fflush(stdout);
    }
    if (n >= 20) {
        g_tick_enabled.store(false);
        ti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_BREAKPOINT, nullptr);
        ti->ClearBreakpoint(g_tick_method, g_tick_location);
        std::printf("[SlinkyWinkyAgent] GAME_THREAD_TICK_PROBE_PASS ticks=%d\n", n);
        std::fflush(stdout);
    }
}
bool install_tick_probe(JavaVM* vm) {
    jvmtiEnv* ti = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&ti), JVMTI_VERSION_1_0) != JNI_OK || !ti)
        return false;
    jvmtiCapabilities capabilities{};
    capabilities.can_generate_breakpoint_events = 1;
    const jvmtiError capError = ti->AddCapabilities(&capabilities);
    if (capError != JVMTI_ERROR_NONE) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] breakpoint capability unavailable err=%d\n",
                     static_cast<int>(capError));
        return false;
    }
    jint count = 0; jclass* classes = nullptr;
    if (ti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return false;
    jmethodID tick = nullptr;
    jlocation start = 0;
    for (jint i = 0; i < count && !tick; ++i) {
        char* signature = nullptr; char* generic = nullptr;
        bool minecraft = ti->GetClassSignature(classes[i], &signature, &generic) ==
                         JVMTI_ERROR_NONE && known_minecraft_class(signature);
        if (signature) ti->Deallocate(reinterpret_cast<unsigned char*>(signature));
        if (generic) ti->Deallocate(reinterpret_cast<unsigned char*>(generic));
        if (!minecraft) continue;
        std::fprintf(stderr, "[SlinkyWinkyAgent] inspect Minecraft method list\n");
        jint methodCount = 0; jmethodID* methods = nullptr;
        const jvmtiError methodError = ti->GetClassMethods(classes[i], &methodCount, &methods);
        if (methodError != JVMTI_ERROR_NONE) {
            std::fprintf(stderr, "[SlinkyWinkyAgent] GetClassMethods failed err=%d\n",
                         static_cast<int>(methodError));
            break;
        }
        for (jint m = 0; m < methodCount && !tick; ++m) {
            char* name = nullptr; char* desc = nullptr; char* gen = nullptr;
            if (ti->GetMethodName(methods[m], &name, &desc, &gen) == JVMTI_ERROR_NONE &&
                name && desc && std::strcmp(desc, "()V") == 0 &&
                (std::strcmp(name, "runTick") == 0 ||
                 std::strcmp(name, "func_71407_l") == 0)) {
                jlocation end = 0;
                if (ti->GetMethodLocation(methods[m], &start, &end) == JVMTI_ERROR_NONE)
                    tick = methods[m];
            }
            if (name) ti->Deallocate(reinterpret_cast<unsigned char*>(name));
            if (desc) ti->Deallocate(reinterpret_cast<unsigned char*>(desc));
            if (gen) ti->Deallocate(reinterpret_cast<unsigned char*>(gen));
        }
        if (methods) ti->Deallocate(reinterpret_cast<unsigned char*>(methods));
    }
    if (classes) ti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    if (!tick) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] runTick method not mapped; no hook installed\n");
        return false; // Do not guess obfuscated method IDs.
    }
    jvmtiEventCallbacks callbacks{};
    callbacks.Breakpoint = &on_breakpoint;
    const jvmtiError callbackError = ti->SetEventCallbacks(&callbacks, sizeof(callbacks));
    if (callbackError != JVMTI_ERROR_NONE) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] SetEventCallbacks failed err=%d\n",
                     static_cast<int>(callbackError));
        return false;
    }
    g_tick_method = tick; g_tick_location = start;
    g_tick_count.store(0); g_tick_enabled.store(true);
    const jvmtiError bpError = ti->SetBreakpoint(tick, start);
    const jvmtiError eventError = bpError == JVMTI_ERROR_NONE ?
        ti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, nullptr) : bpError;
    if (bpError != JVMTI_ERROR_NONE || eventError != JVMTI_ERROR_NONE) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] SetBreakpoint/Event failed err=%d,%d\n",
                     static_cast<int>(bpError), static_cast<int>(eventError));
        g_tick_enabled.store(false);
        ti->ClearBreakpoint(tick, start);
        return false;
    }
    std::printf("[SlinkyWinkyAgent] read-only game tick probe installed\n");
    std::fflush(stdout);
    return true;
}
bool engine_probe() {
    constexpr std::uint32_t seed[4] = {1, 2, 3, 4};
    SWContext* context = nullptr;
    if (sw_create(seed, &context) != 0 || !context) return false;
    SWFrame frame{};
    frame.abi_version = SW_ABI_VERSION;
    frame.time_ms = 1000;
    frame.generation = 1;
    frame.world_valid = 1;
    SWSettings settings{};
    settings.abi_version = SW_ABI_VERSION;
    SWOutput output{};
    const bool good = sw_step(context, &frame, &settings, &output) == 0 &&
                      output.abi_version == SW_ABI_VERSION &&
                      !output.left_held && !output.right_held &&
                      !output.attempt_attack && !output.has_rotation && !output.has_reach;
    sw_destroy(&context);
    return good;
}
}

extern "C" JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char* options, void*) {
    // Public JDK8 Attach API; no Win32 remote-thread injection, stealth hooks or Forge.
    const bool test = options && std::strcmp(options, "probe") == 0;
    const bool game = options && std::strcmp(options, "minecraft") == 0;
    const bool ticks = options && std::strcmp(options, "minecraft-ticks") == 0;
    if (!vm || (!test && !game && !ticks)) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] invalid attach mode\n");
        return JNI_ERR;
    }
    if (!inspect_vm(vm, game || ticks) || !engine_probe() ||
        (ticks && !install_tick_probe(vm))) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] rejected JVM or native core admission\n");
        return JNI_ERR;
    }
    std::printf("[SlinkyWinkyAgent] ATTACH_PROBE_PASS mode=%s (no gameplay hooks installed)\n",
                ticks ? "minecraft-ticks" : (game ? "minecraft" : "probe"));
    std::fflush(stdout);
    // Agent remains mapped by the JVM. Real event hooks and game-thread callbacks
    // must be independently validated before this becomes a playable client.
    return JNI_OK;
}
