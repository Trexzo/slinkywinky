#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/windows_stop_gate.hpp"
#include "slinkywinky/windows_insert_edge.hpp"
#include "slinkywinky/windows_menu_operator_events.hpp"
#include "slinkywinky/windows_game_window_gate.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_hit_test.hpp"
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
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
    JNIEnv*,jclass,jlong,jlong,jint,jint);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenManualVisibility(JNIEnv*,jclass);
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
    JNIEnv*,jclass,jlong,jlong);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseManualVisibility(JNIEnv*,jclass);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeArmRetainedPreview(JNIEnv*,jclass);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
    JNIEnv*,jclass,jlong,jlong,jint,jint);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
    JNIEnv*,jclass,jint);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeEndRetainedPreview(
    JNIEnv*,jclass);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDiscardRetainedPreview(
    JNIEnv*,jclass);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenOperatorMenu(JNIEnv*,jclass);
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollOperatorMenu(
    JNIEnv*,jclass,jlong,jlong);
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseOperatorMenu(JNIEnv*,jclass);
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
        // R95 pure focus/key-edge policy: no SendInput, hooks or real keypress.
        VisibilityInsertEdge edge;
        REQUIRE(!edge.armed());
        REQUIRE(!edge.sample(true,true));  // Held during admission.
        REQUIRE(!edge.sample(true,true));  // No auto-repeat toggle.
        REQUIRE(!edge.sample(true,false)); // Focused key release arms.
        REQUIRE(edge.armed());
        REQUIRE(edge.sample(true,true));   // First rising Insert.
        REQUIRE(!edge.sample(true,true));  // Held once; never repeats.
        REQUIRE(!edge.sample(true,false));
        REQUIRE(edge.sample(true,true));   // Second fresh key press.
        REQUIRE(!edge.sample(false,true)); // Lost focus with key down.
        REQUIRE(!edge.armed());
        REQUIRE(!edge.sample(true,true));  // Return while held: no ghost.
        REQUIRE(!edge.sample(false,false));
        REQUIRE(!edge.sample(true,true));  // Lost focus + key held.
        REQUIRE(!edge.sample(true,false)); // Must release IN game.
        REQUIRE(edge.sample(true,true));   // Re-press only.
        REQUIRE(!edge.sample(false,false));
        REQUIRE(!edge.sample(true,false));
        REQUIRE(edge.sample(true,true));
        edge.reset();
        REQUIRE(!edge.armed());
        REQUIRE(!edge.sample(true,true)); // New session held Insert.
        REQUIRE(!edge.sample(true,false));
        REQUIRE(edge.sample(true,true));
        std::cout<<"R95_FOREGROUND_INSERT_EDGE_FOCUS_REENTRY_REPEAT_AND_RESET_PASS\\n";
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
        // R93: invoke the real compact JNI export on the disposable GL host.
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const State compactBefore=snapshot();
        const auto compactOutside=readback(0,315);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
            nullptr,nullptr,0,admittedDc,676,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
            nullptr,nullptr,admittedContext,admittedDc,677,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(compactBefore,snapshot());
        REQUIRE(readback(0,315)==compactOutside);
        REQUIRE(readback(675,315)==compactOutside);
        REQUIRE(readback(105,522)!=compactOutside);
        bool compactForeignAccepted=false;
        std::thread compactForeign([&] {
            compactForeignAccepted=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
                    nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE;
        });
        compactForeign.join();
        REQUIRE(!compactForeignAccepted);
        same(compactBefore,snapshot());
        std::cout<<"R93_REAL_COMPACT_JNI_GL11_PIXELS_AND_RESTORE_PASS\\n";

        // R93: reproduce the finite visibility phase with a full redraw of
        // the underlying scene on EVERY frame. Hidden phases must never
        // call JNI, so they retain no overlay pixels across game frames.
        int shownPixels=0,hiddenPixels=0;
        for(int attempt=9;attempt<=72;attempt++) {
            glClearColor(0.10f,0.38f,0.69f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            const auto noMenu=readback(105,522);
            const bool visible=attempt<=28 || attempt>=45;
            if(visible) {
                REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawCompactOverlay(
                    nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
                REQUIRE(readback(105,522)!=noMenu);
                shownPixels++;
            } else {
                REQUIRE(readback(105,522)==noMenu);
                hiddenPixels++;
            }
            same(compactBefore,snapshot());
        }
        REQUIRE(shownPixels==48 && hiddenPixels==16);
        // R99: CALL THE REAL EXPORTED JNI FUNCTIONS, not a mock menu.
        // Admission is inert until explicitly armed; a model is constructed
        // lazily on this WGL owner and survives successive native calls.
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeArmRetainedPreview(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeArmRetainedPreview(
            nullptr,nullptr)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,2)==JNI_FALSE); // cannot navigate before first draw
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,0,admittedDc,676,630)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,677,630)==JNI_FALSE);
        const State retainedBefore=snapshot();
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
        same(retainedBefore,snapshot());
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,2)==JNI_TRUE); // NextRow = 2
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,4)==JNI_TRUE); // ExpandFocused = 4
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,5)==JNI_TRUE); // ToggleFocusedPreview = 5
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,-1)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,8)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
        same(retainedBefore,snapshot());
        bool foreignRetainedAction=false,foreignRetainedDraw=false;
        std::thread foreignRetained([&] {
            foreignRetainedAction=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
                    nullptr,nullptr,2)==JNI_TRUE;
            foreignRetainedDraw=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
                    nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE;
        });
        foreignRetained.join();
        REQUIRE(!foreignRetainedAction && !foreignRetainedDraw);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeEndRetainedPreview(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeEndRetainedPreview(
            nullptr,nullptr)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeArmRetainedPreview(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDiscardRetainedPreview(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDiscardRetainedPreview(
            nullptr,nullptr)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_FALSE);
        same(retainedBefore,snapshot());
        // R100 production JNI operator events are not game key/mouse
        // events. Their action is applied ONLY on the current WGL owner.
        const auto selfPid=static_cast<std::uint32_t>(GetCurrentProcessId());
        std::uint64_t selfBirth=0;
        REQUIRE(menu_operator_process_birth(selfPid,&selfBirth) && selfBirth!=0);
        REQUIRE(menu_operator_event_name(selfPid,selfBirth,12).empty());
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollOperatorMenu(
            nullptr,nullptr,admittedContext,admittedDc)==-1);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenOperatorMenu(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenOperatorMenu(
            nullptr,nullptr)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeArmRetainedPreview(
            nullptr,nullptr)==JNI_FALSE); // Cannot mix R99 and R100 modes.
        std::array<HANDLE,11> operatorTestEvents{};
        for(int i=0;i<11;i++) {
            const auto name=menu_operator_event_name(selfPid,selfBirth,i);
            REQUIRE(!name.empty());
            operatorTestEvents[static_cast<std::size_t>(i)]=OpenEventW(
                EVENT_MODIFY_STATE|SYNCHRONIZE,FALSE,name.c_str());
            REQUIRE(operatorTestEvents[static_cast<std::size_t>(i)]!=nullptr);
        }
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollOperatorMenu(
            nullptr,nullptr,admittedContext,admittedDc)==-1); // no model yet
        const State beforeOperator=snapshot();
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
        same(beforeOperator,snapshot());
        auto commandNative=[&](int slot,int expected,bool accepted) {
            REQUIRE(ResetEvent(operatorTestEvents[9]));
            REQUIRE(ResetEvent(operatorTestEvents[10]));
            REQUIRE(SetEvent(operatorTestEvents[static_cast<std::size_t>(slot)]));
            REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollOperatorMenu(
                nullptr,nullptr,admittedContext,admittedDc)==expected);
            REQUIRE(WaitForSingleObject(operatorTestEvents[accepted?9:10],0)==WAIT_OBJECT_0);
            REQUIRE(WaitForSingleObject(operatorTestEvents[accepted?10:9],0)==WAIT_TIMEOUT);
            REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
                nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_TRUE);
            same(beforeOperator,snapshot());
        };
        bool foreignOperatorPoll=false;
        std::thread foreignOperator([&] {
            foreignOperatorPoll=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollOperatorMenu(
                    nullptr,nullptr,admittedContext,admittedDc)!=-1;
        });
        foreignOperator.join();
        REQUIRE(!foreignOperatorPoll);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeRetainedPreviewAction(
            nullptr,nullptr,2)==JNI_FALSE); // scripts forbidden in operator mode
        commandNative(2,3,true); // NextRow (index 1)
        commandNative(4,5,true); // Expand index 1
        commandNative(5,6,true); // Local preview toggle only
        commandNative(0,1,true); // NextTab (Move, focus index 17)
        commandNative(5,15,false); // Unsupported row denied + refused ACK
        commandNative(8,9,true); // Cooperative operator stop
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseOperatorMenu(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseOperatorMenu(
            nullptr,nullptr)==JNI_FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeDrawRetainedPreview(
            nullptr,nullptr,admittedContext,admittedDc,676,630)==JNI_FALSE);
        for(HANDLE event:operatorTestEvents)CloseHandle(event);
        const auto closedName=menu_operator_event_name(selfPid,selfBirth,2);
        REQUIRE(!OpenEventW(EVENT_MODIFY_STATE,FALSE,closedName.c_str()));
        same(beforeOperator,snapshot());
        std::cout<<"R100_REAL_JNI_PID_BIRTH_MENU_COMMAND_ACK_REFUSAL_AND_STOP_PASS\\n";
        std::cout<<"R99_REAL_JNI_RETAINED_MENU_ARM_DRAW_NAVIGATE_END_AND_DISCARD_PASS\\n";
        std::cout<<"R93_COMPACT_SHOW_HIDE_SHOW_GL_PIXELS_PASS\\n";

        // R94 production stop/toggle event registration and JNI polling.
        // Event opens do not attach Java or touch any Minecraft JVM.
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenManualVisibility(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeOpenManualVisibility(
            nullptr,nullptr)==JNI_FALSE);
        const auto pid=static_cast<std::uint32_t>(GetCurrentProcessId());
        const auto toggleName=slinkywinky::visibility_toggle_event_name(pid);
        const auto stopName=slinkywinky::visibility_stop_event_name(pid);
        REQUIRE(toggleName.rfind(L"Local\\",0)==0 && stopName.rfind(L"Local\\",0)==0);
        HANDLE toggleEvent=OpenEventW(EVENT_MODIFY_STATE,FALSE,toggleName.c_str());
        HANDLE stopEvent=OpenEventW(EVENT_MODIFY_STATE,FALSE,stopName.c_str());
        REQUIRE(toggleEvent && stopEvent);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
            nullptr,nullptr,0,admittedDc)==-1);
        REQUIRE(SetEvent(toggleEvent)!=FALSE);
        bool foreignGateAccepted=false;
        std::thread foreignGate([&] {
            foreignGateAccepted=
                Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
                    nullptr,nullptr,admittedContext,admittedDc)!=-1;
        });
        foreignGate.join();
        REQUIRE(!foreignGateAccepted);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
            nullptr,nullptr,admittedContext,admittedDc)==1);
        REQUIRE(SetEvent(stopEvent)!=FALSE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
            nullptr,nullptr,admittedContext,admittedDc)==2);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseManualVisibility(
            nullptr,nullptr)==JNI_TRUE);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativePollManualVisibility(
            nullptr,nullptr,admittedContext,admittedDc)==-1);
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeCloseManualVisibility(
            nullptr,nullptr)==JNI_FALSE);
        CloseHandle(toggleEvent);
        CloseHandle(stopEvent);
        REQUIRE(!OpenEventW(EVENT_MODIFY_STATE,FALSE,toggleName.c_str()));
        REQUIRE(!OpenEventW(EVENT_MODIFY_STATE,FALSE,stopName.c_str()));
        same(compactBefore,snapshot());
        std::cout<<"R94_NATIVE_PID_CONTROL_TOGGLE_STOP_AND_FOREIGN_THREAD_REFUSAL_PASS\\n";

#endif
        // R96: completely isolated native-first keyboard navigation model.
        // No Windows keyboard hooks, JVM, Minecraft interaction or game input.
        MenuState focusMenu;
        MenuProfileBridge focusProfile(focusMenu);
        MenuNavigation focusNavigation(focusMenu,focusProfile);
        MenuRenderSession focusSession;
        REQUIRE(focusSession.attach(210,676,630));
        auto focusedFrame=focusSession.next(focusMenu,&focusProfile,&focusNavigation);
        REQUIRE(focusedFrame);
        glClearColor(0.1f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const State focusBefore=snapshot();
        REQUIRE(draw_menu_gl11(*focusedFrame,context.gl,context.dc,
                               MenuGL11Mode::StandalonePreview));
        const auto unselected=readback(40,475);
        same(focusBefore,snapshot());
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(*focusedFrame,context.gl,context.dc,
                               MenuGL11Mode::StandalonePreview,
                               focusNavigation.visible_focus()));
        const auto firstSelected=readback(40,475);
        REQUIRE(firstSelected!=unselected);
        REQUIRE(firstSelected[0]>unselected[0] &&
                firstSelected[1]>unselected[1]);
        same(focusBefore,snapshot());
        REQUIRE(focusNavigation.apply(MenuNavAction::NextRow));
        auto nextFocusedFrame=focusSession.next(focusMenu,&focusProfile,
                                                &focusNavigation);
        REQUIRE(nextFocusedFrame && focusNavigation.visible_focus()==1);
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(*nextFocusedFrame,context.gl,context.dc,
                               MenuGL11Mode::StandalonePreview,
                               focusNavigation.visible_focus()));
        REQUIRE(readback(40,475)==unselected);
        const auto secondSelected=readback(40,426);
        REQUIRE(secondSelected[0]>unselected[0] &&
                secondSelected[1]>unselected[1]);
        same(focusBefore,snapshot());
        const auto badFocus=std::optional<std::size_t>{64};
        REQUIRE(!draw_menu_gl11(*nextFocusedFrame,context.gl,context.dc,
                                MenuGL11Mode::StandalonePreview,badFocus));
        same(focusBefore,snapshot());
        WglMenuOverlayHost focusHost;
        REQUIRE(focusHost.open(context.gl,context.dc,676,630));
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(focusHost.draw(focusMenu,&focusProfile,
                               MenuGL11Mode::StandalonePreview,&focusNavigation));
        REQUIRE(focusHost.close());
        same(focusBefore,snapshot());
        REQUIRE(focusSession.detach());
        // R97: use the exact GL11 compact transform for client-space input.
        // These pixels come from the real native renderer and are sampled
        // without any keyboard/mouse message sent to Minecraft or Windows.
        const auto compactPanel=menu_panel_geometry(
            676,630,false,MenuPanelMode::CompactOverlay);
        REQUIRE(compactPanel && compactPanel->left==98 &&
                compactPanel->top==105 && compactPanel->scale==0.70f);
        const float logicalW=compactPanel->logical_width();
        const int toggleX=static_cast<int>(compactPanel->left)+
            static_cast<int>(std::lround(compactPanel->scale*(logicalW-54.0f)));
        const int toggleY=static_cast<int>(compactPanel->top)+
            static_cast<int>(std::lround(compactPanel->scale*(153.0f+23.0f)));
        const auto compactHit=hit_test_menu(
            *focusedFrame,MenuPanelMode::CompactOverlay,toggleX,toggleY);
        REQUIRE(compactHit && compactHit->kind==MenuHitKind::Toggle &&
                compactHit->catalog_index==0);
        const int expandX=static_cast<int>(compactPanel->left)+
            static_cast<int>(std::lround(compactPanel->scale*(logicalW-114.0f)));
        const auto compactExpand=hit_test_menu(
            *focusedFrame,MenuPanelMode::CompactOverlay,expandX,toggleY);
        REQUIRE(compactExpand && compactExpand->kind==MenuHitKind::Expand &&
                compactExpand->catalog_index==0);
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const auto withoutMenu=readback(toggleX,629-toggleY);
        REQUIRE(draw_menu_gl11(*focusedFrame,context.gl,context.dc,
                               MenuGL11Mode::CompactOverlay,
                               std::optional<std::size_t>{0}));
        REQUIRE(readback(toggleX,629-toggleY)!=withoutMenu);
        REQUIRE(!hit_test_menu(*focusedFrame,MenuPanelMode::CompactOverlay,
                               static_cast<int>(compactPanel->left)-1,toggleY));
        same(focusBefore,snapshot());
        std::cout<<"R97_COMPACT_PIXEL_TO_CLIENT_HIT_ALIGNMENT_PASS\\n";
        std::cout<<"R96_FOCUSED_ROW_GL11_PIXELS_OWNER_AND_RESTORE_PASS\\n";

        // R99: a genuine, per-frame current-WGL renderer whose inert menu
        // navigation survives across draws instead of resetting each frame.
        // No Minecraft JVM, hooks, OS keys or combat module execution.
        WglRetainedMenuPreview retained;
        REQUIRE(!retained.active() && retained.owner_thread());
        REQUIRE(!retained.apply(MenuNavAction::NextRow));
        REQUIRE(!retained.draw(nullptr,context.dc,676,630));
        REQUIRE(!retained.draw(context.gl,nullptr,676,630));
        REQUIRE(!retained.draw(context.gl,context.dc,999,630));
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        const State r99Before=snapshot();
        REQUIRE(retained.draw(context.gl,context.dc,676,630));
        REQUIRE(retained.active() && retained.drawn_frames()==1);
        REQUIRE(retained.focused_row() && *retained.focused_row()==0);
        same(r99Before,snapshot());
        REQUIRE(retained.apply(MenuNavAction::NextRow));
        REQUIRE(retained.focused_row() && *retained.focused_row()==1);
        REQUIRE(retained.apply(MenuNavAction::ExpandFocused));
        REQUIRE(retained.apply(MenuNavAction::ToggleFocusedPreview));
        REQUIRE(retained.preview_enabled(1));
        REQUIRE(retained.apply(MenuNavAction::IncreaseSetting));
        REQUIRE(!retained.preview_enabled(3));
        REQUIRE(retained.draw(context.gl,context.dc,676,630));
        REQUIRE(retained.drawn_frames()==2 && retained.preview_enabled(1));
        REQUIRE(retained.focused_row() && *retained.focused_row()==1);
        REQUIRE(retained.accepted_actions()==4);
        same(r99Before,snapshot());
        REQUIRE(retained.apply(MenuNavAction::NextRow));
        REQUIRE(retained.apply(MenuNavAction::NextRow)); // unsupported #3
        REQUIRE(retained.focused_row() && *retained.focused_row()==3);
        REQUIRE(!retained.apply(MenuNavAction::ToggleFocusedPreview));
        REQUIRE(!retained.apply(MenuNavAction::ExpandFocused));
        REQUIRE(!retained.preview_enabled(3));
        REQUIRE(retained.refused_actions()==2);
        REQUIRE(retained.apply(MenuNavAction::NextTab));
        REQUIRE(retained.tab()==MenuTab::Move);
        REQUIRE(retained.draw(context.gl,context.dc,676,630));
        REQUIRE(retained.focused_row() && *retained.focused_row()==17);
        REQUIRE(retained.drawn_frames()==3);
        bool r99ForeignDraw=false,r99ForeignAction=false,r99ForeignClose=false;
        std::thread r99Foreign([&] {
            r99ForeignDraw=retained.draw(context.gl,context.dc,676,630);
            r99ForeignAction=retained.apply(MenuNavAction::NextTab);
            r99ForeignClose=retained.close();
        });
        r99Foreign.join();
        REQUIRE(!r99ForeignDraw && !r99ForeignAction && !r99ForeignClose);
        REQUIRE(retained.drawn_frames()==3);
        REQUIRE(!retained.draw(context.gl,context.dc,650,610));
        REQUIRE(retained.drawn_frames()==3);
        REQUIRE(retained.close());
        REQUIRE(!retained.active());
        REQUIRE(!retained.apply(MenuNavAction::NextRow));
        REQUIRE(!retained.close());
        same(r99Before,snapshot());
        std::cout<<"R99_RETAINED_WGL_MENU_STATE_THREE_FRAMES_AND_OWNER_RESTORE_PASS\\n";

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
        // R101: spoofing a title alone must not grant input admission.
        // The HDC must be the actual current WGL renderer window, owned by
        // the exact current thread and source-pinned original process.
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)==
                GameWindowFocus::Refused);
        REQUIRE(SetWindowTextW(context.hwnd,L"Minecraft 1.8.9"));
        ShowWindow(context.hwnd,SW_SHOWNOACTIVATE);
        REQUIRE(IsWindowVisible(context.hwnd));
        const auto localWindow=audit_game_render_window(
            context.gl,context.dc,676,630);
        REQUIRE(localWindow==GameWindowFocus::ValidBackground ||
                localWindow==GameWindowFocus::ValidForeground);
        REQUIRE(audit_game_render_window(context.gl,context.dc,677,630)==
                GameWindowFocus::Refused);
        REQUIRE(audit_game_render_window(nullptr,context.dc,676,630)==
                GameWindowFocus::Refused);
        REQUIRE(audit_game_render_window(context.gl,nullptr,676,630)==
                GameWindowFocus::Refused);
        // Ambiguous extra same-title root in the same PID is never usable.
        HWND duplicate=CreateWindowW(klass,L"Minecraft 1.8.9",WS_POPUP,
                                     30,30,320,240,nullptr,nullptr,
                                     GetModuleHandleW(nullptr),nullptr);
        REQUIRE(duplicate!=nullptr);
        ShowWindow(duplicate,SW_SHOWNOACTIVATE);
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)==
                GameWindowFocus::Refused);
        ShowWindow(duplicate,SW_HIDE);
        DestroyWindow(duplicate);
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)!=
                GameWindowFocus::Refused);
        std::thread otherWindow([&] {
            REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)==
                    GameWindowFocus::Refused);
        });
        otherWindow.join();
        ShowWindow(context.hwnd,SW_HIDE);
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)==
                GameWindowFocus::Refused);
        REQUIRE(SetWindowTextW(context.hwnd,L"R67 isolated GL fixture"));
        same(before,snapshot());
        std::cout<<"R101_READ_ONLY_WGL_BOUND_GAME_WINDOW_IDENTITY_NEGATIVE_CONTROLS_PASS\\n";
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
