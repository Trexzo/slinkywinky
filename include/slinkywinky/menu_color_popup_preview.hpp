#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace slinkywinky {

// Isolated, first-party color-picker state. NOT a Windows color dialog,
// copied renderer, Minecraft input hook, or gameplay setting. R198 draws
// behavioral constraints from both the C++ and Java recovered references.
struct MenuPopupHsl {
    float hue=0.0f, saturation=0.0f, lightness=0.0f;
};

class MenuColorPopupPreview final {
public:
    bool open(std::uint32_t rgb) noexcept {
        if(open_ || rgb>0xFFFFFFu)return false;
        open_=true;
        original_=current_=rgb;
        hsl_=rgb_to_hsl(rgb);
        hex_editing_=false;
        hex_buffer_.clear();
        return true;
    }
    bool active() const noexcept {return open_;}
    std::optional<std::uint32_t> current() const noexcept {
        if(!open_)return std::nullopt;
        return current_;
    }
    std::optional<std::uint32_t> original() const noexcept {
        if(!open_)return std::nullopt;
        return original_;
    }
    static std::optional<std::uint32_t> parse_hex6(
            std::string_view text) noexcept {
        if(text.size()!=6u)return std::nullopt;
        std::uint32_t result=0u;
        for(const char c:text) {
            int digit=-1;
            if(c>='0' && c<='9')digit=c-'0';
            else if(c>='a' && c<='f')digit=c-'a'+10;
            else if(c>='A' && c<='F')digit=c-'A'+10;
            if(digit<0)return std::nullopt;
            result=(result<<4u)|static_cast<std::uint32_t>(digit);
        }
        return result;
    }
    std::optional<std::string> hex6() const {
        if(!open_)return std::nullopt;
        static constexpr char digits[]="0123456789abcdef";
        std::string result(6u,'0');
        for(int i=5;i>=0;--i)
            result[static_cast<std::size_t>(5-i)]=
                digits[(current_>>(4u*static_cast<unsigned>(i)))&0xFu];
        return result;
    }
    // R201: the displayed hex field supports bounded, first-party editing.
    // Partial input never changes RGB. A sixth valid digit atomically
    // refreshes RGB/HSL; Enter or field blur ends editing.
    bool hex_editing() const noexcept {return open_ && hex_editing_;}
    std::string_view hex_buffer() const noexcept {return hex_buffer_;}
    bool begin_hex_edit() {
        if(!open_ || hex_editing_)return false;
        hex_buffer_.clear();
        hex_editing_=true;
        return true;
    }
    bool append_hex_digit(char c) noexcept {
        if(!hex_editing_ || hex_buffer_.size()>=6u ||
           !((c>='0' && c<='9') ||
             (c>='a' && c<='f') || (c>='A' && c<='F')))
            return false;
        hex_buffer_.push_back(c>='A' && c<='F'?
            static_cast<char>(c-'A'+'a'):c);
        if(hex_buffer_.size()==6u)return set_hex6(hex_buffer_);
        return true;
    }
    bool erase_hex_digit() noexcept {
        if(!hex_editing_ || hex_buffer_.empty())return false;
        hex_buffer_.pop_back();
        return true;
    }
    bool end_hex_edit() noexcept {
        if(!hex_editing_)return false;
        hex_editing_=false;
        hex_buffer_.clear();
        return true;
    }
    bool set_hex6(std::string_view text) noexcept {
        if(!open_)return false;
        const auto parsed=parse_hex6(text);
        if(!parsed)return false;
        current_=*parsed;
        hsl_=rgb_to_hsl(current_);
        return true;
    }
    static MenuPopupHsl rgb_to_hsl(std::uint32_t rgb) noexcept {
        const float r=static_cast<float>((rgb>>16u)&255u)/255.0f;
        const float g=static_cast<float>((rgb>>8u)&255u)/255.0f;
        const float b=static_cast<float>(rgb&255u)/255.0f;
        const float hi=std::max({r,g,b});
        const float lo=std::min({r,g,b});
        const float l=(hi+lo)*0.5f;
        if(hi==lo)return {0.0f,0.0f,l};
        const float delta=hi-lo;
        const float s=delta/(l>0.5f?(2.0f-hi-lo):(hi+lo));
        float h=0.0f;
        if(hi==r)h=(g-b)/delta+(g<b?6.0f:0.0f);
        else if(hi==g)h=(b-r)/delta+2.0f;
        else h=(r-g)/delta+4.0f;
        return {h/6.0f,s,l};
    }
    std::optional<MenuPopupHsl> hsl() const noexcept {
        if(!open_)return std::nullopt;
        return hsl_;
    }
    static std::optional<std::uint32_t> hsl_to_rgb(
            MenuPopupHsl hsl) noexcept {
        if(!std::isfinite(hsl.hue) ||
           !std::isfinite(hsl.saturation) ||
           !std::isfinite(hsl.lightness) ||
           hsl.hue<0.0f || hsl.hue>1.0f ||
           hsl.saturation<0.0f || hsl.saturation>1.0f ||
           hsl.lightness<0.0f || hsl.lightness>1.0f)return std::nullopt;
        auto channel=[](float p,float q,float t) noexcept {
            if(t<0.0f)t+=1.0f;
            if(t>1.0f)t-=1.0f;
            if(t<1.0f/6.0f)return p+(q-p)*6.0f*t;
            if(t<0.5f)return q;
            if(t<2.0f/3.0f)return p+(q-p)*(2.0f/3.0f-t)*6.0f;
            return p;
        };
        float r=hsl.lightness,g=hsl.lightness,b=hsl.lightness;
        if(hsl.saturation>0.0f) {
            const float q=hsl.lightness<0.5f?
                hsl.lightness*(1.0f+hsl.saturation):
                (hsl.lightness+hsl.saturation-
                 hsl.lightness*hsl.saturation);
            const float p=2.0f*hsl.lightness-q;
            r=channel(p,q,hsl.hue+1.0f/3.0f);
            g=channel(p,q,hsl.hue);
            b=channel(p,q,hsl.hue-1.0f/3.0f);
        }
        auto byte=[](float v) noexcept {
            return static_cast<std::uint32_t>(
                std::lround(std::clamp(v,0.0f,1.0f)*255.0f));
        };
        return (byte(r)<<16u)|(byte(g)<<8u)|byte(b);
    }
    bool set_hsl(MenuPopupHsl desired) noexcept {
        if(!open_)return false;
        const auto rgb=hsl_to_rgb(desired);
        if(!rgb)return false;
        current_=*rgb;
        // R199: preserve full-precision HSL slider state independently
        // from the 24-bit rounded swatch, as in both references.
        hsl_=desired;
        if(hex_editing_)end_hex_edit();
        return true;
    }
    std::optional<std::uint32_t> recent(unsigned newest_first) const noexcept {
        if(newest_first>=history_.size())return std::nullopt;
        const std::size_t index=(history_head_+newest_first)%history_.size();
        return history_[index];
    }
    bool select_recent(unsigned newest_first) noexcept {
        if(!open_)return false;
        const auto rgb=recent(newest_first);
        if(!rgb)return false;
        current_=*rgb;
        hsl_=rgb_to_hsl(current_);
        if(hex_editing_)end_hex_edit();
        return true;
    }
    // The reference native popup records a unique recent color on close
    // independently of the apply/callback flag. NO callback runs here.
    std::optional<std::uint32_t> close(bool apply) noexcept {
        if(!open_)return std::nullopt;
        const std::uint32_t result=current_;
        open_=false;
        hex_editing_=false;
        hex_buffer_.clear();
        bool seen=false;
        for(const auto& prior:history_)
            if(prior==result){seen=true;break;}
        if(!seen) {
            history_head_=(history_head_+history_.size()-1u)%history_.size();
            history_[history_head_]=result;
        }
        return apply?std::optional<std::uint32_t>(result):std::nullopt;
    }
private:
    bool open_=false;
    bool hex_editing_=false;
    std::string hex_buffer_{};
    std::uint32_t original_=0u,current_=0u;
    // Native and Java picker state keep these independent HSL floats.
    MenuPopupHsl hsl_{};
    // R199: source .data reserves 96 zero bytes: eight RGB float triples.
    // Java also allocates new float[8][3]. All history cells begin black;
    // a closing black color is therefore an existing duplicate.
    std::array<std::uint32_t,8> history_{};
    std::size_t history_head_=0u;
};
} // namespace slinkywinky
