// R166: DISPOSABLE Windows JNI test DLL, not linked into the agent or
// slinkywinky_jni.dll. No original Minecraft process, hooks or input.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <jni.h>
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include <utility>
#include "slinkywinky/menu_profile.hpp"
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace {
constexpr unsigned kWidth=676u,kHeight=630u;
constexpr wchar_t kWindowClass[]=L"SlinkyWinky-R166-Disposable-JNI-WGL";
std::mutex gate;
struct DisposableSession final {
    HWND hwnd=nullptr;
    HDC dc=nullptr;
    HGLRC gl=nullptr;
    bool registered=false;
    std::thread::id owner=std::this_thread::get_id();
    std::unique_ptr<slinkywinky::WglRetainedMenuPreview> ui;
    bool isOwner() const {return owner==std::this_thread::get_id();}
    bool valid() const {
        return isOwner() && hwnd && dc && gl && ui && ui->active() &&
               wglGetCurrentContext()==gl && wglGetCurrentDC()==dc;
    }
    bool open() {
        HINSTANCE instance=GetModuleHandleW(nullptr);
        WNDCLASSW wc{};
        wc.style=CS_OWNDC;
        wc.lpfnWndProc=DefWindowProcW;
        wc.hInstance=instance;
        wc.lpszClassName=kWindowClass;
        registered=RegisterClassW(&wc)!=0;
        if(!registered)return false;
        hwnd=CreateWindowW(kWindowClass,L"R166 isolated Java 8 JNI fixture",
                           WS_POPUP,0,0,static_cast<int>(kWidth),
                           static_cast<int>(kHeight),nullptr,nullptr,
                           instance,nullptr);
        if(!hwnd)return false;
        dc=GetDC(hwnd);
        if(!dc)return false;
        PIXELFORMATDESCRIPTOR descriptor{};
        descriptor.nSize=sizeof(descriptor);
        descriptor.nVersion=1;
        descriptor.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
        descriptor.iPixelType=PFD_TYPE_RGBA;
        descriptor.cColorBits=24;
        descriptor.cDepthBits=16;
        descriptor.iLayerType=PFD_MAIN_PLANE;
        const int format=ChoosePixelFormat(dc,&descriptor);
        if(!format || !SetPixelFormat(dc,format,&descriptor))return false;
        gl=wglCreateContext(dc);
        if(!gl || !wglMakeCurrent(dc,gl))return false;
        glViewport(0,0,kWidth,kHeight);
        ui=std::make_unique<slinkywinky::WglRetainedMenuPreview>();
        return ui->draw(gl,dc,kWidth,kHeight) && glGetError()==GL_NO_ERROR;
    }
    bool close() {
        if(!isOwner())return false;
        const bool menuOk=!ui || !ui->active() || ui->close();
        ui.reset();
        wglMakeCurrent(nullptr,nullptr);
        if(gl)wglDeleteContext(gl);
        gl=nullptr;
        if(dc && hwnd)ReleaseDC(hwnd,dc);
        dc=nullptr;
        if(hwnd)DestroyWindow(hwnd);
        hwnd=nullptr;
        if(registered)UnregisterClassW(kWindowClass,GetModuleHandleW(nullptr));
        registered=false;
        return menuOk;
    }
    // This source-validated target is the same first module toggle reached
    // by tests/test_windows_retained_pointer.cpp, in physical CLIENT pixels.
    std::optional<std::pair<int,int>> firstToggle() const {
        if(!valid())return std::nullopt;
        slinkywinky::MenuState mirror;
        slinkywinky::MenuProfileBridge profile(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(166u,kWidth,kHeight))return std::nullopt;
        const auto frame=frames.next(mirror,&profile);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f)return std::nullopt;
        const auto rect=slinkywinky::menu_module_card_rect(
            *frame,panel->logical_width(),0u);
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround((rect.x+rect.width-30.0f)*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround((rect.y+17.0f)*panel->scale));
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        frames.detach();
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::Toggle ||
           hit->catalog_index!=0u)return std::nullopt;
        return std::pair<int,int>{x,y};
    }
};
std::unique_ptr<DisposableSession> session;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_begin(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(session)return JNI_FALSE;
    try {
        auto created=std::make_unique<DisposableSession>();
        if(!created->open()) {
            created->close();
            return JNI_FALSE;
        }
        session=std::move(created);
        return JNI_TRUE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_toggleTarget(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto hit=session->firstToggle();
        if(!hit)return nullptr;
        const jint data[2]={hit->first,hit->second};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,data);
        return result;
    }catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_pointer(
        JNIEnv*,jclass,jint phase,jint x,jint y,jlong epoch,
        jboolean exclusive,jboolean foreground,jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() || phase<0 || phase>3 ||
       epoch<=0 || width<320 || height<240 || width>8192 ||
       height>8192)return JNI_FALSE;
    try {
        return session->ui->pointer(
            static_cast<slinkywinky::RetainedPointerPhase>(phase),
            session->gl,session->dc,
            static_cast<unsigned>(width),static_cast<unsigned>(height),
            x,y,static_cast<std::uint64_t>(epoch),
            exclusive==JNI_TRUE,foreground==JNI_TRUE)?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jstring JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_profile(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto snapshot=session->ui->profile_snapshot(
            session->gl,session->dc,kWidth,kHeight);
        return snapshot?env->NewStringUTF(snapshot->c_str()):nullptr;
    }catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_redraw(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid())return JNI_FALSE;
    try {
        return session->ui->draw(session->gl,session->dc,kWidth,kHeight) &&
               glGetError()==GL_NO_ERROR?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_end(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->isOwner())return JNI_FALSE;
    try {
        const bool closed=session->close();
        session.reset();
        return closed?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
