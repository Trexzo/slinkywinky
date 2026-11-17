#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/windows_menu_system_font.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/menu_unload_modal_geometry.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/windows_gui_module_click_fixture.hpp"
#include "slinkywinky/windows_stop_gate.hpp"
#include "slinkywinky/windows_insert_edge.hpp"
#include "slinkywinky/windows_menu_operator_events.hpp"
#include "slinkywinky/windows_game_window_gate.hpp"
#include "slinkywinky/windows_original_pointer_edge_trace.hpp"
#include "slinkywinky/windows_menu_focus_fence.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_expander_geometry.hpp"
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_settings_vertical.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_color_modal_geometry.hpp"
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
#include <memory>
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
    GLint blendSrc=0,blendDst=0,shadeModel=0;
    GLfloat lineWidth=0.0f;
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
    glGetIntegerv(GL_SHADE_MODEL,&s.shadeModel);
    glGetFloatv(GL_LINE_WIDTH,&s.lineWidth);
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
    REQUIRE(a.shadeModel==b.shadeModel);
    REQUIRE(std::fabs(a.lineWidth-b.lineWidth)<0.0001f);
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
        // R225: press AWAY from the original knob. Original Slinky
        // immediately positions the selected handle on Down, without
        // requiring an intermediate mouse Move callback.
        const float start=handle==0?
            source.example_value-0.5f:source.example_other+0.5f;
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
        // R225: a WGL paint immediately after Down, with NO Move, must
        // show the source-selected new value and keep profile unchanged.
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* downFrame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(downFrame && downFrame->numeric_drag_preview &&
                !downFrame->recovered_range_overrides[rangeIndex] &&
                downFrame->numeric_drag_preview->handle==handle);
        const auto& downDrag=*downFrame->numeric_drag_preview;
        REQUIRE(std::fabs((handle==0?downDrag.lower:downDrag.upper)-
                          start)<source.step*1.5f);
        REQUIRE(std::fabs((handle==0?downDrag.upper:downDrag.lower)-
                          (handle==0?source.example_other:
                                     source.example_value))<0.0001f);
        REQUIRE(preview.profile_snapshot(ctx.gl,ctx.dc,676,630)==original);

        // Another held Move, repaint, then a second Move across the
        // original opposite knob. The owner and source hit cannot drift.
        const float intermediate=handle==0?
            source.example_other+0.6f:source.example_value-0.6f;
        const float middleRatio=(intermediate-source.min_value)/
            (source.max_value-source.min_value);
        const auto middle=physical(
            track->x+track->width*middleRatio,
            track->y+track->height*0.5f);
        REQUIRE(preview.fixture_pointer_event(
            move,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,middle[0],middle[1],30020,true));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* midFrame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(midFrame && midFrame->numeric_drag_preview &&
                !midFrame->recovered_range_overrides[rangeIndex]);
        REQUIRE(std::fabs(midFrame->numeric_drag_preview->lower-
                          intermediate)<source.step*1.5f &&
                std::fabs(midFrame->numeric_drag_preview->upper-
                          intermediate)<source.step*1.5f);
        REQUIRE(preview.profile_snapshot(ctx.gl,ctx.dc,676,630)==original);
        REQUIRE(preview.fixture_pointer_event(
            move,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,last[0],last[1],30050,true));
        // R224/R225: the native WGL paint updates before Up, including
        // push across the opposite endpoint, without early persistence.
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* heldFrame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        const float expectedHeld=handle==0?
            source.max_value:source.min_value;
        REQUIRE(heldFrame && heldFrame->numeric_drag_preview &&
                !heldFrame->recovered_range_overrides[rangeIndex]);
        REQUIRE(heldFrame->numeric_drag_preview->widget_index==rangeIndex &&
                heldFrame->numeric_drag_preview->handle==handle &&
                std::fabs(heldFrame->numeric_drag_preview->lower-
                          expectedHeld)<0.0001f &&
                std::fabs(heldFrame->numeric_drag_preview->upper-
                          expectedHeld)<0.0001f);
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,last[0],last[1],30070,true));
        REQUIRE(!preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630)); // repaint after commit
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* committed=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(committed && !committed->numeric_drag_preview &&
                committed->recovered_range_overrides[rangeIndex]);
        const auto selected=*committed->recovered_range_overrides[rangeIndex];
        // R220: the native source range handler PUSHES the opposite
        // endpoint on crossing, rather than clamping at its old value.
        // Each verified mouse-Up lands just inside an extreme endpoint.
        const float expectedValue=handle==0?
            source.max_value:source.min_value;
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
    std::cout<<"R225_WGL_DOWN_FIRST_FRAME_REPEATED_MOVE_RANGE_PUSH_PASS\n";
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
        // R234: the native source bind chip now accepts a paired
        // synthetic key commit rather than stopping at pointer focus.
        // Background Tab, Escape and page navigation remain refused.
        REQUIRE(!preview.apply(MenuNavAction::NextTab));
        REQUIRE(!preview.scroll_visible(1,3u));
        REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Move));
        REQUIRE(preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630)==waiting);
        const auto f6=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,676,630,1970,ledger,router,
            64u,true,true);
        REQUIRE(f6.delivery==MenuKeyDelivery::MenuExclusive &&
                f6.action_applied && !f6.request_close);
        REQUIRE(!preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630));
        const auto f6Release=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,676,630,1970,ledger,router,
            64u,false,true);
        REQUIRE(f6Release.delivery==MenuKeyDelivery::MenuExclusive &&
                !f6Release.action_applied && !f6Release.request_close);
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* bound=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(bound && !bound->bind_capture_index &&
                bound->bind_previews[0u].code==0x75u &&
                bound->tab==MenuTab::Combat &&
                !preview.preview_enabled(0u));
        REQUIRE(preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630)==nativeProfile);
        REQUIRE(preview.apply(MenuNavAction::NextTab));
        REQUIRE(preview.tab()==MenuTab::Move);
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
        // R234: source UTF-8 editing is owned by the recovered text
        // field. An untrusted physical key without typed bytes cannot
        // invent a letter, and malformed scalar payloads are inert.
        REQUIRE(!preview.apply(MenuNavAction::NextTab));
        REQUIRE(!preview.scroll_visible(1,3u));
        REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Move));
        REQUIRE(preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630)==focused);
        const auto type=[&](unsigned key,std::string_view bytes,
                            bool edited) {
            const auto result=preview.fixture_keyboard_typed_event(
                ctx.gl,ctx.dc,676,630,1971,ledger,router,
                key,true,true,bytes);
            REQUIRE(result.delivery==MenuKeyDelivery::MenuExclusive &&
                    result.action_applied==edited && !result.request_close);
            const auto up=preview.fixture_keyboard_event(
                ctx.gl,ctx.dc,676,630,1971,ledger,router,
                key,false,true);
            REQUIRE(up.delivery==MenuKeyDelivery::MenuExclusive &&
                    !up.action_applied && !up.request_close);
            REQUIRE(ledger.tracked_released());
            REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        };
        type(15u,{},false);  // Tab is not a background switch.
        type(30u,{},false);  // physical A without text cannot append.
        type(30u,"a",true);
        type(48u,"B",true);
        type(18u,"\xC3\xA4",true); // Unicode ä
        type(17u,"\xF0\x9F\x98\x80",true); // emoji
        type(34u,"\xC0\xAF",false); // invalid UTF-8
        type(16u,std::string_view("Z\0Q",3u),true);
        type(14u,{},true); // Backspace Z
        type(14u,{},true); // Backspace whole 4-byte emoji
        const auto* draft=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(draft && draft->tab==MenuTab::Block &&
                draft->focused_text_widget==textIndex &&
                draft->recovered_text_overrides.size()==1u &&
                draft->recovered_text_overrides[0].widget_index==textIndex &&
                draft->recovered_text_overrides[0].value=="aB\xC3\xA4");
        // R235: prove a paired source event cannot write only the
        // first byte when the 128-byte field has one byte remaining.
        for(unsigned n=0u;n<30u;++n)type(30u,"xxxx",true);
        type(30u,"xxx",true);
        const std::string nearLimit=
            std::string("aB\xC3\xA4")+std::string(123u,'x');
        REQUIRE(nearLimit.size()==127u);
        const auto* nearFrame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(nearFrame &&
                nearFrame->recovered_text_overrides.size()==1u &&
                nearFrame->recovered_text_overrides[0].value==nearLimit);
        type(30u,"YZ",false); // invalid total length: no partial Y
        const auto* afterDenied=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(afterDenied &&
                afterDenied->recovered_text_overrides[0].value==nearLimit);
        type(30u,"Q",true);  // fill exactly 128 bytes
        type(30u,"!",false); // refuse all further input
        const std::string maxText=nearLimit+"Q";
        type(28u,{},true); // Enter blurs, not expand or close GUI.
        const auto* committed=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(committed && !committed->focused_text_widget &&
                committed->recovered_text_overrides.size()==1u &&
                committed->recovered_text_overrides[0].value==maxText &&
                preview.tab()==MenuTab::Block);
        REQUIRE(preview.profile_snapshot(
            ctx.gl,ctx.dc,676,630)==nativeProfile);
        REQUIRE(!preview.preview_enabled(42u));
        REQUIRE(preview.fixture_navigation().select_tab(MenuTab::Combat));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.fixture_end_keyboard(ledger,router));
        REQUIRE(preview.fixture_finish_restore(
            ledger,router,true,true,true));
        REQUIRE(preview.close());
        REQUIRE(!ledger.may_deliver_live_minecraft_event());
    }
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R235_WGL_ATOMIC_SOURCE_TEXT_EVENT_CAPACITY_PASS\n";
    std::cout<<"R234_WGL_MODULE_BIND_AND_UTF8_TEXT_KEYBOARD_COMMIT_PASS\n";
    std::cout<<"R197_WGL_NATIVE_BIND_CHIP_FOCUS_AND_TEXT_FIELD_PASS\n";
}

// R200: actual retained GL11 color popup with native source hitbox,
// modal-only click routing, HSL, recent swatches and Apply/Cancel. This
// is still synthetic foreground input; Minecraft does not receive it.
void verify_r200_native_color_popup(Context& ctx,const State& expected) {
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    const auto& widgets=recovered_widget_catalog();
    std::size_t accent=widgets.size();
    for(std::size_t i=0;i<widgets.size();++i)
        if(widgets[i].kind==RecoveredWidgetKind::Color &&
           std::string_view(widgets[i].module)=="GUI" &&
           std::string_view(widgets[i].label)=="Accent color")accent=i;
    REQUIRE(accent<widgets.size());
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_navigation().select_tab(MenuTab::Visual));
    // R135's source-authenticated compact pane has only three rows.
    // Full eight-row navigation alone left GUI's swatch clipped.
    REQUIRE(preview.scroll_visible(5,3));
    REQUIRE(preview.fixture_navigation().focus_visible_row(32u));
    REQUIRE(preview.apply(MenuNavAction::ExpandFocused));
    const auto panel=menu_panel_geometry(
        676,630,false,MenuPanelMode::CompactOverlay);
    REQUIRE(panel);
    const auto xy=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    std::optional<MenuColorSwatchRect> swatch;
    for(unsigned page=0u;page<12u && !swatch;++page) {
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
        const auto* frame=preview.fixture_painted_frame(
            ctx.gl,ctx.dc,676,630);
        REQUIRE(frame && frame->tab==MenuTab::Visual);
        swatch=menu_color_swatch_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            32u,accent);
        if(!swatch)REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
    }
    REQUIRE(swatch);
    const auto point=xy(
        swatch->x+swatch->width*0.5f,
        swatch->y+swatch->height*0.5f);
    const auto* source=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    const auto originalHit=source?hit_test_menu(
        *source,MenuPanelMode::CompactOverlay,point[0],point[1]):
        std::nullopt;
    REQUIRE(originalHit &&
            originalHit->kind==MenuHitKind::RecoveredColorSwatch &&
            originalHit->catalog_index==32u &&
            originalHit->control_index==accent);
    const auto initialProfile=preview.profile_snapshot(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(initialProfile);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    constexpr std::uint64_t epoch=2000u;
    REQUIRE(ledger.request(epoch,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
    const auto click=[&](const std::array<int,2>& at,
                         std::uint64_t when) {
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,at[0],at[1],when,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,676,630,epoch,
            ledger,router,at[0],at[1],when+40u,true));
    };
    const auto key=[&](unsigned code,bool edits) {
        const auto pressed=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,676,630,epoch,ledger,router,code,true,true);
        REQUIRE(pressed.delivery==MenuKeyDelivery::MenuExclusive &&
                !pressed.request_close && pressed.action_applied==edits);
        const auto released=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,676,630,epoch,ledger,router,code,false,true);
        REQUIRE(released.delivery==MenuKeyDelivery::MenuExclusive &&
                !released.request_close && !released.action_applied);
        REQUIRE(!ledger.tracked_down(InputHandoffDevice::Keyboard,code));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    };
    click(point,20000);
    REQUIRE(!preview.fixture_painted_frame(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* popup=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(popup && popup->dropdown_preview &&
            popup->dropdown_preview->kind==MenuDropdownKind::ColorPicker &&
            popup->dropdown_preview->catalog_index==32u &&
            popup->dropdown_preview->widget_index==accent);
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0x8B7CF6u);
    // R203: opt-in actual GL_BACK readback of this own, disposable WGL
    // context, never any desktop or original Minecraft game window.
    if(const char* capture=std::getenv("SLINKYWINKY_R203_COLOR_MODAL_PPM"))
        REQUIRE(save_gl_framebuffer_ppm(capture));
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.fixture_navigation().apply(MenuNavAction::NextTab));
    REQUIRE(preview.tab()==MenuTab::Visual);
    const auto background=xy(12.0f,menu_nav_y+10.0f);
    REQUIRE(!hit_test_menu(*popup,MenuPanelMode::CompactOverlay,
                           background[0],background[1]));
    const auto modal=menu_color_modal_geometry(
        panel->logical_width(),panel->logical_height());
    REQUIRE(modal);
    // R202: the removed synthetic footer has NO clickable actions.
    // This guards against a silent reintroduction of fake Apply/Cancel.
    for(const float footerX:{
            modal->outer.x+40.0f,
            modal->outer.x+modal->outer.width-40.0f}) {
        const auto footer=xy(footerX,modal->outer.y+285.0f);
        REQUIRE(!hit_test_menu(*popup,MenuPanelMode::CompactOverlay,
                               footer[0],footer[1]));
    }
    const auto hue=xy(modal->hsl[0].x+
                      modal->hsl[0].width*0.333f,
                      modal->hsl[0].y+modal->hsl[0].height*0.5f);
    const auto hueHit=hit_test_menu(
        *popup,MenuPanelMode::CompactOverlay,hue[0],hue[1]);
    REQUIRE(hueHit && hueHit->kind==MenuHitKind::ColorPopupHsl &&
            hueHit->control_index==0u);
    // R201: the color model updates on held MOVE, not just button UP.
    // A repaint during the drag retains the exact captured HSL channel.
    const auto draggedHue=xy(
        modal->hsl[0].x+modal->hsl[0].width*0.59f,
        modal->hsl[0].y+modal->hsl[0].height*0.5f);
    REQUIRE(preview.fixture_pointer_event(
        down,ctx.gl,ctx.dc,676,630,epoch,
        ledger,router,hue[0],hue[1],22000,true));
    REQUIRE(preview.fixture_pointer_event(
        RetainedPointerPhase::Move,ctx.gl,ctx.dc,676,630,epoch,
        ledger,router,draggedHue[0],draggedHue[1],22060,true));
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)!=0x8B7CF6u);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_pointer_event(
        up,ctx.gl,ctx.dc,676,630,epoch,
        ledger,router,draggedHue[0],draggedHue[1],22100,true));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)!=0x8B7CF6u);
    if(const char* capture=std::getenv("SLINKYWINKY_R203_HSL_CHANGED_PPM"))
        REQUIRE(save_gl_framebuffer_ppm(capture));
    // R203: explicitly adversarial GL state before painting the source
    // six-sector gradient and outlined color knobs. The renderer must
    // leave caller shading and line width untouched after draw.
    glShadeModel(GL_FLAT);
    glLineWidth(3.0f);
    const State adversarial=snapshot();
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    same(adversarial,snapshot());
    glShadeModel(expected.shadeModel);
    glLineWidth(expected.lineWidth);
    const auto black=xy(
        modal->recent[0].x+modal->recent[0].width*0.5f,
        modal->recent[0].y+modal->recent[0].height*0.5f);
    const auto* chooser=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    const auto blackHit=hit_test_menu(
        *chooser,MenuPanelMode::CompactOverlay,black[0],black[1]);
    REQUIRE(blackHit && blackHit->kind==MenuHitKind::ColorPopupRecent &&
            blackHit->control_index==0u);
    click(black,24000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,676,630)==initialProfile);
    // R202 source close X is the sole visible exit and *commits*
    // the current local selection. Synthetic Apply footer is removed.
    const auto close=xy(
        modal->close.x+modal->close.width*0.5f,
        modal->close.y+modal->close.height*0.5f);
    const auto* beforeClose=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    const auto closeHit=hit_test_menu(
        *beforeClose,MenuPanelMode::CompactOverlay,close[0],close[1]);
    REQUIRE(closeHit && closeHit->kind==MenuHitKind::ColorPopupClose);
    click(close,26000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* committed=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(committed && !committed->dropdown_preview &&
            committed->recovered_color_overrides[accent]==0u &&
            committed->appearance.accent_rgb==0xF35D12u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,676,630)==initialProfile);
    click(point,28000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_color_popup_rgb(ctx.gl,ctx.dc,676,630)==0u);
    const auto light=xy(modal->hsl[2].x+
                        modal->hsl[2].width*0.99f,
                        modal->hsl[2].y+modal->hsl[2].height*0.5f);
    click(light,30000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(preview.fixture_color_popup_rgb(ctx.gl,ctx.dc,676,630)!=0u);
    // Escape is a first-party safe local cancel; candidate source's
    // clicked close X instead commits. Never close the main GUI.
    key(1u,true);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* canceled=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(canceled && !canceled->dropdown_preview &&
            canceled->recovered_color_overrides[accent]==0u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,676,630)==initialProfile);

    // R201: open exactly the same source swatch and click the actual
    // painted HEX field. The paired first-party keyboard ledger, not OS
    // messages, owns all six digit events and releases.
    click(point,34000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto hexField=xy(modal->hex.x+modal->hex.width*0.5f,
                           modal->hex.y+modal->hex.height*0.5f);
    const auto* beforeHex=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    const auto hexHit=hit_test_menu(
        *beforeHex,MenuPanelMode::CompactOverlay,
        hexField[0],hexField[1]);
    REQUIRE(hexHit && hexHit->kind==MenuHitKind::ColorPopupHexField);
    click(hexField,36000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    key(15u,false); // Tab cannot switch tabs while modal owns input
    REQUIRE(preview.tab()==MenuTab::Visual);
    key(2u,true);  // 1
    key(30u,true); // a
    key(3u,true);  // 2
    key(48u,true); // b
    key(4u,true);  // 3
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0u); // partial hex never changes RGB
    key(46u,true); // c
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0x1A2B3Cu);
    key(14u,true); // backspace
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0x1A2B3Cu);
    key(32u,true); // d
    REQUIRE(preview.fixture_color_popup_rgb(
        ctx.gl,ctx.dc,676,630)==0x1A2B3Du);
    key(28u,true); // Enter ends field edit, keeps popup
    const auto* edited=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(edited && edited->dropdown_preview &&
            edited->dropdown_preview->kind==MenuDropdownKind::ColorPicker);
    click(close,47000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    const auto* hexApplied=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(hexApplied && !hexApplied->dropdown_preview &&
            hexApplied->recovered_color_overrides[accent]==0x1A2B3Du);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,676,630)==initialProfile);
    // Esc without text focus closes the modal, NOT original Minecraft
    // or even the top-level retained GUI. No original input is touched.
    click(point,49000);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,676,630));
    key(1u,true);
    REQUIRE(preview.tab()==MenuTab::Visual);
    const auto* afterEscape=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,676,630);
    REQUIRE(afterEscape && !afterEscape->dropdown_preview &&
            afterEscape->recovered_color_overrides[accent]==0x1A2B3Du);
    REQUIRE(ledger.tracked_released());
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(
        ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R202_NATIVE_WGL_SOURCE_X_HSL_HEX_ESCAPE_PASS\n";
}

// R211: an actual source-exact first-party toggle hit on an admitted
// disposable WGL panel can enqueue Sprint without mutating preview state.
// The handoff ledger and pointer events are entirely synthetic; no
// Minecraft LWJGL/Win32 input interception is asserted here.
void verify_r211_source_click_request(Context& ctx,const State& expected) {
    auto menu=std::make_unique<WglRetainedMenuPreview>();
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(menu->fixture_navigation());
    GuiModuleControlHandoff handoff;
    ModuleArrayListFeed feed;
    GuiModuleClickTransaction clicks;
    ModuleArrayListSnapshot source;
    source.source_sequence=211u;
    source.world_generation=7u;
    source.world_ready=true;
    source.modules={{"Sprint",false,false,7u}};
    REQUIRE(feed.publish(source));
    REQUIRE(menu->draw(ctx.gl,ctx.dc,676,630));
    REQUIRE(menu->apply(MenuNavAction::NextTab)); // Move
    for(int i=0;i<8;++i)REQUIRE(menu->apply(MenuNavAction::NextRow));
    REQUIRE(menu->focused_row() && *menu->focused_row()==25u);
    // Compact viewport paints only the upper cards. Scroll Sprint into
    // actually hit-testable pixels; keyboard focus alone is insufficient.
    REQUIRE(menu->scroll_visible(5,3));
    REQUIRE(menu->draw(ctx.gl,ctx.dc,676,630));
    const auto* frame=menu->fixture_painted_frame(ctx.gl,ctx.dc,676,630);
    REQUIRE(frame);
    int x=-1,y=-1;
    // Use exact painted hit testing, not a guessed viewport coordinate.
    for(int row=0;row<630 && x<0;row+=2)
        for(int col=0;col<676;col+=2) {
            const auto hit=hit_test_menu(
                *frame,MenuPanelMode::CompactOverlay,col,row);
            if(hit && hit->kind==MenuHitKind::Toggle &&
               hit->catalog_index==25u) {x=col;y=row;break;}
        }
    REQUIRE(x>=0 && y>=0);
    REQUIRE(ledger.request(211u,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(211u,true,true,true));
    REQUIRE(menu->fixture_begin_keyboard(211u,ledger,router));
    auto event=[&](RetainedPointerPhase phase,int px,int py,
                   std::uint64_t time) {
        return clicks.fixture_event(phase,*menu,ledger,router,
            handoff,feed,ctx.gl,ctx.dc,676,630,211u,
            px,py,time,true);
    };
    REQUIRE(!event(RetainedPointerPhase::Up,x,y,21100u));
    REQUIRE(!event(RetainedPointerPhase::Down,0,0,21200u));
    REQUIRE(!handoff.view().request);
    bool foreign=false;
    std::thread other([&]{
        foreign=event(RetainedPointerPhase::Down,x,y,21300u);
    });
    other.join();
    REQUIRE(!foreign && !clicks.pending());
    REQUIRE(event(RetainedPointerPhase::Down,x,y,21400u));
    REQUIRE(clicks.pending() &&
            ledger.tracked_down(InputHandoffDevice::Mouse,0));
    REQUIRE(!handoff.view().request);
    REQUIRE(event(RetainedPointerPhase::Up,x,y,21470u));
    REQUIRE(!clicks.pending() && ledger.tracked_released());
    const auto pending=handoff.view();
    REQUIRE(pending.phase==GuiModuleControlPhase::Pending &&
            pending.request && pending.request->enable);
    REQUIRE(!menu->preview_enabled(25u));
    REQUIRE(make_module_arraylist_display(*feed.latest())
            ->active_names.empty());
    // The request remains pending until the separate module owner acts.
    REQUIRE(handoff.view().request &&
            handoff.view().request->id==pending.request->id);
    REQUIRE(ledger.phase()==InputHandoffPhase::MenuFixture);
    // Change the admitted owner-state copy during the next held gesture.
    // The pending request cannot be overwritten; the stale press is revoked.
    REQUIRE(event(RetainedPointerPhase::Down,x,y,23000u));
    source.source_sequence=212u;
    REQUIRE(feed.publish(source));
    REQUIRE(!event(RetainedPointerPhase::Up,x,y,23050u));
    REQUIRE(ledger.phase()==InputHandoffPhase::Revoked);
    REQUIRE(!clicks.pending());
    REQUIRE(handoff.view().request &&
            handoff.view().request->id==pending.request->id);
    REQUIRE(!menu->preview_enabled(25u));
    REQUIRE(menu->fixture_finish_restore(ledger,router,true,true,true));
    REQUIRE(menu->close());
    handoff.retire();
    feed.retire();
    REQUIRE(!ledger.may_deliver_live_minecraft_event());
    REQUIRE(!ledger.may_suppress_vanilla_event());
    same(expected,snapshot());
    std::cout<<"R211_SOURCE_EXACT_WGL_FIXTURE_MOUSE_TO_PENDING_SPRINT_PASS\n";
}

// R227: actual GL11 retained Select item popup with source-only
// two-column case-insensitive search. All input is a disposable WGL
// fixture, never an original Minecraft Keyboard.next handler.
void verify_r227_wgl_select_item(Context& ctx,const State& expected) {
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    constexpr auto mode=MenuPanelMode::CompactOverlay;
    constexpr unsigned width=676u,height=630u;
    const auto& all=recovered_widget_catalog();
    std::size_t item=all.size();
    for(std::size_t i=0;i<all.size();++i) {
        if(menu_source_large_optional_choice(i)){item=i;break;}
    }
    REQUIRE(item<all.size());
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    for(unsigned i=0u;i<4u;++i) {
        REQUIRE(preview.apply(MenuNavAction::NextTab));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    }
    REQUIRE(preview.tab()==MenuTab::Utility);
    REQUIRE(preview.scroll_visible(9,8));
    REQUIRE(preview.fixture_navigation().focus_visible_row(58u));
    REQUIRE(preview.apply(MenuNavAction::ExpandFocused));
    REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto panel=menu_panel_geometry(width,height,false,mode);
    REQUIRE(panel);
    const auto* frame=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(frame);
    std::size_t position=frame->visible.size();
    for(std::size_t i=0;i<frame->visible.size();++i)
        if(frame->visible[i].catalog_index==58u)position=i;
    REQUIRE(position<frame->visible.size() &&
            frame->visible[position].expanded);
    std::array<const RecoveredWidget*,5> widgets{};
    REQUIRE(recovered_widget_visible_page("Inv Manager",
        frame->visible[position].settings_offset,
        static_cast<int>(frame->aim_preview_mode),
        static_cast<int>(frame->target_preview_mode),widgets));
    std::size_t widgetPos=widgets.size();
    for(std::size_t j=0;j<widgets.size();++j)
        if(widgets[j]==&all[item])widgetPos=j;
    REQUIRE(widgetPos<widgets.size());
    const auto rect=menu_module_card_rect(
        *frame,panel->logical_width(),position);
    const auto column=menu_setting_column(rect.x,rect.width);
    const auto physical=[&](float x,float y){
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    const auto openAt=physical(
        column.x+column.width*0.4f,
        rect.y+menu_row_body_height(*frame)+
        menu_settings_control_offset(widgets,widgetPos)+10.0f);
    const auto hit=hit_test_menu(*frame,mode,openAt[0],openAt[1]);
    REQUIRE(hit && hit->kind==MenuHitKind::RecoveredChoiceControl &&
            hit->catalog_index==58u && hit->control_index==item);
    const auto baseline=preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(baseline);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    const std::uint64_t epoch=2270u;
    REQUIRE(ledger.request(epoch,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
    const auto click=[&](const std::array<int,2>& point,
                         std::uint64_t tick) {
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            point[0],point[1],tick,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            point[0],point[1],tick+40u,true));
    };
    const auto key=[&](unsigned code,bool edited,
                       std::string_view source_chars={}) {
        // The original GUI supplies typed characters in input.ch[4].
        // Explicit payload must override any US keycode guess.
        const auto pressed=source_chars.empty()?
            preview.fixture_keyboard_event(
                ctx.gl,ctx.dc,width,height,epoch,ledger,router,
                code,true,true):
            preview.fixture_keyboard_typed_event(
                ctx.gl,ctx.dc,width,height,epoch,ledger,router,
                code,true,true,source_chars);
        REQUIRE(pressed.delivery==MenuKeyDelivery::MenuExclusive &&
                !pressed.request_close &&
                pressed.action_applied==edited);
        const auto released=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            code,false,true);
        REQUIRE(released.delivery==MenuKeyDelivery::MenuExclusive &&
                !released.action_applied && !released.request_close);
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    };
    click(openAt,30000u);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto* opened=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(opened && opened->dropdown_preview &&
            opened->dropdown_preview->widget_index==item &&
            opened->dropdown_search_query.empty());
    const auto full=menu_search_popup_geometry(
        *opened,panel->logical_width(),panel->logical_height());
    REQUIRE(full && full->matches.count==9u &&
            full->matches.indices[0]==1u &&
            full->matches.indices[8]==9u);
    // R229: requests rejected by the retained WGL wrapper MUST NOT
    // invalidate a real painted popup hit frame or its navigation focus.
    const auto beforeRejected=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    const auto ownedFocus=preview.focused_row();
    REQUIRE(!preview.scroll_visible(1,8));
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.apply(MenuNavAction::NextSettingsPage));
    REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Combat));
    REQUIRE(!preview.fixture_navigation().focus_visible_row(58u));
    REQUIRE(preview.tab()==MenuTab::Utility &&
            preview.focused_row()==ownedFocus &&
            preview.fixture_painted_frame(ctx.gl,ctx.dc,width,height)==
                beforeRejected &&
            preview.profile_snapshot(ctx.gl,ctx.dc,width,height)==baseline);
    const auto behind=physical(25.0f,20.0f);
    REQUIRE(!hit_test_menu(*opened,mode,behind[0],behind[1]));
    // A live popup must steal Tab from background navigation.
    key(15u,false);
    REQUIRE(preview.tab()==MenuTab::Utility);
    key(2u,false,"!"); // invalid char cannot fall back to key=1
    key(34u,true,"G");
    key(24u,true,"o");
    key(38u,true,"L");
    const auto* filteredFrame=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(filteredFrame &&
            filteredFrame->dropdown_search_query=="GoL");
    const auto search=menu_search_popup_geometry(
        *filteredFrame,panel->logical_width(),
        panel->logical_height());
    REQUIRE(search && search->matches.count==1u &&
            search->matches.indices[0]==7u);
    key(57u,true," "); // trailing space -> real zero results
    const auto* emptyFrame=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(emptyFrame && emptyFrame->dropdown_search_query=="GoL ");
    const auto empty=menu_search_popup_geometry(
        *emptyFrame,panel->logical_width(),panel->logical_height());
    REQUIRE(empty && empty->matches.count==0u &&
            empty->panel.height<search->panel.height);
    const auto oldPoint=physical(
        search->result_cell(0u).x+8.0f,
        search->result_cell(0u).y+12.0f);
    REQUIRE(!hit_test_menu(
        *emptyFrame,mode,oldPoint[0],oldPoint[1]));
    key(14u,true); // backspace restores the original match
    const auto* restored=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(restored && restored->dropdown_search_query=="GoL");
    const auto current=menu_search_popup_geometry(
        *restored,panel->logical_width(),panel->logical_height());
    REQUIRE(current && current->matches.count==1u);
    const auto cell=current->result_cell(0u);
    const auto selectedAt=physical(cell.x+8.0f,cell.y+12.0f);
    const auto selectedHit=hit_test_menu(
        *filteredFrame,mode,selectedAt[0],selectedAt[1]);
    REQUIRE(selectedHit &&
            selectedHit->kind==MenuHitKind::SearchPopupOption &&
            selectedHit->catalog_index==58u &&
            selectedHit->control_index==7u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height)==baseline);
    click(selectedAt,31000u);
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto* applied=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(applied && !applied->dropdown_preview &&
            applied->dropdown_search_query.empty() &&
            applied->recovered_choice_overrides[item] &&
            *applied->recovered_choice_overrides[item]==7u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height)==baseline);
    REQUIRE(!preview.preview_enabled(58u));
    REQUIRE(ledger.tracked_released());
    // The same navigation command is admitted immediately on closure.
    REQUIRE(preview.apply(MenuNavAction::NextTab));
    REQUIRE(preview.tab()==MenuTab::Profiles);
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(
        ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_deliver_live_minecraft_event());
    REQUIRE(!ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R227_WGL_ORIGINAL_SELECT_ITEM_SEARCH_MODAL_PASS\\n";
}

// R230: real first-party WGL modal buttons, with paired synthetic
// input only. No original Minecraft process or live key hook exists.
void verify_r230_wgl_confirmation_ownership(
        Context& ctx,const State& expected) {
    constexpr unsigned width=676u,height=630u;
    constexpr auto mode=MenuPanelMode::CompactOverlay;
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto panel=menu_panel_geometry(width,height,false,mode);
    REQUIRE(panel);
    const auto physical=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    const auto baseline=preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(baseline);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    constexpr std::uint64_t epoch=2300u;
    REQUIRE(ledger.request(epoch,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
    const auto click=[&](const std::array<int,2>& pt,
                         std::uint64_t moment) {
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            pt[0],pt[1],moment,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            pt[0],pt[1],moment+40u,true));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    };
    const auto guarded_key=[&](unsigned key) {
        const auto press=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            key,true,true);
        REQUIRE(press.delivery==MenuKeyDelivery::MenuExclusive &&
                !press.action_applied && !press.request_close);
        const auto release=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            key,false,true);
        REQUIRE(release.delivery==MenuKeyDelivery::MenuExclusive &&
                !release.action_applied && !release.request_close);
        REQUIRE(ledger.tracked_released());
    };
    const auto* main=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(main && !main->unload_confirmation_preview);
    const auto unload=physical(
        menu_nav_x+6.5f*menu_tab_width(panel->logical_width()),
        menu_nav_y+21.0f);
    const auto uhit=hit_test_menu(
        *main,mode,unload[0],unload[1]);
    REQUIRE(uhit && uhit->kind==MenuHitKind::UnloadTab);
    click(unload,33000u);
    const auto* open=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(open && open->unload_confirmation_preview &&
            preview.tab()==MenuTab::Combat &&
            preview.profile_snapshot(ctx.gl,ctx.dc,width,height)==baseline);
    const auto layout=menu_unload_modal_geometry(
        *open,panel->logical_width(),panel->logical_height());
    REQUIRE(layout);
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.scroll_visible(1,3u));
    REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Move));
    REQUIRE(!preview.fixture_navigation().focus_visible_row(0u));
    REQUIRE(preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height)==open);
    guarded_key(15u);  // Tab
    guarded_key(1u);   // Escape may not close entire GUI
    guarded_key(209u); // Page down
    const auto close=physical(
        layout->close_x+10.0f,layout->close_y+10.0f);
    const auto closeHit=hit_test_menu(*open,mode,close[0],close[1]);
    REQUIRE(closeHit && closeHit->kind==MenuHitKind::ModalClose);
    click(close,33100u);
    const auto* afterUnload=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(afterUnload && !afterUnload->unload_confirmation_preview);
    // Five tabs from Combat through Profiles. Background navigation
    // is restored as soon as the modal explicitly closes.
    for(unsigned i=0u;i<5u;++i) {
        REQUIRE(preview.apply(MenuNavAction::NextTab));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    }
    REQUIRE(preview.tab()==MenuTab::Profiles);
    const auto* profiles=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(profiles && profiles->profile_cards.size()==1u &&
            !profiles->profile_action_preview);
    const auto icon=menu_profile_action_rect(
        *profiles,panel->logical_width(),panel->logical_height(),
        0u,0u);
    REQUIRE(icon);
    const auto openAction=physical(
        icon->x+10.0f,icon->y+10.0f);
    const auto aHit=hit_test_menu(
        *profiles,mode,openAction[0],openAction[1]);
    REQUIRE(aHit && aHit->kind==MenuHitKind::ProfileAction &&
            aHit->catalog_index==0u && aHit->control_index==0u);
    click(openAction,33200u);
    const auto* dialog=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(dialog && dialog->profile_action_preview &&
            dialog->profile_action_preview->action==
                MenuProfileAction::Load);
    const auto modal=menu_profile_modal_geometry(
        *dialog,panel->logical_width(),panel->logical_height());
    REQUIRE(modal);
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.scroll_visible(1,3u));
    REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Combat));
    REQUIRE(preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height)==dialog);
    guarded_key(15u);
    guarded_key(1u);
    const auto profileX=physical(
        modal->close_x+10.0f,modal->close_y+10.0f);
    const auto phit=hit_test_menu(
        *dialog,mode,profileX[0],profileX[1]);
    REQUIRE(phit && phit->kind==MenuHitKind::ProfileModalClose);
    click(profileX,33300u);
    const auto* dismissed=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(dismissed && !dismissed->profile_action_preview &&
            dismissed->profile_cards.size()==1u);
    REQUIRE(preview.profile_snapshot(ctx.gl,ctx.dc,width,height)==baseline);
    REQUIRE(preview.apply(MenuNavAction::NextTab));
    REQUIRE(preview.tab()==MenuTab::Combat);
    REQUIRE(ledger.tracked_released());
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(
        ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_deliver_live_minecraft_event() &&
            !ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R230_WGL_UNLOAD_PROFILE_MODAL_INPUT_FENCE_PASS\\n";
}

// R231: the original Profile name/Create/bind chip controls now
// exercise real retained GL11 hit geometry and paired synthetic keys.
void verify_r231_wgl_profile_creation_binding(
        Context& ctx,const State& expected) {
    constexpr unsigned width=676u,height=630u;
    constexpr auto mode=MenuPanelMode::CompactOverlay;
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto panel=menu_panel_geometry(width,height,false,mode);
    REQUIRE(panel);
    const auto point=[&](float x,float y) {
        return std::array<int,2>{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
    };
    for(unsigned n=0u;n<5u;++n) {
        REQUIRE(preview.apply(MenuNavAction::NextTab));
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    }
    REQUIRE(preview.tab()==MenuTab::Profiles);
    const auto baseline=preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(baseline);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    constexpr std::uint64_t epoch=2310u;
    REQUIRE(ledger.request(epoch,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
    const auto click=[&](const std::array<int,2>& xy,
                         std::uint64_t tick) {
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            xy[0],xy[1],tick,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            xy[0],xy[1],tick+40u,true));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    };
    const auto key=[&](unsigned lwjgl,bool edited,
                       std::string_view chars={}) {
        const auto result=chars.empty()?
            preview.fixture_keyboard_event(
                ctx.gl,ctx.dc,width,height,epoch,ledger,router,
                lwjgl,true,true):
            preview.fixture_keyboard_typed_event(
                ctx.gl,ctx.dc,width,height,epoch,ledger,router,
                lwjgl,true,true,chars);
        REQUIRE(result.delivery==MenuKeyDelivery::MenuExclusive &&
                !result.request_close && result.action_applied==edited);
        const auto release=preview.fixture_keyboard_event(
            ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            lwjgl,false,true);
        REQUIRE(release.delivery==MenuKeyDelivery::MenuExclusive &&
                !release.action_applied && !release.request_close);
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    };
    const auto* blank=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(blank && blank->profile_cards.size()==1u &&
            blank->profile_cards[0].name=="Default");
    const auto name=menu_profile_input_rect(
        *blank,panel->logical_width(),panel->logical_height());
    REQUIRE(name);
    const auto field=point(name->x+12.0f,name->y+10.0f);
    const auto fieldHit=hit_test_menu(
        *blank,mode,field[0],field[1]);
    REQUIRE(fieldHit && fieldHit->kind==MenuHitKind::ProfileNameField);
    click(field,40000u);
    const auto* focused=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(focused && focused->profile_name_focused &&
            focused->profile_name_draft.empty());
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.scroll_visible(1,3u));
    REQUIRE(!preview.fixture_navigation().select_tab(MenuTab::Combat));
    REQUIRE(preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height)==focused);
    key(15u,false); // Tab is owned by focused field, not navigation.
    REQUIRE(preview.tab()==MenuTab::Profiles);
    key(30u,true,"N");
    key(18u,true,"e");
    key(17u,true,"w");
    key(34u,false,"/"); // invalid explicit source char; no fallback G.
    key(30u,false); // physical A without source text must NOT append A.
    key(57u,false); // physical Space without source text must NOT append.
    key(48u,false,std::string_view("B\\0Z",5u)); // >4 rejected
    key(48u,true,std::string_view("B\0Z",3u)); // NUL terminates
    key(14u,true); // erase the last B, restoring "New"
    const auto* drafted=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(drafted && drafted->profile_name_focused &&
            drafted->profile_name_draft=="New");
    const auto create=menu_profile_create_rect(
        *drafted,panel->logical_width(),panel->logical_height());
    REQUIRE(create);
    const auto createAt=point(create->x+15.0f,create->y+10.0f);
    const auto createHit=hit_test_menu(
        *drafted,mode,createAt[0],createAt[1]);
    REQUIRE(createHit && createHit->kind==MenuHitKind::ProfileCreate);
    click(createAt,40100u);
    const auto* created=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(created && !created->profile_name_focused &&
            created->profile_name_draft.empty() &&
            created->profile_cards.size()==2u &&
            created->profile_cards[1].name=="New");
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height)==baseline);
    const auto chip=menu_profile_bind_rect(
        *created,panel->logical_width(),panel->logical_height(),1u);
    REQUIRE(chip);
    const auto chipAt=point(chip->x+12.0f,chip->y+10.0f);
    const auto chipHit=hit_test_menu(
        *created,mode,chipAt[0],chipAt[1]);
    REQUIRE(chipHit && chipHit->kind==MenuHitKind::ProfileBindChip &&
            chipHit->catalog_index==1u);
    click(chipAt,40200u);
    const auto* capture=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(capture && capture->profile_bind_capture_index==1u);
    REQUIRE(!preview.apply(MenuNavAction::NextTab));
    REQUIRE(!preview.scroll_visible(1,3u));
    // Tab is a legitimate bind key, not a background page switch.
    key(15u,true);
    const auto* tabBound=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(tabBound && !tabBound->profile_bind_capture_index &&
            tabBound->profile_cards[1].bind=="TAB" &&
            preview.tab()==MenuTab::Profiles);
    // The original R98 gesture gate rejects a repeat of the same
    // chip within 500ms. These are distinct intentional edits.
    const auto f6chip=menu_profile_bind_rect(
        *tabBound,panel->logical_width(),panel->logical_height(),1u);
    REQUIRE(f6chip);
    click(point(f6chip->x+12.0f,f6chip->y+10.0f),41000u);
    key(64u,true); // exact LWJGL F6 -> preview VK_F6.
    const auto* f6Bound=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(f6Bound && !f6Bound->profile_bind_capture_index &&
            f6Bound->profile_cards[1].bind=="F6");
    const auto backspaceChip=menu_profile_bind_rect(
        *f6Bound,panel->logical_width(),panel->logical_height(),1u);
    REQUIRE(backspaceChip);
    click(point(backspaceChip->x+12.0f,
                backspaceChip->y+10.0f),41800u);
    // Unsupported NumLock scan code must not end the source capture.
    key(69u,false);
    const auto* holding=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(holding && holding->profile_bind_capture_index==1u);
    key(14u,true); // valid Backspace binding (nine-glyph label)
    const auto* backspaceBound=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(backspaceBound &&
            !backspaceBound->profile_bind_capture_index &&
            backspaceBound->profile_cards[1].bind=="BACKSPACE");
    const auto wideChip=menu_profile_bind_rect(
        *backspaceBound,panel->logical_width(),
        panel->logical_height(),1u);
    REQUIRE(wideChip && wideChip->width>=75.0f);
    click(point(wideChip->x+12.0f,wideChip->y+10.0f),42600u);
    key(39u,true); // LWJGL semicolon -> native COLON label
    const auto* punctuation=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(punctuation && punctuation->profile_cards[1].bind=="COLON");
    const auto punctuationChip=menu_profile_bind_rect(
        *punctuation,panel->logical_width(),panel->logical_height(),1u);
    REQUIRE(punctuationChip);
    click(point(punctuationChip->x+12.0f,
                punctuationChip->y+10.0f),43400u);
    key(54u,true); // source right SHIFT retains physical side
    const auto* sided=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(sided && sided->profile_cards[1].bind=="RSHIFT");
    const auto clearChip=menu_profile_bind_rect(
        *sided,panel->logical_width(),panel->logical_height(),1u);
    REQUIRE(clearChip);
    click(point(clearChip->x+12.0f,clearChip->y+10.0f),44200u);
    key(1u,true); // Escape clears the binding without closing the GUI.
    const auto* cleared=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(cleared && !cleared->profile_bind_capture_index &&
            cleared->profile_cards[1].bind.empty() &&
            cleared->profile_cards[1].name=="New");
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height)==baseline);
    REQUIRE(preview.apply(MenuNavAction::NextTab));
    REQUIRE(preview.tab()==MenuTab::Combat);
    REQUIRE(ledger.tracked_released());
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(
        ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_deliver_live_minecraft_event() &&
            !ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R231_WGL_PROFILE_NAME_CREATE_BIND_CAPTURE_PASS\\n";
}

// R233: the R125 Piercing model and portable hit test had no
// retained-GL11 pointer dispatch. Exercise the *actually painted*
// controls on the same source-backed GUI, with no game/module action.
void verify_r233_wgl_piercing_controls(
        Context& ctx,const State& expected) {
    constexpr unsigned width=676u,height=630u;
    constexpr auto mode=MenuPanelMode::CompactOverlay;
    constexpr auto down=RetainedPointerPhase::Down;
    constexpr auto up=RetainedPointerPhase::Up;
    WglRetainedMenuPreview preview;
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    REQUIRE(preview.scroll_visible(9,3u));
    REQUIRE(preview.fixture_navigation().focus_visible_row(11u));
    REQUIRE(preview.apply(MenuNavAction::ExpandFocused));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto panel=menu_panel_geometry(width,height,false,mode);
    REQUIRE(panel);
    const auto baseline=preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(baseline);
    MenuInputHandoffLedger ledger;
    MenuKeyboardRouter router(preview.fixture_navigation());
    constexpr std::uint64_t epoch=2330u;
    REQUIRE(ledger.request(epoch,true,true,true,true));
    REQUIRE(ledger.confirm_fixture(epoch,true,true,true));
    REQUIRE(preview.fixture_begin_keyboard(epoch,ledger,router));
    // Convert exact source-row geometry to the same top-left physical
    // client pixels the original WGL hit tester verifies.
    const auto source_control=[&](
            const MenuRenderFrame& frame,std::size_t control)
            ->std::optional<std::array<int,2>> {
        std::size_t row=frame.visible.size();
        for(std::size_t i=0;i<frame.visible.size();++i) {
            if(frame.visible[i].catalog_index==11u) {
                row=i;
                break;
            }
        }
        if(row>=frame.visible.size() || !frame.visible[row].expanded)
            return std::nullopt;
        const auto offset=frame.visible[row].settings_offset;
        if(control<offset || control-offset>=5u)return std::nullopt;
        std::array<const RecoveredWidget*,5> widgets{};
        if(!recovered_widget_visible_page(
               "Piercing",offset,
               static_cast<int>(frame.aim_preview_mode),
               static_cast<int>(frame.target_preview_mode),widgets))
            return std::nullopt;
        const auto local=control-offset;
        if(!widgets[local])return std::nullopt;
        const auto card=menu_module_card_rect(
            frame,panel->logical_width(),row);
        const auto column=menu_setting_column(card.x,card.width);
        const float x=control==0u?
            column.x+column.width*0.5f:
            column.x+column.width-17.0f;
        const float y=card.y+menu_row_body_height(frame)+
            menu_settings_control_offset(widgets,local)+10.0f;
        if(y>=panel->logical_height()-menu_panel_padding)
            return std::nullopt;
        const std::array<int,2> px{
            static_cast<int>(panel->left+std::lround(panel->scale*x)),
            static_cast<int>(panel->top+std::lround(panel->scale*y))};
        const auto hit=hit_test_menu(frame,mode,px[0],px[1]);
        if(!hit || hit->kind!=MenuHitKind::PiercingControl ||
           hit->catalog_index!=11u || hit->control_index!=control)
            return std::nullopt;
        return px;
    };
    const auto click=[&](
            const std::array<int,2>& px,std::uint64_t moment) {
        REQUIRE(preview.fixture_pointer_event(
            down,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            px[0],px[1],moment,true));
        REQUIRE(preview.fixture_pointer_event(
            up,ctx.gl,ctx.dc,width,height,epoch,ledger,router,
            px[0],px[1],moment+40u,true));
        REQUIRE(ledger.tracked_released());
        REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    };
    const auto* initial=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(initial && initial->tab==MenuTab::Combat &&
            initial->piercing_preview.sort==0u &&
            !initial->piercing_preview.enabled[0] &&
            initial->piercing_preview.enabled[6]);
    const auto sort=source_control(*initial,0u);
    REQUIRE(sort);
    click(*sort,50000u);
    const auto* sorted=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(sorted && sorted->piercing_preview.sort==1u &&
            !sorted->piercing_preview.enabled[0] &&
            sorted->piercing_preview.enabled[6] &&
            sorted->piercing_selected==0u);
    const auto firstSwitch=source_control(*sorted,1u);
    REQUIRE(firstSwitch);
    click(*firstSwitch,51000u);
    const auto* enabled=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(enabled && enabled->piercing_preview.sort==1u &&
            enabled->piercing_preview.enabled[0] &&
            enabled->piercing_preview.enabled[6] &&
            enabled->piercing_selected==1u);
    REQUIRE(preview.profile_snapshot(
        ctx.gl,ctx.dc,width,height)==baseline);
    REQUIRE(preview.apply(MenuNavAction::NextSettingsPage));
    REQUIRE(preview.draw(ctx.gl,ctx.dc,width,height));
    const auto* secondPage=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(secondPage && secondPage->piercing_preview.enabled[6] &&
            !source_control(*secondPage,1u));
    const auto finalSwitch=source_control(*secondPage,7u);
    REQUIRE(finalSwitch);
    click(*finalSwitch,53000u);
    const auto* result=preview.fixture_painted_frame(
        ctx.gl,ctx.dc,width,height);
    REQUIRE(result && result->piercing_preview.sort==1u &&
            result->piercing_preview.enabled[0] &&
            !result->piercing_preview.enabled[6] &&
            result->piercing_selected==7u &&
            preview.profile_snapshot(ctx.gl,ctx.dc,width,height)==baseline);
    REQUIRE(ledger.tracked_released());
    REQUIRE(preview.fixture_end_keyboard(ledger,router));
    REQUIRE(preview.fixture_finish_restore(
        ledger,router,true,true,true));
    REQUIRE(preview.close());
    REQUIRE(!ledger.may_deliver_live_minecraft_event() &&
            !ledger.may_suppress_vanilla_event());
    REQUIRE(glGetError()==GL_NO_ERROR);
    same(expected,snapshot());
    std::cout<<"R233_WGL_PIERCING_SOURCE_CONTROL_DISPATCH_PASS\\n";
}

// R237: same pure state machine used by the ORIGINAL WGL render JNI path.
// Test simulated GetKeyState samples, not any synthetic click dispatch.
// A source target is required at both the clean-release arm and the pair.
void verify_r237_original_read_only_pointer_edges() {
    using slinkywinky::GameWindowFocus;
    using slinkywinky::OriginalPointerEdgeTrace;
    constexpr auto foreground=GameWindowFocus::ValidForeground;
    constexpr auto background=GameWindowFocus::ValidBackground;
    constexpr auto refused=GameWindowFocus::Refused;
    OriginalPointerEdgeTrace trace;
    REQUIRE(!trace.may_deliver_minecraft_input() &&
            !trace.may_suppress_vanilla_input());
    REQUIRE(!trace.observe(foreground,false,true,0u));
    REQUIRE(!trace.observe(background,true,false,1u));
    REQUIRE(!trace.observe(foreground,true,true,32u));
    REQUIRE(trace.observe(background,true,false,0u));
    REQUIRE(trace.observe(foreground,true,true,1u)); // held on entry inert
    REQUIRE(trace.counts()[2]==0u);
    REQUIRE(trace.observe(foreground,true,true,0u)); // arm
    REQUIRE(trace.observe(foreground,true,true,1u)); // source down
    REQUIRE(trace.observe(foreground,true,true,1u)); // repeat held
    REQUIRE(trace.observe(foreground,true,true,0u)); // paired release
    REQUIRE(trace.counts()[2]==1u && trace.counts()[3]==1u &&
            trace.counts()[4]==0u);
    REQUIRE(trace.observe(foreground,true,true,1u));
    REQUIRE(trace.observe(foreground,true,false,1u)); // dragged outside
    REQUIRE(trace.observe(foreground,true,true,0u));
    REQUIRE(trace.observe(foreground,true,true,1u));
    REQUIRE(trace.observe(foreground,true,true,3u)); // chording cancels
    REQUIRE(trace.observe(foreground,true,true,0u));
    REQUIRE(trace.observe(foreground,true,true,1u));
    REQUIRE(trace.observe(background,true,false,0u)); // revoke held
    REQUIRE(trace.observe(foreground,true,true,0u));
    REQUIRE(trace.observe(foreground,true,true,1u));
    REQUIRE(trace.observe(foreground,true,true,0u)); // cannot revive
    REQUIRE(trace.counts()[2]==4u &&
            trace.counts()[3]==1u && trace.counts()[4]==3u);
    std::atomic<bool> foreignRefused{false};
    std::thread foreign([&]{
        foreignRefused=!trace.observe(foreground,true,true,0u) &&
            trace.counts()==std::array<std::uint64_t,5>{};
    });
    foreign.join();
    REQUIRE(foreignRefused);
    const auto counts=trace.counts();
    REQUIRE(counts[0]==19u && counts[1]==17u &&
            counts[2]==4u && counts[3]==1u && counts[4]==3u);
    std::cout<<"R237_ORIGINAL_READ_ONLY_SOURCE_POINTER_EDGE_TRACE_PASS\\n";
}

int main() {
    try {
        verify_r237_original_read_only_pointer_edges();
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
        verify_r200_native_color_popup(context,before);
        verify_r227_wgl_select_item(context,before);
        verify_r230_wgl_confirmation_ownership(context,before);
        verify_r231_wgl_profile_creation_binding(context,before);
        verify_r233_wgl_piercing_controls(context,before);
        verify_r211_source_click_request(context,before);
        REQUIRE(glGetError()==GL_NO_ERROR);
        same(before,snapshot());
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"R159 retained pointer fixture failure: "<<e.what()<<"\n";
        return 1;
    }
}
