#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_menu_gl11.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#define REQUIRE(x) do {if(!(x))throw std::runtime_error("R67 check failed: " #x);}while(false)

namespace {
using namespace slinkywinky;
const wchar_t klass[]=L"SlinkyWinky-R67-Disposable-WGL";
LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    return DefWindowProcW(hwnd,msg,w,l);
}
struct Context {
    HWND hwnd=nullptr;
    HDC dc=nullptr;
    HGLRC gl=nullptr;
    Context() {
        HINSTANCE inst=GetModuleHandleW(nullptr);
        WNDCLASSW wc{};wc.style=CS_OWNDC;
        wc.lpfnWndProc=proc;wc.hInstance=inst;wc.lpszClassName=klass;
        if(!RegisterClassW(&wc))throw std::runtime_error("Window registration");
        hwnd=CreateWindowW(klass,L"R67 isolated GL fixture",WS_POPUP,
                           0,0,676,630,nullptr,nullptr,inst,nullptr);
        if(!hwnd)throw std::runtime_error("Isolated WGL window");
        dc=GetDC(hwnd);
        if(!dc)throw std::runtime_error("Isolated WGL DC");
        PIXELFORMATDESCRIPTOR descriptor{};
        descriptor.nSize=sizeof(descriptor);
        descriptor.nVersion=1;
        descriptor.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
        descriptor.iPixelType=PFD_TYPE_RGBA;
        descriptor.cColorBits=24;descriptor.cDepthBits=16;
        descriptor.iLayerType=PFD_MAIN_PLANE;
        const int pf=ChoosePixelFormat(dc,&descriptor);
        if(!pf || !SetPixelFormat(dc,pf,&descriptor))
            throw std::runtime_error("WGL pixel format");
        gl=wglCreateContext(dc);
        if(!gl || !wglMakeCurrent(dc,gl))
            throw std::runtime_error("WGL context");
    }
    ~Context() {
        wglMakeCurrent(nullptr,nullptr);
        if(gl)wglDeleteContext(gl);
        if(dc && hwnd)ReleaseDC(hwnd,dc);
        if(hwnd)DestroyWindow(hwnd);
        UnregisterClassW(klass,GetModuleHandleW(nullptr));
    }
};
struct State {
    GLint viewport[4]{},mode=0,serverDepth=0,clientDepth=0;
    GLint modelDepth=0,projectionDepth=0,textureDepth=0;
    GLint blendSrc=0,blendDst=0;
    GLboolean depth=FALSE,blend=FALSE,texture=FALSE,scissor=FALSE;
    GLboolean cull=FALSE,lighting=FALSE,alpha=FALSE,fog=FALSE;
    GLdouble model[16]{},projection[16]{},textureMatrix[16]{};
    GLfloat color[4]{};
};
State snapshot() {
    State s{};
    glGetIntegerv(GL_VIEWPORT,s.viewport);
    glGetIntegerv(GL_MATRIX_MODE,&s.mode);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&s.serverDepth);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH,&s.clientDepth);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&s.modelDepth);
    glGetIntegerv(GL_PROJECTION_STACK_DEPTH,&s.projectionDepth);
    glGetIntegerv(GL_TEXTURE_STACK_DEPTH,&s.textureDepth);
    glGetIntegerv(GL_BLEND_SRC,&s.blendSrc);
    glGetIntegerv(GL_BLEND_DST,&s.blendDst);
    s.depth=glIsEnabled(GL_DEPTH_TEST);
    s.blend=glIsEnabled(GL_BLEND);
    s.texture=glIsEnabled(GL_TEXTURE_2D);
    s.scissor=glIsEnabled(GL_SCISSOR_TEST);
    s.cull=glIsEnabled(GL_CULL_FACE);
    s.lighting=glIsEnabled(GL_LIGHTING);
    s.alpha=glIsEnabled(GL_ALPHA_TEST);
    s.fog=glIsEnabled(GL_FOG);
    glGetDoublev(GL_MODELVIEW_MATRIX,s.model);
    glGetDoublev(GL_PROJECTION_MATRIX,s.projection);
    glGetDoublev(GL_TEXTURE_MATRIX,s.textureMatrix);
    glGetFloatv(GL_CURRENT_COLOR,s.color);
    return s;
}
void same(const State& a,const State& b) {
    for(int i=0;i<4;++i) {
        REQUIRE(a.viewport[i]==b.viewport[i]);
        REQUIRE(std::fabs(a.color[i]-b.color[i])<0.001f);
    }
    REQUIRE(a.mode==b.mode);
    REQUIRE(a.serverDepth==b.serverDepth);
    REQUIRE(a.clientDepth==b.clientDepth);
    REQUIRE(a.modelDepth==b.modelDepth);
    REQUIRE(a.projectionDepth==b.projectionDepth);
    REQUIRE(a.textureDepth==b.textureDepth);
    REQUIRE(a.blendSrc==b.blendSrc && a.blendDst==b.blendDst);
    REQUIRE(a.depth==b.depth && a.blend==b.blend &&
            a.texture==b.texture && a.scissor==b.scissor);
    REQUIRE(a.cull==b.cull && a.lighting==b.lighting &&
            a.alpha==b.alpha && a.fog==b.fog);
    for(int i=0;i<16;++i) {
        REQUIRE(std::fabs(a.model[i]-b.model[i])<0.000001);
        REQUIRE(std::fabs(a.projection[i]-b.projection[i])<0.000001);
        REQUIRE(std::fabs(a.textureMatrix[i]-b.textureMatrix[i])<0.000001);
    }
}
}

int main() {
    try {
        Context context;
        glViewport(0,0,676,630);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_FOG);
        glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_ONE,GL_ZERO);
        glColor4f(0.25f,0.5f,0.75f,1.0f);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();glTranslatef(3,5,7);
        glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-2,2,-3,3,-1,1);
        glMatrixMode(GL_TEXTURE);glLoadIdentity();glTranslatef(0.3f,0.4f,0);
        glMatrixMode(GL_MODELVIEW);
        REQUIRE(glGetError()==GL_NO_ERROR);
        MenuState menu;
        MenuRenderSession admitted;
        REQUIRE(admitted.attach(1,676,630));
        const auto frame=admitted.next(menu);
        REQUIRE(frame.has_value());
        const State before=snapshot();
        REQUIRE(!draw_menu_gl11(*frame,nullptr,context.dc));
        REQUIRE(!draw_menu_gl11(*frame,context.gl,nullptr));
        REQUIRE(!draw_menu_gl11(*frame,context.gl,
                                reinterpret_cast<HDC>(static_cast<uintptr_t>(2))));
        MenuRenderFrame wrong=*frame;
        wrong.viewport_width=999;
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*frame;wrong.visible.front().catalog_index=9999;
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*frame;wrong.epoch=0;
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        same(before,snapshot());
        REQUIRE(draw_menu_gl11(*frame,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        REQUIRE(menu.toggle_preview(frame->visible.front().catalog_index));
        const auto changed=admitted.next(menu);
        REQUIRE(changed && changed->visible.front().preview_enabled);
        REQUIRE(draw_menu_gl11(*changed,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        // No current WGL context: invocation MUST be refused before any GL call.
        REQUIRE(wglMakeCurrent(nullptr,nullptr));
        REQUIRE(!draw_menu_gl11(*changed,context.gl,context.dc));
        REQUIRE(wglMakeCurrent(context.dc,context.gl));
        same(before,snapshot());
        bool foreignAccepted=false;
        std::thread other([&]{
            foreignAccepted=draw_menu_gl11(*changed,context.gl,context.dc);
        });
        other.join();
        REQUIRE(!foreignAccepted);
        same(before,snapshot());
        REQUIRE(admitted.detach());
        std::cout<<"R67_ISOLATED_WGL_CONTEXT_PASS\n";
        std::cout<<"R67_FIXED_FUNCTION_GL_STATE_RESTORED_PASS\n";
        std::cout<<"R67_NO_GAME_RENDER_HOOK_OR_INPUT\n";
        return 0;
    } catch(const std::exception& failure) {
        std::cerr<<failure.what()<<'\n';
        return 1;
    }
}
