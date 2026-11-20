#pragma once
// R258: strictly typed, read-only observation ABI. This consumes
// ONLY the already-sampled Java8 frame metadata from the admitted
// Minecraft client thread; it must NOT promote geometry into Reach,
// Hitboxes or Ghost Hand behavior without missing authoritative inputs.
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <thread>
#include <array>

namespace slinkywinky {
struct R258ReadOnlyEvent8Frame {
    std::uint64_t sequence=0,world_generation=0,time_ms=0;
    unsigned flags=0;
    bool world_and_player() const noexcept{return (flags&3u)==3u;}
    bool selected_entity() const noexcept{return (flags&4u)!=0u;}
    // The original sampler does not supply attack cooldown, sprinting,
    // player classifications, held items, or source module settings.
    bool complete_gameplay_evidence() const noexcept{return false;}
};

inline std::optional<R258ReadOnlyEvent8Frame> parse_r258_event8_observation(
        std::string_view record) {
    constexpr std::string_view marker="R258_EVENT8_OBS";
    std::array<std::string_view,5> fields{};
    std::size_t pos=0;
    for(std::size_t i=0;i<fields.size();++i){
        if(pos>=record.size())return std::nullopt;
        const auto end=record.find(' ',pos);
        if(end==pos || (i==fields.size()-1 &&
                        end!=std::string_view::npos))
            return std::nullopt;
        fields[i]=record.substr(pos,end==std::string_view::npos?
                                record.size()-pos:end-pos);
        if(i<fields.size()-1 && end==std::string_view::npos)
            return std::nullopt;
        pos=end==std::string_view::npos?record.size():end+1;
    }
    if(fields[0]!=marker)return std::nullopt;
    auto parse=[](std::string_view text,std::uint64_t& value) {
        if(text.empty())return false;
        for(const char digit:text)if(digit<'0'||digit>'9')return false;
        const auto converted=std::from_chars(
            text.data(),text.data()+text.size(),value);
        return converted.ec==std::errc{} &&
               converted.ptr==text.data()+text.size();
    };
    std::uint64_t flags=0;
    R258ReadOnlyEvent8Frame frame;
    if(!parse(fields[1],frame.sequence) ||
       !parse(fields[2],frame.world_generation) ||
       !parse(fields[3],flags) ||
       !parse(fields[4],frame.time_ms) ||
       !frame.sequence || !frame.world_generation || flags>7 ||
       (flags!=0 && flags!=1 && flags!=3 && flags!=7))
        return std::nullopt;
    frame.flags=static_cast<unsigned>(flags);
    return frame;
}

// Test-only value model. The consumer is confined to a single native
// owner thread and cannot mutate Minecraft state or dispatch source
// module callbacks from incomplete observation metadata.
class R258ReadOnlyEvent8Gate final {
public:
    R258ReadOnlyEvent8Gate():owner_(std::this_thread::get_id()){}
    R258ReadOnlyEvent8Gate(const R258ReadOnlyEvent8Gate&)=delete;
    R258ReadOnlyEvent8Gate& operator=(const R258ReadOnlyEvent8Gate&)=delete;
    bool accept(std::string_view text) noexcept {
        if(retired_ || std::this_thread::get_id()!=owner_)
            return false;
        const auto frame=parse_r258_event8_observation(text);
        if(!frame)return false;
        if(last_sequence_ &&
           (frame->sequence!=last_sequence_+1 ||
            frame->world_generation<last_generation_ ||
            frame->time_ms<last_time_ms_ ||
            ((frame->flags&3u)!=(last_flags_&3u) &&
             frame->world_generation<=last_generation_)))
            return false;
        if(!last_sequence_ && frame->sequence!=1)return false;
        last_sequence_=frame->sequence;
        last_generation_=frame->world_generation;
        last_time_ms_=frame->time_ms;
        last_flags_=frame->flags;
        return true;
    }
    bool may_evaluate_reach() const noexcept{return false;}
    bool may_evaluate_hitboxes() const noexcept{return false;}
    bool may_evaluate_ghost_hand() const noexcept{return false;}
    std::uint64_t accepted() const noexcept{return last_sequence_;}
    unsigned latest_flags() const noexcept{return last_flags_;}
    bool retire() noexcept {
        if(retired_ || std::this_thread::get_id()!=owner_)return false;
        retired_=true;
        last_sequence_=last_generation_=last_time_ms_=0;
        last_flags_=0;
        return true;
    }
private:
    const std::thread::id owner_;
    std::uint64_t last_sequence_=0,last_generation_=0,last_time_ms_=0;
    unsigned last_flags_=0;
    bool retired_=false;
};
} // namespace slinkywinky
