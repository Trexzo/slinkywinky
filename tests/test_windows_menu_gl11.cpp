#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/windows_menu_system_font.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/windows_stop_gate.hpp"
#include "slinkywinky/windows_insert_edge.hpp"
#include "slinkywinky/windows_menu_operator_events.hpp"
#include "slinkywinky/windows_game_window_gate.hpp"
#include "slinkywinky/windows_original_pointer_audit.hpp"
#include "slinkywinky/windows_menu_focus_fence.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_expander_geometry.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_keyboard_router.hpp"
#include <array>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstring>
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
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeAuditRenderWindow(
    JNIEnv*,jclass,jlong,jlong,jint,jint);
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeObservePhysicalPointer(
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
    bool registered=false;
    Context(unsigned width=676u,unsigned height=630u) {
        HINSTANCE inst=GetModuleHandleW(nullptr);
        WNDCLASSW wc{};wc.style=CS_OWNDC;
        wc.lpfnWndProc=proc;wc.hInstance=inst;wc.lpszClassName=klass;
        if(RegisterClassW(&wc))registered=true;
        else if(GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Window registration");
        hwnd=CreateWindowW(klass,L"R67 isolated GL fixture",WS_POPUP,
                           0,0,static_cast<int>(width),
                           static_cast<int>(height),
                           nullptr,nullptr,inst,nullptr);
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
        if(registered)UnregisterClassW(klass,GetModuleHandleW(nullptr));
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
// Optional, own-frame WGL screenshot: never captures another application,
// game window, keyboard input, or clipboard.
bool save_gl_framebuffer_ppm(const char* output,
                             unsigned width=676u,unsigned height=630u) {
    if(!output || std::strlen(output)>=500 || width<320 || width>1500 ||
       height<240 || height>8192)return false;
    GLint old=0;
    glGetIntegerv(GL_READ_BUFFER,&old);
    glReadBuffer(GL_BACK);
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width)*height*3u);
    glReadPixels(0,0,static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height),
                 GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    glReadBuffer(static_cast<GLenum>(old));
    if(glGetError()!=GL_NO_ERROR)return false;
    std::ofstream out(output,std::ios::binary);
    if(!out)return false;
    out<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int y=static_cast<int>(height)-1;y>=0;--y)
        out.write(reinterpret_cast<const char*>(
            pixels.data()+static_cast<std::size_t>(y)*width*3u),
            static_cast<std::streamsize>(width*3u));
    out.close();
    return out.good();
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
        // R207: the real GL11 HUD painter receives a copied runtime
        // ACTIVE snapshot, never the ClickGUI's preview_enabled flags.
        ModuleArrayListSnapshot arraylist;
        arraylist.contract_version=1u;
        arraylist.source_sequence=77u;
        arraylist.world_generation=9u;
        arraylist.world_ready=true;
        arraylist.modules={
            {"Sprint",true,true,9u},
            {"Timer",true,false,9u},
            {"Keep Sprint",true,true,9u}
        };
        REQUIRE(!draw_module_arraylist_gl11(
            arraylist,nullptr,context.dc,676u,630u));
        const auto rows=make_module_arraylist_display(arraylist);
        REQUIRE(rows && rows->active_names.size()==2u);
        REQUIRE(std::find(rows->active_names.begin(),rows->active_names.end(),
                          "Timer")==rows->active_names.end());
        REQUIRE(draw_module_arraylist_gl11(
            arraylist,context.gl,context.dc,676u,630u));
        same(before,snapshot());
        if(const char* output=std::getenv("SLINKYWINKY_R207_ARRAYLIST_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(output));
        const auto paintedArraylistAccent=readback(666,612);
        REQUIRE(paintedArraylistAccent[0]==0xF3u &&
                paintedArraylistAccent[1]==0x5Du &&
                paintedArraylistAccent[2]==0x12u);
        auto invalidArraylist=arraylist;
        invalidArraylist.modules[0].canonical_name="Unknown";
        REQUIRE(!draw_module_arraylist_gl11(
            invalidArraylist,context.gl,context.dc,676u,630u));
        same(before,snapshot());
        std::cout<<"R207_ACTUAL_WGL_RUNTIME_ARRAYLIST_ROWS_AND_GL_RESTORE_PASS\n";
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
        // Opt-in deterministic raw framebuffer evidence. This uses only the
        // disposable WGL test context; no Minecraft or Windows screen capture.
        if(const char* output=std::getenv("SLINKYWINKY_GL11_SNAPSHOT_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(output));
        // R113: source-style black menu rather than old orange title strip.
        // Stable samples avoid glyph edges and validate rounded panel ink.
        const auto background=readback(4,628);
        REQUIRE(background[0]<65 && background[1]<65 && background[2]<65);
        REQUIRE(measure_menu_system_text("Aim Assist",20.0f)>55.0f);
        const auto selectedTab=readback(60,595);
        REQUIRE(selectedTab[0]>background[0] && selectedTab[1]>background[1]);
        const auto cardFill=readback(19,534);
        REQUIRE(cardFill[0]<55 && cardFill[1]<55 && cardFill[2]<55);
        const auto inactiveTrack=readback(640,549);
        REQUIRE(inactiveTrack[0]>cardFill[0]);
        REQUIRE(menu.toggle_preview(frame->visible.front().catalog_index));
        const auto changed=admitted.next(menu);
        REQUIRE(changed && changed->visible.front().preview_enabled);
        REQUIRE(draw_menu_gl11(*changed,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        const auto activeTrack=readback(640,549);
        REQUIRE(activeTrack[0]>inactiveTrack[0]+40);
        // Optional source-owned evidence of the orange enabled preview.
        if(const char* enabled=std::getenv("SLINKYWINKY_GL11_ENABLED_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(enabled));
        // R88: the native GL renderer consumes typed, immutable profile
        // snapshots. It rejects untrusted or stale row captions before GL.
        MenuProfileBridge profile(menu);
        REQUIRE(menu.toggle_expanded(0));
        REQUIRE(profile.adjust_primary(0,1));
        const auto configured=admitted.next(menu,&profile);
        REQUIRE(configured && configured->parameters.size()==1);
        REQUIRE(configured->parameters[0].catalog_index==0);
        REQUIRE(configured->parameters[0].caption=="Range: 4.25");
        REQUIRE(draw_menu_gl11(*configured,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        if(const char* expanded=std::getenv("SLINKYWINKY_GL11_EXPANDED_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(expanded));
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
        wrong.parameters[0].minimum=5.0f;
        wrong.parameters[0].maximum=4.0f;
        REQUIRE(!draw_menu_gl11(wrong,context.gl,context.dc));
        wrong=*configured;
        wrong.parameters[0].value=wrong.parameters[0].maximum+0.25f;
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
        const auto overlayFill=readback(15,615);
        REQUIRE(overlayFill!=outsideLeft);
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
        const auto unselected=readback(10,535);
        same(focusBefore,snapshot());
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(*focusedFrame,context.gl,context.dc,
                               MenuGL11Mode::StandalonePreview,
                               focusNavigation.visible_focus()));
        const auto firstSelected=readback(10,535);
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
        REQUIRE(readback(10,535)==unselected);
        const auto secondSelected=readback(10,471);
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

        // R114: the live preview model contains only its actual first-party
        // Default card. Additional cards below are explicitly disposable
        // screenshot fixtures: never populated by filesystem or Minecraft.
        MenuState profilesMenu;
        REQUIRE(profilesMenu.select(MenuTab::Profiles));
        MenuRenderSession profilesSession;
        REQUIRE(profilesSession.attach(311,676,630));
        const auto realProfiles=profilesSession.next(profilesMenu);
        REQUIRE(realProfiles && realProfiles->is_placeholder &&
                realProfiles->profile_cards.size()==1 &&
                realProfiles->profile_cards[0].is_default);
        REQUIRE(menu_profile_preview_valid(*realProfiles,676,630));
        const State profileBefore=snapshot();
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(*realProfiles,context.gl,context.dc));
        same(profileBefore,snapshot());
        REQUIRE(!hit_test_menu(*realProfiles,
            MenuPanelMode::StandalonePreview,32,88));
        REQUIRE(!hit_test_menu(*realProfiles,
            MenuPanelMode::StandalonePreview,303,89));
        if(const char* output=std::getenv("SLINKYWINKY_GL11_PROFILES_REAL_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(output));

        MenuRenderFrame profileGallery=*realProfiles;
        profileGallery.profile_cards.push_back({
            "Practice","Illustrative profile, not loaded.","NONE",false
        });
        profileGallery.profile_cards.push_back({
            "BedWars","Illustrative profile, not loaded.","NONE",false
        });
        profileGallery.profile_cards.push_back({
            "Duels","Illustrative profile, not loaded.","TAB",false
        });
        REQUIRE(menu_profile_preview_valid(profileGallery,676,630));
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(profileGallery,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(profileBefore,snapshot());
        REQUIRE(!hit_test_menu(profileGallery,
            MenuPanelMode::StandalonePreview,460,190));
        if(const char* output=std::getenv("SLINKYWINKY_GL11_PROFILES_DEMO_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(output));
        auto forgedProfiles=profileGallery;
        forgedProfiles.profile_cards[1].name="../traversal";
        REQUIRE(!menu_profile_preview_valid(forgedProfiles,676,630));
        REQUIRE(!draw_menu_gl11(forgedProfiles,context.gl,context.dc));
        forgedProfiles=profileGallery;
        forgedProfiles.profile_cards[2].name="Practice";
        REQUIRE(!draw_menu_gl11(forgedProfiles,context.gl,context.dc));
        forgedProfiles=profileGallery;
        forgedProfiles.profile_cards[1].description=std::string(65,'a');
        REQUIRE(!draw_menu_gl11(forgedProfiles,context.gl,context.dc));
        forgedProfiles=profileGallery;
        forgedProfiles.profile_cards[0].is_default=false;
        REQUIRE(!draw_menu_gl11(forgedProfiles,context.gl,context.dc));
        forgedProfiles=profileGallery;
        forgedProfiles.profile_cards.push_back({"Fifth","Example","",false});
        forgedProfiles.profile_cards.push_back({"Sixth","Example","",false});
        forgedProfiles.profile_cards.push_back({"Seventh","Example","",false});
        REQUIRE(!draw_menu_gl11(forgedProfiles,context.gl,context.dc));
        same(profileBefore,snapshot());
        REQUIRE(profilesSession.detach());
        std::cout<<"R114_PROFILE_GRID_ACTUAL_DEFAULT_AND_DEMO_GALLERY_PASS\\n";
        std::cout<<"R114_PROFILE_ICONS_NONINTERACTIVE_AND_FORGED_DATA_REFUSED_PASS\\n";

        // R114 screenshot of the source-style "In-game" settings heading for
        // the existing first-party Auto Clicker rate parameter. No new
        // click pattern enum or gameplay operation is falsely implemented.
        MenuState clicksMenu;
        REQUIRE(clicksMenu.toggle_expanded(2));
        MenuProfileBridge clicksProfile(clicksMenu);
        MenuRenderSession clicksSession;
        REQUIRE(clicksSession.attach(312,676,630));
        const auto clicksFrame=clicksSession.next(clicksMenu,&clicksProfile);
        REQUIRE(clicksFrame && clicksFrame->parameters.size()==1 &&
                clicksFrame->parameters[0].catalog_index==2);
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(draw_menu_gl11(*clicksFrame,context.gl,context.dc));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(profileBefore,snapshot());
        if(const char* output=std::getenv("SLINKYWINKY_GL11_AUTOCLICKER_PPM"))
            REQUIRE(save_gl_framebuffer_ppm(output));
        REQUIRE(clicksSession.detach());
        std::cout<<"R114_AUTO_CLICKER_GROUPED_NATIVE_RATE_CONTROL_PASS\\n";

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
            static_cast<int>(std::lround(compactPanel->scale*(62.0f+18.0f)));
        const auto compactHit=hit_test_menu(
            *focusedFrame,MenuPanelMode::CompactOverlay,toggleX,toggleY);
        REQUIRE(compactHit && compactHit->kind==MenuHitKind::Toggle &&
                compactHit->catalog_index==0);
        const auto expRect=menu_expander_geometry(
            *focusedFrame,logicalW,0u);
        REQUIRE(expRect);
        const int expandX=static_cast<int>(compactPanel->left)+
            static_cast<int>(std::lround(compactPanel->scale*
                (expRect->plus_x+10.0f)));
        const auto compactExpand=hit_test_menu(
            *focusedFrame,MenuPanelMode::CompactOverlay,expandX,
            static_cast<int>(compactPanel->top)+
            static_cast<int>(std::lround(compactPanel->scale*
                (expRect->y+10.0f))));
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
        // R158 owner-only retained native profile readback.
        const auto initialProfile=retained.profile_snapshot(
            context.gl,context.dc,676,630);
        REQUIRE(initialProfile && initialProfile->find(
            "sprint_enabled=false\n")!=std::string::npos);
        REQUIRE(initialProfile->find(
            "auto_block_enabled=false\n")!=std::string::npos);
        REQUIRE(parse_settings(*initialProfile).valid);
        REQUIRE(!retained.profile_snapshot(context.gl,context.dc,677,630));
        REQUIRE(!retained.profile_snapshot(nullptr,context.dc,676,630));
        REQUIRE(!retained.profile_snapshot(context.gl,nullptr,676,630));
        REQUIRE(retained.focused_row() && *retained.focused_row()==0);
        same(r99Before,snapshot());
        REQUIRE(retained.apply(MenuNavAction::NextRow));
        REQUIRE(retained.focused_row() && *retained.focused_row()==1);
        REQUIRE(retained.apply(MenuNavAction::ExpandFocused));
        REQUIRE(retained.apply(MenuNavAction::ToggleFocusedPreview));
        REQUIRE(retained.preview_enabled(1));
        REQUIRE(retained.apply(MenuNavAction::IncreaseSetting));
        const auto modifiedProfile=retained.profile_snapshot(
            context.gl,context.dc,676,630);
        REQUIRE(modifiedProfile && parse_settings(*modifiedProfile).valid);
        const auto nativeModified=parse_settings(*modifiedProfile);
        REQUIRE(nativeModified.settings.combat.auto_block_enabled);
        REQUIRE(std::fabs(nativeModified.settings.combat.block_range-
                          3.35f)<0.001f);
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
        REQUIRE(retained.apply(MenuNavAction::ExpandFocused));
        REQUIRE(!retained.preview_enabled(3)); // still no gameplay toggle
        REQUIRE(retained.refused_actions()==1);
        REQUIRE(retained.apply(MenuNavAction::NextTab));
        REQUIRE(retained.tab()==MenuTab::Move);
        REQUIRE(retained.draw(context.gl,context.dc,676,630));
        REQUIRE(retained.focused_row() && *retained.focused_row()==17);
        REQUIRE(retained.drawn_frames()==3);
        bool r99ForeignDraw=false,r99ForeignAction=false,r99ForeignClose=false;
        bool r158ForeignRead=false;
        std::thread r99Foreign([&] {
            r158ForeignRead=retained.profile_snapshot(
                context.gl,context.dc,676,630).has_value();
            r99ForeignDraw=retained.draw(context.gl,context.dc,676,630);
            r99ForeignAction=retained.apply(MenuNavAction::NextTab);
            r99ForeignClose=retained.close();
        });
        r99Foreign.join();
        REQUIRE(!r99ForeignDraw && !r99ForeignAction && !r99ForeignClose);
        REQUIRE(!r158ForeignRead);
        REQUIRE(retained.drawn_frames()==3);
        REQUIRE(!retained.draw(context.gl,context.dc,650,610));
        REQUIRE(retained.drawn_frames()==3);
        REQUIRE(retained.close());
        REQUIRE(!retained.active());
        REQUIRE(!retained.profile_snapshot(context.gl,context.dc,676,630));
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
        // R208 host owns GL context; publisher supplies state by value only.
        ModuleArrayListFeed wglFeed;
        REQUIRE(wglFeed.publish(arraylist));
        REQUIRE(host.draw_active_modules(wglFeed));
        same(before,snapshot());
        wglFeed.retire();
        REQUIRE(!host.draw_active_modules(wglFeed));
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
        // R172: strict physical-client bounds and a default-denied
        // original-window pointer observer. No cursor is moved here.
        REQUIRE(point_inside_original_client(0,0,676,630));
        REQUIRE(point_inside_original_client(675,629,676,630));
        REQUIRE(!point_inside_original_client(676,629,676,630));
        REQUIRE(!point_inside_original_client(675,630,676,630));
        REQUIRE(!point_inside_original_client(-1,0,676,630));
        REQUIRE(!point_inside_original_client(0,-1,676,630));
        // R188: all 18 orderings of two exact HWND/OS audits and the
        // nullable physical cursor sample. Only stable matched states
        // survive; foreground loss/recovery between reads is REFUSED.
        constexpr std::array<GameWindowFocus,3> focusStatuses{{
            GameWindowFocus::Refused,
            GameWindowFocus::ValidBackground,
            GameWindowFocus::ValidForeground}};
        for(const auto firstStatus:focusStatuses)
            for(const auto lastStatus:focusStatuses)
                for(const bool cursorAdmitted:{false,true}) {
                    const auto observed=reconcile_original_pointer_focus(
                        firstStatus,cursorAdmitted,lastStatus);
                    const bool foregroundPass=
                        firstStatus==GameWindowFocus::ValidForeground &&
                        lastStatus==GameWindowFocus::ValidForeground &&
                        cursorAdmitted;
                    const bool backgroundPass=
                        firstStatus==GameWindowFocus::ValidBackground &&
                        lastStatus==GameWindowFocus::ValidBackground &&
                        !cursorAdmitted;
                    REQUIRE(observed==(foregroundPass?
                        GameWindowFocus::ValidForeground:(backgroundPass?
                        GameWindowFocus::ValidBackground:GameWindowFocus::Refused)));
                }
        // R189: all 31 nonzero combinations of five sampled Win32
        // mouse buttons (including chords) are ineligible for passive
        // source-slider hover; focus observation is a separate concept.
        OriginalPointerObservation buttonSample{};
        for(std::uint32_t mask=0;mask<32u;++mask) {
            buttonSample.button_down_mask=mask;
            REQUIRE(original_pointer_released_for_hover(buttonSample)==
                    (mask==0u));
        }
        REQUIRE(!audit_original_render_pointer(context.gl,context.dc,676,630));
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
#ifdef SW_R91_TEST_JNI_OVERLAY
        const auto auditJni=Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeAuditRenderWindow(
            nullptr,nullptr,
            static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context.gl)),
            static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context.dc)),676,630);
        REQUIRE(auditJni==static_cast<jint>(localWindow));
#endif
        REQUIRE(localWindow==GameWindowFocus::ValidBackground ||
                localWindow==GameWindowFocus::ValidForeground);
        const auto physical=audit_original_render_pointer(
            context.gl,context.dc,676,630);
        // R188: actual read-only audit brackets on the disposable WGL
        // context. The foreground can change externally at any instant,
        // so REFUSED remains a valid result and no focus is changed here.
        const auto afterWindow=audit_game_render_window(
            context.gl,context.dc,676,630);
        const auto paired=reconcile_original_pointer_focus(
            localWindow,physical.has_value(),afterWindow);
        if(paired==GameWindowFocus::ValidForeground)
            REQUIRE(localWindow==GameWindowFocus::ValidForeground &&
                    afterWindow==GameWindowFocus::ValidForeground && physical);
        if(paired==GameWindowFocus::ValidBackground)
            REQUIRE(localWindow==GameWindowFocus::ValidBackground &&
                    afterWindow==GameWindowFocus::ValidBackground && !physical);
        if(physical) {
            // An admitted physical sample was foreground at its own audit
            // but may race with the independent before/after observations.
            REQUIRE(physical->button_down_mask<=31u);
            REQUIRE(physical->inside_client==
                point_inside_original_client(
                    physical->client_x,physical->client_y,676,630));
        }
#ifdef SW_R91_TEST_JNI_OVERLAY
        // Invalid JNI environment cannot expose cursor data.
        REQUIRE(Java_dev_slinkywinky_instrument_RendererWglEvidence_nativeObservePhysicalPointer(
            nullptr,nullptr,static_cast<jlong>(
                reinterpret_cast<std::uintptr_t>(context.gl)),
            static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context.dc)),
            676,630)==nullptr);
#endif
        REQUIRE(!audit_original_render_pointer(context.gl,context.dc,677,630));
        REQUIRE(!audit_original_render_pointer(nullptr,context.dc,676,630));
        // R104 connects its read-only focus lifetime contract to the actual
        // WGL owner/window evidence, without synthesizing a single key.
        MenuFocusFence focusFence;
        REQUIRE(focusFence.begin(101,true));
        bool allForeground=true;
        for(int sample=0;sample<3;sample++) {
            const auto observation=audit_game_render_window(
                context.gl,context.dc,676,630);
            REQUIRE(observation!=GameWindowFocus::Refused);
            allForeground&=observation==GameWindowFocus::ValidForeground;
            REQUIRE(focusFence.observe(observation)!=FocusFenceState::Refused);
        }
        REQUIRE(focusFence.foreground_candidate()==allForeground);
        REQUIRE(!focusFence.can_consume_game_input());
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
        REQUIRE(!audit_original_render_pointer(
            context.gl,context.dc,676,630));
        REQUIRE(focusFence.observe(audit_game_render_window(
            context.gl,context.dc,676,630))==FocusFenceState::Refused);
        REQUIRE(!focusFence.foreground_candidate());
        REQUIRE(!focusFence.can_consume_game_input());
        ShowWindow(duplicate,SW_HIDE);
        DestroyWindow(duplicate);
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)!=
                GameWindowFocus::Refused);
        REQUIRE(focusFence.observe(audit_game_render_window(
            context.gl,context.dc,676,630))==FocusFenceState::Refused);
        REQUIRE(!focusFence.begin(101,true)); // can't re-use an epoch
        REQUIRE(!focusFence.begin(102,false)); // no verified key/button release
        REQUIRE(focusFence.begin(102,true));
        REQUIRE(!focusFence.foreground_candidate());
        bool otherWindowRefused=false;
        std::thread otherWindow([&] {
            otherWindowRefused=audit_game_render_window(
                context.gl,context.dc,676,630)==GameWindowFocus::Refused &&
                !audit_original_render_pointer(context.gl,context.dc,676,630);
        });
        otherWindow.join();
        REQUIRE(otherWindowRefused);
        ShowWindow(context.hwnd,SW_HIDE);
        REQUIRE(audit_game_render_window(context.gl,context.dc,676,630)==
                GameWindowFocus::Refused);
        REQUIRE(SetWindowTextW(context.hwnd,L"R67 isolated GL fixture"));
        same(before,snapshot());
        // R115: second disposable WGL context at the recovered 900px width.
        // This proves native two-column layout, pixel output and hit areas
        // without opening Minecraft or capturing another desktop window.
        {
            Context sourceWide(900u,630u);
            glViewport(0,0,900,630);
            const State wideBefore=snapshot();
            REQUIRE(menu_module_columns(676.0f)==1u);
            REQUIRE(menu_module_columns(900.0f)==2u);
            REQUIRE(menu_module_columns(1500.0f)==3u);
            MenuState sourceMenu;
            MenuProfileBridge sourceProfile(sourceMenu);
            MenuRenderSession sourceFrames;
            REQUIRE(sourceFrames.attach(901,900,630));
            auto sourceFrame=sourceFrames.next(sourceMenu,&sourceProfile);
            REQUIRE(sourceFrame && sourceFrame->visible.size()==8);
            const auto firstCard=menu_module_card_rect(*sourceFrame,900.0f,0);
            const auto secondCard=menu_module_card_rect(*sourceFrame,900.0f,1);
            REQUIRE(firstCard.x==8.0f && secondCard.x>firstCard.x+
                    firstCard.width);
            REQUIRE(firstCard.y==secondCard.y);
            const auto secondToggle=hit_test_menu(*sourceFrame,
                MenuPanelMode::StandalonePreview,
                static_cast<int>(secondCard.x+secondCard.width-30.0f),
                static_cast<int>(secondCard.y+17.0f));
            REQUIRE(secondToggle && secondToggle->kind==MenuHitKind::Toggle &&
                    secondToggle->catalog_index==1u);
            REQUIRE(!hit_test_menu(*sourceFrame,
                MenuPanelMode::StandalonePreview,
                static_cast<int>(firstCard.x+firstCard.width+6.0f),
                static_cast<int>(firstCard.y+17.0f)));
            glClearColor(0.0f,0.0f,0.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            REQUIRE(draw_menu_gl11(*sourceFrame,sourceWide.gl,sourceWide.dc));
            REQUIRE(glGetError()==GL_NO_ERROR);
            same(wideBefore,snapshot());
            if(const char* output=std::getenv("SLINKYWINKY_R115_900_COMBAT_PPM"))
                REQUIRE(save_gl_framebuffer_ppm(output,900,630));

            REQUIRE(sourceMenu.toggle_expanded(2));
            const auto expanded=sourceFrames.next(sourceMenu,&sourceProfile);
            REQUIRE(expanded && expanded->parameters.size()==1 &&
                    expanded->parameters.front().numeric);
            const auto thirdCard=menu_module_card_rect(*expanded,900.0f,2);
            REQUIRE(thirdCard.height>menu_row_body_height(*expanded));
            glClear(GL_COLOR_BUFFER_BIT);
            REQUIRE(draw_menu_gl11(*expanded,sourceWide.gl,sourceWide.dc));
            REQUIRE(glGetError()==GL_NO_ERROR);
            same(wideBefore,snapshot());
            if(const char* output=std::getenv("SLINKYWINKY_R115_900_SETTINGS_PPM"))
                REQUIRE(save_gl_framebuffer_ppm(output,900,630));
            REQUIRE(sourceMenu.select(MenuTab::Profiles));
            const auto profilesFrame=sourceFrames.next(sourceMenu,&sourceProfile);
            REQUIRE(profilesFrame && profilesFrame->profile_cards.size()==1);
            glClear(GL_COLOR_BUFFER_BIT);
            REQUIRE(draw_menu_gl11(*profilesFrame,sourceWide.gl,sourceWide.dc));
            REQUIRE(glGetError()==GL_NO_ERROR);
            same(wideBefore,snapshot());
            REQUIRE(!hit_test_menu(*profilesFrame,
                MenuPanelMode::StandalonePreview,150,82));
            if(const char* output=std::getenv("SLINKYWINKY_R115_900_PROFILES_PPM"))
                REQUIRE(save_gl_framebuffer_ppm(output,900,630));
            // R116: four diverse control-kind fixtures rendered in the
            // *actual* 900px WGL preview, with no game input or module
            // activation. Dropdown/slider/bool, text/buttons, multiselect,
            // and color are all present in recovered source records.
            const auto renderSettings=[&](MenuTab tab,const char* name,
                                          RecoveredWidgetKind expected,
                                          const char* env) {
                REQUIRE(sourceMenu.select(tab));
                const auto& sourceCatalog=official_module_catalog();
                std::size_t catalogIndex=sourceCatalog.size(),ordinal=0,total=0;
                for(std::size_t idx=0;idx<sourceCatalog.size();++idx) {
                    if(sourceCatalog[idx].tab!=tab)continue;
                    if(std::string_view(sourceCatalog[idx].name)==name) {
                        catalogIndex=idx;
                        ordinal=total;
                    }
                    ++total;
                }
                REQUIRE(catalogIndex<sourceCatalog.size());
                const std::size_t maxScroll=total>8?total-8:0;
                const std::size_t desired=std::min(ordinal,maxScroll);
                if(desired)
                    REQUIRE(sourceMenu.scroll_by(static_cast<int>(desired)));
                const auto page=sourceMenu.page();
                bool displayed=false;
                for(const auto& row:page)
                    displayed=displayed||row.catalog_index==catalogIndex;
                REQUIRE(displayed);
                REQUIRE(recovered_widget_count(name)>0);
                std::array<const RecoveredWidget*,5> preview{};
                REQUIRE(recovered_widget_preview(name,preview)>0);
                REQUIRE(preview[0] && preview[0]->kind==expected);
                REQUIRE(sourceMenu.toggle_expanded(catalogIndex));
                const auto snap=sourceFrames.next(sourceMenu,&sourceProfile);
                REQUIRE(snap);
                const auto found=std::find_if(snap->visible.begin(),
                    snap->visible.end(),[&](const MenuItem& row) {
                        return row.catalog_index==catalogIndex;
                    });
                REQUIRE(found!=snap->visible.end() && found->expanded);
                const auto card=menu_module_card_rect(*snap,900.0f,
                    static_cast<std::size_t>(
                        std::distance(snap->visible.begin(),found)));
                REQUIRE(!hit_test_menu(*snap,
                    MenuPanelMode::StandalonePreview,
                    static_cast<int>(card.x+card.width-24.0f),
                    static_cast<int>(card.y+menu_row_body_height(*snap)+27.0f)));
                glClear(GL_COLOR_BUFFER_BIT);
                REQUIRE(draw_menu_gl11(*snap,sourceWide.gl,sourceWide.dc));
                REQUIRE(glGetError()==GL_NO_ERROR);
                same(wideBefore,snapshot());
                if(env && std::getenv(env))
                    REQUIRE(save_gl_framebuffer_ppm(std::getenv(env),900,630));
                // An unsupported module can expose its reference settings
                // for inspection, but still cannot toggle gameplay state.
                if(!module_has_profile_setting(catalogIndex))
                    REQUIRE(!sourceProfile.toggle(catalogIndex));
                REQUIRE(sourceMenu.toggle_expanded(catalogIndex));
            };
            REQUIRE(sourceMenu.select(MenuTab::Combat));
            REQUIRE(sourceMenu.toggle_expanded(2)); // previous R115 demo
            renderSettings(MenuTab::Combat,"Aim Assist",
                RecoveredWidgetKind::Dropdown,"SLINKYWINKY_R116_AIM_PPM");
            {
                std::array<const RecoveredWidget*,5> controls{};
                REQUIRE(recovered_widget_preview("Backtrack",controls)>=3);
                REQUIRE(controls[0] &&
                        controls[0]->kind==RecoveredWidgetKind::Range);
            }
            renderSettings(MenuTab::Combat,"Backtrack",
                RecoveredWidgetKind::Range,"SLINKYWINKY_R116_RANGE_PPM");
            renderSettings(MenuTab::Utility,"Friends",
                RecoveredWidgetKind::Boolean,"SLINKYWINKY_R116_FRIENDS_PPM");
            renderSettings(MenuTab::Combat,"Piercing",
                RecoveredWidgetKind::Dropdown,"SLINKYWINKY_R116_PIERCING_PPM");
            // Retain actual GL multiselect coverage on another source
            // metadata-only module; Piercing is not a multiselect.
            renderSettings(MenuTab::Block,"Ghost Hand",
                RecoveredWidgetKind::MultiSelect,nullptr);
            renderSettings(MenuTab::Visual,"Item ESP",
                RecoveredWidgetKind::Text,"SLINKYWINKY_R116_ITEM_ESP_PPM");
            std::cout<<"R116_TYPED_RECOVERED_WIDGETS_FIVE_REAL_WGL_PREVIEWS_PASS\n";
            // R117: page through every recovered Aim Assist control using
            // the real, first-party, owner-thread native navigation model.
            // All resulting screenshots are from this disposable WGL context.
            REQUIRE(sourceMenu.select(MenuTab::Combat));
            MenuNavigation pageNav(sourceMenu,sourceProfile);
            REQUIRE(pageNav.visible_focus() && *pageNav.visible_focus()==0);
            // The modal input-owner fixture uses the source-verified LWJGL2
            // key codes. No actual Java/Minecraft hook is installed.
            MenuKeyboardRouter pageKeys(pageNav);
            REQUIRE(!pageKeys.begin(118,false,true));
            REQUIRE(!pageKeys.begin(118,true,false));
            REQUIRE(pageKeys.begin(118,true,true));
            auto enter=pageKeys.event(28,true,true,true,true);
            REQUIRE(enter.delivery==MenuKeyDelivery::MenuExclusive &&
                    enter.action_applied);
            REQUIRE(pageKeys.event(28,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            const auto pageCount=recovered_widget_visible_count(
                "Aim Assist",static_cast<int>(sourceMenu.aim_preview_mode()),
                static_cast<int>(sourceMenu.target_preview_mode()));
            REQUIRE(pageCount>10);
            const auto capturePage=[&](const char* env)->MenuRenderFrame {
                const auto frame=sourceFrames.next(
                    sourceMenu,&sourceProfile,&pageNav);
                REQUIRE(frame && menu_settings_offsets_valid(*frame));
                REQUIRE(!frame->visible.empty() &&
                        frame->visible[0].expanded);
                glClear(GL_COLOR_BUFFER_BIT);
                REQUIRE(draw_menu_gl11(*frame,sourceWide.gl,sourceWide.dc));
                REQUIRE(glGetError()==GL_NO_ERROR);
                same(wideBefore,snapshot());
                if(env && std::getenv(env))
                    REQUIRE(save_gl_framebuffer_ppm(std::getenv(env),900,630));
                return *frame;
            };
            const auto pageFirst=capturePage(
                "SLINKYWINKY_R117_FIRST_PAGE_PPM");
            REQUIRE(pageFirst.visible[0].settings_offset==0);
            MenuMouseGesture heldClick;
            REQUIRE(heldClick.press(pageFirst,
                MenuPanelMode::StandalonePreview,40,30,100,true));
            const auto pageDown=pageKeys.event(209,true,true,true,true);
            REQUIRE(pageDown.delivery==MenuKeyDelivery::MenuExclusive &&
                    pageDown.action_applied && !pageDown.request_close);
            REQUIRE(!pageKeys.event(209,true,true,true,true).action_applied);
            REQUIRE(pageKeys.event(209,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            const auto pageSecond=capturePage(
                "SLINKYWINKY_R117_SECOND_PAGE_PPM");
            REQUIRE(pageSecond.visible[0].settings_offset==5);
            REQUIRE(!heldClick.release(pageSecond,
                MenuPanelMode::StandalonePreview,40,30,120,true));
            REQUIRE(!heldClick.pending());
            std::size_t visitedPages=2;
            while(true) {
                const auto pressed=pageKeys.event(209,true,true,true,true);
                REQUIRE(pressed.delivery==MenuKeyDelivery::MenuExclusive);
                REQUIRE(pageKeys.event(209,false,true,true,true).delivery==
                        MenuKeyDelivery::MenuExclusive);
                if(!pressed.action_applied)break;
                ++visitedPages;
                REQUIRE(visitedPages<=100);
            }
            REQUIRE(visitedPages==(pageCount+4)/5);
            const auto pageLast=capturePage(
                "SLINKYWINKY_R117_LAST_PAGE_PPM");
            REQUIRE(pageLast.visible[0].settings_offset==
                    ((pageCount-1)/5)*5);
            REQUIRE(!pageKeys.event(209,true,true,true,true).action_applied);
            REQUIRE(pageKeys.event(209,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            auto tampered=pageLast;
            tampered.visible[0].settings_offset=pageCount+500;
            REQUIRE(!draw_menu_gl11(tampered,sourceWide.gl,sourceWide.dc));
            REQUIRE(!hit_test_menu(tampered,
                MenuPanelMode::StandalonePreview,40,30));
            tampered=pageLast;
            tampered.visible[0].settings_offset=7;
            REQUIRE(!draw_menu_gl11(tampered,sourceWide.gl,sourceWide.dc));
            REQUIRE(!hit_test_menu(tampered,
                MenuPanelMode::StandalonePreview,40,30));
            const auto up=pageKeys.event(201,true,true,true,true);
            REQUIRE(up.delivery==MenuKeyDelivery::MenuExclusive &&
                    up.action_applied);
            REQUIRE(pageKeys.event(201,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            REQUIRE(sourceMenu.settings_offset(0)==
                    pageLast.visible[0].settings_offset-5);
            REQUIRE(pageKeys.event(28,true,true,true,true).action_applied);
            REQUIRE(pageKeys.event(28,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            REQUIRE(sourceMenu.settings_offset(0)==0);
            REQUIRE(!pageKeys.event(209,true,true,true,true).action_applied);
            REQUIRE(pageKeys.event(209,false,true,true,true).delivery==
                    MenuKeyDelivery::MenuExclusive);
            const auto close=pageKeys.event(1,true,true,true,true);
            REQUIRE(close.delivery==MenuKeyDelivery::MenuExclusive &&
                    close.request_close && !close.action_applied);
            REQUIRE(pageKeys.end());
            REQUIRE(pageKeys.event(1,false,true,true,false).delivery==
                    MenuKeyDelivery::Quarantined);
            REQUIRE(!pageKeys.active() && !pageKeys.revoked());
            // R123: render every Aim Assist branch in the ACTUAL WGL
            // context. These transitions are owner-scoped preview-only
            // navigation, not a live Minecraft keyboard hook.
            REQUIRE(pageNav.apply(MenuNavAction::ExpandFocused));
            const auto regular=capturePage(nullptr);
            REQUIRE(regular.aim_preview_mode==0);
            REQUIRE(recovered_widget_visible_count("Aim Assist",0,0)==18);
            REQUIRE(pageNav.apply(MenuNavAction::NextSettingsPage));
            REQUIRE(sourceMenu.settings_offset(0)==5);
            MenuMouseGesture modeHeld;
            const auto beforeMode=capturePage(nullptr);
            REQUIRE(modeHeld.press(beforeMode,
                MenuPanelMode::StandalonePreview,40,30,300,true));
            REQUIRE(pageNav.apply(MenuNavAction::NextAimPreviewMode));
            const auto linear=capturePage(nullptr);
            REQUIRE(linear.aim_preview_mode==1 &&
                    linear.visible[0].settings_offset==0);
            REQUIRE(!modeHeld.release(linear,
                MenuPanelMode::StandalonePreview,40,30,310,true));
            REQUIRE(pageNav.apply(MenuNavAction::NextAimPreviewMode));
            const auto locked=capturePage("SLINKYWINKY_R123_LOCKON_PPM");
            REQUIRE(locked.aim_preview_mode==2 &&
                    recovered_widget_visible_count("Aim Assist",2,0)==17);
            REQUIRE(pageNav.apply(MenuNavAction::NextAimPreviewMode));
            const auto silent=capturePage("SLINKYWINKY_R123_SILENT_PPM");
            REQUIRE(silent.aim_preview_mode==3 &&
                    recovered_widget_visible_count("Aim Assist",3,0)==19);
            REQUIRE(pageNav.apply(MenuNavAction::NextAimPreviewMode));
            REQUIRE(capturePage(nullptr).aim_preview_mode==0);
            REQUIRE(pageNav.apply(MenuNavAction::NextTargetPreviewMode));
            const auto switched=capturePage("SLINKYWINKY_R123_SWITCH_PPM");
            REQUIRE(switched.target_preview_mode==1);
            REQUIRE(pageNav.apply(MenuNavAction::NextTargetPreviewMode));
            REQUIRE(capturePage(nullptr).target_preview_mode==0);
            tampered=switched;
            tampered.aim_preview_mode=99;
            REQUIRE(!draw_menu_gl11(tampered,sourceWide.gl,sourceWide.dc));
            REQUIRE(!hit_test_menu(tampered,
                MenuPanelMode::StandalonePreview,40,30));
            tampered=switched;
            tampered.target_preview_mode=99;
            REQUIRE(!draw_menu_gl11(tampered,sourceWide.gl,sourceWide.dc));
            REQUIRE(!hit_test_menu(tampered,
                MenuPanelMode::StandalonePreview,40,30));
            REQUIRE(pageNav.apply(MenuNavAction::ExpandFocused));
            REQUIRE(sourceMenu.settings_offset(0)==0);
            std::cout<<"R123_EIGHT_SOURCE_CONDITIONAL_AIM_WGL_STATES_PASS\n";
            std::cout<<"R123_STALE_MODAL_SELECTION_AND_FORGED_FRAME_DENIAL_PASS\n";
            // R124: a real isolated WGL context paints first-party
            // Piercing preview switches and dropdown mutations. No game hook.
            REQUIRE(sourceMenu.scroll_by(4));
            REQUIRE(pageNav.focus_visible_row(11));
            REQUIRE(pageNav.apply(MenuNavAction::ExpandFocused));
            const auto piercingStart=sourceFrames.next(
                sourceMenu,&sourceProfile,&pageNav);
            REQUIRE(piercingStart && piercingStart->piercing_preview.sort==0);
            REQUIRE(draw_menu_gl11(*piercingStart,
                sourceWide.gl,sourceWide.dc));
            same(wideBefore,snapshot());
            REQUIRE(pageNav.apply(MenuNavAction::ActivatePiercingControl));
            const auto piercingChanged=sourceFrames.next(
                sourceMenu,&sourceProfile,&pageNav);
            REQUIRE(piercingChanged && piercingChanged->piercing_preview.sort==1);
            REQUIRE(piercingStart->piercing_preview.sort==0);
            REQUIRE(draw_menu_gl11(*piercingChanged,
                sourceWide.gl,sourceWide.dc));
            same(wideBefore,snapshot());
            REQUIRE(pageNav.apply(MenuNavAction::NextPiercingControl));
            REQUIRE(pageNav.apply(MenuNavAction::ActivatePiercingControl));
            const auto toggled=sourceFrames.next(
                sourceMenu,&sourceProfile,&pageNav);
            REQUIRE(toggled && toggled->piercing_selected==1 &&
                    toggled->piercing_preview.enabled[0]);
            REQUIRE(draw_menu_gl11(*toggled,
                sourceWide.gl,sourceWide.dc));
            same(wideBefore,snapshot());
            auto forgedPiercing=*toggled;
            forgedPiercing.piercing_preview.sort=2;
            REQUIRE(!draw_menu_gl11(forgedPiercing,
                sourceWide.gl,sourceWide.dc));
            REQUIRE(!hit_test_menu(forgedPiercing,
                MenuPanelMode::StandalonePreview,40,30));
            std::cout<<"R124_ISOLATED_PIERCING_EDIT_RENDER_AND_GL_RESTORE_PASS\n";
            std::cout<<"R118_SYNTHETIC_LWJGL2_PAGE_KEYS_TO_REAL_WGL_PAGES_PASS\n";
            std::cout<<"R117_ALL_RECOVERED_AIM_SETTINGS_PAGED_AND_WGL_RESTORED_PASS\n";
            std::cout<<"R117_STALE_MOUSE_AND_FORGED_PAGE_REFUSAL_PASS\n";
            REQUIRE(sourceFrames.detach());
        }
        REQUIRE(wglMakeCurrent(context.dc,context.gl));
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        std::cout<<"R115_ORIGINAL_RECOVERED_900PX_TWO_COLUMN_WGL_PASS\\n";
        std::cout<<"R115_NATIVE_SOURCE_SLIDERS_AND_PROFILE_GRID_PREVIEW_PASS\\n";
        std::cout<<"R104_REAL_WGL_WINDOW_FOCUS_FENCE_NEVER_CAPTURES_GAME_INPUT_PASS\\n";
        std::cout<<"R101_READ_ONLY_WGL_BOUND_GAME_WINDOW_IDENTITY_NEGATIVE_CONTROLS_PASS\\n";
        std::cout<<"R172_ORIGINAL_WINDOW_PASSIVE_POINTER_ONLY_PASS"<<std::endl;
        std::cout<<"R188_BRACKETED_FOCUS_CURSOR_READS_FAIL_CLOSED_PASS"<<std::endl;
        std::cout<<"R189_ALL_WIN32_MOUSE_BUTTON_MASKS_PASS"<<std::endl;
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
