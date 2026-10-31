#include "slinkywinky/session.hpp"

namespace slinkywinky {
HostSession::HostSession(std::array<std::uint32_t,4> seed) : coordinator_(seed) {}
bool HostSession::attach() {return mailbox_.attach_owner();}
bool HostSession::start_world() {return mailbox_.begin_world();}
bool HostSession::queue_packet_notice(std::uint32_t id,std::uint64_t received_ms) {
    if(mailbox_.phase()!=RuntimePhase::running) return false;
    return held_.enqueue(id,received_ms,mailbox_.generation());
}
HostBatch HostSession::tick(TickSnapshot snapshot,const ClientSettings& settings) {
    HostBatch batch;
    if(mailbox_.phase()!=RuntimePhase::running) return batch;
    const auto gen=mailbox_.generation();
    if(snapshot.generation!=gen || !snapshot.world_valid) {
        // Inconsistent frame must never send queued operations to a new world.
        held_.discard_all();
        batch.decision=coordinator_.disable(snapshot.mouse_left,snapshot.mouse_right);
        batch.emergency_release=true;
        return batch;
    }
    batch.decision=coordinator_.tick(snapshot,settings.combat);
    auto submit=[&](CommandKind k,float a=0,float b=0){
        if (!mailbox_.submit(k,a,b)) batch.emergency_release=true;
    };
    if(batch.decision.rotation)
        submit(CommandKind::set_rotation,batch.decision.rotation->yaw,batch.decision.rotation->pitch);
    if(batch.decision.left_edge!=KeyEdge::none)
        submit(CommandKind::set_attack,batch.decision.left_held?1.f:0.f);
    if(batch.decision.right_edge!=KeyEdge::none)
        submit(CommandKind::set_block,batch.decision.right_held?1.f:0.f);
    if(batch.decision.attempt_attack) submit(CommandKind::set_attack,2.f);
    batch.commands=mailbox_.drain();
    if(settings.backtrack_enabled) {
        batch.released_packet_notices=held_.expire(snapshot.elapsed_ms,settings.backtrack_ms,gen);
    } else {
        batch.released_packet_notices=held_.release_all(gen);
    }
    batch.valid=!batch.emergency_release;
    return batch;
}
HostBatch HostSession::release_state(bool left,bool right) {
    HostBatch batch;
    batch.decision=coordinator_.disable(left,right);
    batch.emergency_release=true;
    batch.valid=true;
    batch.commands.push_back({CommandKind::release_all,mailbox_.generation(),0,0,0});
    return batch;
}
HostBatch HostSession::leave_world(bool left,bool right) {
    HostBatch batch;
    if(mailbox_.phase()!=RuntimePhase::running) return batch;
    batch=release_state(left,right);
    held_.discard_all();
    if(!mailbox_.world_changed()) batch.valid=false;
    return batch;
}
HostBatch HostSession::shutdown(bool left,bool right) {
    HostBatch batch;
    if(mailbox_.phase()!=RuntimePhase::attached && mailbox_.phase()!=RuntimePhase::running)
        return batch;
    batch=release_state(left,right);
    held_.discard_all();
    if(!mailbox_.shutdown()) batch.valid=false;
    return batch;
}
} // namespace slinkywinky
