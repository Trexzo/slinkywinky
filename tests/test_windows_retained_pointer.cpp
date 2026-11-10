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
#include "slinkywinky/windows_menu_focus_fence.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_expander_geometry.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
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
// Keep the additional retained frames in their OWN stack frame:
 // this legacy Windows test has a 1 MB stack and hundreds of GL fixtures.
void verify_r159_pointer_admission(Context& context,const State& r99Before) {
        // R159: actual retained WGL menu owns its mouse transaction and
        // source-indexed configuration, without any original Minecraft
        // input hook. The disposable owner is the only event producer.
        {
            WglRetainedMenuPreview pointerMenu;
            constexpr auto down=RetainedPointerPhase::Down;
            constexpr auto up=RetainedPointerPhase::Up;
            constexpr auto move=RetainedPointerPhase::Move;
            constexpr auto cancel=RetainedPointerPhase::Cancel;
            REQUIRE(!pointerMenu.pointer(down,context.gl,context.dc,676,630,
                                         1,1,1000,true,true));
            REQUIRE(pointerMenu.draw(context.gl,context.dc,676,630));
            const auto panel=menu_panel_geometry(
                676,630,false,MenuPanelMode::CompactOverlay);
            REQUIRE(panel);
            auto px=[&](float logical) {
                return static_cast<int>(panel->left)+static_cast<int>(
                    std::lround(logical*panel->scale));
            };
            auto py=[&](float logical) {
                return static_cast<int>(panel->top)+static_cast<int>(
                    std::lround(logical*panel->scale));
            };
            MenuState mirror;
            MenuProfileBridge mirrorProfile(mirror);
            MenuRenderSession reference;
            REQUIRE(reference.attach(159,676,630));
            const auto refFrame=reference.next(mirror,&mirrorProfile);
            REQUIRE(refFrame);
            const auto first=menu_module_card_rect(
                *refFrame,panel->logical_width(),0);
            const int x=px(first.x+first.width-30.0f);
            const int y=py(first.y+17.0f);
            const auto toggle=hit_test_menu(*refFrame,
                MenuPanelMode::CompactOverlay,x,y);
            REQUIRE(toggle && toggle->kind==MenuHitKind::Toggle &&
                    toggle->catalog_index==0u);
            REQUIRE(!pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,1000,false,true));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,1010,true,true));
            REQUIRE(!pointerMenu.pointer(
                down,context.gl,context.dc,677,630,x,y,1020,true,true));
            REQUIRE(!pointerMenu.pointer(
                down,nullptr,context.dc,676,630,x,y,1025,true,true));
            REQUIRE(!pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,1030,true,false));
            REQUIRE(!pointerMenu.preview_enabled(0));
            // R186: wrong WGL size/context revoked the previous paint.
            // The previous test's later successful Down now needs repaint.
            REQUIRE(pointerMenu.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,2000,true,true));
            REQUIRE(pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,2040,true,true));
            REQUIRE(pointerMenu.preview_enabled(0));
            const auto changedPointerProfile=pointerMenu.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(changedPointerProfile &&
                parse_settings(*changedPointerProfile).settings.combat.aim_enabled);
            // R184: the earlier click invalidated painted geometry. A
            // second synthetic click is rejected until a real repaint.
            REQUIRE(!pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,2080,true,true));
            REQUIRE(pointerMenu.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,2100,true,true));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,2140,true,true));
            REQUIRE(pointerMenu.preview_enabled(0)); // double-click suppress
            REQUIRE(pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,2600,true,true));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,677,630,x,y,2610,true,true));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,2620,true,true));
            REQUIRE(pointerMenu.preview_enabled(0)); // stale viewport abort
            REQUIRE(pointerMenu.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,3000,true,true));
            REQUIRE(pointerMenu.pointer(
                cancel,context.gl,context.dc,676,630,x,y,3010,false,false));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,3020,true,true));
            REQUIRE(pointerMenu.preview_enabled(0));
            REQUIRE(pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,3200,true,true));
            REQUIRE(!pointerMenu.pointer(
                move,context.gl,context.dc,676,630,-20,-20,3210,true,true));
            REQUIRE(!pointerMenu.pointer(
                up,context.gl,context.dc,676,630,x,y,3220,true,true));
            REQUIRE(pointerMenu.preview_enabled(0));
            bool foreignPointerAccepted=false;
            std::thread foreignPointer([&] {
                foreignPointerAccepted=pointerMenu.pointer(
                    down,context.gl,context.dc,676,630,x,y,3400,true,true);
            });
            foreignPointer.join();
            REQUIRE(!foreignPointerAccepted);
            REQUIRE(pointerMenu.draw(context.gl,context.dc,676,630));
            REQUIRE(glGetError()==GL_NO_ERROR);
            same(r99Before,snapshot());
            REQUIRE(pointerMenu.close());
            REQUIRE(!pointerMenu.pointer(
                down,context.gl,context.dc,676,630,x,y,3700,true,true));
            REQUIRE(reference.detach());
        }
        {
            // Exact source slider control -> validated native Auto Block
            // profile, with no game input or gameplay action.
            WglRetainedMenuPreview pointerNumeric;
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.apply(MenuNavAction::NextRow));
            REQUIRE(pointerNumeric.apply(MenuNavAction::ExpandFocused));
            MenuState mirror;
            REQUIRE(mirror.toggle_expanded(1));
            MenuProfileBridge mirroredProfile(mirror);
            MenuRenderSession reference;
            REQUIRE(reference.attach(160,676,630));
            const auto f=reference.next(mirror,&mirroredProfile);
            REQUIRE(f);
            const auto panel=menu_panel_geometry(
                676,630,false,MenuPanelMode::CompactOverlay);
            REQUIRE(panel);
            std::size_t widget=recovered_widget_catalog().size();
            const auto& source=recovered_widget_catalog();
            for(std::size_t i=0;i<source.size();++i)
                if(std::string_view(source[i].module)=="Auto Block" &&
                   std::string_view(source[i].label)=="Range" &&
                   source[i].kind==RecoveredWidgetKind::Slider)widget=i;
            REQUIRE(widget<source.size());
            const auto track=menu_numeric_track_geometry(
                *f,panel->logical_width(),panel->logical_height(),1u,widget);
            REQUIRE(track);
            const auto px=[&](float logical) {
                return static_cast<int>(panel->left)+static_cast<int>(
                    std::lround(logical*panel->scale));
            };
            const auto py=[&](float logical) {
                return static_cast<int>(panel->top)+static_cast<int>(
                    std::lround(logical*panel->scale));
            };
            const int x=px(track->x+track->width*0.5f);
            const int y=py(track->y+10.0f);
            const auto hit=hit_test_menu(*f,MenuPanelMode::CompactOverlay,x,y);
            REQUIRE(hit && hit->kind==MenuHitKind::RecoveredSliderPreview &&
                    hit->control_index==widget);
            // R184: NextRow/ExpandFocused are only local model actions.
            // Do not accept an input target before this panel is painted.
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,x,y,3980,true,true));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            const auto paintedTarget=pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630);
            REQUIRE(paintedTarget && paintedTarget->catalog_index==1u &&
                    paintedTarget->widget_index==widget);
            // R185: a FOREIGN renderer cannot revoke the real WGL owner.
            bool foreignDrawAdmitted=true;
            std::thread foreignDraw([&] {
                foreignDrawAdmitted=pointerNumeric.draw(
                    context.gl,context.dc,676,630);
            });
            foreignDraw.join();
            REQUIRE(!foreignDrawAdmitted);
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            // A same-owner refused viewport paint invalidates the prior
            // hit surface; restoring the old viewport alone is not enough.
            REQUIRE(!pointerNumeric.draw(context.gl,context.dc,677,630));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            // A passive cursor sample after a failed paint remains EMPTY;
            // it cannot turn the rejected last frame into a source hit.
            REQUIRE(pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,true));
            // Synthetic OS status is confined to the disposable WGL test.
            REQUIRE(pointerNumeric.observe_original_window_focus(
                context.gl,context.dc,676,630,
                OriginalWindowFocusState::Foreground,true));
            REQUIRE(!pointerNumeric.observed_numeric_hover());
            const auto rejectedHover=pointerNumeric.source_hover_evidence(
                context.gl,context.dc,676,630);
            REQUIRE(rejectedHover && (*rejectedHover)[2]==0u);
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,3800,true,true));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,4000,true,true));
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,4050,true,true));
            const auto saved=pointerNumeric.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(saved && parse_settings(*saved).valid);
            const float configured=parse_settings(*saved).settings.combat.block_range;
            REQUIRE(configured>3.1f && configured<6.01f);
            // R185: failed paint DURING an already held press must cancel
            // its eventual Up. The previous source frame cannot return.
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,false));
            REQUIRE(pointerNumeric.observe_original_window_focus(
                context.gl,context.dc,676,630,
                OriginalWindowFocusState::Background,false));
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,5000,true,true));
            REQUIRE(!pointerNumeric.draw(context.gl,nullptr,676,630));
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,5040,true,true));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            REQUIRE(pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,true));
            REQUIRE(pointerNumeric.observe_original_window_focus(
                context.gl,context.dc,676,630,
                OriginalWindowFocusState::Foreground,true));
            const auto osAfterRefusal=pointerNumeric.original_window_focus_evidence(
                context.gl,context.dc,676,630);
            const auto admissionAfterRefusal=pointerNumeric.focus_cycle_evidence(
                context.gl,context.dc,676,630);
            REQUIRE(osAfterRefusal && (*osAfterRefusal)[0]==2u &&
                    (*osAfterRefusal)[1]==1u && (*osAfterRefusal)[2]==0u &&
                    (*osAfterRefusal)[3]==1u && (*osAfterRefusal)[4]==0u &&
                    (*osAfterRefusal)[5]==0u);
            REQUIRE(admissionAfterRefusal && (*admissionAfterRefusal)[0]==1u &&
                    (*admissionAfterRefusal)[1]==0u &&
                    (*admissionAfterRefusal)[2]==0u);
            const auto afterRefusal=pointerNumeric.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(afterRefusal && *saved==*afterRefusal);
            // A GL viewport mismatch (not just a caller-supplied size)
            // is equally terminal for the previous painted hit geometry.
            glViewport(0,0,650,610);
            REQUIRE(!pointerNumeric.draw(context.gl,context.dc,676,630));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,5500,true,true));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            // R186: the real GL viewport can drift AFTER a successful
            // paint, without another draw or a changed WGL context/DC.
            // Source hover must fail closed and stay revoked when viewport
            // dimensions alone are subsequently restored.
            glViewport(4,0,676,630);
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());

            // A passive physical hover cannot count a numeric/source hit
            // while the actual viewport is different from the WGL draw.
            glViewport(0,0,675,630);
            REQUIRE(!pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,true));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.observed_numeric_hover());
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());

            // A first mouse Down with a drifted real viewport cannot claim
            // old geometry, even when the caller passes the correct size.
            glViewport(0,0,676,629);
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,6000,true,true));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,6050,true,true));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());

            // A mid-transaction viewport mutation cancels an already
            // admitted press. The subsequent Up cannot alter settings,
            // even after the caller restores the previous viewport.
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,6500,true,true));
            glViewport(0,0,674,630);
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,6550,true,true));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,6580,true,true));
            const auto afterDrift=pointerNumeric.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(afterDrift && *saved==*afterDrift);
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            // R187: a wrong-thread passive hover is denied but must never
            // mutate the legitimate pinned renderer's painted source.
            bool foreignHoverAdmitted=true;
            std::thread foreignHover([&] {
                foreignHoverAdmitted=pointerNumeric.observe_hover(
                    context.gl,context.dc,676,630,
                    paintedTarget->center_x,paintedTarget->center_y,true);
            });
            foreignHover.join();
            REQUIRE(!foreignHoverAdmitted);
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());

            // Actual WGL context loss without a menu redraw: the passive
            // target query must revoke the last paint, not merely return
            // null and let the old geometry reappear after rebind.
            REQUIRE(wglMakeCurrent(nullptr,nullptr));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(wglMakeCurrent(context.dc,context.gl));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,7100,true,true));
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());

            // A held press followed by real context loss via a passive
            // read must cancel the gesture even though pending() normally
            // denies passive hover observations. After rebind, Up fails.
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,7500,true,true));
            REQUIRE(wglMakeCurrent(nullptr,nullptr));
            REQUIRE(!pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,
                paintedTarget->center_x,paintedTarget->center_y,true));
            REQUIRE(wglMakeCurrent(context.dc,context.gl));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,7540,true,true));
            const auto afterWglLoss=pointerNumeric.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(afterWglLoss && *saved==*afterWglLoss);
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            // R187: pending() must not hide a passive OBSERVED viewport
            // change. That hover is refused and the press is canceled.
            REQUIRE(pointerNumeric.pointer(RetainedPointerPhase::Down,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,8000,true,true));
            glViewport(0,0,675,630);
            REQUIRE(!pointerNumeric.observe_hover(
                context.gl,context.dc,676,630,
                paintedTarget->center_x,paintedTarget->center_y,true));
            glViewport(0,0,676,630);
            REQUIRE(!pointerNumeric.pointer(RetainedPointerPhase::Up,
                context.gl,context.dc,676,630,paintedTarget->center_x,
                paintedTarget->center_y,8040,true,true));
            REQUIRE(!pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630));
            const auto afterPendingViewport=pointerNumeric.profile_snapshot(
                context.gl,context.dc,676,630);
            REQUIRE(afterPendingViewport && *saved==*afterPendingViewport);
            REQUIRE(pointerNumeric.draw(context.gl,context.dc,676,630));
            REQUIRE(pointerNumeric.source_hover_target(
                context.gl,context.dc,676,630).has_value());
            REQUIRE(pointerNumeric.close());
            // R189: held Win32 mouse buttons can still coexist with an
            // independently confirmed foreground window, but may NOT
            // certify a source slider hover. This fixture uses explicit
            // synthetic released/held flags; it never presses an OS button.
            {
                WglRetainedMenuPreview quietHover;
                REQUIRE(quietHover.draw(context.gl,context.dc,676,630));
                REQUIRE(quietHover.apply(MenuNavAction::NextRow));
                REQUIRE(quietHover.apply(MenuNavAction::ExpandFocused));
                REQUIRE(quietHover.draw(context.gl,context.dc,676,630));
                const auto target=quietHover.source_hover_target(
                    context.gl,context.dc,676,630);
                REQUIRE(target);
                for(unsigned mask=1u;mask<32u;++mask) {
                    REQUIRE(quietHover.observe_hover(
                        context.gl,context.dc,676,630,
                        target->center_x,target->center_y,true,false));
                    REQUIRE(quietHover.observe_original_window_focus(
                        context.gl,context.dc,676,630,
                        OriginalWindowFocusState::Foreground,true));
                    REQUIRE(!quietHover.observed_numeric_hover());
                    const auto evidence=quietHover.source_hover_evidence(
                        context.gl,context.dc,676,630);
                    REQUIRE(evidence && (*evidence)[2]==0u);
                }
                const auto held=quietHover.hover_evidence(
                    context.gl,context.dc,676,630);
                // [draws, samples, foreground, numeric, bind]: held
                // buttons preserve 31 valid foreground samples, but zero
                // numeric hits (and zero exact source hits as checked).
                REQUIRE(held && (*held)[1]==31u && (*held)[2]==31u &&
                        (*held)[3]==0u && (*held)[4]==0u);
                REQUIRE(quietHover.observe_hover(
                    context.gl,context.dc,676,630,
                    target->center_x,target->center_y,true,true));
                REQUIRE(quietHover.observe_original_window_focus(
                    context.gl,context.dc,676,630,
                    OriginalWindowFocusState::Foreground,true));
                REQUIRE(quietHover.observed_numeric_hover());
                const auto accepted=quietHover.source_hover_evidence(
                    context.gl,context.dc,676,630);
                REQUIRE(accepted && (*accepted)[2]==1u);
                REQUIRE(quietHover.observe_hover(
                    context.gl,context.dc,676,630,
                    target->center_x,target->center_y,true,false));
                REQUIRE(quietHover.observe_original_window_focus(
                    context.gl,context.dc,676,630,
                    OriginalWindowFocusState::Foreground,true));
                REQUIRE(!quietHover.observed_numeric_hover());
                const auto afterHold=quietHover.source_hover_evidence(
                    context.gl,context.dc,676,630);
                REQUIRE(afterHold && (*afterHold)[2]==1u);
                const auto os=quietHover.original_window_focus_evidence(
                    context.gl,context.dc,676,630);
                REQUIRE(os && (*os)[0]==33u && (*os)[1]==0u &&
                        (*os)[2]==0u);
                REQUIRE(quietHover.close());
            }
            // R187: even a separate OS-focus sample MUST NOT pair with a
            // passive hover from before a WGL binding or viewport loss.
            // These are synthetic native fixture states, not real OS input.
            {
                WglRetainedMenuPreview pairedFocus;
                REQUIRE(pairedFocus.draw(context.gl,context.dc,676,630));
                REQUIRE(pairedFocus.observe_hover(
                    context.gl,context.dc,676,630,0,0,false));
                REQUIRE(wglMakeCurrent(nullptr,nullptr));
                REQUIRE(!pairedFocus.observe_original_window_focus(
                    context.gl,context.dc,676,630,
                    OriginalWindowFocusState::Background,false));
                REQUIRE(wglMakeCurrent(context.dc,context.gl));
                REQUIRE(!pairedFocus.original_window_focus_evidence(
                    context.gl,context.dc,676,630));
                REQUIRE(pairedFocus.draw(context.gl,context.dc,676,630));
                REQUIRE(!pairedFocus.original_window_focus_evidence(
                    context.gl,context.dc,676,630));
                REQUIRE(pairedFocus.close());
            }
            {
                WglRetainedMenuPreview pairedViewport;
                REQUIRE(pairedViewport.draw(context.gl,context.dc,676,630));
                REQUIRE(pairedViewport.observe_hover(
                    context.gl,context.dc,676,630,0,0,false));
                glViewport(0,0,675,630);
                REQUIRE(!pairedViewport.observe_original_window_focus(
                    context.gl,context.dc,676,630,
                    OriginalWindowFocusState::Background,false));
                glViewport(0,0,676,630);
                REQUIRE(!pairedViewport.original_window_focus_evidence(
                    context.gl,context.dc,676,630));
                REQUIRE(pairedViewport.draw(context.gl,context.dc,676,630));
                REQUIRE(pairedViewport.close());
            }
            // R191: the SAME native retained WGL menu now accepts only
            // synthetic keyboard edges from the R190 fixture handoff
            // ledger. The first real keyboard action changes its rendered
            // tab, invalidates painted input and requires another GL draw.
            {
                WglRetainedMenuPreview keyboardMenu;
                MenuInputHandoffLedger keyLedger;
                MenuKeyboardRouter actualRouter(keyboardMenu.fixture_navigation());
                REQUIRE(keyboardMenu.draw(context.gl,context.dc,676,630));
                const auto oldProfile=keyboardMenu.profile_snapshot(
                    context.gl,context.dc,676,630);
                REQUIRE(oldProfile);
                REQUIRE(!keyboardMenu.fixture_begin_keyboard(191,keyLedger,actualRouter));
                REQUIRE(keyLedger.request(191,true,true,true,true));
                REQUIRE(!keyboardMenu.fixture_begin_keyboard(191,keyLedger,actualRouter));
                REQUIRE(keyLedger.confirm_fixture(191,true,true,true));
                // Two routers on the same owner thread are not equivalent:
                // the fixture must route to THIS retained menu's navigation.
                MenuState wrongMenu;
                MenuProfileBridge wrongProfile(wrongMenu);
                MenuNavigation wrongNav(wrongMenu,wrongProfile);
                MenuKeyboardRouter wrongRouter(wrongNav);
                REQUIRE(!keyboardMenu.fixture_begin_keyboard(
                    191,keyLedger,wrongRouter));
                REQUIRE(keyboardMenu.fixture_begin_keyboard(191,keyLedger,actualRouter));
                bool foreignKeyAccepted=false;
                std::thread foreignKey([&] {
                    foreignKeyAccepted=keyboardMenu.fixture_keyboard_event(
                        context.gl,context.dc,676,630,191,keyLedger,actualRouter,
                        15,true,true).delivery==MenuKeyDelivery::MenuExclusive;
                });
                foreignKey.join();
                REQUIRE(!foreignKeyAccepted);
                REQUIRE(keyboardMenu.tab()==MenuTab::Combat);
                const auto tabKey=keyboardMenu.fixture_keyboard_event(
                    context.gl,context.dc,676,630,191,keyLedger,actualRouter,
                    15,true,true);
                REQUIRE(tabKey.delivery==MenuKeyDelivery::MenuExclusive &&
                        tabKey.action_applied &&
                        keyboardMenu.tab()==MenuTab::Move);
                REQUIRE(!keyboardMenu.pointer(
                    RetainedPointerPhase::Down,context.gl,context.dc,
                    676,630,300,200,192000,true,true));
                REQUIRE(keyboardMenu.fixture_keyboard_event(
                    context.gl,context.dc,676,630,191,keyLedger,actualRouter,
                    15,false,true).delivery==MenuKeyDelivery::MenuExclusive);
                REQUIRE(keyLedger.tracked_released());
                REQUIRE(keyboardMenu.draw(context.gl,context.dc,676,630));
                const auto afterKeyboard=keyboardMenu.profile_snapshot(
                    context.gl,context.dc,676,630);
                REQUIRE(afterKeyboard && *afterKeyboard==*oldProfile);
                REQUIRE(keyboardMenu.fixture_end_keyboard(keyLedger,actualRouter));
                REQUIRE(keyLedger.phase()==InputHandoffPhase::Draining);
                REQUIRE(keyboardMenu.fixture_keyboard_event(
                    context.gl,context.dc,676,630,191,keyLedger,actualRouter,
                    15,false,false).delivery==MenuKeyDelivery::Quarantined);
                REQUIRE(!keyLedger.restore(true,true,false));
                REQUIRE(keyLedger.restore(true,true,true));
                REQUIRE(!keyboardMenu.fixture_begin_keyboard(191,keyLedger,actualRouter));
                // New epoch, real WGL viewport loss during a held key:
                // fixture session is revoked, geometry remains invalid,
                // and no key is injected into the original game.
                REQUIRE(keyboardMenu.draw(context.gl,context.dc,676,630));
                REQUIRE(keyLedger.request(192,true,true,true,true));
                REQUIRE(keyLedger.confirm_fixture(192,true,true,true));
                REQUIRE(keyboardMenu.fixture_begin_keyboard(192,keyLedger,actualRouter));
                REQUIRE(keyboardMenu.fixture_keyboard_event(
                    context.gl,context.dc,676,630,192,keyLedger,actualRouter,
                    17,true,true).delivery==MenuKeyDelivery::MenuExclusive);
                REQUIRE(!keyLedger.tracked_released());
                glViewport(0,0,675,630);
                REQUIRE(keyboardMenu.fixture_keyboard_event(
                    context.gl,context.dc,676,630,192,keyLedger,actualRouter,
                    17,false,true).delivery==MenuKeyDelivery::Quarantined);
                glViewport(0,0,676,630);
                REQUIRE(keyLedger.phase()==InputHandoffPhase::Revoked);
                REQUIRE(!keyboardMenu.fixture_end_keyboard(keyLedger,actualRouter));
                REQUIRE(!keyboardMenu.pointer(
                    RetainedPointerPhase::Down,context.gl,context.dc,
                    676,630,300,200,192500,true,true));
                REQUIRE(!keyLedger.restore(true,false,true));
                REQUIRE(keyLedger.restore(true,true,true));
                REQUIRE(keyboardMenu.close());
                REQUIRE(!keyLedger.may_deliver_live_minecraft_event());
                REQUIRE(!keyLedger.may_suppress_vanilla_event());
            }
            REQUIRE(reference.detach());
            same(r99Before,snapshot());
        }
        std::cout<<"R159_WGL_RETAINED_POINTER_EXCLUSIVE_GESTURES_AND_PROFILE_PASS\\n";
        std::cout<<"R185_WGL_OWNER_RENDER_REFUSAL_INVALIDATES_PAINT_AND_PRESS_PASS\\n";
        std::cout<<"R186_POST_PAINT_VIEWPORT_DRIFT_FAIL_CLOSED_PASS\\n";
        std::cout<<"R187_WGL_BINDING_LOSS_REQUIRES_REPAINT_AND_BREAKS_OS_PAIR_PASS\\n";
        std::cout<<"R189_HELD_BUTTON_NEVER_CERTIFIES_SOURCE_HOVER_PASS\\n";
        std::cout<<"R191_RETAINED_WGL_KEYBOARD_HANDOFF_NAVIGATION_REPAINT_PASS\\n";

}

}

int main() {
    try {
        Context context(676,630);
        glViewport(0,0,676,630);
        glClearColor(0.10f,0.38f,0.69f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        REQUIRE(glGetError()==GL_NO_ERROR);
        const State before=snapshot();
        verify_r159_pointer_admission(context,before);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"R159 retained pointer fixture failure: "<<e.what()<<"\n";
        return 1;
    }
}
