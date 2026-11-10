// R195 independent synthetic harness; links a separately compiled COFF object.
// No third-party source, original DLL, JNI JVM, Minecraft or injector involved.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>

using Handler = void (*)(void *);
struct Module;
struct Reg { Module **list; unsigned long long n; };
struct Mgr {
    Module **mods;
    unsigned long long count;
    Reg cat[5];
    Reg reg[27];
    Handler *lists[27];
    Module **en_list;
    unsigned long long en_count;
    Module **dis_list;
    unsigned long long dis_count;
    void (*set308)(void *, int);
};
struct HSlot { Handler f; void *x; };
struct Module {
    const char *name;
    char padding_08[8];
    unsigned cat;
    unsigned char enabled;
    char padding_15[0x33];
    HSlot h[27];
};
struct HCtx {
    void *jvm;
    Mgr *module_manager;
    void *profile_list;
    void *minecraft;
    void *world;
    void *player;
};
struct QEvent {
    int type;
    int padding_04;
    Module *mod;
    unsigned char cancel;
    char rest[0x37];
};
struct ToggleEvent {
    long long type;
    HCtx *ctx;
    Module *mod;
    unsigned char on;
    char rest[0x2f];
};

static_assert(sizeof(void *) == 8, "This harness requires x64");
static_assert(offsetof(Mgr, count) == 0x8, "Mgr count layout");
static_assert(offsetof(Mgr, reg) == 0x60, "Mgr event registry layout");
static_assert(offsetof(Mgr, lists) == 0x210, "Mgr handler-list layout");
static_assert(offsetof(Mgr, en_list) == 0x2e8, "Mgr enable queue");
static_assert(offsetof(Mgr, dis_list) == 0x2f8, "Mgr disable queue");
static_assert(offsetof(Mgr, set308) == 0x308, "Mgr state callback");
static_assert(offsetof(Module, cat) == 0x10, "Module category layout");
static_assert(offsetof(Module, enabled) == 0x14, "Module enabled layout");
static_assert(offsetof(Module, h) == 0x48, "Module handler layout");
static_assert(offsetof(HSlot, x) == 8, "Handler priority layout");

extern "C" {
int create_module_manager(Mgr *, void (*)(void *, int));
bool queue_module_toggle(Mgr *, Module *, unsigned char);
void apply_module_toggles(HCtx *);
void destroy_module_manager(Mgr *);
void dispatch_event(Mgr *, unsigned *);
bool has_queued_modules(Mgr *);
unsigned module_manager_count = 0;
Module *ptr_to_aim_assist_module_descriptor[67] = {};
}

namespace {
Module modules[67] = {};
std::size_t allocations = 0;
std::size_t releases = 0;
unsigned released_owners = 0;
unsigned delivered_event_zero = 0;
unsigned delivered_toggle = 0;
unsigned delivered_disable_request = 0;
unsigned emitted_order[4] = {};
unsigned order_cursor = 0;
bool veto_disable = false;

void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
void state_setter(void *, int) {}
void event_low(void *event) {
    require(*static_cast<unsigned *>(event) == 0, "unexpected event type in low handler");
    emitted_order[order_cursor++] = 0;
    ++delivered_event_zero;
}
void event_high(void *event) {
    require(*static_cast<unsigned *>(event) == 0, "unexpected event type in high handler");
    emitted_order[order_cursor++] = 1;
    ++delivered_event_zero;
}
void disable_observer(void *event) {
    QEvent *e = static_cast<QEvent *>(event);
    require(e->type == 4, "disable observer type mismatch");
    ++delivered_disable_request;
    if (veto_disable && e->mod == &modules[0]) e->cancel = 1;
}
void toggle_observer(void *event) {
    ToggleEvent *e = static_cast<ToggleEvent *>(event);
    require(e->type == 5, "toggle observer type mismatch");
    require(e->ctx != nullptr, "toggle notification missing context");
    ++delivered_toggle;
}
void reset_synthetic_modules() {
    for (unsigned i = 0; i < 67; ++i) {
        modules[i] = {};
        modules[i].name = "synthetic-native-manager-fixture";
        modules[i].cat = i % 5;
        ptr_to_aim_assist_module_descriptor[i] = &modules[i];
    }
    modules[0].h[0] = {event_low, reinterpret_cast<void *>(std::uintptr_t(10))};
    modules[1].h[0] = {event_high, reinterpret_cast<void *>(std::uintptr_t(30))};
    modules[0].h[4] = {disable_observer, nullptr};
    modules[0].h[5] = {toggle_observer, nullptr};
}
}

extern "C" {
// Deliberate first-party replacements for external helper symbols used by the
// *compiled* module_manager.cpp object. All Java symbols are fail-closed stubs.
void *libc_calloc(unsigned long long n, unsigned long long width) {
    void *result = std::calloc(static_cast<size_t>(n), static_cast<size_t>(width));
    if (result) ++allocations;
    return result;
}
void libc_free(void *ptr) {
    if (ptr) { ++releases; std::free(ptr); }
}
void *libc_memcpy_alt(void *dst, const void *src, unsigned long long length) {
    return std::memcpy(dst, src, static_cast<size_t>(length));
}
void *libc_memset(void *dst, int value, unsigned long long length) {
    return std::memset(dst, value, static_cast<size_t>(length));
}
unsigned long long libc_strlen(const char *name) {
    return static_cast<unsigned long long>(std::strlen(name));
}
void libc_qsort(void *ptr, unsigned long long n, unsigned long long width,
                int (*compare)(const void *, const void *)) {
    std::qsort(ptr, static_cast<size_t>(n), static_cast<size_t>(width), compare);
}
void release_owner(void *) { ++released_owners; }
void slinky_logf(const char *, ...) { /* synthetic/no side effects */ }
void *get_block_state_id(void *, void *) { return nullptr; }
int get_block_id(void *, void *) { return 0; }
void *java_get_entity_player_inventory(void *, void *) { return nullptr; }
int java_get_inventory_player_current_item(void *, void *) { return -1; }
void *java_get_inventory_player_main_inventory(void *, void *) { return nullptr; }
void *java_call_item_stack_get_item(void *, void *) { return nullptr; }
int java_call_static_item_get_id_from_item(void *, void *) { return 0; }
void *java_call_entity_get_command_sender_name(void *, void *) { return nullptr; }
void *java_call_entity_living_base_get_team(void *, void *) { return nullptr; }
void *java_get_score_player_team(void *, void *) { return nullptr; }
int java_jni_is_instance_of_entity_living_base(void *, void *) { return 0; }
int java_jni_is_instance_of_score_player_team(void *, void *) { return 0; }
}

int main() {
    try {
        reset_synthetic_modules();
        Mgr manager = {};
        require(create_module_manager(&manager, state_setter) == 0, "constructor failed");
        require(manager.count == 66, "compiled constructor is not 66-entry");
        require(manager.mods != nullptr && manager.mods[0] == &modules[0],
                "first descriptor not retained");
        require(manager.mods[16] == &modules[16], "pre-skip descriptor not retained");
        require(manager.mods[17] == &modules[18], "index 17 was not removed");
        require(manager.mods[65] == &modules[66], "last descriptor not retained");
        for (unsigned i = 0; i < 66; ++i)
            require(manager.mods[i] != &modules[17], "excluded descriptor appeared");
        unsigned total_categories = 0;
        for (unsigned i = 0; i < 5; ++i) total_categories += unsigned(manager.cat[i].n);
        require(total_categories == 66, "category buckets do not contain 66 modules");
        require(manager.reg[0].n == 2 && manager.reg[0].list[0] == &modules[1]
                && manager.reg[0].list[1] == &modules[0],
                "compiled manager priority sorting differs");
        require(manager.reg[4].n == 1 && manager.reg[5].n == 1,
                "event registration counts unexpectedly changed");
        require(!has_queued_modules(&manager), "fresh manager has pending toggles");

        unsigned event0 = 0;
        dispatch_event(&manager, &event0);
        require(delivered_event_zero == 0, "disabled callbacks are live");

        for (unsigned i = 0; i < 67; ++i) {
            if (i == 17) continue;
            require(queue_module_toggle(&manager, &modules[i], 1), "valid enable failed");
        }
        require(manager.en_count == 66, "enable queue did not reach 66");
        require(!queue_module_toggle(&manager, &modules[17], 1),
                "manager accepted 67th enable past capacity");
        require(has_queued_modules(&manager), "pending enable queue not reported");

        HCtx ctx = {};
        ctx.module_manager = &manager;
        apply_module_toggles(&ctx);
        require(manager.en_count == 66 && modules[0].enabled == 0,
                "world/player null gate did not preserve queue");
        int dummy_world = 1;
        int dummy_player = 1;
        ctx.world = &dummy_world;
        ctx.player = &dummy_player;
        apply_module_toggles(&ctx);
        require(manager.en_count == 0 && !has_queued_modules(&manager),
                "enable queue not drained");
        require(modules[0].enabled == 1 && modules[66].enabled == 1
                && modules[17].enabled == 0, "enabled state leaked to excluded module");
        require(delivered_toggle == 66, "missing event-5 enable notifications");
        dispatch_event(&manager, &event0);
        require(delivered_event_zero == 2 && order_cursor == 2
                && emitted_order[0] == 1 && emitted_order[1] == 0,
                "compiled priority dispatch order is not descending");

        veto_disable = true;
        require(!queue_module_toggle(&manager, &modules[0], 0),
                "event-4 disable veto ignored");
        require(manager.dis_count == 0, "vetoed disable was queued");
        veto_disable = false;
        require(queue_module_toggle(&manager, &modules[0], 0),
                "unvetoed disable request rejected");
        require(manager.dis_count == 1, "disable queue count wrong");
        apply_module_toggles(&ctx);
        require(manager.dis_count == 0 && modules[0].enabled == 0,
                "disable queue not applied");
        require(released_owners == 1, "disable did not release owner");
        require(delivered_disable_request == 2, "event-4 notification count changed");
        dispatch_event(&manager, &event0);
        require(delivered_event_zero == 3 && emitted_order[2] == 1,
                "disabled handler remained active after rebuild");

        destroy_module_manager(&manager);
        require(allocations > 0 && allocations == releases,
                "native manager allocator cleanup was not balanced");
        std::printf(
            "R195_SYNTHETIC_COMPILED_MANAGER_PASS count=66 "
            "priority=high-before-low enable=66 veto=PASS disable=PASS "
            "allocations=%zu frees=%zu\n", allocations, releases
        );
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "R195_COMPILED_MANAGER_FAIL %s\n", error.what());
        return 2;
    }
}
