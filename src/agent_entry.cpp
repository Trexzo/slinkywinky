#include "slinkywinky/bridge.h"
#include <jni.h>
#include <jvmti.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

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
    if (!vm || (!test && !game)) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] invalid attach mode\n");
        return JNI_ERR;
    }
    if (!inspect_vm(vm, game) || !engine_probe()) {
        std::fprintf(stderr, "[SlinkyWinkyAgent] rejected JVM or native core admission\n");
        return JNI_ERR;
    }
    std::printf("[SlinkyWinkyAgent] ATTACH_PROBE_PASS mode=%s (no gameplay hooks installed)\n",
                game ? "minecraft" : "probe");
    std::fflush(stdout);
    // Agent remains mapped by the JVM. Real event hooks and game-thread callbacks
    // must be independently validated before this becomes a playable client.
    return JNI_OK;
}
