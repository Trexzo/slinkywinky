#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/menu_bitmap_font.hpp"
#include <GL/gl.h>
#include <algorithm>
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

// Draw original 5x7 block glyphs in the currently admitted GL1.1 context.
// Clamp characters and width before issuing any geometry. Glyph data is
// first-party, textureless and uppercase-only by explicit design.
void text(std::string_view label,float x,float y,
          float pixelScale,float maxWidth) noexcept {
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
    // Never use a context inferred only from a PID. Require exact equality.
    if((mode!=MenuGL11Mode::StandalonePreview &&
        mode!=MenuGL11Mode::BoundedOverlay) || !expected_context || !expected_dc ||
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
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(frame.viewport_width) ||
       viewport[3]!=static_cast<GLint>(frame.viewport_height))
        return false;
    ScopedFixedFunctionState state;
    if(!state.ready())return false;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);
    const bool overlay=mode==MenuGL11Mode::BoundedOverlay;
    // Leave at least a 12 px border in overlay mode. The standalone preview
    // intentionally retains the earlier full-canvas behavior.
    const unsigned paneWidth=overlay?
        std::min<unsigned>(frame.appearance.wide?844u:676u,
                           frame.viewport_width-24u):frame.viewport_width;
    const unsigned paneHeight=overlay?
        std::min<unsigned>(630u,frame.viewport_height-24u):
        frame.viewport_height;
    const unsigned paneLeft=(frame.viewport_width-paneWidth)/2u;
    const unsigned paneTop=(frame.viewport_height-paneHeight)/2u;
    if(overlay) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(paneLeft),
                  static_cast<GLint>(frame.viewport_height-paneTop-paneHeight),
                  static_cast<GLsizei>(paneWidth),
                  static_cast<GLsizei>(paneHeight));
    }else{
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const GLdouble width=static_cast<GLdouble>(frame.viewport_width);
    const GLdouble height=static_cast<GLdouble>(frame.viewport_height);
    glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(static_cast<GLfloat>(paneLeft),
                 static_cast<GLfloat>(paneTop),0.0f);
    const float w=static_cast<float>(paneWidth);
    const float h=static_cast<float>(paneHeight);
    rgba(0x161617,overlay?static_cast<GLubyte>(225):static_cast<GLubyte>(255));
    quad(0,0,w,h);
    rgb(frame.appearance.accent_rgb);
    quad(0,0,w,4);
    rgba(0x101011,overlay?static_cast<GLubyte>(238):static_cast<GLubyte>(255));
    quad(18,78,w-36,42);
    const auto selected=static_cast<unsigned>(frame.tab);
    if(selected<6u) {
        const float cell=(w-36.0f)/7.0f;
        rgb(frame.appearance.accent_rgb);
        quad(18.0f+static_cast<float>(selected)*cell,115,cell,3);
    }
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& item=frame.visible[i];
        const float y=153.0f+static_cast<float>(i)*49.0f;
        rgba(i%2==0?0x1c1c1d:0x252526,
             overlay?static_cast<GLubyte>(235):static_cast<GLubyte>(255));
        quad(18,y,w-36,44);
        if(item.preview_enabled) {
            rgb(frame.appearance.accent_rgb);
            quad(18,y,3,44);
        }
        rgb(item.expanded?0x8c8c8f:0x515154);
        quad(w-123,y+13,18,18);
        rgb(item.preview_enabled?frame.appearance.accent_rgb:0x555558);
        quad(w-76,y+12,45,22);
        rgb(0xcacacc);
        quad(w-73+(item.preview_enabled?21.0f:0.0f),y+15,17,16);
    }
    // Render text last to keep it legible above the opaque controls.
    // UI toggles remain inert: these glyphs never enable gameplay behavior.
    rgb(0xf2f2f3);
    text("SLINKYWINKY",23,18,2.6f,w-40.0f);
    rgb(0x929295);
    text("NATIVE MENU / PREVIEW ONLY",23,53,1.3f,w-45.0f);
    const float cell=(w-36.0f)/7.0f;
    for(unsigned i=0;i<7;++i) {
        const auto tab=static_cast<MenuTab>(i);
        rgb(i==selected?0xf2f2f3:0xb7b7b9);
        text(menu_tab_name(tab),
             21.0f+static_cast<float>(i)*cell,92,
             1.35f,cell-6.0f);
    }
    rgb(0xf2f2f3);
    text(menu_tab_name(frame.tab),25,131,1.7f,w-45.0f);
    if(frame.is_placeholder) {
        rgb(0x929295);
        text("PROFILES NOT CONNECTED",32,178,1.45f,w-64.0f);
    }
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& item=frame.visible[i];
        const float y=153.0f+static_cast<float>(i)*49.0f;
        rgb(0xf2f2f3);
        text(item.title,31.0f,y+9.0f,2.0f,w-208.0f);
        if(!frame.appearance.compact) {
            rgb(0x929295);
            text(item.expanded?"SETTINGS PREVIEW":"UI DEMO ONLY",
                 31.0f,y+31.0f,1.1f,w-208.0f);
        }
        rgb(0xf0f0f0);
        text(item.expanded?"-":"+",w-117.0f,y+17.0f,1.5f,25.0f);
    }
    rgb(0x929295);
    text("UI PREVIEW - NO GAME ACTIONS",23,585,1.2f,w-46.0f);
    // We intentionally avoid altering GL_VIEWPORT, FBO, shader, texture
    // selection or app-managed caches; state restoration is GL1.1 only.
    return true;
}
} // namespace slinkywinky
