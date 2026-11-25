#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_system_font.hpp"
#include "slinkywinky/menu_glyph_metrics.hpp"
#include "slinkywinky/menu_utf8.hpp"
#include <utility>
#include <windows.h>
#include <GL/gl.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace slinkywinky {
namespace {
// R149: the recovered HTML requests Inter at font-weight 500.
// Prefer a *locally installed* Inter face only if GDI confirms that the
// exact family exists; otherwise retain the Windows Segoe UI fallback.
// Neither font binaries nor cross-process font registration are used.
constexpr float kRasterHeight=24.0f;
int CALLBACK find_exact_inter_family(
        const LOGFONTW* record,const TEXTMETRICW*,DWORD,
        LPARAM result) noexcept {
    if(!record || !result)return 1;
    if(lstrcmpiW(record->lfFaceName,L"Inter")==0) {
        *reinterpret_cast<bool*>(result)=true;
        return 0;
    }
    return 1;
}
bool inter_installed(HDC dc) noexcept {
    if(!dc)return false;
    LOGFONTW query{};
    const wchar_t* name=L"Inter";
    for(std::size_t i=0;name[i] && i<LF_FACESIZE-1u;++i)
        query.lfFaceName[i]=name[i];
    query.lfCharSet=DEFAULT_CHARSET;
    bool found=false;
    EnumFontFamiliesExW(
        dc,&query,find_exact_inter_family,
        reinterpret_cast<LPARAM>(&found),0);
    return found;
}
struct Glyph {
    int left=0, origin_y=0, width=0, height=0, pitch=0, advance=12;
    std::vector<unsigned char> coverage;
};
class SystemGlyphCache {
public:
    SystemGlyphCache() noexcept {
        try {
            dc_=CreateCompatibleDC(nullptr);
            if(!dc_)return;
            const wchar_t* family=inter_installed(dc_)?
                L"Inter":L"Segoe UI";
            font_=CreateFontW(-24,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,
                DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_SWISS,family);
            if(!font_)return;
            old_=SelectObject(dc_,font_);
            if(!old_ || old_==HGDI_ERROR)return;
            TEXTMETRICW tm{};
            if(!GetTextMetricsW(dc_,&tm))return;
            ascent_=tm.tmAscent;
            for(wchar_t c=32;c<=126;++c)
                if(!load_glyph(c,glyphs_[static_cast<std::size_t>(c-32)]))return;
            // Optional Latin-1 glyphs must not invalidate the mandatory
            // ASCII cache. The local font may omit individual characters.
            for(wchar_t c=160;c<=255;++c) {
                WORD index=0xffffu;
                if(GetGlyphIndicesW(dc_,&c,1,&index,GGI_MARK_NONEXISTING_GLYPHS)==GDI_ERROR ||
                   index==0xffffu)continue;
                try {
                    if(load_glyph(c,glyphs_[static_cast<std::size_t>(c-32)]))
                        latin1_[static_cast<std::size_t>(c-160)]=true;
                }catch(...) {}
            }
            ready_=true;
        } catch(...) {ready_=false;}
    }
    ~SystemGlyphCache() noexcept {
        if(dc_ && old_ && old_!=HGDI_ERROR)SelectObject(dc_,old_);
        if(font_)DeleteObject(font_);
        if(dc_)DeleteDC(dc_);
    }
    bool ready() const noexcept {return ready_;}
    int ascent() const noexcept {return ascent_;}
    const Glyph& glyph(std::uint32_t c) const noexcept {
        if((c>=32 && c<=126)||(c>=160 && c<=255 && latin1_[c-160]))
            return glyphs_[c-32];
        return glyphs_[static_cast<unsigned>('?')-32];
    }
private:
    bool load_glyph(wchar_t c,Glyph& out) {
        const MAT2 transform={ {0,1},{0,0},{0,0},{0,1} };
        GLYPHMETRICS metrics{};
        const DWORD count=GetGlyphOutlineW(dc_,c,GGO_GRAY8_BITMAP,
            &metrics,0,nullptr,&transform);
        if(count==GDI_ERROR)return false;
        Glyph glyph;
        glyph.left=metrics.gmptGlyphOrigin.x;
        glyph.origin_y=metrics.gmptGlyphOrigin.y;
        const auto layout=menu_glyph_raster_layout(
            metrics.gmBlackBoxX,metrics.gmBlackBoxY,metrics.gmCellIncX,count);
        if(!layout)return false;
        glyph.width=static_cast<int>(layout->width);
        glyph.height=static_cast<int>(layout->height);
        glyph.pitch=static_cast<int>(layout->pitch);
        glyph.advance=static_cast<int>(layout->advance);
        // Whitespace can have no raster bytes even when GDI
        // reports a nonzero black box. It still advances the pen.
        if(!count){out=std::move(glyph);return true;}
        glyph.coverage.resize(count);
        const DWORD copied=GetGlyphOutlineW(dc_,c,GGO_GRAY8_BITMAP,
            &metrics,count,glyph.coverage.data(),&transform);
        if(copied==GDI_ERROR || copied>count ||
           metrics.gmBlackBoxX!=layout->width ||
           metrics.gmBlackBoxY!=layout->height ||
           metrics.gmCellIncX!=glyph.advance ||
           copied<static_cast<DWORD>(glyph.pitch*glyph.height))return false;
        glyph.coverage.resize(copied);
        out=std::move(glyph);
        return true;
    }
    HDC dc_=nullptr;
    HFONT font_=nullptr;
    HGDIOBJ old_=nullptr;
    int ascent_=20;
    bool ready_=false;
    std::array<Glyph,224> glyphs_{};
    std::array<bool,96> latin1_{};
};
const SystemGlyphCache& font_cache() noexcept {
    // Constructed on the WGL render thread. GDI objects stay on that thread
    // until its normal teardown; no game or disk font resources are retained.
    thread_local const SystemGlyphCache cache;
    return cache;
}
}

float measure_menu_system_text(std::string_view label,float font_px) noexcept {
    const auto& cache=font_cache();
    if(!cache.ready() || !std::isfinite(font_px) || font_px<=0 || font_px>48)
        return 0.0f;
    const float scale=font_px/kRasterHeight;
    float advance=0;
    MenuDisplayScalars scalars(label);
    std::uint32_t point=0;
    while(scalars.next(point))
        advance+=static_cast<float>(cache.glyph(point).advance)*scale;
    return advance;
}

bool draw_menu_system_text(std::string_view label,float x,float top,
                           float font_px,float max_width) noexcept {
    const auto& cache=font_cache();
    if(!cache.ready() || !std::isfinite(font_px) || font_px<8 || font_px>48 ||
       !std::isfinite(x) || !std::isfinite(top) ||
       !std::isfinite(max_width) || max_width<0 ||
       !std::isfinite(x+max_width) ||
       !std::isfinite(top+font_px))return false;
    GLfloat base[4]{};
    glGetFloatv(GL_CURRENT_COLOR,base);
    const float scale=font_px/kRasterHeight;
    const float baseline=top+static_cast<float>(cache.ascent())*scale;
    float pen=0;
    glBegin(GL_QUADS);
    MenuDisplayScalars scalars(label);
    std::uint32_t point=0;
    while(scalars.next(point)) {
        const Glyph& g=cache.glyph(point);
        const float next=static_cast<float>(g.advance)*scale;
        if(pen+next>max_width)break;
        for(int row=0;row<g.height;++row) {
            for(int col=0;col<g.width;++col) {
                const auto offset=static_cast<std::size_t>(row*g.pitch+col);
                if(offset>=g.coverage.size())continue;
                const unsigned alpha=g.coverage[offset];
                if(alpha==0)continue;
                glColor4f(base[0],base[1],base[2],
                    base[3]*static_cast<float>(std::min(alpha,64u))/64.0f);
                const float left=x+pen+(g.left+col)*scale;
                const float y=baseline-(g.origin_y-row)*scale;
                const float right=left+scale,bottom=y+scale;
                glVertex2f(left,y);glVertex2f(right,y);
                glVertex2f(right,bottom);glVertex2f(left,bottom);
            }
        }
        pen+=next;
    }
    glEnd();
    glColor4fv(base);
    return true;
}
} // namespace slinkywinky
