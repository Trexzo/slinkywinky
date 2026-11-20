#pragma once
#include "slinkywinky/native_module_snapshot_adapter.hpp"
#include <cstdint>
#include <limits>
#include <memory>

namespace slinkywinky {

// R250: OFFLINE owner-session recovery preflight, not live JVM/OS identity
// authentication. The caller must independently authorize any new attach.
// This fence cannot reopen a quarantined feed, request channel, or adapter.
// The old native and adapter must have fully quiesced before calling it.
class NativeModuleRecoveryFence final {
public:
    template <typename NativeOwner>
    static bool admit_after_quiescence(
        const NativeOwner& old_native,
        const NativeModuleSnapshotAdapter& old_adapter,
        const ModuleArrayListFeed& old_feed,
        const GuiModuleControlHandoff& old_gui,
        const NativeOwner& fresh_native,
        NativeModuleSnapshotAdapter& fresh_adapter,
        ModuleArrayListFeed& fresh_feed,
        std::uint64_t fresh_sequence, bool world_ready) {
        try {
            // Never treat an old pending/dispatched request as a new session,
            // or silently revive a view that was retired for ambiguity.
            if(!old_adapter.retired() || !old_feed.retired() ||
               old_feed.latest().has_value() ||
               old_gui.view().phase!=GuiModuleControlPhase::Retired ||
               !old_native.stopped() || fresh_native.stopped() ||
               std::addressof(old_native)==std::addressof(fresh_native) ||
               fresh_adapter.retired() || fresh_feed.retired() ||
               fresh_feed.latest().has_value() || !world_ready)
                return false;
            const auto generation_floor=
                old_adapter.last_confirmed_world_generation();
            const auto sequence_floor=
                old_adapter.last_confirmed_source_sequence();
            if(generation_floor==0u ||
               generation_floor==std::numeric_limits<std::uint64_t>::max() ||
               sequence_floor==0u ||
               sequence_floor==std::numeric_limits<std::uint64_t>::max() ||
               fresh_native.generation()<=generation_floor ||
               fresh_sequence<=sequence_floor)
                return false;
            // Publish only the NEW manager's actual module-state snapshot,
            // with separate publication, ownership and request objects.
            // A bad new owner snapshot quarantines this fresh adapter/feed.
            return fresh_adapter.publish_on_owner(
                fresh_native,fresh_feed,fresh_sequence,true);
        } catch(...) {
            // No native state is modified here. An invalid preflight is
            // refused; source-authority remains with the actual owner.
            return false;
        }
    }
};

} // namespace slinkywinky
