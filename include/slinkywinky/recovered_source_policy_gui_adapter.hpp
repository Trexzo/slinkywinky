#pragma once
// R255: GUI request/acknowledgement adapter for the three PR1
// independently reconstructed OFFLINE-only R254 policies.
// Not a gameplay/OS/JNI dispatcher. Selection changes decision model
// state only, and the source feed is never derived from GUI previews.
#include "slinkywinky/recovered_source_module_policies.hpp"
#include "slinkywinky/gui_module_control_handoff.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>
#include <thread>

namespace slinkywinky {
class RecoveredSourcePolicyGuiAdapter final {
public:
    RecoveredSourcePolicyGuiAdapter()
        :owner_(std::this_thread::get_id()) {}
    RecoveredSourcePolicyGuiAdapter(const RecoveredSourcePolicyGuiAdapter&)=delete;
    RecoveredSourcePolicyGuiAdapter& operator=(const RecoveredSourcePolicyGuiAdapter&)=delete;
    bool publish(RecoveredSourcePolicyOwner& native,
                 ModuleArrayListFeed& feed,std::uint64_t seq) {
        if(!on_owner() || retired_)return false;
        try{
            const auto next=native.snapshot(seq);
            if(!next || seq<=last_sequence_ || !feed.publish(*next)){
                retire(feed);
                return false;
            }
            last_sequence_=seq;
            return true;
        }catch(...){
            retire(feed);
            return false;
        }
    }
    bool apply(RecoveredSourcePolicyOwner& native,
               GuiModuleControlHandoff& gui,ModuleArrayListFeed& feed,
               std::uint64_t response_sequence) {
        if(!on_owner() || retired_)return false;
        try{
            const auto request=gui.take_on_module_owner();
            if(!request)return false;
            const auto previous=feed.latest();
            const auto now=previous?
                native.snapshot(previous->source_sequence):std::nullopt;
            if(!previous || !now || !same(*previous,*now) ||
               response_sequence<=last_sequence_ ||
               response_sequence<=previous->source_sequence ||
               request->observed_source_sequence!=previous->source_sequence ||
               request->world_generation!=previous->world_generation){
                gui.retire();
                retire(feed);
                return false;
            }
            const auto found=std::find_if(
                kRecoveredSourceRegistrations.begin(),
                kRecoveredSourceRegistrations.end(),
                [&](const RecoveredSourceRegistration& spec){
                    return std::string_view(spec.name)==request->canonical_name;
                });
            if(found==kRecoveredSourceRegistrations.end()){
                // The source owner is NOT the R211 controller.
                gui.retire();
                retire(feed);
                return false;
            }
            const auto module=static_cast<RecoveredSourceModule>(
                found-kRecoveredSourceRegistrations.begin());
            const auto row=static_cast<std::size_t>(
                found-kRecoveredSourceRegistrations.begin());
            if(previous->modules[row].requested_enabled==request->enable ||
               !native.select(module,request->enable)){
                gui.retire();
                retire(feed);
                return false;
            }
            const auto next=native.snapshot(response_sequence);
            if(!next || !gui.acknowledge_on_module_owner(
                             request->id,*next,feed)){
                // The OFFLINE selection may have changed even if the
                // GUI ACK failed; retain no misleading active feed.
                gui.retire();
                retire(feed);
                return false;
            }
            last_sequence_=response_sequence;
            return true;
        }catch(...){
            gui.retire();
            retire(feed);
            return false;
        }
    }
    bool retire(ModuleArrayListFeed& feed){
        if(!on_owner())return false;
        retired_=true;
        feed.retire();
        return true;
    }
    bool retired() const noexcept{return retired_;}
private:
    static bool same(const ModuleArrayListSnapshot& a,
                     const ModuleArrayListSnapshot& b){
        if(a.contract_version!=b.contract_version ||
           a.source_sequence!=b.source_sequence ||
           a.world_generation!=b.world_generation ||
           a.world_ready!=b.world_ready ||
           a.modules.size()!=b.modules.size())return false;
        for(std::size_t i=0;i<a.modules.size();++i){
            const auto& x=a.modules[i];
            const auto& y=b.modules[i];
            if(x.canonical_name!=y.canonical_name ||
               x.requested_enabled!=y.requested_enabled ||
               x.active_in_world!=y.active_in_world ||
               x.world_generation!=y.world_generation)
                return false;
        }
        return true;
    }
    bool on_owner() const noexcept{
        return std::this_thread::get_id()==owner_;
    }
    const std::thread::id owner_;
    std::uint64_t last_sequence_=0;
    bool retired_=false;
};
} // namespace slinkywinky
