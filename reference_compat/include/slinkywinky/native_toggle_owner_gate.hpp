#pragma once
// R200 first-party module-toggle admission fence for isolated OpenSlinky
// compatibility experiments. No JNI, third-party code or gameplay actions.
//
// All calls are restricted to one owner thread. This is NOT an implicit
// cross-thread message queue. The manager and module identities must remain
// alive until shutdown. Every native enqueue/apply entry point must pass
// through this fence for the invariants below to hold.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {

class NativeToggleOwnerGate final {
public:
    using Key = void*;
    using EnqueueNative = std::function<bool(Key, bool)>;
    using ReadEnabled = std::function<bool(Key)>;
    using ApplyNative = std::function<bool()>;

    struct Pending {
        Key key = nullptr;
        bool enable = false;
    };

    explicit NativeToggleOwnerGate(
        std::uint64_t generation, std::vector<Key> registered,
        EnqueueNative enqueue, ReadEnabled read_enabled, ApplyNative apply,
        std::size_t capacity = 66)
        : owner_(std::this_thread::get_id()),
          generation_(generation),
          registered_(std::move(registered)),
          enqueue_(std::move(enqueue)),
          read_enabled_(std::move(read_enabled)),
          apply_(std::move(apply)),
          capacity_(capacity) {
        if (!generation_ || capacity_ == 0 || capacity_ > 66 ||
            registered_.empty() || registered_.size() > 66 ||
            !enqueue_ || !read_enabled_ || !apply_)
            throw std::invalid_argument("invalid native toggle owner gate");
        for (std::size_t i = 0; i < registered_.size(); ++i) {
            if (!registered_[i]) throw std::invalid_argument("null registered module");
            if (std::find(registered_.begin(), registered_.begin() + i,
                          registered_[i]) != registered_.begin() + i)
                throw std::invalid_argument("duplicate registered native module");
        }
        pending_.reserve(capacity_);
        deferred_.reserve(capacity_);
    }

    NativeToggleOwnerGate(const NativeToggleOwnerGate&) = delete;
    NativeToggleOwnerGate& operator=(const NativeToggleOwnerGate&) = delete;
    NativeToggleOwnerGate(NativeToggleOwnerGate&&) = delete;
    NativeToggleOwnerGate& operator=(NativeToggleOwnerGate&&) = delete;

    bool submit(std::uint64_t generation, Key key, bool enable) {
        if (!on_owner() || stopped_ || generation != generation_ ||
            !key || !is_member(key)) return false;
        const Pending* known = find_pending(pending_, key);
        if (!known) known = find_pending(deferred_, key);
        if (known) return known->enable == enable;
        try {
            if (read_enabled_(key) == enable) return true;
            if (in_native_) {
                if (deferred_.size() >= capacity_) return false;
                deferred_.push_back({key, enable});
                return true;
            }
            if (pending_.size() >= capacity_) return false;
            in_native_ = true;
            const bool ok = enqueue_(key, enable);
            in_native_ = false;
            if (stopped_) return false;
            if (!ok) {
                // A veto can occur after native event-4 observers have emitted
                // other requests. Do not silently replay those side effects.
                if (!deferred_.empty()) shutdown();
                return false;
            }
            pending_.push_back({key, enable});
            return true;
        } catch (...) {
            shutdown();
            throw;
        }
    }

    // Call only after the caller has verified both world/player readiness.
    // Reentrant requests are deferred and admitted *after* native apply
    // returns, for processing on the next owner-thread tick.
    bool apply(std::uint64_t generation, bool world_ready, bool player_ready) {
        if (!on_owner() || stopped_ || generation != generation_ ||
            in_native_ || !world_ready || !player_ready) return false;
        if (pending_.empty() && deferred_.empty()) return true;
        try {
            if (!pending_.empty()) {
                in_native_ = true;
                const bool ok = apply_();
                in_native_ = false;
                if (stopped_) return false;
                if (!ok) {
                    shutdown();
                    return false;
                }
                pending_.clear();
            }
            // Fixed snapshot: requests spawned by native admission callbacks
            // during this flush must NOT be processed recursively.
            const auto work = std::move(deferred_);
            deferred_.clear();
            deferred_.reserve(capacity_);
            for (const auto& item : work) {
                if (!submit(generation_, item.key, item.enable)) {
                    shutdown();
                    return false;
                }
            }
            return true;
        } catch (...) {
            shutdown();
            throw;
        }
    }

    // Irreversible lifetime fence: caller must destroy its native manager
    // after this. Any native-owned pending queue is not implicitly flushed.
    bool shutdown() noexcept {
        if (!on_owner() || stopped_) return false;
        stopped_ = true;
        pending_.clear();
        deferred_.clear();
        return true;
    }

    bool on_owner() const noexcept {
        return owner_ == std::this_thread::get_id();
    }
    bool stopped() const noexcept { return !on_owner() || stopped_; }
    std::uint64_t generation() const noexcept { return generation_; }
    std::size_t pending_count() const noexcept {
        return on_owner() ? pending_.size() : 0;
    }
    std::size_t deferred_count() const noexcept {
        return on_owner() ? deferred_.size() : 0;
    }

private:
    static const Pending* find_pending(const std::vector<Pending>& values,
                                       Key key) noexcept {
        const auto it = std::find_if(values.begin(), values.end(),
                                     [key](const Pending& p) { return p.key == key; });
        return it == values.end() ? nullptr : &*it;
    }
    bool is_member(Key key) const noexcept {
        return std::find(registered_.begin(), registered_.end(), key) !=
               registered_.end();
    }

    const std::thread::id owner_;
    const std::uint64_t generation_;
    const std::vector<Key> registered_;
    const EnqueueNative enqueue_;
    const ReadEnabled read_enabled_;
    const ApplyNative apply_;
    const std::size_t capacity_;
    std::vector<Pending> pending_;
    std::vector<Pending> deferred_;
    bool in_native_ = false;
    bool stopped_ = false;
};

} // namespace slinkywinky::reference_compat
