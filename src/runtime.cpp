#include "slinkywinky/runtime.hpp"
#include <stdexcept>
#include <utility>

namespace slinkywinky {
RuntimeMailbox::RuntimeMailbox(std::size_t capacity) : capacity_(capacity) {
    if (!capacity) throw std::invalid_argument("mailbox capacity must be positive");
}
bool RuntimeMailbox::is_owner() const noexcept { return owner_ == std::this_thread::get_id(); }
bool RuntimeMailbox::attach_owner() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (phase_ != RuntimePhase::detached && phase_ != RuntimePhase::stopped) return false;
    owner_ = std::this_thread::get_id();
    phase_ = RuntimePhase::attached;
    queue_.clear();
    return true;
}
bool RuntimeMailbox::begin_world() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_owner() || phase_ != RuntimePhase::attached) return false;
    ++generation_;
    phase_ = RuntimePhase::running;
    queue_.clear();
    return true;
}
bool RuntimeMailbox::submit(CommandKind kind, float first, float second) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (phase_ != RuntimePhase::running || queue_.size() >= capacity_) return false;
    queue_.push_back({kind, generation_, ++sequence_, first, second});
    return true;
}
std::vector<Command> RuntimeMailbox::drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_owner() || phase_ != RuntimePhase::running) return {};
    std::vector<Command> result;
    result.reserve(queue_.size());
    while (!queue_.empty()) {
        auto command = queue_.front();
        queue_.pop_front();
        if (command.generation == generation_) result.push_back(command);
    }
    return result;
}
bool RuntimeMailbox::world_changed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_owner() || phase_ != RuntimePhase::running) return false;
    queue_.clear();
    ++generation_;
    phase_ = RuntimePhase::attached;
    return true;
}
bool RuntimeMailbox::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_owner() || phase_ == RuntimePhase::detached || phase_ == RuntimePhase::stopped) return false;
    queue_.clear();
    ++generation_;
    phase_ = RuntimePhase::stopped;
    return true;
}
bool RuntimeMailbox::on_owner_thread() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_owner();
}
RuntimePhase RuntimeMailbox::phase() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return phase_;
}
std::uint64_t RuntimeMailbox::generation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return generation_;
}
std::size_t RuntimeMailbox::queued() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}
PacketHoldQueue::PacketHoldQueue(std::size_t capacity): max_pending_(capacity) {
    if (!capacity) throw std::invalid_argument("packet capacity must be positive");
}
bool PacketHoldQueue::enqueue(std::uint32_t identifier, std::uint64_t received_ms,
                               std::uint64_t generation) {
    if (queue_.size() >= max_pending_) return false;
    if (!queue_.empty() && (received_ms < queue_.back().received_ms ||
                            generation != queue_.back().generation)) return false;
    queue_.push_back({identifier, received_ms, generation});
    return true;
}
std::vector<PacketNotice> PacketHoldQueue::expire(std::uint64_t now_ms, std::uint64_t max_delay_ms,
                                                   std::uint64_t generation) {
    std::vector<PacketNotice> result;
    while (!queue_.empty()) {
        const auto& front = queue_.front();
        if (front.generation != generation) { queue_.pop_front(); continue; }
        if (now_ms < front.received_ms || now_ms-front.received_ms < max_delay_ms) break;
        result.push_back(front);
        queue_.pop_front();
    }
    return result;
}
std::vector<PacketNotice> PacketHoldQueue::release_all(std::uint64_t generation) {
    std::vector<PacketNotice> result;
    while (!queue_.empty()) {
        if (queue_.front().generation == generation) result.push_back(queue_.front());
        queue_.pop_front();
    }
    return result;
}
void PacketHoldQueue::discard_all() noexcept { queue_.clear(); }
} // namespace slinkywinky
