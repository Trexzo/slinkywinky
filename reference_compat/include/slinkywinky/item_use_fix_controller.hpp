#pragma once
// R185: independent offline Item Use Fix. Never writes JVM/game state.
#include <optional>
namespace slinkywinky::reference_compat {
inline constexpr unsigned kItemUseFixBeforeEvent=14;
inline constexpr unsigned kItemUseFixAfterEvent=15;
struct ItemUseFixInput {bool skip=false; bool hitting_block=false;};
struct ItemUseFixDecision {
    std::optional<bool> proposed_hitting_block{};
    bool restore_pending=false;
};
class ItemUseFixController final {
public:
    ItemUseFixDecision before(const ItemUseFixInput& in) noexcept {
        ItemUseFixDecision out;
        if (!in.skip) {
            restore_pending_=in.hitting_block;
            if (in.hitting_block) out.proposed_hitting_block=false;
        }
        out.restore_pending=restore_pending_;
        return out;
    }
    ItemUseFixDecision after(const ItemUseFixInput& in) noexcept {
        ItemUseFixDecision out;
        if (restore_pending_ && !in.skip) out.proposed_hitting_block=true;
        // The candidate does not clear this flag in the after handler.
        out.restore_pending=restore_pending_;
        return out;
    }
    bool restore_pending() const noexcept {return restore_pending_;}
    void reset() noexcept {restore_pending_=false;}
private:
    bool restore_pending_=false;
};
} // namespace slinkywinky::reference_compat
