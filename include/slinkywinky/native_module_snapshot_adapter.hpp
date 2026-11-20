#pragma once
#include "slinkywinky/gui_module_control_handoff.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

namespace slinkywinky {

// R246: header-only contract adapter so PR #1 does NOT copy PR #2's
// module_control_contract.hpp or link its native engine before merge.
// Owner must provide the R211 signature:
//   control_states() -> optional<vector<ModuleControlState>>
//   generation() -> uint64_t
//   request_module({string_view canonical_name, bool enable}) -> bool
// Every call touching the native manager must run on this pinned owner.
// No Java pointers, gameplay callbacks, or GUI preview booleans cross it.
class NativeModuleSnapshotAdapter final {
public:
    NativeModuleSnapshotAdapter()
        : owner_(std::this_thread::get_id()) {}
    NativeModuleSnapshotAdapter(const NativeModuleSnapshotAdapter&)=delete;
    NativeModuleSnapshotAdapter& operator=(const NativeModuleSnapshotAdapter&)=delete;

    template <typename NativeOwner>
    bool publish_on_owner(const NativeOwner& native, ModuleArrayListFeed& feed,
            std::uint64_t sequence,bool world_ready) {
        if(!on_owner() || retired_)return false;
        try {
            auto snapshot=copy_state(native,sequence,world_ready);
            if(!snapshot || sequence<=last_sequence_ ||
               !feed.publish(*snapshot)) {
                retire_owner(feed);
                return false;
            }
            last_sequence_=sequence;
            return true;
        } catch(...) {
            // An owner read may fail after observing a partial session
            // transition. Never retain earlier active HUD rows.
            retire_owner(feed);
            return false;
        }
    }

    // The GUI must already have authenticated the gesture. This function
    // never accepts GUI preview state as an authoritative source.
    template <typename NativeOwner>
    bool apply_request_on_owner(NativeOwner& native,
            GuiModuleControlHandoff& handoff,ModuleArrayListFeed& feed,
            std::uint64_t response_sequence,bool world_ready) {
        if(!on_owner() || retired_)return false;
        // R249: catch even ambiguous state/read exceptions. The native
        // request itself might already have changed module selections.
        try {
        const auto request=handoff.take_on_module_owner();
        if(!request)return false;
        const auto prior=feed.latest();
        // Re-derive the entire source state from the actual owner. A GUI
        // request against a stale generation or altered module selection
        // must never reach request_module, even with a matching sequence.
        const auto check=prior ?
            copy_state(native,prior->source_sequence,prior->world_ready)
            : std::nullopt;
        if(!prior || !check || !same(*prior,*check) ||
           prior->source_sequence!=request->observed_source_sequence ||
           prior->world_generation!=request->world_generation ||
           prior->world_ready!=world_ready ||
           response_sequence<=prior->source_sequence ||
           response_sequence<=last_sequence_) {
            handoff.retire();
            retire_owner(feed);
            return false;
        }
        // Deliberate brace-init of PR #2 R211's ModuleControlRequest,
        // without reproducing its header or wrapping a gameplay API.
        if(!native.request_module({std::string_view(request->canonical_name),
                                   request->enable})) {
            // A false return alone is NOT proof that the native owner
            // left its state untouched. Re-read the entire source state,
            // including generation, before publishing a safe denial.
            const auto denied=copy_state(
                native,prior->source_sequence,prior->world_ready);
            if(!denied || !same(*prior,*denied) ||
               !handoff.deny_on_module_owner(request->id)) {
                handoff.retire();
                retire_owner(feed);
            }
            return false;
        }
        const auto next=copy_state(native,response_sequence,world_ready);
        if(!next ||
           !handoff.acknowledge_on_module_owner(
                request->id,*next,feed)) {
            // Native owner may have changed state before a malformed
            // acknowledgement. This is a quarantine, NOT a rollback of
            // game actions or a claim that disabling succeeded.
            handoff.retire();
            retire_owner(feed);
            return false;
        }
        last_sequence_=response_sequence;
        return true;
        } catch(...) {
            // A throw before/after a native mutation, or during an
            // acknowledgement, is an UNKNOWN outcome; it is neither
            // a safe denial nor a successful game-state rollback.
            handoff.retire();
            retire_owner(feed);
            return false;
        }
    }

    bool retire_owner(ModuleArrayListFeed& feed) {
        if(!on_owner())return false;
        retired_=true;
        feed.retire();
        return true;
    }
    bool retired() const noexcept {return retired_;}
private:
    bool on_owner() const noexcept {
        return std::this_thread::get_id()==owner_;
    }
    static bool same(const ModuleArrayListSnapshot& a,
                     const ModuleArrayListSnapshot& b) {
        if(a.contract_version!=b.contract_version ||
           a.source_sequence!=b.source_sequence ||
           a.world_generation!=b.world_generation ||
           a.world_ready!=b.world_ready ||
           a.modules.size()!=b.modules.size())return false;
        for(std::size_t i=0;i<a.modules.size();++i) {
            const auto& x=a.modules[i];
            const auto& y=b.modules[i];
            if(x.canonical_name!=y.canonical_name ||
               x.requested_enabled!=y.requested_enabled ||
               x.active_in_world!=y.active_in_world ||
               x.world_generation!=y.world_generation)return false;
        }
        return true;
    }
    template <typename NativeOwner>
    static std::optional<ModuleArrayListSnapshot> copy_state(
            const NativeOwner& native,std::uint64_t sequence,bool world_ready) {
        if(sequence==0)return std::nullopt;
        const auto original=native.control_states();
        if(!original || original->size()!=3u)return std::nullopt;
        const std::uint64_t generation=native.generation();
        if(world_ready && generation==0u)return std::nullopt;
        constexpr std::array<std::string_view,3> names{
            "No Jump Delay","No Slow","Sprint"
        };
        ModuleArrayListSnapshot copy;
        copy.source_sequence=sequence;
        copy.world_generation=generation;
        copy.world_ready=world_ready;
        copy.modules.reserve(3u);
        for(std::size_t i=0;i<names.size();++i) {
            const auto& state=(*original)[i];
            if(std::string_view(state.canonical_name)!=names[i] ||
               !candidate_native_manager_registered(names[i]) ||
               state.world_generation!=generation ||
               (state.active_in_world &&
                  (!state.requested_enabled || !world_ready)))
                return std::nullopt;
            copy.modules.push_back({
                std::string(state.canonical_name),
                state.requested_enabled,
                state.active_in_world,
                state.world_generation
            });
        }
        if(!make_module_arraylist_display(copy))return std::nullopt;
        return copy;
    }

    const std::thread::id owner_;
    bool retired_=false;
    std::uint64_t last_sequence_=0;
};

} // namespace slinkywinky
