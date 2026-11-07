#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_system_font.hpp"
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
            const MAT2 transform={ {0,1},{0,0},{0,0},{0,1} };
            for(wchar_t c=32;c<=126;++c) {
                GLYPHMETRICS metrics{};
                const DWORD count=GetGlyphOutlineW(dc_,c,GGO_GRAY8_BITMAP,
                    &metrics,0,nullptr,&transform);
                if(count==GDI_ERROR)return;
                Glyph& glyph=glyphs_[static_cast<std::size_t>(c-32)];
                glyph.left=metrics.gmptGlyphOrigin.x;
                glyph.origin_y=metrics.gmptGlyphOrigin.y;
                glyph.width=static_cast<int>(metrics.gmBlackBoxX);
                glyph.height=static_cast<int>(metrics.gmBlackBoxY);
                glyph.pitch=(glyph.width+3)&~3;
                glyph.advance=metrics.gmCellIncX;
                if(glyph.advance<=0 || glyph.advance>80 ||
                   glyph.width>80 || glyph.height>80 || count>16384)
                    return;
                glyph.coverage.resize(count);
                if(count && GetGlyphOutlineW(dc_,c,GGO_GRAY8_BITMAP,
                    &metrics,count,glyph.coverage.data(),&transform)==GDI_ERROR)
                    return;
                if(count && glyph.pitch*glyph.height>static_cast<int>(count))
                    return;
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
    const Glyph& glyph(unsigned char c) const noexcept {
        return glyphs_[(c>=32 && c<=126)?c-32:static_cast<unsigned char>('?')-32];
    }
private:
    HDC dc_=nullptr;
    HFONT font_=nullptr;
    HGDIOBJ old_=nullptr;
    int ascent_=20;
    bool ready_=false;
    std::array<Glyph,95> glyphs_{};
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
    for(std::size_t i=0;i<label.size() && i<80;++i)
        advance+=static_cast<float>(cache.glyph(
            static_cast<unsigned char>(label[i])).advance)*scale;
    return advance;
}

bool draw_menu_system_text(std::string_view label,float x,float top,
                           float font_px,float max_width) noexcept {
    const auto& cache=font_cache();
    if(!cache.ready() || !std::isfinite(font_px) || font_px<8 || font_px>48 ||
       !std::isfinite(max_width) || max_width<0) return false;
    GLfloat base[4]{};
    glGetFloatv(GL_CURRENT_COLOR,base);
    const float scale=font_px/kRasterHeight;
    const float baseline=top+static_cast<float>(cache.ascent())*scale;
    float pen=0;
    glBegin(GL_QUADS);
    for(std::size_t i=0;i<label.size() && i<80;++i) {
        const Glyph& g=cache.glyph(static_cast<unsigned char>(label[i]));
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
