#pragma once
// R267: one source-authenticated GUI/ArrayList feed for the R211 three native
// owner *decision probes* and R254 three separate OFFLINE policy owners.
// No JNI, game input, packet delivery or Minecraft writes are performed here.
// The two providers must be on the SAME owner thread and same world generation.
#include "slinkywinky/gui_module_control_handoff.hpp"
#include "slinkywinky/module_control_capabilities.hpp"
#include "slinkywinky/recovered_source_module_policies.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <thread>

namespace slinkywinky {
class CombinedModuleOwnerAdapter final {
public:
    CombinedModuleOwnerAdapter() : owner_(std::this_thread::get_id()) {}
    CombinedModuleOwnerAdapter(const CombinedModuleOwnerAdapter&)=delete;
    CombinedModuleOwnerAdapter& operator=(const CombinedModuleOwnerAdapter&)=delete;

    template<class NativeOwner>
    bool publish_on_owner(const NativeOwner& native,
                          const RecoveredSourcePolicyOwner& policy,
                          ModuleArrayListFeed& feed,std::uint64_t sequence,
                          bool world_ready) {
        if(!on_owner() || retired_)return false;
        try {
            auto state=copy_state(native,policy,sequence,world_ready);
            if(!state || sequence<=last_sequence_ ||
               !feed.publish(*state)) {
                retire_on_owner(feed);
                return false;
            }
            last_sequence_=sequence;
            last_generation_=state->world_generation;
            return true;
        }catch(...) {
            retire_on_owner(feed);
            return false;
        }
    }

    // A real GUI thread must have already admitted the gesture; no preview
    // state, game pointer or false activation can reach this owner.
    template<class NativeOwner>
    bool apply_request_on_owner(NativeOwner& native,
                          RecoveredSourcePolicyOwner& policy,
                          GuiModuleControlHandoff& gui,ModuleArrayListFeed& feed,
                          std::uint64_t response_sequence,bool world_ready) {
        if(!on_owner() || retired_)return false;
        try {
            const auto request=gui.take_on_module_owner();
            if(!request)return false;
            const auto prior=feed.latest();
            const auto current=prior?copy_state(native,policy,
                prior->source_sequence,world_ready):std::nullopt;
            if(!prior || !current || !same(*prior,*current) ||
               request->observed_source_sequence!=prior->source_sequence ||
               request->world_generation!=prior->world_generation ||
               response_sequence<=prior->source_sequence ||
               response_sequence<=last_sequence_ ||
               (last_generation_ && prior->world_generation<last_generation_)) {
                gui.retire();
                retire_on_owner(feed);
                return false;
            }
            const auto row=std::find_if(
                prior->modules.begin(),prior->modules.end(),
                [&](const ModuleArrayListState& state) {
                    return state.canonical_name==request->canonical_name;
                });
            if(row==prior->modules.end() ||
               !module_control_request_supported(
                    module_control_capability(request->canonical_name)) ||
               row->requested_enabled==request->enable) {
                gui.retire();
                retire_on_owner(feed);
                return false;
            }
            bool accepted=false;
            const std::string_view name(request->canonical_name);
            if(module_control_name_in(name,kNativeOwnerProbeNames)) {
                // Template call has the actual PR2's R211 request signature.
                accepted=native.request_module({name,request->enable});
            } else if(module_control_name_in(name,kOfflinePolicyNames)) {
                const auto it=std::find_if(
                    kRecoveredSourceRegistrations.begin(),
                    kRecoveredSourceRegistrations.end(),
                    [name](const RecoveredSourceRegistration& entry) {
                        return name==entry.name;
                    });
                if(it!=kRecoveredSourceRegistrations.end())
                    accepted=policy.select(
                        static_cast<RecoveredSourceModule>(
                            it-kRecoveredSourceRegistrations.begin()),
                        request->enable);
            }
            if(!accepted) {
                // The native owner may have mutated before refusing.
                // A denial is safe ONLY if the entire owner state is intact.
                const auto denied=copy_state(native,policy,
                    prior->source_sequence,world_ready);
                if(!denied || !same(*prior,*denied) ||
                   !gui.deny_on_module_owner(request->id)) {
                    gui.retire();
                    retire_on_owner(feed);
                }
                return false;
            }
            const auto next=copy_state(native,policy,response_sequence,world_ready);
            if(!next || next->world_generation!=prior->world_generation ||
               next->world_ready!=prior->world_ready ||
               next->modules.size()!=prior->modules.size()) {
                gui.retire();
                retire_on_owner(feed);
                return false;
            }
            // Owner acknowledge means ONE requested module changed.
            // No other module may silently change within the same request.
            bool found=false;
            for(std::size_t i=0;i<next->modules.size();++i) {
                const auto& before=prior->modules[i];
                const auto& after=next->modules[i];
                if(after.canonical_name!=before.canonical_name ||
                   after.world_generation!=before.world_generation) {
                    gui.retire();retire_on_owner(feed);return false;
                }
                if(after.canonical_name==name) {
                    found=true;
                    if(after.requested_enabled!=request->enable) {
                        gui.retire();retire_on_owner(feed);return false;
                    }
                } else if(after.requested_enabled!=before.requested_enabled ||
                          after.active_in_world!=before.active_in_world) {
                    gui.retire();retire_on_owner(feed);return false;
                }
            }
            if(!found || !gui.acknowledge_on_module_owner(
                              request->id,*next,feed)) {
                gui.retire();
                retire_on_owner(feed);
                return false;
            }
            last_sequence_=response_sequence;
            last_generation_=next->world_generation;
            return true;
        }catch(...) {
            gui.retire();
            retire_on_owner(feed);
            return false;
        }
    }

    bool retire_on_owner(ModuleArrayListFeed& feed) noexcept {
        if(!on_owner())return false;
        retired_=true;
        feed.retire();
        return true;
    }
    bool retired() const noexcept {return retired_;}
    std::uint64_t last_sequence() const noexcept {return last_sequence_;}
    std::uint64_t last_generation() const noexcept {return last_generation_;}
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
    template<class NativeOwner>
    static std::optional<ModuleArrayListSnapshot> copy_state(
            const NativeOwner& native,
            const RecoveredSourcePolicyOwner& policy,
            std::uint64_t sequence,bool world_ready) {
        if(!sequence)return std::nullopt;
        const auto native_states=native.control_states();
        const auto offline=policy.snapshot(sequence);
        if(!native_states || native_states->size()!=3u ||
           !offline || offline->modules.size()!=3u ||
           offline->world_ready!=world_ready)return std::nullopt;
        const std::uint64_t gen=native.generation();
        if(!gen || gen!=offline->world_generation ||
           gen!=policy.generation())return std::nullopt;
        ModuleArrayListSnapshot merged;
        merged.source_sequence=sequence;
        merged.world_generation=gen;
        merged.world_ready=world_ready;
        merged.modules.reserve(6u);
        for(std::size_t i=0;i<kNativeOwnerProbeNames.size();++i) {
            const auto& row=(*native_states)[i];
            if(std::string_view(row.canonical_name)!=kNativeOwnerProbeNames[i] ||
               row.world_generation!=gen ||
               (row.active_in_world && (!row.requested_enabled || !world_ready)))
                return std::nullopt;
            merged.modules.push_back({
                row.canonical_name,row.requested_enabled,
                row.active_in_world,row.world_generation
            });
        }
        for(std::size_t i=0;i<kOfflinePolicyNames.size();++i) {
            const auto& row=offline->modules[i];
            if(std::string_view(row.canonical_name)!=kOfflinePolicyNames[i] ||
               row.world_generation!=gen ||
               (row.active_in_world && (!row.requested_enabled || !world_ready)))
                return std::nullopt;
            merged.modules.push_back(row);
        }
        if(!make_module_arraylist_display(merged))return std::nullopt;
        return merged;
    }
    const std::thread::id owner_;
    std::uint64_t last_sequence_=0;
    std::uint64_t last_generation_=0;
    bool retired_=false;
};
} // namespace slinkywinky
