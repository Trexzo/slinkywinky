// R195 independent synthetic harness; links a separately compiled COFF object.
// No third-party source, original DLL, JNI JVM, Minecraft or injector involved.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

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
static_assert(sizeof(Module) == 0x1f8, "Private raw native module record layout");

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


namespace {
unsigned linked_callback_calls[27] = {};
unsigned linked_expected_per_event[27] = {};
unsigned linked_expected_per_category[5] = {};

void linked_mock_handler(void *event) {
    unsigned type = *static_cast<unsigned *>(event);
    if (type < 27) ++linked_callback_calls[type];
}
int linked_slot_priority(const Module *module, unsigned event) {
    const std::uintptr_t repr = reinterpret_cast<std::uintptr_t>(module->h[event].x);
    const std::uint32_t bits = static_cast<std::uint32_t>(repr);
    std::int32_t out = 0;
    std::memcpy(&out, &bits, sizeof(out));
    return static_cast<int>(out);
}
void prepare_linked_fixture(const char *path) {
    reset_synthetic_modules();
    // Discard R195's fabricated callback assignments; use the 66 public-PE
    // metadata records, while all executable callback pointers remain stubs.
    for (auto &module : modules) {
        for (auto &slot : module.h) slot = {};
        module.enabled = 0;
    }
    std::ifstream file(path);
    require(file.good(), "linked descriptor fixture could not be opened");
    std::string magic;
    unsigned count = 0, total = 0;
    require(bool(file >> magic >> count >> total), "fixture header missing");
    require(magic == "R196-LINKED-DESCRIPTORS-V1" && count == 66 && total == 171,
            "fixture header version or totals mismatch");
    for (unsigned compact = 0; compact < 66; ++compact) {
        unsigned source = 0, category = 0, initial = 0, n = 0;
        require(bool(file >> source >> category >> initial >> n),
                "truncated native descriptor row");
        const unsigned expected_source = compact < 17 ? compact : compact + 1;
        require(source == expected_source && category < 5 && initial == 0 && n <= 27,
                "native descriptor ordering/category/default drift");
        Module &mod = modules[source];
        mod.cat = category;
        mod.enabled = static_cast<unsigned char>(initial);
        ++linked_expected_per_category[category];
        for (unsigned j = 0; j < n; ++j) {
            std::string token;
            require(bool(file >> token), "truncated native event slot");
            int event = -1, priority = 0;
            char extra = 0;
            require(sscanf_s(token.c_str(), "%d:%d%c", &event, &priority, &extra, 1) == 2,
                    "malformed native event priority token");
            require(event >= 0 && event < 27 && mod.h[event].f == nullptr,
                    "duplicate or invalid native event subscription");
            mod.h[event].f = &linked_mock_handler;
            // Constructor comparator reads signed int at module + 0x50 + 16*t.
            mod.h[event].x = reinterpret_cast<void *>(
                static_cast<std::uintptr_t>(static_cast<std::uint32_t>(priority))
            );
            ++linked_expected_per_event[event];
        }
    }
    std::string unexpected;
    require(!(file >> unexpected), "native fixture has extra trailing records");
    unsigned subscription_total = 0;
    for (unsigned value : linked_expected_per_event) subscription_total += value;
    require(subscription_total == 171, "native linked fixture subscription count changed");
}

void replace_metadata_with_raw_records(const char *path) {
    // The local fixture contains full pinned PE .data records, including
    // native addresses. Every callable pointer and name is scrubbed before
    // the original manager object ever receives a module pointer.
    struct Header {
        char magic[8];
        std::uint32_t count;
        std::uint32_t record_size;
        unsigned char dll_sha256[32];
    };
    static_assert(sizeof(Header) == 48, "R197 binary header layout");
    static const unsigned char expected_sha[32] = {
        0x36,0x13,0xa4,0x66,0x43,0x3f,0xb1,0xfa,
        0x83,0xd2,0xd9,0xe9,0x68,0x15,0xec,0xe7,
        0x19,0xc1,0x66,0xf8,0x81,0xc7,0x86,0x42,
        0xdc,0x18,0xd3,0xe1,0x3a,0xe2,0xaa,0x14
    };
    std::ifstream file(path, std::ios::binary);
    require(file.good(), "private linked native record fixture cannot be opened");
    Header header = {};
    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    require(file.gcount() == sizeof(header)
            && std::memcmp(header.magic, "R197REC1", 8) == 0
            && header.count == 66 && header.record_size == sizeof(Module)
            && std::memcmp(header.dll_sha256, expected_sha, 32) == 0,
            "private linked PE record file header or DLL fingerprint invalid");

    for (unsigned compact = 0; compact < 66; ++compact) {
        std::uint32_t source = 0;
        file.read(reinterpret_cast<char *>(&source), sizeof(source));
        require(file.gcount() == sizeof(source), "truncated private native record index");
        const unsigned expected_source = compact < 17 ? compact : compact + 1;
        require(source == expected_source, "private native record source order changed");
        std::array<unsigned char, sizeof(Module)> raw = {};
        file.read(reinterpret_cast<char *>(raw.data()), raw.size());
        require(file.gcount() == static_cast<std::streamsize>(raw.size()),
                "truncated private linked native record bytes");

        Module &mod = modules[source];
        Module original = {};
        // The exact 0x1f8 compiled DLL bytes enter memory, but no pointer is
        // invoked. Compare against separately verified R196 metadata first.
        std::memcpy(&original, raw.data(), raw.size());
        require(original.cat == mod.cat && original.enabled == mod.enabled,
                "linked PE native record category/default differs from R196 manifest");
        for (unsigned event = 0; event < 27; ++event) {
            std::uint64_t raw_handler_va = 0;
            std::memcpy(&raw_handler_va, raw.data() + 0x48 + event * 16,
                        sizeof(raw_handler_va));
            const bool expected_present = mod.h[event].f != nullptr;
            require((raw_handler_va != 0) == expected_present,
                    "native raw callback-presence mismatch");
            if (expected_present) {
                std::int32_t raw_priority = 0;
                std::memcpy(&raw_priority, raw.data() + 0x50 + event * 16,
                            sizeof(raw_priority));
                require(raw_priority == linked_slot_priority(&mod, event),
                        "native raw event priority differs from R196 manifest");
            }
        }
        // Preserve native record scalar layout, then scrub potentially
        // executable/dangling addresses before any compiled code sees it.
        std::memcpy(&mod, raw.data(), raw.size());
        mod.name = "private-raw-record-safe-name";
        std::memset(mod.padding_08, 0, sizeof(mod.padding_08));
        std::memset(mod.padding_15, 0, sizeof(mod.padding_15));
        for (unsigned event = 0; event < 27; ++event) {
            std::uint64_t native_callback = 0;
            std::int32_t priority = 0;
            std::memcpy(&native_callback, raw.data() + 0x48 + event * 16, 8);
            std::memcpy(&priority, raw.data() + 0x50 + event * 16, 4);
            mod.h[event].f = native_callback ? &linked_mock_handler : nullptr;
            mod.h[event].x = native_callback
                ? reinterpret_cast<void *>(
                    static_cast<std::uintptr_t>(static_cast<std::uint32_t>(priority))
                )
                : nullptr;
        }
    }
    char trailer = '\0';
    require(!file.read(&trailer, 1), "private native record fixture has trailing bytes");
    require(file.eof() && !file.bad(), "private native record fixture read error");
}

int run_linked_native_manifest(const char *path, const char *record_path = nullptr) {
    prepare_linked_fixture(path);
    if (record_path) replace_metadata_with_raw_records(record_path);
    Mgr manager = {};
    require(create_module_manager(&manager, state_setter) == 0,
            "compiled real-metadata native manager constructor failed");
    require(manager.count == 66, "real-metadata manager count not 66");
    require(manager.mods[17] == &modules[18] && manager.mods[65] == &modules[66],
            "real-metadata manager source-order compaction changed");
    for (unsigned source = 0; source < 67; ++source) {
        if (source == 17) continue;
        const unsigned compact = source < 17 ? source : source - 1;
        require(manager.mods[compact] == &modules[source], "native compacted slot drift");
    }
    for (unsigned cat = 0; cat < 5; ++cat)
        require(manager.cat[cat].n == linked_expected_per_category[cat],
                "compiled native category count differs from linked descriptor data");

    for (unsigned event = 0; event < 27; ++event) {
        require(manager.reg[event].n == linked_expected_per_event[event],
                "compiled event bucket differs from linked descriptor data");
        int last_priority = 2147483647;
        for (unsigned j = 0; j < manager.reg[event].n; ++j) {
            Module *mod = manager.reg[event].list[j];
            require(mod >= &modules[0] && mod < &modules[67] && mod != &modules[17],
                    "compiled registry included excluded or foreign descriptor");
            require(mod->h[event].f == &linked_mock_handler,
                    "compiled registry contains a callback absent from linked manifest");
            const int priority = linked_slot_priority(mod, event);
            require(priority <= last_priority, "compiled event sorting is not nonincreasing");
            last_priority = priority;
        }
    }
    require(linked_expected_per_event[0] == 24 && linked_expected_per_event[5] == 25,
            "pinned event 0 / event 5 distribution changed");
    unsigned event0 = 0;
    dispatch_event(&manager, &event0);
    require(linked_callback_calls[0] == 0, "disabled linked-data callbacks executed");

    for (unsigned source = 0; source < 67; ++source) {
        if (source == 17) continue;
        require(queue_module_toggle(&manager, &modules[source], 1),
                "compiled linked-data enable request rejected");
    }
    require(manager.en_count == 66, "linked-data native queue did not reach 66");
    require(!queue_module_toggle(&manager, &modules[17], 1),
            "compiled linked-data queue accepted 67th entry");

    HCtx ctx = {};
    ctx.module_manager = &manager;
    apply_module_toggles(&ctx);
    require(manager.en_count == 66, "null-world gate did not preserve linked-data queue");
    int dummy_world = 1, dummy_player = 1;
    ctx.world = &dummy_world;
    ctx.player = &dummy_player;
    apply_module_toggles(&ctx);
    require(!has_queued_modules(&manager), "linked-data queue not drained");
    require(linked_callback_calls[5] == 66 * linked_expected_per_event[5],
            "linked-data event-5 callbacks not delivered for all 66 enables");
    dispatch_event(&manager, &event0);
    require(linked_callback_calls[0] == linked_expected_per_event[0],
            "linked-data event-0 callback dispatch count differs");

    unsigned target = 67;
    for (unsigned source = 0; source < 67; ++source) {
        if (source != 17 && modules[source].h[0].f != nullptr) {
            target = source;
            break;
        }
    }
    require(target < 67, "no linked-data event-0 target for disable");
    require(queue_module_toggle(&manager, &modules[target], 0),
            "linked-data disable request rejected");
    apply_module_toggles(&ctx);
    dispatch_event(&manager, &event0);
    require(linked_callback_calls[0] == linked_expected_per_event[0] * 2 - 1,
            "linked-data callback list did not drop the disabled handler");
    require(released_owners == 1, "linked-data native disable owner not released");
    destroy_module_manager(&manager);
    require(allocations == releases, "linked-data compiled manager did not clean up");
    std::printf(
        "%s registrations=66 callbacks=171 "
        "event0=%u event5=%u allocations=%zu frees=%zu\n",
        record_path ? "R197_COMPILED_NATIVE_RECORDS_PASS"
                    : "R196_COMPILED_LINKED_DATA_PASS",
        linked_expected_per_event[0], linked_expected_per_event[5], allocations, releases
    );
    return 0;
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc == 4 && std::strcmp(argv[1], "--native-records") == 0)
            return run_linked_native_manifest(argv[3], argv[2]);
        if (argc == 2) return run_linked_native_manifest(argv[1]);
        require(argc == 1, "expected no arguments or one linked metadata fixture");
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
