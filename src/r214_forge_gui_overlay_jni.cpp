#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <jni.h>
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/gui_module_control_handoff.hpp"
#include "slinkywinky/module_arraylist.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu.hpp"
#include <algorithm>
#include <string_view>
#include <memory>
#include <thread>
#include <cstdint>
#include <limits>
#include <array>

namespace {
using namespace slinkywinky;
// R214: deliberately isolated Forge-Java owner + original GL11 renderer.
// Never registered in the original injector, never hooks LWJGL/OS input.
struct Session {
    const std::thread::id owner=std::this_thread::get_id();
    WglRetainedMenuPreview menu;
    ModuleArrayListFeed feed;
    GuiModuleControlHandoff handoff;
    MenuMouseGesture source_mouse;
    bool source_held=false;
    std::uint64_t mouse_source_sequence=0;
    std::uint64_t mouse_generation=0;
    std::uint64_t mouse_down_ms=0;
    HGLRC mouse_context=nullptr;
    HDC mouse_dc=nullptr;
    std::uint64_t sequence=0;
    std::uint64_t generation=0;
    bool configured=false;
    bool closed=false;
};
thread_local std::unique_ptr<Session> session;
bool owned() noexcept {
    return session && !session->closed &&
           std::this_thread::get_id()==session->owner;
}
bool surface(jint width,jint height,HGLRC& gl,HDC& dc) noexcept {
    if(width<676 || width>8192 || height<480 || height>8192)
        return false;
    gl=wglGetCurrentContext();
    dc=wglGetCurrentDC();
    if(!gl || !dc)return false;
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    return viewport[0]==0 && viewport[1]==0 &&
        viewport[2]==width && viewport[3]==height;
}
jlongArray packed(JNIEnv* env,const std::array<jlong,5>& parts) {
    if(!env)return nullptr;
    jlongArray answer=env->NewLongArray(5);
    if(answer)env->SetLongArrayRegion(answer,0,5,parts.data());
    return answer;
}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeOpen(
    JNIEnv*,jclass,jlong sequence,jlong generation,jboolean requested,
    jboolean active) {
    if(session || sequence!=1 || generation!=0 ||
       requested!=JNI_FALSE || active!=JNI_FALSE)return JNI_FALSE;
    try {
        session=std::make_unique<Session>();
        ModuleArrayListSnapshot snap;
        snap.source_sequence=1;
        snap.world_generation=0;
        snap.world_ready=false;
        snap.modules.push_back({"Sprint",false,false,0});
        if(!session->feed.publish(snap)) {
            session.reset();
            return JNI_FALSE;
        }
        session->sequence=1;
        return JNI_TRUE;
    } catch(...) {
        session.reset();
        return JNI_FALSE;
    }
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativePaint(
    JNIEnv*,jclass,jint width,jint height) {
    if(!owned())return JNI_FALSE;
    HGLRC gl=nullptr;HDC dc=nullptr;
    if(!surface(width,height,gl,dc))return JNI_FALSE;
    try {
        if(!session->menu.draw(gl,dc,width,height))return JNI_FALSE;
        if(!session->configured) {
            if(!session->menu.apply(MenuNavAction::NextTab))return JNI_FALSE;
            for(int i=0;i<8;++i)
                if(!session->menu.apply(MenuNavAction::NextRow))
                    return JNI_FALSE;
            // Sprint must be physically inside the compact menu viewport.
            if(!session->menu.scroll_visible(5,3))return JNI_FALSE;
            if(!session->menu.draw(gl,dc,width,height))return JNI_FALSE;
            if(!session->menu.focused_row() ||
                *session->menu.focused_row()!=25u)return JNI_FALSE;
            session->configured=true;
        }
        if(!session->feed.latest())return JNI_FALSE;
        WglMenuOverlayHost hud;
        if(!hud.open(gl,dc,width,height))return JNI_FALSE;
        const bool painted=hud.draw_active_modules(session->feed);
        const bool closed=hud.close();
        return (painted && closed)?JNI_TRUE:JNI_FALSE;
    } catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeRequest(
    JNIEnv* env,jclass,jint width,jint height) {
    if(!owned() || !session->configured)return nullptr;
    HGLRC gl=nullptr;HDC dc=nullptr;
    if(!surface(width,height,gl,dc))return nullptr;
    try {
        if(!session->menu.queue_focused_runtime_toggle(
               session->handoff,session->feed,gl,dc,width,height,
               session->sequence))return nullptr;
        const auto request=session->handoff.take_on_module_owner();
        if(!request || request->canonical_name!="Sprint" ||
           request->observed_source_sequence!=session->sequence ||
           request->world_generation!=session->generation)return nullptr;
        return packed(env,{
            static_cast<jlong>(request->id),
            request->enable?1L:0L,
            static_cast<jlong>(request->observed_source_sequence),
            static_cast<jlong>(request->world_generation),
            static_cast<jlong>(session->menu.drawn_frames())});
    } catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeAck(
    JNIEnv*,jclass,jlong id,jlong sequence,jlong generation,
    jboolean requested,jboolean active) {
    if(!owned() || id<=0 || sequence<=0 || generation<0 ||
       requested!=JNI_TRUE && requested!=JNI_FALSE ||
       active!=JNI_TRUE && active!=JNI_FALSE ||
       (active==JNI_TRUE && requested==JNI_FALSE) ||
       (active==JNI_TRUE && generation==0) ||
       sequence<=static_cast<jlong>(session->sequence) ||
       generation<static_cast<jlong>(session->generation))
        return JNI_FALSE;
    try {
        ModuleArrayListSnapshot snapshot;
        snapshot.source_sequence=static_cast<std::uint64_t>(sequence);
        snapshot.world_generation=static_cast<std::uint64_t>(generation);
        snapshot.world_ready=(generation!=0);
        snapshot.modules.push_back({"Sprint",requested==JNI_TRUE,
            active==JNI_TRUE,static_cast<std::uint64_t>(generation)});
        if(!session->handoff.acknowledge_on_module_owner(
            static_cast<std::uint64_t>(id),snapshot,session->feed))
            return JNI_FALSE;
        session->sequence=snapshot.source_sequence;
        session->generation=snapshot.world_generation;
        return JNI_TRUE;
    } catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeView(
    JNIEnv* env,jclass) {
    if(!owned())return nullptr;
    try {
        const auto snap=session->feed.latest();
        if(!snap || snap->modules.size()!=1u ||
           snap->modules.front().canonical_name!="Sprint")
            return nullptr;
        const auto display=make_module_arraylist_display(*snap);
        if(!display)return nullptr;
        const auto& sprint=snap->modules.front();
        return packed(env,{
            static_cast<jlong>(snap->source_sequence),
            sprint.requested_enabled?1L:0L,
            sprint.active_in_world?1L:0L,
            static_cast<jlong>(display->active_names.size()),
            static_cast<jlong>(session->menu.drawn_frames())});
    } catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeClose(
    JNIEnv*,jclass) {
    if(!owned())return JNI_FALSE;
    try {
        session->source_mouse.cancel();
        session->source_held=false;
        const bool closed=session->menu.close();
        session->handoff.retire();
        session->feed.retire();
        session->closed=true;
        session.reset();
        return closed?JNI_TRUE:JNI_FALSE;
    } catch(...) {return JNI_FALSE;}
}

// R215: Minecraft's OWN GuiScreen callbacks, not a WndProc or LWJGL hook.
// The source-pinned painted GL11 toggle must be the SAME hit for both
// mouse edges. Repaint is allowed only while the model remains equivalent.
namespace {
bool sprint_source_hit(const MenuRenderFrame& frame,int x,int y) noexcept {
    const auto hit=hit_test_menu(frame,MenuPanelMode::CompactOverlay,x,y);
    return hit && hit->kind==MenuHitKind::Toggle &&
        hit->catalog_index==25u &&
        hit->catalog_index<official_module_catalog().size() &&
        std::string_view(official_module_catalog()[hit->catalog_index].name)
            =="Sprint";
}
const MenuRenderFrame* source_frame(HGLRC gl,HDC dc,
                                   jint width,jint height) noexcept {
    if(!owned() || !session->configured)return nullptr;
    const auto* frame=session->menu.fixture_painted_frame(
        gl,dc,static_cast<unsigned>(width),static_cast<unsigned>(height));
    if(!frame || frame->appearance.wide ||
       !session->menu.focused_row() ||
       *session->menu.focused_row()!=25u)return nullptr;
    return frame;
}
void cancel_source() noexcept {
    if(!owned())return;
    session->source_mouse.cancel();
    session->source_held=false;
    session->mouse_source_sequence=session->mouse_generation=
        session->mouse_down_ms=0;
    session->mouse_context=nullptr;
    session->mouse_dc=nullptr;
}
}
// Source coordinates for an in-process SCRIPTED test, not an input event
// or permission to capture OS buttons. Physical input uses real mouseClicked.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeTogglePoint(
        JNIEnv* env,jclass,jint width,jint height) {
    if(!env || !owned() || session->source_held)return nullptr;
    HGLRC gl=nullptr;HDC dc=nullptr;
    if(!surface(width,height,gl,dc))return nullptr;
    const auto* frame=source_frame(gl,dc,width,height);
    if(!frame)return nullptr;
    const auto panel=menu_panel_geometry(
        static_cast<unsigned>(width),static_cast<unsigned>(height),
        frame->appearance.wide,MenuPanelMode::CompactOverlay);
    if(!panel)return nullptr;
    const int left=static_cast<int>(panel->left)+4;
    const int top=static_cast<int>(panel->top)+4;
    const int right=static_cast<int>(panel->left+panel->width)-4;
    const int bottom=static_cast<int>(panel->top+panel->height)-4;
    for(int y=top;y<bottom;y+=3) {
        for(int x=left;x<right;x+=3) {
            // Keep the scripted point deep inside the actual painted hit
            // rather than on a pixel edge that scale conversion can miss.
            if(!sprint_source_hit(*frame,x,y) ||
               !sprint_source_hit(*frame,x+3,y) ||
               !sprint_source_hit(*frame,x,y+3) ||
               !sprint_source_hit(*frame,x+3,y+3))continue;
            const jint position[2]={x+2,y+2};
            jintArray result=env->NewIntArray(2);
            if(result)env->SetIntArrayRegion(result,0,2,position);
            return result;
        }
    }
    return nullptr;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeMouseDown(
        JNIEnv*,jclass,jint width,jint height,jint x,jint y,jlong ms) {
    if(!owned() || ms<=0 || session->source_held)return JNI_FALSE;
    HGLRC gl=nullptr;HDC dc=nullptr;
    if(!surface(width,height,gl,dc))return JNI_FALSE;
    const auto* frame=source_frame(gl,dc,width,height);
    if(!frame || !sprint_source_hit(*frame,x,y))return JNI_FALSE;
    const auto snap=session->feed.latest();
    if(!snap || snap->source_sequence!=session->sequence ||
       snap->world_generation!=session->generation ||
       !make_module_arraylist_display(*snap))
        return JNI_FALSE;
    if(!session->source_mouse.press(
        *frame,MenuPanelMode::CompactOverlay,x,y,
        static_cast<std::uint64_t>(ms),true))
        return JNI_FALSE;
    session->source_held=true;
    session->mouse_source_sequence=snap->source_sequence;
    session->mouse_generation=snap->world_generation;
    session->mouse_down_ms=static_cast<std::uint64_t>(ms);
    session->mouse_context=gl;
    session->mouse_dc=dc;
    return JNI_TRUE;
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeMouseUp(
        JNIEnv* env,jclass,jint width,jint height,jint x,jint y,jlong ms) {
    if(!env || !owned() || !session->source_held)return nullptr;
    const auto deny=[]() noexcept -> jlongArray {
        cancel_source();
        return nullptr;
    };
    HGLRC gl=nullptr;HDC dc=nullptr;
    if(!surface(width,height,gl,dc) ||
       gl!=session->mouse_context || dc!=session->mouse_dc ||
       ms<=0 || static_cast<std::uint64_t>(ms)<=session->mouse_down_ms)
        return deny();
    const auto* frame=source_frame(gl,dc,width,height);
    const auto snap=session->feed.latest();
    if(!frame || !sprint_source_hit(*frame,x,y) ||
       !snap || snap->source_sequence!=session->mouse_source_sequence ||
       snap->world_generation!=session->mouse_generation ||
       snap->source_sequence!=session->sequence ||
       !make_module_arraylist_display(*snap))
        return deny();
    const auto released=session->source_mouse.release(
        *frame,MenuPanelMode::CompactOverlay,x,y,
        static_cast<std::uint64_t>(ms),true);
    if(!released || released->kind!=MenuHitKind::Toggle ||
       released->catalog_index!=25u)return deny();
    const auto sequence=session->mouse_source_sequence;
    cancel_source();
    try {
        if(!session->menu.queue_focused_runtime_toggle(
            session->handoff,session->feed,gl,dc,width,height,sequence))
            return nullptr;
        const auto request=session->handoff.take_on_module_owner();
        if(!request || request->canonical_name!="Sprint" ||
           request->observed_source_sequence!=sequence ||
           request->world_generation!=session->generation)
            return nullptr;
        return packed(env,{
            static_cast<jlong>(request->id),
            request->enable?1L:0L,
            static_cast<jlong>(request->observed_source_sequence),
            static_cast<jlong>(request->world_generation),
            static_cast<jlong>(session->menu.drawn_frames())});
    } catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_forgegui_R214NativeMenuOverlay_nativeMouseCancel(
    JNIEnv*,jclass) {
    if(!owned())return JNI_FALSE;
    cancel_source();
    return JNI_TRUE;
}
#endif
