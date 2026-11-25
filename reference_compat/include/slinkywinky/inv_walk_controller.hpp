#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace slinkywinky::reference_compat {
enum class InvWalkMode : unsigned{vanilla=0,silent=1};
enum class InvWalkStop : unsigned{always=0,sprint_only=1,never=2};
enum class InvWalkScreen : unsigned{none=0,inventory=1,other=2};
enum class InvWalkPhase : unsigned{no_screen=0,other_screen=1,server_closed=2,server_open=3};
enum class InvWalkPacketType{other,open_inventory,click_window,close_window};
enum class InvWalkPacketAction{pass,consume,queue,queue_after_server_open,overflow_rejected};
struct InvWalkSettings{
    InvWalkMode mode=InvWalkMode::vanilla;
    InvWalkStop stop_on_click=InvWalkStop::sprint_only;
    InvWalkStop stop_non_inventory=InvWalkStop::sprint_only;
    float stop_duration_ticks=4;
};
struct InvWalkInput{
    bool in_game=false;
    InvWalkScreen screen=InvWalkScreen::none;
    std::array<bool,5> physically_held{}; // back,left,right,forward,jump
    bool cursor_busy=false,crafting_busy=false;
};
struct InvWalkPacketProposal{
    InvWalkPacketAction action=InvWalkPacketAction::pass;
    bool send_open_inventory=false;
};
struct InvWalkProposal {
    bool update_keys=false;
    std::array<bool,5> pressed{};
    bool cancel_sprint=false;
    bool close_on_server=false,open_on_server=false;
    bool drain_stale_keyboard_events=false;
};
class InvWalkController{
public:
    void configure(InvWalkSettings s){
        if(static_cast<unsigned>(s.mode)>1||static_cast<unsigned>(s.stop_on_click)>2||
           static_cast<unsigned>(s.stop_non_inventory)>2||
           !std::isfinite(s.stop_duration_ticks)||s.stop_duration_ticks<0||
           s.stop_duration_ticks>100000)
            throw std::invalid_argument("bad Inv Walk settings");
        settings_=s;
    }
    void reset_world()noexcept{
        phase_=InvWalkPhase::no_screen;queue_count_=0;
        tick_count_=last_activity_=0;
        replaying_=released_last_tick_=false;
    }
    InvWalkPacketProposal packet(InvWalkPacketType kind)noexcept{
        InvWalkPacketProposal out{};
        if(settings_.mode!=InvWalkMode::silent)return out;
        if(kind==InvWalkPacketType::open_inventory){
            phase_=InvWalkPhase::server_closed;
            out.action=InvWalkPacketAction::consume;
        }else if(kind==InvWalkPacketType::click_window){
            if(phase_==InvWalkPhase::server_closed){
                out.send_open_inventory=true;phase_=InvWalkPhase::server_open;
                replaying_=true;
                out.action=queue_one()?InvWalkPacketAction::queue_after_server_open:
                                       InvWalkPacketAction::overflow_rejected;
            }else if(queue_count_==0&&!replaying_){
                last_activity_=tick_count_;
            }else{
                out.action=queue_one()?InvWalkPacketAction::queue:
                                       InvWalkPacketAction::overflow_rejected;
            }
        }else if(kind==InvWalkPacketType::close_window){
            if(phase_==InvWalkPhase::server_closed)
                out.action=InvWalkPacketAction::consume;
            else if(queue_count_!=0)
                out.action=queue_one()?InvWalkPacketAction::queue:
                                       InvWalkPacketAction::overflow_rejected;
        }
        return out;
    }
    // Caller reports queue flush after sending its owned packets; no refs stored.
    InvWalkProposal inventory_tick(bool flush_confirmed,bool in_game)noexcept{
        InvWalkProposal out{};
        if(replaying_||!flush_confirmed)return out;
        if(queue_count_==0)return out;
        queue_count_=0;
        last_activity_=tick_count_;
        out.drain_stale_keyboard_events=in_game;
        return out;
    }
    InvWalkProposal tick(const InvWalkInput& f){
        if(static_cast<unsigned>(f.screen)>2)
            throw std::invalid_argument("bad Inv Walk screen");
        InvWalkProposal out{};
        ++tick_count_;
        const bool moving=any(f.physically_held);
        bool cancel=settings_.mode==InvWalkMode::silent&&
                    settings_.stop_on_click!=InvWalkStop::never&&queue_count_!=0;
        bool release=settings_.stop_on_click==InvWalkStop::always&&cancel;
        bool update=false;
        if(f.screen==InvWalkScreen::none){
            if(phase_!=InvWalkPhase::no_screen){
                out.update_keys=true;out.pressed=f.physically_held;
            }
            phase_=InvWalkPhase::no_screen;
        }else if(f.screen==InvWalkScreen::other){
            phase_=InvWalkPhase::other_screen;
            if(settings_.mode==InvWalkMode::silent){
                cancel|=!moving;
                const bool recent=settings_.stop_on_click!=InvWalkStop::never&&clicked_recently();
                const bool stop=cancel||recent;
                const bool stop_non_inv=settings_.stop_non_inventory!=InvWalkStop::never||stop;
                cancel=stop_non_inv;
                if((settings_.stop_on_click==InvWalkStop::always&&stop)||
                   (settings_.stop_non_inventory==InvWalkStop::always&&stop_non_inv))
                    release=true;
                else update=true;
            }else update=true;
        }else{
            bool busy=false;
            if(settings_.mode!=InvWalkMode::silent){
                phase_=InvWalkPhase::server_open;update=true;
            }else{
                busy=f.cursor_busy||f.crafting_busy;
                if(phase_==InvWalkPhase::other_screen && !busy)busy=cancel;
                const bool allowed_move=moving&&!busy;
                if(phase_==InvWalkPhase::server_closed&&!allowed_move){
                    phase_=InvWalkPhase::server_open;
                    out.open_on_server=true;
                }else if(allowed_move&&queue_count_==0&&
                    phase_==InvWalkPhase::server_open &&
                    (settings_.stop_on_click==InvWalkStop::never||!clicked_recently())){
                    phase_=InvWalkPhase::server_closed;out.close_on_server=true;
                }
                busy|=phase_==InvWalkPhase::server_open;
                update=phase_!=InvWalkPhase::no_screen;
            }
            cancel|=settings_.stop_on_click!=InvWalkStop::never&&busy;
            release=settings_.stop_on_click==InvWalkStop::always&&cancel;
        }
        if(release){
            out.update_keys=true;
            out.pressed.fill(false);
        }else if(update||released_last_tick_){
            out.update_keys=true;
            out.pressed=f.physically_held;
        }
        out.cancel_sprint=cancel;
        released_last_tick_=release;
        replaying_=false;
        return out;
    }
    // Module disable: the game layer must flush its owned queued packets,
    // restore physical keys, and clear all external refs before detach.
    InvWalkProposal disable(const std::array<bool,5>& physical)noexcept{
        InvWalkProposal out{};
        out.update_keys=true;out.pressed=physical;
        reset_world();
        return out;
    }
    std::size_t queued()const noexcept{return queue_count_;}
    InvWalkPhase phase()const noexcept{return phase_;}
    std::uint64_t tick_count()const noexcept{return tick_count_;}
    bool replaying()const noexcept{return replaying_;}
private:
    bool clicked_recently()const noexcept{
        const auto delta=tick_count_>=last_activity_?tick_count_-last_activity_:0;
        return delta<static_cast<std::uint64_t>(settings_.stop_duration_ticks);
    }
    bool queue_one()noexcept{
        if(queue_count_==32)return false;
        ++queue_count_;return true;
    }
    static bool any(const std::array<bool,5>& k)noexcept{
        for(bool held:k)if(held)return true;
        return false;
    }
    InvWalkSettings settings_{};
    InvWalkPhase phase_=InvWalkPhase::no_screen;
    std::size_t queue_count_=0;
    std::uint64_t tick_count_=0,last_activity_=0;
    bool replaying_=false,released_last_tick_=false;
};
} // namespace slinkywinky::reference_compat
