#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_gl11.hpp"
#include <GL/gl.h>
#include <algorithm>
#include <cstdint>

namespace slinkywinky {
namespace {
bool stack_available(GLenum depth, GLenum capacity,
                     GLint minimum_depth) noexcept {
    GLint current=0, max=0;
    glGetIntegerv(depth,&current);
    glGetIntegerv(capacity,&max);
    return current>=minimum_depth && max>current;
}

// This only draws opaque first-party rectangles. No text renderer,
// textures, shaders, VBO, framebuffer binding or game input are involved.
void quad(float x,float y,float w,float h) noexcept {
    glBegin(GL_QUADS);
    glVertex2f(x,y);
    glVertex2f(x+w,y);
    glVertex2f(x+w,y+h);
    glVertex2f(x,y+h);
    glEnd();
}
void rgb(std::uint32_t c) noexcept {
    glColor4ub(static_cast<GLubyte>((c>>16)&255),
               static_cast<GLubyte>((c>>8)&255),
               static_cast<GLubyte>(c&255),255);
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
                    HGLRC expected_context,HDC expected_dc) noexcept {
    // Never use a context inferred only from a PID. Require exact equality.
    if(!expected_context || !expected_dc ||
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
           official_module_catalog()[item.catalog_index].tab!=frame.tab)
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
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const GLdouble width=static_cast<GLdouble>(frame.viewport_width);
    const GLdouble height=static_cast<GLdouble>(frame.viewport_height);
    glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const float w=static_cast<float>(frame.viewport_width);
    rgb(0x161617);
    quad(0,0,w,static_cast<float>(frame.viewport_height));
    rgb(frame.appearance.accent_rgb);
    quad(0,0,w,4);
    rgb(0x101011);
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
        rgb(i%2==0?0x1c1c1d:0x252526);
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
    // We intentionally avoid altering GL_VIEWPORT, FBO, shader, texture
    // selection or app-managed caches; state restoration is GL1.1 only.
    return true;
}
} // namespace slinkywinky
