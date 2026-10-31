#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace slinkywinky {

enum class RuntimePhase { detached, attached, running, stopped };
enum class CommandKind { set_rotation, set_attack, set_block, attack_pulse, release_all, world_changed };
struct Command {
    CommandKind kind = CommandKind::release_all;
    std::uint64_t generation = 0;
    std::uint64_t sequence = 0;
    float first = 0;
    float second = 0;
};

// Thread safe producers, single explicitly admitted owner-thread consumer.
class RuntimeMailbox {
public:
    explicit RuntimeMailbox(std::size_t capacity = 1024);
    bool attach_owner();
    bool begin_world();
    bool submit(CommandKind kind, float first = 0, float second = 0);
    std::vector<Command> drain();
    bool world_changed();
    bool shutdown();
    RuntimePhase phase() const;
    bool on_owner_thread() const;
    std::uint64_t generation() const;
    std::size_t queued() const;
private:
    bool is_owner() const noexcept;
    mutable std::mutex mutex_;
    std::deque<Command> queue_;
    const std::size_t capacity_;
    std::thread::id owner_;
    RuntimePhase phase_ = RuntimePhase::detached;
    std::uint64_t generation_ = 0;
    std::uint64_t sequence_ = 0;
};

struct PacketNotice {
    std::uint32_t identifier = 0;
    std::uint64_t received_ms = 0;
    std::uint64_t generation = 0;
};
// Bounded identity-only packet holding model. No packet bytes are stored.
class PacketHoldQueue {
public:
    explicit PacketHoldQueue(std::size_t max_pending = 128);
    bool enqueue(std::uint32_t identifier, std::uint64_t received_ms,
                 std::uint64_t generation);
    std::vector<PacketNotice> expire(std::uint64_t now_ms, std::uint64_t max_delay_ms,
                                     std::uint64_t generation);
    std::vector<PacketNotice> release_all(std::uint64_t generation);
    void discard_all() noexcept;
    std::size_t size() const noexcept { return queue_.size(); }
private:
    const std::size_t max_pending_;
    std::deque<PacketNotice> queue_;
};
} // namespace slinkywinky
