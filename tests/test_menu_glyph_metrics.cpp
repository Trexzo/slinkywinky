#include "slinkywinky/menu_glyph_metrics.hpp"
#include <stdexcept>
#include <limits>
#include <iostream>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error("glyph layout: " #x);}while(false)
int main() {
    try {
        for(unsigned w=0;w<=80;++w)for(unsigned h=0;h<=80;++h) {
            const auto blank=menu_glyph_raster_layout(w,h,12,0);
            CHECK(blank && !blank->width && !blank->height && !blank->pitch && blank->advance==12);
            if(w && h) {
                const auto pitch=(w+3u)&~3u;
                const auto bytes=pitch*h;
                const auto exact=menu_glyph_raster_layout(w,h,12,bytes);
                CHECK(exact && exact->width==w && exact->height==h && exact->pitch==pitch);
                CHECK(!menu_glyph_raster_layout(w,h,12,bytes-1u));
                CHECK(menu_glyph_raster_layout(w,h,12,bytes+1u));
            } else {
                CHECK(!menu_glyph_raster_layout(w,h,12,1u));
            }
        }
        CHECK(!menu_glyph_raster_layout(81,1,12,84));
        CHECK(!menu_glyph_raster_layout(1,81,12,324));
        CHECK(!menu_glyph_raster_layout(1,1,0,4));
        CHECK(!menu_glyph_raster_layout(1,1,-1,4));
        CHECK(!menu_glyph_raster_layout(1,1,81,4));
        CHECK(!menu_glyph_raster_layout(1,1,12,16385));
        CHECK(!menu_glyph_raster_layout(std::numeric_limits<std::uint32_t>::max(),1,12,0));
        CHECK(!menu_glyph_raster_layout(1,std::numeric_limits<std::uint32_t>::max(),12,0));
        std::cout<<"R325_GLYPH_LAYOUT_BOUNDS_PASS\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
