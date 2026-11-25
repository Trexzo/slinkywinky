#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace slinkywinky {
struct MenuUtf8Scalar {
    std::uint32_t point=0;
    std::size_t bytes=0;
};
inline bool menu_printable_scalar(std::uint32_t point) noexcept {
    return point>=0x20u && point<=0x10ffffu &&
        !(point>=0x7fu && point<=0x9fu) &&
        !(point>=0xd800u && point<=0xdfffu);
}
// Strict UTF-8 prefix decoder. A zero byte count is an invalid prefix.
// This is scalar validation, not grapheme segmentation or font shaping.
inline MenuUtf8Scalar menu_decode_utf8(std::string_view input) noexcept {
    if(input.empty())return {};
    const auto first=static_cast<unsigned char>(input.front());
    std::uint32_t point=0;
    std::size_t n=0;
    if(first<0x80u){point=first;n=1;}
    else if(first>=0xc2u && first<=0xdfu){point=first&0x1fu;n=2;}
    else if(first>=0xe0u && first<=0xefu){point=first&0x0fu;n=3;}
    else if(first>=0xf0u && first<=0xf4u){point=first&7u;n=4;}
    else return {};
    if(input.size()<n)return {};
    for(std::size_t i=1;i<n;++i) {
        const auto c=static_cast<unsigned char>(input[i]);
        if((c&0xc0u)!=0x80u)return {};
        point=(point<<6u)|(c&0x3fu);
    }
    if((n==2 && point<0x80u)||(n==3 && point<0x800u)||
       (n==4 && point<0x10000u)||!menu_printable_scalar(point))return {};
    return {point,n};
}
inline bool menu_one_printable_utf8(std::string_view input) noexcept {
    const auto s=menu_decode_utf8(input);
    return s.bytes!=0 && s.bytes==input.size();
}
inline bool menu_utf8_text_valid(std::string_view input,std::size_t maximum=128u) noexcept {
    if(input.size()>maximum)return false;
    while(!input.empty()) {
        const auto s=menu_decode_utf8(input);
        if(!s.bytes)return false;
        input.remove_prefix(s.bytes);
    }
    return true;
}
struct MenuUtf8Encoded {
    std::array<char,4> bytes{};
    std::size_t size=0;
    std::string_view view() const noexcept {return {bytes.data(),size};}
};
inline MenuUtf8Encoded menu_encode_utf8(std::uint32_t point) noexcept {
    MenuUtf8Encoded r;
    if(!menu_printable_scalar(point))return r;
    if(point<0x80u){r.size=1;r.bytes[0]=static_cast<char>(point);return r;}
    r.size=point<0x800u?2u:point<0x10000u?3u:4u;
    for(std::size_t i=r.size-1;i>0;--i) {
        r.bytes[i]=static_cast<char>(0x80u|(point&0x3fu));
        point>>=6u;
    }
    r.bytes[0]=static_cast<char>((r.size==2?0xc0u:r.size==3?0xe0u:0xf0u)|point);
    return r;
}
// Bounded allocation-free display stream shared by measurement and draw.
// Invalid bytes consume one byte; a valid unsupported scalar consumes its
// entire encoding and becomes one replacement glyph at the font boundary.
class MenuDisplayScalars {
    std::string_view remaining_;
    std::size_t budget_;
public:
    explicit MenuDisplayScalars(std::string_view text,std::size_t max=80u) noexcept
        :remaining_(text),budget_(max<80u?max:80u){}
    bool next(std::uint32_t& point) noexcept {
        if(!budget_ || remaining_.empty())return false;
        --budget_;
        const auto s=menu_decode_utf8(remaining_);
        point=s.bytes?s.point:static_cast<std::uint32_t>('?');
        remaining_.remove_prefix(s.bytes?s.bytes:1u);
        return true;
    }
};
inline char menu_ascii_display_scalar(std::uint32_t point) noexcept {
    return point>=32u && point<=126u?static_cast<char>(point):'?';
}
} // namespace slinkywinky
