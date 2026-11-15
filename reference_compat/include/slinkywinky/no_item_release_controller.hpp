#pragma once
// R187: independent offline No Item Release packet decision state machine.
// No actual packet cancellation, JVM calls, sockets or live game integration.
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kNoItemReleaseEvent = 3;

enum class NoItemReleasePacketKind : unsigned char {
    other, c08_item_use, c07_digging, c00_keepalive
};
struct NoItemReleaseInput {
    NoItemReleasePacketKind packet = NoItemReleasePacketKind::other;
    bool stack_present = false;
    bool item_present = false;
    int item_id = -1;
    int digging_status = 0;
    unsigned mode = 0; // 0=swords, 1=non-swords, other=all except bow/rod
};
struct NoItemReleaseDecision {
    std::optional<int> proposed_packet_state{}; // 2 = cancel; offline only
    bool release_locked = false;
};
class NoItemReleaseController final {
public:
    NoItemReleaseDecision evaluate(const NoItemReleaseInput& input) noexcept {
        NoItemReleaseDecision out;
        if (input.packet == NoItemReleasePacketKind::c08_item_use) {
            // Starting a new use overwrites any previous lock, including
            // missing stack/item cases.
            release_locked_ = false;
            if (input.stack_present && input.item_present) {
                const bool sword =
                    input.item_id == 267 || input.item_id == 268 ||
                    input.item_id == 272 || input.item_id == 276 ||
                    input.item_id == 283;
                const bool bow_or_rod =
                    input.item_id == 261 || input.item_id == 346;
                if (!bow_or_rod) {
                    release_locked_ = sword ? input.mode != 1 : input.mode != 0;
                }
            }
        } else if (input.packet == NoItemReleasePacketKind::c07_digging &&
                   release_locked_) {
            if (input.digging_status == 5)
                out.proposed_packet_state = 2;
            // Even non-release C07 digging consumes the one-shot lock.
            release_locked_ = false;
        }
        out.release_locked = release_locked_;
        return out;
    }

    void reset() noexcept { release_locked_ = false; }
    bool release_locked() const noexcept { return release_locked_; }
private:
    bool release_locked_ = false;
};
} // namespace slinkywinky::reference_compat
