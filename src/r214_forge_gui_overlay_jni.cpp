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
        const bool closed=session->menu.close();
        session->handoff.retire();
        session->feed.retire();
        session->closed=true;
        session.reset();
        return closed?JNI_TRUE:JNI_FALSE;
    } catch(...) {return JNI_FALSE;}
}
#endif
