#include "slinkywinky/menu_utf8.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error("R326 UTF8: " #x);}while(false)
int main(){
 try{
    std::size_t accepted=0;
    for(std::uint32_t point=0;point<=0x10ffffu;++point) {
        const bool expected=point>=32u &&
            !(point>=127u && point<=159u) &&
            !(point>=0xd800u && point<=0xdfffu);
        const auto encoded=menu_encode_utf8(point);
        CHECK((encoded.size!=0)==expected);
        if(!expected)continue;
        ++accepted;
        const auto decoded=menu_decode_utf8(encoded.view());
        CHECK(decoded.point==point && decoded.bytes==encoded.size);
        CHECK(menu_one_printable_utf8(encoded.view()));
        for(std::size_t n=0;n<encoded.size;++n)
            CHECK(!menu_decode_utf8(encoded.view().substr(0,n)).bytes);
        MenuDisplayScalars stream(encoded.view());
        std::uint32_t value=0;
        CHECK(stream.next(value) && value==point && !stream.next(value));
    }
    CHECK(accepted==1111999u);
    CHECK(!menu_encode_utf8(0x110000u).size);
    CHECK(!menu_encode_utf8(0xffffffffu).size);
    for(unsigned first=0;first<256;++first) {
        const char c=static_cast<char>(first);
        CHECK(menu_one_printable_utf8(std::string_view(&c,1))==(first>=32 && first<=126));
    }
    for(const std::string bad:{std::string("\xc0\xaf",2),std::string("\xe0\x80\x80",3),
        std::string("\xf0\x80\x80\x80",4),std::string("\xed\xa0\x80",3),
        std::string("\xf4\x90\x80\x80",4),std::string("\xc2\x80",2),
        std::string("\xe2\x82",2),std::string("a\0b",3)}) {
        CHECK(!menu_utf8_text_valid(bad));
    }
    const std::string mixed=u8"A\u00e5\u00e4\u00f6 \U0001f600\u4e2d";
    CHECK(menu_utf8_text_valid(mixed));
    MenuDisplayScalars stream(mixed);
    std::vector<std::uint32_t> values;
    std::uint32_t point=0;
    std::string fallback;
    while(stream.next(point)){values.push_back(point);fallback+=menu_ascii_display_scalar(point);}
    CHECK(values.size()==7 && values[1]==0xe5u && values[5]==0x1f600u && values[6]==0x4e2du);
    CHECK(fallback=="A??? ??");
    const std::string longText(128,'x');
    CHECK(menu_utf8_text_valid(longText) && !menu_utf8_text_valid(longText+"x"));
    MenuDisplayScalars limited(longText,999u);
    unsigned count=0;while(limited.next(point))++count;CHECK(count==80u);
    MenuDisplayScalars zero(longText,0u);CHECK(!zero.next(point));
    MenuDisplayScalars malformed(std::string_view("\xc0\xaf" "A",3));
    CHECK(malformed.next(point)&&point=='?');
    CHECK(malformed.next(point)&&point=='?');
    CHECK(malformed.next(point)&&point=='A');
    CHECK(!malformed.next(point));
    std::cout<<"R326_UNICODE_SCALAR_ROUNDTRIP_COUNT="<<accepted<<'\n';
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
