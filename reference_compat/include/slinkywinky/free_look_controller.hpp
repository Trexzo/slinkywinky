#pragma once
// R253: independent read-only Free Look lifecycle and event-13 policy.
// No Minecraft pointers, third-person camera writes, input or live hook.
#include <cmath>
#include <cstdint>

namespace slinkywinky::reference_compat {

struct FreeLookFrame {
    bool game_ready=false;
    std::uint64_t world_token=0,player_token=0;
    bool stored_camera_valid=false;
    float stored_camera_yaw=0.0f,stored_camera_pitch=0.0f;
    float player_yaw=0.0f,player_pitch=0.0f;
    int current_perspective=0,target_perspective=2;
};
struct FreeLookProposal {
    bool admitted=false,rotation_active=false;
    bool set_perspective=false,restore_perspective=false,restore_camera=false;
    int perspective=0;
    float yaw=0.0f,pitch=0.0f;
};
class FreeLookController final {
public:
    FreeLookProposal on_enable(const FreeLookFrame& frame) noexcept {
        FreeLookProposal out{};
        if(!valid(frame))return out;
        if(saved_) {
            if(!same(frame)){reset();return out;}
            out.admitted=true;
            return out;
        }
        out.admitted=true;
        if(!frame.game_ready)return out;
        saved_=true;world_=frame.world_token;player_=frame.player_token;
        previous_view_=frame.current_perspective;
        yaw_=frame.stored_camera_valid?
            frame.stored_camera_yaw:frame.player_yaw;
        pitch_=frame.stored_camera_valid?
            frame.stored_camera_pitch:frame.player_pitch;
        out.set_perspective=true;
        out.perspective=frame.target_perspective;
        return out;
    }
    // Original rotation callback type 13 lazy-initializes if not yet saved.
    FreeLookProposal on_event13(const FreeLookFrame& frame) noexcept {
        FreeLookProposal out=on_enable(frame);
        if(!out.admitted)return out;
        if(saved_) {
            out.rotation_active=true;
            out.yaw=yaw_;out.pitch=pitch_;
        }
        return out;
    }
    FreeLookProposal on_disable(std::uint64_t world,
                                std::uint64_t player) noexcept {
        FreeLookProposal out{};
        out.admitted=true;
        if(saved_ && world==world_ && player==player_) {
            out.restore_perspective=true;
            out.restore_camera=true;
            out.perspective=previous_view_;
            out.yaw=yaw_;out.pitch=pitch_;
        }
        // On an ownership change, discard old camera data, never write it
        // into a newly joined world/player (safe divergence from source).
        reset();
        return out;
    }
    FreeLookProposal on_world_unload(std::uint64_t world,
                                    std::uint64_t player) noexcept {
        return on_disable(world,player);
    }
    void reset() noexcept {
        saved_=false;world_=player_=0;
        previous_view_=0;yaw_=pitch_=0.0f;
    }
    bool saved() const noexcept {return saved_;}
private:
    bool same(const FreeLookFrame& f) const noexcept {
        return f.world_token==world_ && f.player_token==player_;
    }
    static bool valid(const FreeLookFrame& f) noexcept {
        const auto finite=[](float v) noexcept {
            return std::isfinite(v) && std::fabs(v)<=1000000.0f;
        };
        return f.current_perspective>=0 && f.current_perspective<=2 &&
               f.target_perspective>=0 && f.target_perspective<=2 &&
               finite(f.player_yaw) && finite(f.player_pitch) &&
               finite(f.stored_camera_yaw) && finite(f.stored_camera_pitch) &&
               (!f.game_ready ||
                (f.world_token>0 && f.player_token>0));
    }
    bool saved_=false;
    std::uint64_t world_=0,player_=0;
    int previous_view_=0;
    float yaw_=0.0f,pitch_=0.0f;
};
} // namespace slinkywinky::reference_compat
