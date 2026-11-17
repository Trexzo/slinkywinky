#pragma once
#ifdef _WIN32
#include "slinkywinky/windows_game_window_gate.hpp"
#include <array>
#include <cstdint>
#include <thread>

namespace slinkywinky {

// R237: read-only ORIGINAL render-owner, source-exact observed button
// sequence. This is a GetKeyState SAMPLE trace, not a captured LWJGL
// Mouse.next() event, an exclusive GUI-input lease or a hardware provenance
// proof. No coordinates, HWNDs, key codes or process handles are retained.
class OriginalPointerEdgeTrace final {
public:
    OriginalPointerEdgeTrace() noexcept : owner_(std::this_thread::get_id()) {}

    bool observe(GameWindowFocus focus,bool source_visible,
                 bool on_source,std::uint32_t buttons) noexcept {
        if(std::this_thread::get_id()!=owner_ || samples_>=2048u ||
           buttons>31u || (focus!=GameWindowFocus::ValidForeground &&
                           buttons!=0u) ||
           (on_source && !source_visible))
            return false;
        ++samples_;
        if(focus==GameWindowFocus::ValidForeground)++foreground_;
        if(phase_==Phase::Revoked)return true;
        if(focus!=GameWindowFocus::ValidForeground) {
            // An interrupted press may never be credited as a release
            // after focus is regained. New original session required.
            if(phase_==Phase::Held) {
                ++cancelled_;
                phase_=Phase::Revoked;
            } else phase_=Phase::NeedRelease;
            return true;
        }
        if(!source_visible || !on_source) {
            if(phase_==Phase::Held)++cancelled_;
            phase_=Phase::NeedRelease;
            return true;
        }
        if(phase_==Phase::Held) {
            if(buttons==1u)return true;
            if(buttons==0u) {
                ++completed_;
                phase_=Phase::Ready;
            } else {
                ++cancelled_;
                phase_=Phase::NeedRelease;
            }
            return true;
        }
        // A held button first seen over the source is NOT a new press.
        // Require a clean released sample at this exact painted target.
        if(buttons==0u) {
            phase_=Phase::Ready;
        } else if(buttons==1u && phase_==Phase::Ready) {
            ++presses_;
            phase_=Phase::Held;
        } else phase_=Phase::NeedRelease;
        return true;
    }

    // [sampled_frames, exact_foreground, admitted_left_down,
    //  matching_left_release, cancelled_press]. All bounded aggregates.
    std::array<std::uint64_t,5> counts() const noexcept {
        if(std::this_thread::get_id()!=owner_)return {};
        return {samples_,foreground_,presses_,completed_,cancelled_};
    }
    bool may_deliver_minecraft_input() const noexcept {return false;}
    bool may_suppress_vanilla_input() const noexcept {return false;}

private:
    enum class Phase : unsigned char {NeedRelease,Ready,Held,Revoked};
    const std::thread::id owner_;
    Phase phase_=Phase::NeedRelease;
    std::uint64_t samples_=0,foreground_=0,presses_=0,
                  completed_=0,cancelled_=0;
};
} // namespace slinkywinky
#endif
