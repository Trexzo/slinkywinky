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
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
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

// R192: release-ordered synthetic mouse Down/Up and keyboard navigation
// actually mutate the same retained GL11 GUI, never a Minecraft event queue.
// Kept in its own frame because the Windows test stack is 1 MB.
void verify_r192_joint_handoff(Context& context,const State& expected) {
    auto coords=[]() {
        MenuState mirror;
        MenuProfileBridge profile(mirror);
        MenuRenderSession frames;
        REQUIRE(frames.attach(192,676,630));
        const auto frame=frames.next(mirror,&profile);
        REQUIRE(frame);
        const auto panel=menu_panel_geometry(
            676,630,false,MenuPanelMode::CompactOverlay);
        REQUIRE(panel);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),0);
        const int x=static_cast<int>(panel->left)+
            static_cast<int>(std::lround(
                (card.x+card.width-30.0f)*panel->scale));
        const int y=static_cast<int>(panel->top)+
            static_cast<int>(std::lround(
                (card.y+17.0f)*panel->scale));
        const auto hit=hit_test_menu(
            *frame,MenuPanelMode::CompactOverlay,x,y);
        REQUIRE(hit && hit->kind==MenuHitKind::Toggle &&
                hit->catalog_index==0u);
        REQUIRE(frames.detach());
        return std::array<int,2>{x,y};
    }();
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    const int x=coords[0],y=coords[1];
    {
        WglRetainedMenuPreview nativeMenu;
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(nativeMenu.fixture_navigation());
        REQUIRE(nativeMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(!nativeMenu.preview_enabled(0));
        REQUIRE(ledger.request(1920,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1920,true,true,true));
        REQUIRE(nativeMenu.fixture_begin_keyboard(1920,ledger,router));
        MenuState wrongMenu;
        MenuProfileBridge wrongProfile(wrongMenu);
        MenuNavigation wrongNav(wrongMenu,wrongProfile);
        MenuKeyboardRouter wrongRouter(wrongNav);
        REQUIRE(!nativeMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1920,
            ledger,wrongRouter,x,y,9990,true));
        REQUIRE(ledger.phase()==InputHandoffPhase::MenuFixture);
        REQUIRE(!ledger.tracked_down(InputHandoffDevice::Mouse,0));
        REQUIRE(nativeMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1920,
            ledger,router,x,y,10000,true));
        REQUIRE(ledger.tracked_down(InputHandoffDevice::Mouse,0));
        REQUIRE(!ledger.tracked_released());
        REQUIRE(nativeMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1920,
            ledger,router,x,y,10040,true));
        REQUIRE(!ledger.tracked_down(InputHandoffDevice::Mouse,0));
        REQUIRE(ledger.tracked_released());
        REQUIRE(nativeMenu.preview_enabled(0));
        const auto profile=nativeMenu.profile_snapshot(
            context.gl,context.dc,676,630);
        REQUIRE(profile && parse_settings(*profile).valid);
        REQUIRE(parse_settings(*profile).settings.combat.aim_enabled);
        // A successful click invalidates the last drawn frame, then a
        // fresh physical GL11 repaint permits the next keyboard action.
        REQUIRE(nativeMenu.draw(context.gl,context.dc,676,630));
        const auto tab=nativeMenu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1920,
            ledger,router,15,true,true);
        REQUIRE(tab.delivery==MenuKeyDelivery::MenuExclusive &&
                tab.action_applied && nativeMenu.tab()==MenuTab::Move);
        REQUIRE(ledger.tracked_down(InputHandoffDevice::Keyboard,15));
        REQUIRE(nativeMenu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1920,
            ledger,router,15,false,true).delivery==
            MenuKeyDelivery::MenuExclusive);
        REQUIRE(ledger.tracked_released());
        REQUIRE(nativeMenu.draw(context.gl,context.dc,676,630));
        const auto finalProfile=nativeMenu.profile_snapshot(
            context.gl,context.dc,676,630);
        REQUIRE(finalProfile && *profile==*finalProfile);
        REQUIRE(nativeMenu.fixture_end_keyboard(ledger,router));
        REQUIRE(ledger.phase()==InputHandoffPhase::Draining);
        REQUIRE(!ledger.restore(true,true,false));
        REQUIRE(ledger.restore(true,true,true));
        REQUIRE(nativeMenu.close());
        REQUIRE(!ledger.may_deliver_live_minecraft_event());
        REQUIRE(!ledger.may_suppress_vanilla_event());
    }
    {
        // An orphan Up is quarantined and revokes, not counted as a click.
        WglRetainedMenuPreview orphanMenu;
        MenuInputHandoffLedger orphan;
        MenuKeyboardRouter router(orphanMenu.fixture_navigation());
        REQUIRE(orphanMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(orphan.request(1921,true,true,true,true));
        REQUIRE(orphan.confirm_fixture(1921,true,true,true));
        REQUIRE(orphanMenu.fixture_begin_keyboard(1921,orphan,router));
        REQUIRE(!orphanMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1921,
            orphan,router,x,y,11000,true));
        REQUIRE(orphan.phase()==InputHandoffPhase::Revoked);
        REQUIRE(!orphanMenu.preview_enabled(0));
        REQUIRE(!orphan.restore(true,true,false));
        REQUIRE(orphan.restore(true,true,true));
        REQUIRE(orphanMenu.close());
    }
    {
        // Held keyboard + new mouse Down can never share the model
        // input owner. The session revokes without mutating settings.
        WglRetainedMenuPreview heldMenu;
        MenuInputHandoffLedger held;
        MenuKeyboardRouter router(heldMenu.fixture_navigation());
        REQUIRE(heldMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(held.request(1922,true,true,true,true));
        REQUIRE(held.confirm_fixture(1922,true,true,true));
        REQUIRE(heldMenu.fixture_begin_keyboard(1922,held,router));
        REQUIRE(heldMenu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1922,
            held,router,17,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
        REQUIRE(held.tracked_down(InputHandoffDevice::Keyboard,17));
        REQUIRE(!heldMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1922,
            held,router,x,y,12000,true));
        REQUIRE(held.phase()==InputHandoffPhase::Revoked);
        REQUIRE(!heldMenu.preview_enabled(0));
        REQUIRE(!held.restore(true,false,true));
        REQUIRE(held.restore(true,true,true));
        REQUIRE(heldMenu.close());
    }
    {
        // Once the actual GL11 click mutates the profile, the old input
        // rectangle is not valid until a new paint. Its next Down cannot
        // be accepted by a still-signed synthetic R190 fixture epoch.
        WglRetainedMenuPreview staleMenu;
        MenuInputHandoffLedger stale;
        MenuKeyboardRouter router(staleMenu.fixture_navigation());
        REQUIRE(staleMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(stale.request(1923,true,true,true,true));
        REQUIRE(stale.confirm_fixture(1923,true,true,true));
        REQUIRE(staleMenu.fixture_begin_keyboard(1923,stale,router));
        REQUIRE(staleMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1923,
            stale,router,x,y,13000,true));
        REQUIRE(staleMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1923,
            stale,router,x,y,13050,true));
        REQUIRE(staleMenu.preview_enabled(0));
        REQUIRE(!staleMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1923,
            stale,router,x,y,13100,true));
        REQUIRE(stale.phase()==InputHandoffPhase::Revoked);
        REQUIRE(staleMenu.preview_enabled(0));
        REQUIRE(stale.restore(true,true,true));
        REQUIRE(staleMenu.close());
    }
    {
        // A key cannot navigate the source menu DURING a held mouse
        // press. Both source-hit and key dispatch are invalidated.
        WglRetainedMenuPreview pressedMenu;
        MenuInputHandoffLedger pressed;
        MenuKeyboardRouter router(pressedMenu.fixture_navigation());
        REQUIRE(pressedMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(pressed.request(1924,true,true,true,true));
        REQUIRE(pressed.confirm_fixture(1924,true,true,true));
        REQUIRE(pressedMenu.fixture_begin_keyboard(1924,pressed,router));
        REQUIRE(pressedMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1924,
            pressed,router,x,y,14000,true));
        REQUIRE(pressed.tracked_down(InputHandoffDevice::Mouse,0));
        REQUIRE(pressedMenu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1924,
            pressed,router,15,true,true).delivery==
            MenuKeyDelivery::Quarantined);
        REQUIRE(pressed.phase()==InputHandoffPhase::Revoked);
        REQUIRE(pressedMenu.tab()==MenuTab::Combat);
        REQUIRE(!pressedMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1924,
            pressed,router,x,y,14050,true));
        REQUIRE(!pressedMenu.preview_enabled(0));
        REQUIRE(!pressed.restore(true,false,true));
        REQUIRE(pressed.restore(true,true,true));
        REQUIRE(pressedMenu.close());
    }
    {
        // An actual post-press WGL viewport drift invalidates the input
        // epoch and prevents its late Up from changing a native profile.
        WglRetainedMenuPreview driftMenu;
        MenuInputHandoffLedger drift;
        MenuKeyboardRouter router(driftMenu.fixture_navigation());
        REQUIRE(driftMenu.draw(context.gl,context.dc,676,630));
        REQUIRE(drift.request(1925,true,true,true,true));
        REQUIRE(drift.confirm_fixture(1925,true,true,true));
        REQUIRE(driftMenu.fixture_begin_keyboard(1925,drift,router));
        REQUIRE(driftMenu.fixture_pointer_event(
            down,context.gl,context.dc,676,630,1925,
            drift,router,x,y,15000,true));
        glViewport(1,0,676,630);
        REQUIRE(!driftMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1925,
            drift,router,x,y,15040,true));
        glViewport(0,0,676,630);
        REQUIRE(drift.phase()==InputHandoffPhase::Revoked);
        REQUIRE(!driftMenu.preview_enabled(0));
        REQUIRE(!driftMenu.fixture_pointer_event(
            up,context.gl,context.dc,676,630,1925,
            drift,router,x,y,15050,true));
        REQUIRE(drift.restore(true,true,true));
        REQUIRE(driftMenu.close());
    }
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R192_SAME_NATIVE_WGL_MOUSE_KEY_HANDOFF_PROFILE_TAB_PASS\n";
    std::cout<<"R192_ORPHAN_MOUSE_UP_AND_HELD_KEY_CANCEL_PASS\n";
}

// R193: synthetic input release stays quarantined even after native GUI
// close()/WGL release. The fixture never forwards an event to vanilla.
void verify_r193_closed_gui_drain(Context& context,const State& expected) {
    using Device=InputHandoffDevice;
    {
        WglRetainedMenuPreview menu;
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(menu.fixture_navigation());
        REQUIRE(menu.draw(context.gl,context.dc,676,630));
        REQUIRE(ledger.request(1930,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1930,true,true,true));
        REQUIRE(menu.fixture_begin_keyboard(1930,ledger,router));
        REQUIRE(menu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1930,
            ledger,router,17,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
        REQUIRE(ledger.tracked_down(Device::Keyboard,17));
        REQUIRE(menu.fixture_end_keyboard(ledger,router));
        REQUIRE(ledger.phase()==InputHandoffPhase::Draining);
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,true,true)); // even an attested claim
        REQUIRE(menu.close());
        // R193: release and restoration MUST work after WGL is genuinely
        // detached, not just after the GUI model marks itself closed.
        REQUIRE(wglMakeCurrent(nullptr,nullptr));
        REQUIRE(wglGetCurrentContext()==nullptr);
        bool foreignRelease=false;
        std::thread foreign([&] {
            foreignRelease=menu.fixture_drain_release(
                ledger,router,Device::Keyboard,17);
        });
        foreign.join();
        REQUIRE(!foreignRelease);
        REQUIRE(ledger.tracked_down(Device::Keyboard,17));
        REQUIRE(!menu.fixture_drain_release(
            ledger,router,Device::Keyboard,18)); // never held
        REQUIRE(menu.fixture_drain_release(
            ledger,router,Device::Keyboard,17));
        REQUIRE(!menu.fixture_drain_release(
            ledger,router,Device::Keyboard,17)); // duplicate Up
        REQUIRE(ledger.tracked_released());
        REQUIRE(router.fixture_idle());
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,true,false));
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,false,true));
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,false,true,true));
        REQUIRE(menu.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(ledger.phase()==InputHandoffPhase::Idle);
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(!ledger.may_deliver_live_minecraft_event());
        REQUIRE(wglMakeCurrent(context.dc,context.gl));
        REQUIRE(wglGetCurrentContext()==context.gl);
    }
    {
        // A genuine painted native toggle is pressed but NOT released
        // before modal closure: its late Up can never commit the toggle.
        MenuState mirror;
        MenuProfileBridge profile(mirror);
        MenuRenderSession frames;
        REQUIRE(frames.attach(1931,676,630));
        const auto frame=frames.next(mirror,&profile);
        REQUIRE(frame);
        const auto panel=menu_panel_geometry(
            676,630,false,MenuPanelMode::CompactOverlay);
        REQUIRE(panel);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),0);
        const int x=static_cast<int>(panel->left)+
            static_cast<int>(std::lround(
                (card.x+card.width-30.0f)*panel->scale));
        const int y=static_cast<int>(panel->top)+
            static_cast<int>(std::lround(
                (card.y+17.0f)*panel->scale));
        REQUIRE(hit_test_menu(
            *frame,MenuPanelMode::CompactOverlay,x,y));
        REQUIRE(frames.detach());
        WglRetainedMenuPreview menu;
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(menu.fixture_navigation());
        REQUIRE(menu.draw(context.gl,context.dc,676,630));
        REQUIRE(ledger.request(1931,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1931,true,true,true));
        REQUIRE(menu.fixture_begin_keyboard(1931,ledger,router));
        REQUIRE(menu.fixture_pointer_event(
            RetainedPointerPhase::Down,context.gl,context.dc,676,630,
            1931,ledger,router,x,y,10000,true));
        REQUIRE(ledger.tracked_down(Device::Mouse,0));
        REQUIRE(!menu.preview_enabled(0));
        REQUIRE(menu.fixture_end_keyboard(ledger,router));
        REQUIRE(!menu.preview_enabled(0)); // close canceled preview press
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(menu.close());
        REQUIRE(menu.fixture_drain_release(
            ledger,router,Device::Mouse,0));
        REQUIRE(!menu.fixture_drain_release(
            ledger,router,Device::Mouse,0)); // duplicate late Up
        REQUIRE(ledger.tracked_released());
        REQUIRE(!menu.fixture_pointer_event(
            RetainedPointerPhase::Up,context.gl,context.dc,676,630,
            1931,ledger,router,x,y,10100,true));
        REQUIRE(menu.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(ledger.phase()==InputHandoffPhase::Idle);
        REQUIRE(!ledger.may_suppress_vanilla_event());
    }
    {
        // After mid-key focus loss, the synthetic router and ledger may
        // be in Revoked with unpaired held edges. Only explicit external
        // release and original dispatcher restoration assertions recover.
        WglRetainedMenuPreview menu;
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(menu.fixture_navigation());
        REQUIRE(menu.draw(context.gl,context.dc,676,630));
        REQUIRE(ledger.request(1932,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1932,true,true,true));
        REQUIRE(menu.fixture_begin_keyboard(1932,ledger,router));
        REQUIRE(menu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1932,
            ledger,router,17,true,true).delivery==
            MenuKeyDelivery::MenuExclusive);
        REQUIRE(menu.fixture_keyboard_event(
            context.gl,context.dc,676,630,1932,
            ledger,router,17,false,false).delivery==
            MenuKeyDelivery::Quarantined);
        REQUIRE(ledger.phase()==InputHandoffPhase::Revoked);
        REQUIRE(router.revoked());
        REQUIRE(!menu.fixture_drain_release(
            ledger,router,Device::Keyboard,17));
        REQUIRE(menu.close());
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,false,true));
        REQUIRE(!menu.fixture_finish_restore(
            ledger,router,true,true,false));
        REQUIRE(menu.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(ledger.phase()==InputHandoffPhase::Idle);
        REQUIRE(router.fixture_idle());
    }
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R193_CLOSED_WGL_LATE_RELEASE_QUARANTINED_PASS\n";
    std::cout<<"R193_PENDING_MOUSE_CLICK_DROPPED_AND_RESTORE_ATTESTED_PASS\n";
}

// R196: exercise actual native retained GL11 multi-choice dispatch, not
// merely the separate R134 model or source hit-test. No original input.
void verify_r196_wgl_multiselect(Context& ctx,const State& expected) {
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_navigation().select_tab(MenuTab::Block));
    REQUIRE(preview.fixture_navigation().focus_visible_row(42u));
    REQUIRE(preview.apply(MenuNavAction::ExpandFocused));
    const auto& all=recovered_widget_catalog();
    std::size_t chosen=all.size();
    for(std::size_t i=0;i<all.size();++i)
        if(std::string_view(all[i].module)=="Auto Tool" &&
           all[i].kind==RecoveredWidgetKind::MultiSelect &&
           menu_source_multi_count(i)==4u) {chosen=i;break;}
    REQUIRE(chosen<all.size());
    const auto panel=menu_panel_geometry(
        676,630,false,MenuPanelMode::CompactOverlay);
    REQUIRE(panel);
    const auto physical=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    std::optional<std::array<int,2>> trigger;
    for(unsigned page=0;page<10 && !trigger;++page) {
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* frame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(frame && frame->tab==MenuTab::Block);
        std::size_t row=frame->visible.size();
        for(std::size_t i=0;i<frame->visible.size();++i)
            if(frame->visible[i].catalog_index==42u){row=i;break;}
        REQUIRE(row<frame->visible.size() &&
                frame->visible[row].expanded);
        std::array<const RecoveredWidget*,5> widgets{};
        REQUIRE(recovered_widget_visible_page(
            "Auto Tool",frame->visible[row].settings_offset,
            static_cast<int>(frame->aim_preview_mode),
            static_cast<int>(frame->target_preview_mode),widgets)>0);
        for(std::size_t slot=0;slot<widgets.size();++slot) {
            if(widgets[slot]!=&all[chosen])continue;
            const auto card=menu_module_card_rect(
                *frame,panel->logical_width(),row);
            const auto column=menu_setting_column(card.x,card.width);
            const float y=card.y+menu_row_body_height(*frame)+
                menu_settings_control_offset(widgets,slot)+10.0f;
            const auto point=physical(column.x+column.width*0.5f,y);
            const auto hit=hit_test_menu(
                *frame,MenuPanelMode::CompactOverlay,point[0],point[1]);
            REQUIRE(hit && hit->kind==MenuHitKind::RecoveredMultiControl &&
                    hit->catalog_index==42u &&
                    hit->control_index==chosen);
            trigger=point;
        }
        if(!trigger)REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
    }
    REQUIRE(trigger);
    const auto original=preview.profile_snapshot(ctx.gl,ctx.dc,676,630);
    REQUIRE(original);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    REQUIRE(ledger.request(1960,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(1960,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(1960,ledger,router));
    REQUIRE(preview.fixture_pointer_event(
        down,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        (*trigger)[0],(*trigger)[1],20000,true));
    REQUIRE(preview.fixture_pointer_event(
        up,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        (*trigger)[0],(*trigger)[1],20040,true));
    REQUIRE(preview.preview_dropdown_open());
    // A GUI state mutation invalidates old painted input.
    REQUIRE(!preview.fixture_painted_frame(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* popupFrame=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(popupFrame && popupFrame->dropdown_preview &&
            popupFrame->dropdown_preview->kind==
                MenuDropdownKind::RecoveredMulti);
    const auto popup=menu_dropdown_popup_geometry(
        *popupFrame,panel->logical_width(),panel->logical_height());
    REQUIRE(popup && popup->total_options==4u);
    const auto second=physical(
        popup->x+popup->width*0.5f,
        popup->y+popup->row_height*1.5f);
    const auto secondHit=hit_test_menu(
        *popupFrame,MenuPanelMode::CompactOverlay,second[0],second[1]);
    REQUIRE(secondHit &&
            secondHit->kind==MenuHitKind::DropdownOption &&
            secondHit->control_index==1u);
    REQUIRE(preview.fixture_pointer_event(
        down,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        second[0],second[1],23000,true));
    REQUIRE(preview.fixture_pointer_event(
        up,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        second[0],second[1],23040,true));
    REQUIRE(preview.preview_dropdown_open()); // Multi stays open
    REQUIRE(!preview.preview_enabled(42u));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* selected=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(selected && selected->recovered_multi_overrides[chosen] &&
            *selected->recovered_multi_overrides[chosen]==2u);
    const auto selectedPopup=menu_dropdown_popup_geometry(
        *selected,panel->logical_width(),panel->logical_height());
    REQUIRE(selectedPopup && selectedPopup->selected_mask==2u);
    REQUIRE(preview.fixture_pointer_event(
        down,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        second[0],second[1],25000,true));
    REQUIRE(preview.fixture_pointer_event(
        up,ctx.gl,ctx.dc,676,630,1960,ledger,router,
        second[0],second[1],25040,true));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* deselected=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(deselected &&
            deselected->recovered_multi_overrides[chosen].value_or(9u)==0u);
    REQUIRE(preview.profile_snapshot(ctx.gl,ctx.dc,676,630)==original);
    REQUIRE(ledger.tracked_released());
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R196_WGL_REAL_MULTI_POPUP_SELECTED_DESELECTED_PASS\n";
}

// R196: the REAL retained GL11 ClickGUI commits a range endpoint from
// source-pinned geometry, keeping the original mouse-down handle. This
// runs in a disposable WGL fixture, never through Minecraft's input queue.
void verify_r196_wgl_range(Context& ctx,const State& expected) {
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto move=RetainedPointerPhase::Move;
    constexpr auto up=RetainedPointerPhase::Up;
    const auto& catalog=recovered_widget_catalog();
    std::size_t rangeIndex=catalog.size();
    for(std::size_t i=0;i<catalog.size();++i)
        if(catalog[i].kind==RecoveredWidgetKind::Range &&
           std::string_view(catalog[i].module)=="Aim Assist" &&
           std::string_view(catalog[i].label)=="Range") {
            rangeIndex=i;break;
        }
    REQUIRE(rangeIndex<catalog.size());
    const auto& source=catalog[rangeIndex];
    REQUIRE(source.min_value<source.example_value &&
            source.example_value<source.example_other &&
            source.example_other<source.max_value);
    const auto panel=menu_panel_geometry(
        676,630,false,MenuPanelMode::CompactOverlay);
    REQUIRE(panel);
    const auto physical=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    for(unsigned handle=0;handle<2u;++handle) {
        WglRetainedMenuPreview preview;
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        REQUIRE(preview.apply(MenuNavAction::ExpandFocused)); // Aim
        std::optional<MenuNumericTrack> track;
        for(unsigned page=0;page<10 && !track;++page) {
            REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
            const auto* frame=preview.fixture_painted_frame(
                ctx.gl,ctx.dc,676,630);
            REQUIRE(frame && frame->tab==MenuTab::Combat);
            track=menu_numeric_track_geometry(
                *frame,panel->logical_width(),panel->logical_height(),
                0u,rangeIndex);
            if(!track)
                REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
        }
        REQUIRE(track && track->width>30.0f);
        const auto* frame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(frame && frame->visible.front().expanded);
        const float start=handle==0?
            source.example_value:source.example_other;
        const float ratio=(start-source.min_value)/
            (source.max_value-source.min_value);
        const auto first=physical(
            track->x+track->width*ratio,
            track->y+track->height*0.5f);
        const auto last=physical(
            handle==0?track->x+track->width-2.0f:
                      track->x+2.0f,
            track->y+track->height*0.5f);
        const auto originalHit=hit_test_menu(
            *frame,MenuPanelMode::CompactOverlay,first[0],first[1]);
        REQUIRE(originalHit &&
                originalHit->kind==MenuHitKind::RecoveredRangePreview &&
                originalHit->control_index==rangeIndex &&
                originalHit->sub_index==handle);
        const auto original=preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(original);
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(preview.fixture_navigation());
        const std::uint64_t epoch=1961+handle;
        REQUIRE(ledger.request(epoch,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
        REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,first[0],first[1],30000,true));
        REQUIRE(preview.fixture_pointer_event(
            move,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,last[0],last[1],30050,true));
        // Drag does not silently mutate the source value before Up.
        const auto* heldFrame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(heldFrame &&
                !heldFrame->recovered_range_overrides[rangeIndex]);
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,last[0],last[1],30070,true));
        REQUIRE(!preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630)); // repaint after commit
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* committed=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(committed &&
                committed->recovered_range_overrides[rangeIndex]);
        const auto selected=*committed->recovered_range_overrides[rangeIndex];
        const float expectedValue=handle==0?
            source.example_other:source.example_value;
        REQUIRE(std::fabs(selected.lower-expectedValue)<0.0001f &&
                std::fabs(selected.upper-expectedValue)<0.0001f);
        REQUIRE(preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630)==original);
        REQUIRE(!preview.preview_enabled(0u));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.fixture_end_keyboard(ledger,router));
        REQUIRE(preview.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(preview.close());
        REQUIRE(!ledger.may_deliver_live_minecraft_event());
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(expected,snapshot());
    }
    std::cout<<"R196_WGL_RANGE_PRESSED_HANDLE_CAPTURE_AND_CLAMP_PASS\n";
}

// R197: exercise the real retained GL11 hit-to-preview bridges for the
// reconstructed bind chip and text field, under synthetic WGL input only.
// This does NOT deliver OS keyboard input or register a Minecraft keybind.
void verify_r197_native_focus_receipts(Context& ctx,const State& expected) {
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    const auto panel=menu_panel_geometry(
        676,630,false,MenuPanelMode::CompactOverlay);
    REQUIRE(panel);
    const auto xy=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    {
        WglRetainedMenuPreview preview;
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* painted=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(painted && painted->tab==MenuTab::Combat &&
                painted->visible.front().catalog_index==0u);
        const auto initialBind=painted->bind_previews[0u];
        const auto chips=menu_bind_chip_geometry(
            *painted,panel->logical_width(),panel->logical_height(),
            0u,0u,menu_bind_code_label(initialBind.code),false);
        REQUIRE(chips);
        const auto point=xy(chips->key.x+chips->key.width*0.5f,
                            chips->key.y+chips->key.height*0.5f);
        const auto hit=hit_test_menu(
            *painted,MenuPanelMode::CompactOverlay,point[0],point[1]);
        REQUIRE(hit && hit->kind==MenuHitKind::BindChip &&
                hit->catalog_index==0u);
        const auto nativeProfile=preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(nativeProfile);
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(preview.fixture_navigation());
        REQUIRE(ledger.request(1970,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1970,true,true,true));
        REQUIRE(preview.fixture_begin_keyboard(1970,ledger,router));
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,676,630,1970,
            ledger,router,point[0],point[1],10000,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,676,630,1970,
            ledger,router,point[0],point[1],10040,true));
        REQUIRE(!preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630)); // must repaint after click
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* waiting=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(waiting && waiting->bind_capture_index==0u &&
                waiting->bind_previews[0u].code==initialBind.code);
        REQUIRE(preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630)==nativeProfile);
        REQUIRE(!preview.preview_enabled(0u));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.fixture_end_keyboard(ledger,router));
        REQUIRE(preview.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(preview.close());
        REQUIRE(!ledger.may_suppress_vanilla_event());
    }
    {
        const auto& source=recovered_widget_catalog();
        std::size_t textIndex=source.size();
        for(std::size_t i=0;i<source.size();++i)
            if(source[i].kind==RecoveredWidgetKind::Text &&
               std::string_view(source[i].module)=="Auto Tool" &&
               std::string_view(source[i].auxiliary)=="Search item") {
                textIndex=i;break;
            }
        REQUIRE(textIndex<source.size());
        WglRetainedMenuPreview preview;
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        REQUIRE(preview.fixture_navigation().select_tab(MenuTab::Block));
        REQUIRE(preview.fixture_navigation().focus_visible_row(42u));
        REQUIRE(preview.apply(MenuNavAction::ExpandFocused));
        std::optional<MenuTextFieldRect> field;
        for(unsigned page=0;page<10 && !field;++page) {
            REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
            const auto* painted=preview.fixture_painted_frame(
                ctx.gl,ctx.dc,676,630);
            REQUIRE(painted && painted->tab==MenuTab::Block);
            field=menu_text_field_geometry(
                *painted,panel->logical_width(),panel->logical_height(),
                42u,textIndex);
            if(!field)
                REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
        }
        REQUIRE(field && field->width>30.0f);
        const auto point=xy(field->x+field->width*0.5f,
                            field->y+field->height*0.5f);
        const auto* painted=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        const auto hit=painted?hit_test_menu(
            *painted,MenuPanelMode::CompactOverlay,point[0],point[1]):
            std::nullopt;
        REQUIRE(hit && hit->kind==MenuHitKind::RecoveredTextField &&
                hit->catalog_index==42u &&
                hit->control_index==textIndex);
        const auto nativeProfile=preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(nativeProfile);
        MenuInputHandoffLedger ledger;
        MenuKeyboardRouter router(preview.fixture_navigation());
        REQUIRE(ledger.request(1971,true,true,true,true));
        REQUIRE(ledger.confirm_fixture(1971,true,true,true));
        REQUIRE(preview.fixture_begin_keyboard(1971,ledger,router));
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,676,630,1971,
            ledger,router,point[0],point[1],20000,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,676,630,1971,
            ledger,router,point[0],point[1],20040,true));
        REQUIRE(!preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* focused=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(focused && focused->focused_text_widget==textIndex &&
                focused->recovered_text_overrides.empty());
        REQUIRE(preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630)==nativeProfile);
        REQUIRE(!preview.preview_enabled(42u));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.fixture_end_keyboard(ledger,router));
        REQUIRE(preview.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(preview.close());
        REQUIRE(!ledger.may_deliver_live_minecraft_event());
    }
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R197_WGL_NATIVE_BIND_CHIP_FOCUS_AND_TEXT_FIELD_PASS\n";
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
        verify_r192_joint_handoff(context,before);
        verify_r193_closed_gui_drain(context,before);
        verify_r196_wgl_multiselect(context,before);
        verify_r196_wgl_range(context,before);
        verify_r197_native_focus_receipts(context,before);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"R159 retained pointer fixture failure: "<<e.what()<<"\n";
        return 1;
    }
}
