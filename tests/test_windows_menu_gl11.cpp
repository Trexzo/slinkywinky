#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <array>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef SW_R91_TEST_JNI_OVERLAY
#include <jni.h>
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
    JNIEnv*,jclass,jlong,jlong,jint,jint);
#endif
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
    GLint viewport[4]{},scissorBox[4]{},mode=0,serverDepth=0,clientDepth=0;
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
    glGetIntegerv(GL_SCISSOR_BOX,s.scissorBox);
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
        REQUIRE(a.scissorBox[i]==b.scissorBox[i]);
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
std::array<GLubyte,4> readback(int x,int y) {
    GLint old=0;
    glGetIntegerv(GL_READ_BUFFER,&old);
    glReadBuffer(GL_BACK);
    std::array<GLubyte,4> pixels{};
    glReadPixels(x,y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glReadBuffer(static_cast<GLenum>(old));
    REQUIRE(glGetError()==GL_NO_ERROR);
    return pixels;
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
        wrong=*frame;wrong.visible.front().title="UNTRUSTED";
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        same(before,snapshot());
        REQUIRE(draw_menu_gl11(*frame,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        // A successful call must actually draw first-party glyph pixels:
        // top accent, white 'S' at the first title row, and its empty cell.
        const auto accent=readback(4,628);
        REQUIRE(accent[0]>=210 && accent[1]>=125 && accent[2]>=50);
        const auto titleInk=readback(27,610);
        REQUIRE(titleInk[0]>=190 && titleInk[1]>=190 && titleInk[2]>=190);
        const auto titleGap=readback(24,610);
        REQUIRE(titleGap[0]<80 && titleGap[1]<80 && titleGap[2]<80);
        const auto moduleInk=readback(34,466);
        REQUIRE(moduleInk[0]>=190 && moduleInk[1]>=190 && moduleInk[2]>=190);
        REQUIRE(menu.toggle_preview(frame->visible.front().catalog_index));
        const auto changed=admitted.next(menu);
        REQUIRE(changed && changed->visible.front().preview_enabled);
        REQUIRE(draw_menu_gl11(*changed,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        // R88: the native GL renderer consumes typed, immutable profile
        // snapshots. It rejects untrusted or stale row captions before GL.
        MenuProfileBridge profile(menu);
        REQUIRE(menu.toggle_expanded(0));
        REQUIRE(profile.adjust_primary(0,1));
        const auto configured=admitted.next(menu,&profile);
        REQUIRE(configured && configured->parameters.size()==1);
        REQUIRE(configured->parameters[0].catalog_index==0);
        REQUIRE(configured->parameters[0].caption=="Aim range: 4.25");
        REQUIRE(draw_menu_gl11(*configured,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        wrong=*configured;
        wrong.parameters[0].catalog_index=3; // Unsupported module.
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*configured;
        wrong.parameters[0].catalog_index=2; // Not expanded.
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*configured;
        wrong.parameters[0].caption="\x01";
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*configured;
        wrong.parameters.push_back(wrong.parameters[0]);
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        same(before,snapshot());

        // R69: draw a translucent inset menu *over* an existing frame.
        // Explicitly prove that pixels beyond the menu panel are untouched.
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(glGetError()==GL_NO_ERROR);
        const auto outsideLeft=readback(0,315);
        const auto outsideRight=readback(675,315);
        const auto outsideTop=readback(335,625);
        const auto outsideBottom=readback(335,5);
        const State beforeOverlay=snapshot();
        REQUIRE(!draw_menu_gl11(*changed,context.gl,context.dc,
                   static_cast<MenuGL11Mode>(99)));
        same(beforeOverlay,snapshot());
        REQUIRE(draw_menu_gl11(*changed,context.gl,context.dc,
                   MenuGL11Mode::BoundedOverlay));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(beforeOverlay,snapshot());
        REQUIRE(readback(0,315)==outsideLeft);
        REQUIRE(readback(675,315)==outsideRight);
        REQUIRE(readback(335,625)==outsideTop);
        REQUIRE(readback(335,5)==outsideBottom);
        const auto overlayAccent=readback(15,615);
        REQUIRE(overlayAccent[0]>outsideLeft[0]);
        REQUIRE(overlayAccent[1]>outsideLeft[1]);
        REQUIRE(overlayAccent[2]<outsideLeft[2]);
        const auto panelInk=readback(40,588);
        REQUIRE(panelInk!=outsideLeft);
        // Redraw with a different active tab. Original scene outside the
        // requested menu bounds must remain byte-identical.
        REQUIRE(menu.select(MenuTab::Visual));
        const auto nextTab=admitted.next(menu);
        REQUIRE(nextTab && nextTab->tab==MenuTab::Visual);
        REQUIRE(draw_menu_gl11(*nextTab,context.gl,context.dc,
                   MenuGL11Mode::BoundedOverlay));
        REQUIRE(readback(0,315)==outsideLeft);
        REQUIRE(readback(675,315)==outsideRight);
        same(beforeOverlay,snapshot());
        std::cout<<"R69_BOUNDED_GL_OVERLAY_UNTOUCHED_OUTSIDE_PIXELS_PASS\\n";
        std::cout<<"R69_TRANSPARENT_GL_PANEL_STATE_RESTORE_PASS\\n";
#ifdef SW_R91_TEST_JNI_OVERLAY
        // R91: invoke the EXACT production JNI entrypoint from a disposable
        // owner-thread WGL fixture. The original game is never started here.
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const State beforeNativeOverlay=snapshot();
        const auto originalOutside=readback(0,315);
        const jlong admittedContext=static_cast<jlong>(
            reinterpret_cast<std::uintptr_t>(context.gl));
        const jlong admittedDc=static_cast<jlong>(
            reinterpret_cast<std::uintptr_t>(context.dc));
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
            nullptr,nullptr,0,admittedDc,676,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
            nullptr,nullptr,admittedContext,admittedDc,677,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(beforeNativeOverlay,snapshot());
        REQUIRE(readback(0,315)==originalOutside);
        REQUIRE(readback(675,315)==originalOutside);
        REQUIRE(readback(15,615)!=originalOutside);
        bool wrongThreadAccepted=false;
        std::thread wrongNativeThread([&] {
            wrongThreadAccepted=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawReadOnlyOverlay(
                    nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE;
        });
        wrongNativeThread.join();
        REQUIRE(!wrongThreadAccepted);
        same(beforeNativeOverlay,snapshot());
        std::cout<<"R91_REAL_JNI_WGL_ONE_FRAME_PIXELS_AND_RESTORATION_PASS\\n";
#endif
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

        // R70: integrate the model, owner session, and GL draw through a
        // single admission boundary, without any attached game process.
        WglMenuOverlayHost host;
        REQUIRE(!host.draw(menu));
        REQUIRE(!host.close());
        REQUIRE(!host.open(nullptr,context.dc,676,630));
        REQUIRE(!host.open(context.gl,nullptr,676,630));
        REQUIRE(!host.open(context.gl,context.dc,999,630));
        REQUIRE(host.open(context.gl,context.dc,676,630));
        REQUIRE(!host.open(context.gl,context.dc,676,630));
        REQUIRE(host.active() && host.last_sequence()==0);
        REQUIRE(host.draw(menu));
        REQUIRE(host.last_sequence()==1);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        // A host viewport change is forbidden until the graphics owner has
        // resized both the real GL viewport and the admitted session.
        glViewport(0,0,650,610);
        REQUIRE(!host.draw(menu));
        REQUIRE(host.last_sequence()==1);
        REQUIRE(!host.resize(676,630));
        REQUIRE(host.resize(650,610));
        REQUIRE(host.draw(menu));
        REQUIRE(glGetError()==GL_NO_ERROR);
        glViewport(0,0,676,630);
        REQUIRE(!host.draw(menu));
        REQUIRE(host.last_sequence()==2);
        REQUIRE(host.resize(676,630));
        REQUIRE(host.draw(menu));
        same(before,snapshot());
        // A second context cannot hijack the admitted graphics session.
        HGLRC alternate=wglCreateContext(context.dc);
        REQUIRE(alternate!=nullptr && alternate!=context.gl);
        REQUIRE(wglMakeCurrent(context.dc,alternate));
        glViewport(0,0,676,630);
        REQUIRE(!host.draw(menu));
        REQUIRE(!host.resize(676,630));
        REQUIRE(host.last_sequence()==3);
        bool otherClosed=false;
        std::thread foreign([&] {
            otherClosed=host.close(); // failure required: wrong owner
        });
        foreign.join();
        REQUIRE(!otherClosed && host.active());
        // Closing after an external context loss is permitted on the same
        // owner thread without trying to use the no-longer-current context.
        REQUIRE(host.close());
        REQUIRE(!host.active() && host.last_sequence()==0);
        REQUIRE(!host.close());
        REQUIRE(host.open(alternate,context.dc,676,630));
        REQUIRE(host.draw(menu));
        REQUIRE(host.last_sequence()==1);
        const State secondBefore=snapshot();
        REQUIRE(host.draw(menu));
        same(secondBefore,snapshot());
        REQUIRE(wglMakeCurrent(nullptr,nullptr));
        REQUIRE(!host.draw(menu));
        REQUIRE(host.close());
        REQUIRE(wglMakeCurrent(context.dc,context.gl));
        REQUIRE(wglDeleteContext(alternate));
        // Reattach must consume a new context epoch and make no assumption
        // that a stale frame or old DC is still current.
        REQUIRE(host.open(context.gl,context.dc,676,630));
        REQUIRE(host.draw(menu));
        REQUIRE(host.close());
        REQUIRE(!host.draw(menu));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        std::cout<<"R70_WGL_OWNER_CONTEXT_SWITCH_AND_REATTACH_PASS\n";
        std::cout<<"R70_WGL_CONTEXT_LOSS_FAIL_CLOSED_PASS\n";
        std::cout<<"R67_ISOLATED_WGL_CONTEXT_PASS\n";
        std::cout<<"R67_FIXED_FUNCTION_GL_STATE_RESTORED_PASS\n";
        std::cout<<"R67_NO_GAME_RENDER_HOOK_OR_INPUT\n";
        std::cout<<"R68_GL11_BITMAP_LABEL_PIXELS_PASS\n";
        std::cout<<"R68_GL11_NO_VENDOR_FONT_RESOURCES_PASS\n";
        return 0;
    } catch(const std::exception& failure) {
        std::cerr<<failure.what()<<'\n';
        return 1;
    }
}
