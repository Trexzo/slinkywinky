#pragma once
// Independently authored offline event replay for the candidate Slinky
// module-manager contract. No JNI, injection, gameplay I/O or third-party code.
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kEventSlots = 27;
inline constexpr std::size_t kCandidateModules = 66;
inline constexpr unsigned kBeforeDisable = 4;
inline constexpr unsigned kToggleNotification = 5;

struct Subscription {
    unsigned event = 0;
    int priority = 0;
};
struct ModuleSpec {
    std::string name;
    unsigned category = 0;
    std::vector<Subscription> subscriptions;
    bool initially_enabled = false;
};
struct Delivery {
    unsigned event = 0;
    std::size_t recipient = 0;
    std::size_t target = 0;
    bool new_state = false;
};
using Observer = std::function<void(const Delivery&, bool& veto)>;

class EventReplay final {
public:
    explicit EventReplay(Observer observer = {}) : observer_(std::move(observer)) {}

    std::size_t add(ModuleSpec spec) {
        if (stopped_ || busy_ || modules_.size() >= kCandidateModules || spec.name.empty() ||
            spec.category >= 5) throw std::invalid_argument("invalid module descriptor");
        for (const auto& module : modules_)
            if (module.name == spec.name) throw std::invalid_argument("duplicate module name");
        std::array<bool, kEventSlots> seen{};
        for (const auto& sub : spec.subscriptions) {
            if (sub.event >= kEventSlots || seen[sub.event])
                throw std::invalid_argument("invalid or duplicate event subscription");
            seen[sub.event] = true;
        }
        Module item;
        item.name = std::move(spec.name);
        item.category = spec.category;
        item.enabled = spec.initially_enabled;
        for (const auto& sub : spec.subscriptions) {
            item.events[sub.event] = true;
            item.priorities[sub.event] = sub.priority;
        }
        modules_.push_back(std::move(item));
        rebuild();
        return modules_.size() - 1;
    }

    bool dispatch(unsigned event, std::size_t target = 0, bool state = false) {
        if (stopped_ || busy_ || event >= kEventSlots) return false;
        DeliveryFence fence(busy_);
        bool ignored = false;
        deliver(event, target, state, ignored);
        return !stopped_;
    }

    bool request(std::size_t id, bool enabled) {
        if (stopped_ || busy_ || id >= modules_.size()) return false;
        auto& module = modules_[id];
        // The native queue can admit duplicates. This test model rejects
        // contradictory or duplicate pending requests as a safety divergence.
        if (contains(pending_enable_, id) || contains(pending_disable_, id)) return false;
        if (module.enabled == enabled) return true;
        if (!enabled) {
            bool veto = false;
            {
                DeliveryFence fence(busy_);
                deliver(kBeforeDisable, id, false, veto);
            }
            if (stopped_ || veto) return false;
        }
        auto& queue = enabled ? pending_enable_ : pending_disable_;
        if (queue.size() >= kCandidateModules) return false;
        queue.push_back(id);
        return true;
    }

    bool apply(bool world_present, bool player_present) {
        if (stopped_ || busy_ || !world_present || !player_present) return false;
        if (pending_enable_.empty() && pending_disable_.empty()) return true;
        // Keep stable queue snapshots: an observer may request permanent stop()
        // from a notification. stop() clears the live queues immediately.
        const auto enabling = pending_enable_;
        const auto disabling = pending_disable_;
        DeliveryFence fence(busy_);
        try {
            for (auto id : enabling) modules_[id].enabled = true;
            rebuild();
            for (auto id : enabling) {
                if (stopped_) return false;
                bool ignored = false;
                deliver(kToggleNotification, id, true, ignored);
            }
            if (stopped_) return false;
            pending_enable_.clear();
            for (auto id : disabling) modules_[id].enabled = false;
            // Disable notices use the previous handler list; rebuild comes after.
            for (auto id : disabling) {
                if (stopped_) return false;
                bool ignored = false;
                deliver(kToggleNotification, id, false, ignored);
            }
            if (stopped_) return false;
            pending_disable_.clear();
            rebuild();
            return true;
        } catch (...) {
            // A throwing observer must not leave a half-applied batch live.
            stop();
            throw;
        }
    }

    void stop() noexcept {
        stopped_ = true;
        pending_enable_.clear();
        pending_disable_.clear();
        for (auto& list : active_) list.clear();
    }

    bool enabled(std::size_t id) const { return modules_.at(id).enabled; }
    std::size_t module_count() const noexcept { return modules_.size(); }
    std::size_t pending_enables() const noexcept { return pending_enable_.size(); }
    std::size_t pending_disables() const noexcept { return pending_disable_.size(); }
    const std::vector<std::size_t>& handlers(unsigned event) const {
        if (event >= kEventSlots) throw std::out_of_range("invalid event slot");
        return active_[event];
    }
    const std::vector<Delivery>& trace() const noexcept { return trace_; }
    void clear_trace() { trace_.clear(); }

private:
    // Pure replay observer callbacks are deliberately non-reentrant. This
    // avoids mutations to registration and toggle queues during delivery.
    struct DeliveryFence {
        explicit DeliveryFence(bool& flag) noexcept : flag_(flag) { flag_ = true; }
        ~DeliveryFence() { flag_ = false; }
        DeliveryFence(const DeliveryFence&) = delete;
        DeliveryFence& operator=(const DeliveryFence&) = delete;
    private:
        bool& flag_;
    };

    struct Module {
        std::string name;
        unsigned category = 0;
        std::array<bool, kEventSlots> events{};
        std::array<int, kEventSlots> priorities{};
        bool enabled = false;
    };
    static bool contains(const std::vector<std::size_t>& queue, std::size_t id) {
        return std::find(queue.begin(), queue.end(), id) != queue.end();
    }
    void rebuild() {
        for (unsigned event = 0; event < kEventSlots; ++event) {
            auto& list = active_[event];
            list.clear();
            for (std::size_t id = 0; id < modules_.size(); ++id)
                if (modules_[id].enabled && modules_[id].events[event]) list.push_back(id);
            // Source sorts descending event priority. Equal-priority order is
            // not established by original qsort, so ties use descriptor order.
            std::stable_sort(list.begin(), list.end(), [&](std::size_t a, std::size_t b) {
                return modules_[a].priorities[event] > modules_[b].priorities[event];
            });
        }
    }
    void deliver(unsigned event, std::size_t target, bool state, bool& veto) {
        // Snapshot prevents observer-driven registration from invalidating
        // the handler traversal. Callbacks must remain offline/side-effect free.
        const auto recipients = active_[event];
        for (auto id : recipients) {
            if (stopped_) break;
            Delivery d{event, id, target, state};
            trace_.push_back(d);
            if (observer_) observer_(d, veto);
        }
    }
    Observer observer_;
    std::vector<Module> modules_;
    std::array<std::vector<std::size_t>, kEventSlots> active_{};
    std::vector<std::size_t> pending_enable_;
    std::vector<std::size_t> pending_disable_;
    std::vector<Delivery> trace_;
    bool stopped_ = false;
    bool busy_ = false;
};
} // namespace slinkywinky::reference_compat
