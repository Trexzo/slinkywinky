#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <jni.h>
#include <cstdlib>
#include <memory>
#include "slinkywinky/windows_wgl_frame_probe.hpp"

/*
 * R76 TEST DLL ONLY. Creates/destroys a disposable WGL context belonging to
 * this Java 8 fixture, never discovers or attaches to a running game.
 */
namespace {
constexpr wchar_t CLASS_NAME[]=L"SlinkyWinky-R76-Disposable-JNI-WGL";
struct Context {
    HWND hwnd=nullptr;
    HDC dc=nullptr;
    HGLRC rc=nullptr;
    bool registered=false;
};
Context state;
std::unique_ptr<slinkywinky::WglFrameContextProbe> probe;
LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM w,LPARAM l){
    return DefWindowProcW(hwnd,message,w,l);
}
bool enabled() noexcept {
    wchar_t value[4]{};
    return GetEnvironmentVariableW(L"SLINKYWINKY_R76_WGL_FIXTURE",value,4)==1 &&
           value[0]==L'1';
}
void dispose() noexcept {
    probe.reset();
    if(state.rc) {
        if(wglGetCurrentContext()==state.rc)
            wglMakeCurrent(nullptr,nullptr);
        wglDeleteContext(state.rc);
    }
    if(state.dc && state.hwnd)ReleaseDC(state.hwnd,state.dc);
    if(state.hwnd)DestroyWindow(state.hwnd);
    if(state.registered)
        UnregisterClassW(CLASS_NAME,GetModuleHandleW(nullptr));
    state=Context{};
}
bool setup() {
    if(!enabled() || state.hwnd || probe)return false;
    WNDCLASSW wc{};
    wc.style=CS_OWNDC;
    wc.lpfnWndProc=window_proc;
    wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=CLASS_NAME;
    if(!RegisterClassW(&wc))return false;
    state.registered=true;
    state.hwnd=CreateWindowExW(0,CLASS_NAME,L"Disposable R76 WGL",
         WS_POPUP,0,0,640,480,nullptr,nullptr,wc.hInstance,nullptr);
    if(!state.hwnd)return false;
    state.dc=GetDC(state.hwnd);
    if(!state.dc)return false;
    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize=sizeof(descriptor);
    descriptor.nVersion=1;
    descriptor.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    descriptor.iPixelType=PFD_TYPE_RGBA;
    descriptor.cColorBits=24;
    descriptor.cDepthBits=16;
    descriptor.iLayerType=PFD_MAIN_PLANE;
    const int chosen=ChoosePixelFormat(state.dc,&descriptor);
    if(!chosen || !SetPixelFormat(state.dc,chosen,&descriptor))return false;
    state.rc=wglCreateContext(state.dc);
    if(!state.rc || !wglMakeCurrent(state.dc,state.rc))return false;
    glViewport(0,0,640,480); // Only the disposable fixture changes GL state.
    probe.reset(new slinkywinky::WglFrameContextProbe(8));
    return probe->open();
}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeBegin(JNIEnv*,jclass) {
    // A duplicate request must never tear down the active owner context.
    if(state.hwnd || probe)return JNI_FALSE;
    if(!setup()){
        dispose();
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeSample(JNIEnv*,jclass) {
    return probe?static_cast<jint>(probe->sample()):0;
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeCount(JNIEnv*,jclass) {
    return probe?static_cast<jint>(probe->accepted()):-1;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeClearCurrent(JNIEnv*,jclass) {
    return state.rc && wglGetCurrentContext()==state.rc &&
           wglMakeCurrent(nullptr,nullptr)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeRestoreCurrent(JNIEnv*,jclass) {
    return state.rc && state.dc && wglMakeCurrent(state.dc,state.rc)?
           JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeMismatchViewport(JNIEnv*,jclass) {
    if(!state.rc || wglGetCurrentContext()!=state.rc)return JNI_FALSE;
    glViewport(0,0,400,300);
    return JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeFinish(JNIEnv*,jclass) {
    // Only Java fixture's original thread may tear down this native context.
    if(!state.hwnd || !probe || wglGetCurrentContext()!=state.rc)
        return JNI_FALSE;
    const bool clean=probe->close();
    dispose();
    return clean?JNI_TRUE:JNI_FALSE;
}
#endif
