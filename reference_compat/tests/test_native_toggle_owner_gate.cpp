#include "slinkywinky/native_toggle_owner_gate.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using slinkywinky::reference_compat::NativeToggleOwnerGate;

namespace {
void check(bool condition, const char* expression, int line) {
    if (!condition)
        throw std::runtime_error("R200 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(condition) check(static_cast<bool>(condition), #condition, __LINE__)

struct SyntheticModule {
    bool enabled = false;
    int notify_count = 0;
};

struct MockNative {
    std::array<SyntheticModule, 3> module{};
    std::vector<NativeToggleOwnerGate::Pending> queue;
    std::function<void(void*, bool)> notify;
    std::function<void(void*, bool)> during_admission;
    bool reject_next = false;
    bool throw_next_apply = false;
    unsigned applied = 0;

    bool native_enqueue(void* key, bool enable) {
        if (reject_next) { reject_next = false; return false; }
        if (during_admission) during_admission(key, enable);
        queue.push_back({key, enable});
        return true;
    }
    bool read(void* key) const {
        return static_cast<SyntheticModule*>(key)->enabled;
    }
    bool native_apply() {
        if (throw_next_apply) throw std::runtime_error("synthetic native apply failure");
        const auto snapshot = queue;
        queue.clear();
        for (const auto& operation : snapshot) {
            auto* module_ptr = static_cast<SyntheticModule*>(operation.key);
            module_ptr->enabled = operation.enable;
            ++module_ptr->notify_count;
            ++applied;
            if (notify) notify(operation.key, operation.enable);
        }
        return true;
    }
};

NativeToggleOwnerGate create_gate(MockNative& native, std::uint64_t generation = 42,
                                   std::size_t capacity = 66) {
    return NativeToggleOwnerGate(
        generation, {&native.module[0], &native.module[1], &native.module[2]},
        [&native](void* key, bool state) { return native.native_enqueue(key, state); },
        [&native](void* key) { return native.read(key); },
        [&native]() { return native.native_apply(); },
        capacity);
}

void test_duplicate_conflict_capacity_and_session_fence() {
    MockNative n;
    auto gate = create_gate(n, 42, 2);
    auto* a = &n.module[0];
    auto* b = &n.module[1];
    auto* c = &n.module[2];
    SyntheticModule foreign;

    CHECK(gate.on_owner() && gate.generation() == 42);
    CHECK(!gate.submit(41, a, true));
    CHECK(!gate.submit(42, &foreign, true));
    CHECK(!gate.submit(42, nullptr, true));
    CHECK(gate.submit(42, a, true));
    CHECK(gate.submit(42, a, true));      // same transition is idempotent
    CHECK(!gate.submit(42, a, false));    // contrary pending transition rejected
    CHECK(gate.submit(42, b, true));
    CHECK(!gate.submit(42, c, true));     // aggregate capacity is 2
    CHECK(gate.pending_count() == 2 && n.queue.size() == 2);
    CHECK(!gate.apply(41, true, true));   // stale session generation
    CHECK(!gate.apply(42, false, true));
    CHECK(!gate.apply(42, true, false));
    CHECK(gate.pending_count() == 2 && !a->enabled && !b->enabled);
    CHECK(gate.apply(42, true, true));
    CHECK(a->enabled && b->enabled && a->notify_count == 1 && b->notify_count == 1);
    CHECK(gate.pending_count() == 0);
    CHECK(gate.submit(42, a, true));     // already applied
    CHECK(n.queue.empty());
    CHECK(gate.shutdown() && gate.stopped());
    CHECK(!gate.shutdown());
    CHECK(!gate.submit(42, a, false));
    CHECK(!gate.apply(42, true, true));
}

void test_reentrant_notification_next_tick() {
    MockNative n;
    auto gate = create_gate(n);
    auto* a = &n.module[0];
    auto* b = &n.module[1];
    unsigned reentrant = 0;
    n.notify = [&](void* key, bool enable) {
        if (key == a && enable) {
            ++reentrant;
            CHECK(gate.submit(42, b, true));
            CHECK(gate.submit(42, b, true));
            CHECK(!gate.submit(42, b, false));
            CHECK(gate.deferred_count() == 1);
        }
    };
    CHECK(gate.submit(42, a, true));
    CHECK(gate.apply(42, true, true));
    CHECK(a->enabled && !b->enabled);
    CHECK(gate.pending_count() == 1 && gate.deferred_count() == 0);
    CHECK(n.queue.size() == 1 && reentrant == 1);
    CHECK(gate.apply(42, true, true));
    CHECK(b->enabled && n.module[1].notify_count == 1);
    CHECK(n.queue.empty() && gate.pending_count() == 0);
    CHECK(gate.shutdown());
}

void test_reentrant_native_enqueue_is_deferred() {
    MockNative n;
    auto gate = create_gate(n);
    auto* a = &n.module[0];
    auto* b = &n.module[1];
    n.during_admission = [&](void* key, bool enable) {
        if (key == a && enable) {
            CHECK(gate.submit(42, b, true));
            CHECK(gate.deferred_count() == 1);
        }
    };
    CHECK(gate.submit(42, a, true));
    CHECK(n.queue.size() == 1 && gate.deferred_count() == 1);
    CHECK(gate.apply(42, true, true));
    CHECK(a->enabled && !b->enabled);
    CHECK(n.queue.size() == 1 && gate.pending_count() == 1);
    CHECK(gate.apply(42, true, true));
    CHECK(b->enabled && n.queue.empty());
}

void test_nonowner_denial() {
    MockNative n;
    auto gate = create_gate(n);
    std::atomic<unsigned> rejected{0};
    std::thread stranger([&] {
        if (!gate.on_owner()) ++rejected;
        if (!gate.submit(42, &n.module[0], true)) ++rejected;
        if (!gate.apply(42, true, true)) ++rejected;
        if (!gate.shutdown()) ++rejected;
        if (gate.pending_count() == 0 && gate.deferred_count() == 0) ++rejected;
    });
    stranger.join();
    CHECK(rejected == 5 && !gate.stopped());
    CHECK(gate.submit(42, &n.module[0], true));
    CHECK(gate.apply(42, true, true) && n.module[0].enabled);
    CHECK(gate.shutdown());
}

void test_fail_closed_on_exception_and_veto() {
    MockNative n;
    auto gate = create_gate(n);
    n.reject_next = true;
    CHECK(!gate.submit(42, &n.module[0], true));
    CHECK(gate.pending_count() == 0 && n.queue.empty());
    CHECK(gate.submit(42, &n.module[0], true));
    n.throw_next_apply = true;
    bool threw = false;
    try { gate.apply(42, true, true); }
    catch (const std::runtime_error&) { threw = true; }
    CHECK(threw && gate.stopped());
    CHECK(!gate.submit(42, &n.module[1], true));
}

void test_shutdown_during_callback_and_new_manager() {
    MockNative native;
    {
        auto gate = create_gate(native, 9);
        native.notify = [&](void*, bool) {
            CHECK(gate.shutdown());
            CHECK(!gate.submit(9, &native.module[1], true));
        };
        CHECK(gate.submit(9, &native.module[0], true));
        CHECK(!gate.apply(9, true, true));
        CHECK(gate.stopped() && gate.pending_count() == 0);
    }
    MockNative fresh;
    auto next = create_gate(fresh, 10);
    CHECK(!next.submit(9, &fresh.module[0], true));
    CHECK(next.submit(10, &fresh.module[0], true));
    CHECK(next.apply(10, true, true) && fresh.module[0].enabled);
    CHECK(next.shutdown());
}

void test_constructor_validation() {
    MockNative native;
    bool rejected = false;
    try {
        NativeToggleOwnerGate bad(1, {&native.module[0], &native.module[0]},
            [&](void*, bool) { return true; },
            [&](void*) { return false; },
            [&] { return true; });
    } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    rejected = false;
    try {
        NativeToggleOwnerGate bad(0, {&native.module[0]},
            [&](void*, bool) { return true; },
            [&](void*) { return false; },
            [&] { return true; });
    } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    rejected = false;
    try {
        NativeToggleOwnerGate bad(1, {&native.module[0]},
            [&](void*, bool) { return true; },
            [&](void*) { return false; },
            [&] { return true; }, 67);
    } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}
} // namespace

int main() {
    test_duplicate_conflict_capacity_and_session_fence();
    test_reentrant_notification_next_tick();
    test_reentrant_native_enqueue_is_deferred();
    test_nonowner_denial();
    test_fail_closed_on_exception_and_veto();
    test_shutdown_during_callback_and_new_manager();
    test_constructor_validation();
    std::cout << "R200_NATIVE_TOGGLE_OWNER_GATE_PASS"
              << " duplicate=1 reentrant_apply=1 reentrant_enqueue=1"
              << " cross_thread_denial=1 stale_generation_denial=1"
              << " shutdown=1 recovery=1 capacity=1\n";
}
