#pragma once
// R238: first-party Java-8-snapshot -> native Clutch world evidence adapter.
// Value-only/owner-bound. No JNI endpoint, Minecraft hook, packet or placement.
// The Java producer and JNI transport need separate live authentication.
#include "slinkywinky/clutch_candidate_planner.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <thread>
#include <vector>

namespace slinkywinky::reference_compat {

struct ClutchObservedFrame {
    std::uint64_t generation=0,tick=0;
    // Session-assigned opaque identities; NEVER raw/retained JVM pointers.
    std::uint64_t world_token=0,player_token=0;
    BlockInVec3 box_lo{},box_hi{},eye{};
    int base_x=0,base_y=0,base_z=0;
    unsigned range_index=0;
    float min_height=0.0f,max_aim_degrees=180.0f;
    float current_yaw=0.0f,current_pitch=0.0f;
    float randomisation=1.0f;
    double clock_seconds=0.0;
    bool sideways_only=false,multipoint=true;
    bool ground_blocked=false;
    // Exactly the observed source prefix for range_index; not padded.
    std::vector<int> candidate_block_ids{};
    // -1 => unobserved. Each observed ID uses 0=air, 1..4095=block.
    std::array<int,8> ground_block_ids{{-1,-1,-1,-1,-1,-1,-1,-1}};
    std::optional<ClutchSourcePrior> previous{};
    // -1 if absent/not examined, otherwise the source-observed block ID.
    int previous_block_id=-1;
};

/**
 * Native owner gate for typed Java observation receipts.
 * This converts a bounded, coherent source observation into the existing
 * R235 pure planner input. It does not verify that bytes came from a real JVM:
 * that is the missing R239 JNI/generation and tick transport boundary.
 */
class ClutchObservationOwner final {
public:
    ClutchObservationOwner() noexcept
        : owner_thread_(std::this_thread::get_id()) {}

    std::optional<ClutchCandidateWorld> consume(
        const ClutchObservedFrame& f) noexcept {
        if(closed_ || std::this_thread::get_id()!=owner_thread_) {
            closed_=true;
            return std::nullopt;
        }
        auto deny=[&]() noexcept -> std::optional<ClutchCandidateWorld> {
            closed_=true;
            return std::nullopt;
        };
        if(!f.generation || !f.tick || !f.world_token || !f.player_token ||
           f.tick<=last_tick_ || f.range_index>=kClutchDistanceCounts.size() ||
           !clutch_valid_vec(f.box_lo) || !clutch_valid_vec(f.box_hi) ||
           !clutch_valid_vec(f.eye) ||
           f.box_lo.x>f.box_hi.x || f.box_lo.y>f.box_hi.y ||
           f.box_lo.z>f.box_hi.z ||
           !std::isfinite(f.current_yaw) ||
           !std::isfinite(f.current_pitch) ||
           !std::isfinite(f.min_height) || f.min_height<0.0f ||
           f.min_height>8.0f ||
           !std::isfinite(f.max_aim_degrees) ||
           f.max_aim_degrees<0.0f || f.max_aim_degrees>500.0f ||
           !std::isfinite(f.randomisation) ||
           f.randomisation<0.0f || f.randomisation>1.0f ||
           !std::isfinite(f.clock_seconds) || f.clock_seconds<0.0 ||
           f.clock_seconds>1e12 ||
           f.candidate_block_ids.size()!=
                kClutchDistanceCounts[f.range_index])
            return deny();

        if(world_token_==0) {
            world_token_=f.world_token;
            player_token_=f.player_token;
            generation_=f.generation;
        }
        if(world_token_!=f.world_token || player_token_!=f.player_token ||
           generation_!=f.generation)
            return deny();
        const double cx=(f.box_lo.x+f.box_hi.x)*.5;
        const double cy=f.box_lo.y-.99;
        const double cz=(f.box_lo.z+f.box_hi.z)*.5;
        if(!std::isfinite(cx) || !std::isfinite(cy) ||
           !std::isfinite(cz) || std::fabs(cx)>=1e6 ||
           std::fabs(cy)>=1e6 || std::fabs(cz)>=1e6 ||
           static_cast<int>(std::floor(cx))!=f.base_x ||
           static_cast<int>(std::floor(cy))!=f.base_y ||
           static_cast<int>(std::floor(cz))!=f.base_z)
            return deny();

        ClutchCandidateWorld w{};
        w.box_lo=f.box_lo;w.box_hi=f.box_hi;w.eye=f.eye;
        w.current_yaw=f.current_yaw;w.current_pitch=f.current_pitch;
        w.range_index=f.range_index;w.min_height=f.min_height;
        w.max_aim_degrees=f.max_aim_degrees;
        w.randomisation=f.randomisation;w.clock_seconds=f.clock_seconds;
        w.sideways_only=f.sideways_only;w.multipoint=f.multipoint;

        // Java's scan probes min-height depths 1,2,... until a solid
        // block or the configured depth ends, before reading candidates.
        unsigned sampled=0;
        bool blocked=false;
        if(f.min_height>0.0f) {
            for(unsigned depth=1;depth<=8;++depth) {
                const int id=f.ground_block_ids[depth-1];
                if(id<0 || id>4095)return deny();
                w.ground_by_depth[depth-1]=(id!=0);
                ++sampled;
                if(id!=0) {blocked=true;break;}
                if(!(f.min_height>static_cast<float>(depth)))break;
            }
        }
        for(unsigned depth=sampled;depth<8;++depth)
            if(f.ground_block_ids[depth]!=-1)return deny();
        if(blocked!=f.ground_blocked)return deny();

        if(blocked) {
            // The Java sampler never read candidate IDs or the prior
            // block after it found ground. Reject fabricated evidence.
            if(f.previous_block_id!=-1)return deny();
            for(int id:f.candidate_block_ids)if(id!=0)return deny();
        } else {
            for(std::size_t i=0;i<f.candidate_block_ids.size();++i) {
                const int id=f.candidate_block_ids[i];
                if(id<0 || id>4095)return deny();
                w.source_solid[i]=(id!=0);
            }
            const bool prior_eligible=f.previous &&
                static_cast<double>(f.previous->block.y)<=cy;
            if(prior_eligible) {
                if(f.previous_block_id<0 || f.previous_block_id>4095 ||
                   !clutch_valid_vec(f.previous->last_aim) ||
                   std::fabs(static_cast<double>(f.previous->block.x))>=1e6 ||
                   std::fabs(static_cast<double>(f.previous->block.y))>=1e6 ||
                   std::fabs(static_cast<double>(f.previous->block.z))>=1e6 ||
                   f.previous->ray_known || f.previous->ray_hit)
                    return deny();
                ClutchSourcePrior p=*f.previous;
                p.solid_known=true;
                p.solid=f.previous_block_id!=0;
                p.ray_known=false; // R237 ray phase MUST be separate.
                w.prior=p;
            } else if(f.previous_block_id!=-1) {
                return deny();
            }
        }
        last_tick_=f.tick;
        return w;
    }

    bool closed() const noexcept {return closed_;}
    void close() noexcept {closed_=true;}

private:
    const std::thread::id owner_thread_;
    std::uint64_t generation_=0,last_tick_=0;
    std::uint64_t world_token_=0,player_token_=0;
    bool closed_=false;
};

} // namespace slinkywinky::reference_compat
