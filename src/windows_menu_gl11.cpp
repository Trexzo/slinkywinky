#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_bitmap_font.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_visual_copy.hpp"
#include "slinkywinky/windows_menu_system_font.hpp"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace slinkywinky {
namespace {
bool stack_available(GLenum depth, GLenum capacity,
                     GLint minimum_depth) noexcept {
    GLint current=0, max=0;
    glGetIntegerv(depth,&current);
    glGetIntegerv(capacity,&max);
    return current>=minimum_depth && max>current;
}

// First-party opaque geometry and tiny ASCII bitmaps only. No textures,
// shaders, VBO, font files, framebuffer binding or game input.
void quad(float x,float y,float w,float h) noexcept {
    glBegin(GL_QUADS);
    glVertex2f(x,y);
    glVertex2f(x+w,y);
    glVertex2f(x+w,y+h);
    glVertex2f(x,y+h);
    glEnd();
}
void rgba(std::uint32_t c,GLubyte alpha=255) noexcept {
    glColor4ub(static_cast<GLubyte>((c>>16)&255),
               static_cast<GLubyte>((c>>8)&255),
               static_cast<GLubyte>(c&255),alpha);
}
void rgb(std::uint32_t c) noexcept {
    rgba(c);
}

// R113: rounded, antialiased-free GL1.1 geometry. The radius is clamped
// to prevent invalid vertices at narrow panels or tiny controls.
void round_rect(float x,float y,float w,float h,float radius) noexcept {
    if(w<=0 || h<=0)return;
    const float r=std::max(0.0f,std::min(radius,std::min(w,h)*0.5f));
    if(r<=0.01f){quad(x,y,w,h);return;}
    constexpr float pi=3.14159265358979323846f;
    constexpr int steps=6;
    const float cx[4]={x+w-r,x+w-r,x+r,x+r};
    const float cy[4]={y+r,y+h-r,y+h-r,y+r};
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x+w*0.5f,y+h*0.5f);
    for(int corner=0;corner<4;++corner) {
        const float start=static_cast<float>(corner-1)*0.5f*pi;
        for(int step=0;step<=steps;++step) {
            const float a=start+static_cast<float>(step)*(0.5f*pi/steps);
            glVertex2f(cx[corner]+r*std::cos(a),
                       cy[corner]+r*std::sin(a));
        }
    }
    // Close the fan along its flat top. Without this final point the wedge
    // between the top corners was omitted, producing a diagonal cutout.
    glVertex2f(x+w-r,y);
    glEnd();
}
// Legacy glyphs remain a deterministic fallback if a Windows font isn't
// available; the default is grayscale Segoe UI supplied by the local OS.
void text(std::string_view label,float x,float y,
          float fontPx,float maxWidth) noexcept {
    if(draw_menu_system_text(label,x,y,fontPx,maxWidth))return;
    const float pixelScale=fontPx/9.0f;
    if(pixelScale<=0.0f || maxWidth<5.0f*pixelScale)return;
    const float advance=6.0f*pixelScale;
    const float glyphWidth=5.0f*pixelScale;
    const auto limit=std::min<std::size_t>(label.size(),48);
    glBegin(GL_QUADS);
    for(std::size_t i=0;i<limit;++i) {
        const float offset=static_cast<float>(i)*advance;
        if(offset+glyphWidth>maxWidth)break;
        const auto rows=menu_ascii_glyph(label[i]);
        for(unsigned row=0;row<7;++row) {
            for(unsigned col=0;col<5;++col) {
                if((rows[row]&(1u<<(4-col)))==0u)continue;
                const float left=x+offset+static_cast<float>(col)*pixelScale;
                const float top=y+static_cast<float>(row)*pixelScale;
                glVertex2f(left,top);
                glVertex2f(left+pixelScale,top);
                glVertex2f(left+pixelScale,top+pixelScale);
                glVertex2f(left,top+pixelScale);
            }
        }
    }
    glEnd();
}

/**
 * GL1.1 server/client attributes and the three fixed-function matrices.
 * No OpenGL call in the constructor may be made without the current WGL
 * context being checked by the outer function.
 */
class ScopedFixedFunctionState {
public:
    ScopedFixedFunctionState() noexcept {
        GLint current=0;
        glGetIntegerv(GL_MATRIX_MODE,&current);
        original_mode_=static_cast<GLenum>(current);
        if(original_mode_!=GL_MODELVIEW &&
           original_mode_!=GL_PROJECTION &&
           original_mode_!=GL_TEXTURE)
            return;
        // GL attribute stacks begin at depth 0; matrix stacks begin at 1.
        // Confusing these rejected a valid fresh WGL context.
        if(!stack_available(GL_ATTRIB_STACK_DEPTH,GL_MAX_ATTRIB_STACK_DEPTH,0) ||
           !stack_available(GL_CLIENT_ATTRIB_STACK_DEPTH,
                            GL_MAX_CLIENT_ATTRIB_STACK_DEPTH,0) ||
           !stack_available(GL_MODELVIEW_STACK_DEPTH,GL_MAX_MODELVIEW_STACK_DEPTH,1) ||
           !stack_available(GL_PROJECTION_STACK_DEPTH,GL_MAX_PROJECTION_STACK_DEPTH,1) ||
           !stack_available(GL_TEXTURE_STACK_DEPTH,GL_MAX_TEXTURE_STACK_DEPTH,1))
            return;
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        server_=true;
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        client_=true;
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glMatrixMode(original_mode_);
        ready_=true;
    }
    bool ready() const noexcept {return ready_;}
    ~ScopedFixedFunctionState() noexcept {
        if(!server_)return;
        if(ready_) {
            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(original_mode_);
        }
        if(client_)glPopClientAttrib();
        glPopAttrib();
    }
    ScopedFixedFunctionState(const ScopedFixedFunctionState&)=delete;
    ScopedFixedFunctionState& operator=(const ScopedFixedFunctionState&)=delete;
private:
    GLenum original_mode_=GL_MODELVIEW;
    bool server_=false;
    bool client_=false;
    bool ready_=false;
};
}

bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode) noexcept {
    return draw_menu_gl11(frame,expected_context,expected_dc,mode,std::nullopt);
}
bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode,
                    std::optional<std::size_t> focused_catalog_index) noexcept {
    // Never use a context inferred only from a PID. Require exact equality.
    if((mode!=MenuGL11Mode::StandalonePreview &&
        mode!=MenuGL11Mode::BoundedOverlay &&
        mode!=MenuGL11Mode::CompactOverlay) || !expected_context || !expected_dc ||
       wglGetCurrentContext()!=expected_context ||
       wglGetCurrentDC()!=expected_dc ||
       frame.epoch==0 || frame.sequence==0 ||
       frame.viewport_width<320 || frame.viewport_width>8192 ||
       frame.viewport_height<240 || frame.viewport_height>8192 ||
       frame.visible.size()>8 ||
       frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload ||
       (frame.is_placeholder != (frame.tab==MenuTab::Profiles)))
        return false;
    for(const auto& item:frame.visible)
        if(item.catalog_index>=official_module_catalog().size() ||
           official_module_catalog()[item.catalog_index].tab!=frame.tab ||
           item.title!=official_module_catalog()[item.catalog_index].name)
            return false;
    if(frame.is_placeholder && !frame.visible.empty())return false;
    if(focused_catalog_index) {
        bool inVisiblePage=false;
        for(const auto& row:frame.visible)
            if(row.catalog_index==*focused_catalog_index)
                inVisiblePage=true;
        if(!inVisiblePage)return false; // No stale or forged focus index.
    }
    if(frame.parameters.size()>frame.visible.size())return false;
    for(std::size_t i=0;i<frame.parameters.size();++i) {
        const auto& parameter=frame.parameters[i];
        if(parameter.caption.empty() || parameter.caption.size()>48 ||
           !module_has_profile_setting(parameter.catalog_index))return false;
        for(const unsigned char c:parameter.caption)
            if(c<32 || c>126)return false; // ASCII first-party bitmap only.
        bool matched=false;
        for(const auto& item:frame.visible)
            if(item.catalog_index==parameter.catalog_index && item.expanded)
                matched=true;
        if(!matched)return false;
        for(std::size_t j=0;j<i;++j)
            if(frame.parameters[j].catalog_index==parameter.catalog_index)
                return false;
    }
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(frame.viewport_width) ||
       viewport[3]!=static_cast<GLint>(frame.viewport_height))
        return false;
    const auto geometryMode=mode==MenuGL11Mode::StandalonePreview?
        MenuPanelMode::StandalonePreview:
        (mode==MenuGL11Mode::BoundedOverlay?
         MenuPanelMode::BoundedOverlay:MenuPanelMode::CompactOverlay);
    const auto panel=menu_panel_geometry(frame.viewport_width,frame.viewport_height,
        frame.appearance.wide,geometryMode);
    if(!panel)return false;
    ScopedFixedFunctionState state;
    if(!state.ready())return false;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);

    const bool overlay=mode!=MenuGL11Mode::StandalonePreview;
    // R113: font antialiasing needs blending even in standalone previews.
    // Every modified GL flag, matrix and current color is independently
    // restored by ScopedFixedFunctionState.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const unsigned paneWidth=panel->width;
    const unsigned paneHeight=panel->height;
    const unsigned paneLeft=panel->left;
    const unsigned paneTop=panel->top;
    if(overlay) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(paneLeft),
                  static_cast<GLint>(frame.viewport_height-paneTop-paneHeight),
                  static_cast<GLsizei>(paneWidth),
                  static_cast<GLsizei>(paneHeight));
    } else glDisable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,static_cast<GLdouble>(frame.viewport_width),
            static_cast<GLdouble>(frame.viewport_height),0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(static_cast<GLfloat>(paneLeft),
                 static_cast<GLfloat>(paneTop),0.0f);
    const float scale=panel->scale;
    if(scale!=1.0f)glScalef(scale,scale,1.0f);
    const float w=static_cast<float>(paneWidth)/scale;
    const float h=static_cast<float>(paneHeight)/scale;
    rgba(0x222223,overlay?static_cast<GLubyte>(243):static_cast<GLubyte>(255));
    quad(0,0,w,h);
    // Real Slinky uses a slim rounded navigation bar, rather than a
    // large centered logo/title and an orange underline.
    rgba(0x121213,255);
    round_rect(menu_nav_x,menu_nav_y,w-2*menu_nav_x,
               menu_nav_height,20.0f);
    const float cell=menu_tab_width(w);
    const auto selected=static_cast<unsigned>(frame.tab);
    if(selected<6u) {
        rgb(0x424244);
        round_rect(menu_nav_x+static_cast<float>(selected)*cell+2.0f,
                   menu_nav_y+3.0f,cell-4.0f,menu_nav_height-6.0f,16.0f);
    }
    for(unsigned i=0;i<7;++i) {
        const auto tab=static_cast<MenuTab>(i);
        std::string_view label=menu_tab_name(tab);
        const float fontPx=std::min(16.0f,std::max(9.0f,cell/4.0f));
        const float textWidth=measure_menu_system_text(label,fontPx);
        const float fallbackWidth=label.size()*fontPx*0.62f;
        const float measured=textWidth>0?textWidth:fallbackWidth;
        const float left=menu_nav_x+static_cast<float>(i)*cell+
                         std::max(2.0f,(cell-measured)*0.5f);
        rgb(i==6?0xee5252:(i==selected?0xffffff:0xd0d0d2));
        text(label,left,menu_nav_y+11.0f,fontPx,cell-4.0f);
    }

    if(frame.is_placeholder) {
        rgb(0x111112);
        round_rect(16.0f,menu_first_row_y,w-32.0f,96.0f,14.0f);
        rgb(0xf2f2f4);
        text("Profiles",30.0f,menu_first_row_y+11.0f,20.0f,w-65.0f);
        rgb(0x99999b);
        text("Profile actions are not connected in this preview.",
             30.0f,menu_first_row_y+48.0f,15.0f,w-65.0f);
    }

    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& item=frame.visible[i];
        const float y=menu_row_top(frame,i);
        const float body=menu_row_body_height(frame);
        const float total=menu_row_total_height(frame,i);
        // Never render a partly clipped module as actionable; the matching
        // hit tester makes the identical full-body/expanded admission.
        if(y+total>h-12.0f)continue;
        const float left=16.0f;
        const float right=w-16.0f;
        rgb(0x111112);
        round_rect(left,y,right-left,total,13.0f);
        if(focused_catalog_index && item.catalog_index==*focused_catalog_index) {
            rgb(0x545456);
            round_rect(left,y,total>body?4.0f:3.0f,total,1.5f);
        }

        const bool supported=module_has_profile_setting(item.catalog_index);
        const float titleFont=frame.appearance.compact?18.0f:20.0f;
        const float titleTop=y+(frame.appearance.compact?8.0f:5.0f);
        rgb(supported?0xf3f3f4:0xc7c7ca);
        text(item.title,30.0f,titleTop,titleFont,
             std::max(10.0f,w-188.0f));
        // Authentic Slinky puts a small rounded keybind badge directly
        // beside the module name. Here NONE is an inert preview badge.
        const float natural=measure_menu_system_text(item.title,titleFont);
        const float titleWidth=natural>0?natural:
                                static_cast<float>(item.title.size())*11.0f;
        const float bindX=std::max(32.0f,
            std::min(30.0f+titleWidth+10.0f,w-152.0f));
        if(bindX+51.0f < w-86.0f) {
            rgb(0x29292b);
            round_rect(bindX,y+9.0f,49.0f,22.0f,7.0f);
            rgb(0xb0b0b2);
            text("NONE",bindX+7.0f,y+12.0f,13.0f,40.0f);
        }

        const float toggleX=w-80.0f;
        const float toggleY=y+(frame.appearance.compact?8.0f:8.0f);
        const bool previewActive=supported && item.preview_enabled;
        rgb(previewActive?frame.appearance.accent_rgb:0x48484a);
        round_rect(toggleX,toggleY,54.0f,27.0f,13.0f);
        rgb(0x171719);
        round_rect(toggleX+(previewActive?30.0f:4.0f),
                   toggleY+3.5f,20.0f,20.0f,10.0f);

        // The compact layout hides descriptions; the normal layout places
        // expand beside the description exactly like the source screenshots.
        const float expandX=frame.appearance.compact?w-119.0f:28.0f;
        const float expandY=frame.appearance.compact?y+10.0f:y+33.0f;
        rgb(supported?0x414143:0x303032);
        round_rect(expandX,expandY,21.0f,21.0f,5.0f);
        rgb(supported?0xdadadc:0x777779);
        quad(expandX+5.0f,expandY+10.0f,11.0f,2.0f);
        if(!item.expanded)quad(expandX+10.0f,expandY+5.0f,2.0f,11.0f);
        if(!frame.appearance.compact) {
            rgb(0x98989b);
            text(menu_preview_description(item.catalog_index),
                 59.0f,y+35.0f,14.0f,std::max(0.0f,w-155.0f));
        }
        if(item.expanded) {
            const float settingY=y+body;
            rgb(0x333335);
            quad(31.0f,settingY,w-62.0f,1.0f);
            rgb(0x9b9b9e);
            text("Main",31.0f,settingY+8.0f,14.0f,w-80.0f);
            std::string_view caption="Settings preview";
            for(const auto& parameter:frame.parameters)
                if(parameter.catalog_index==item.catalog_index) {
                    caption=parameter.caption;
                    break;
                }
            const float fieldWidth=std::min(205.0f,std::max(75.0f,w*0.43f));
            rgb(0x3f3f41);
            round_rect(w-fieldWidth-28.0f,settingY+7.0f,
                       fieldWidth,24.0f,12.0f);
            rgb(0xeeeeef);
            text(caption,w-fieldWidth-19.0f,settingY+10.0f,
                 13.0f,fieldWidth-17.0f);
        }
    }
    // We intentionally avoid altering GL_VIEWPORT, FBO, shader, texture
    // selection or app-managed caches; state restoration is GL1.1 only.
    return true;
}
} // namespace slinkywinky
