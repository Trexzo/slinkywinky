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
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <cmath>

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
    HGLRC alternate=nullptr;
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
    if(state.alternate){
        if(wglGetCurrentContext()==state.alternate)
            wglMakeCurrent(nullptr,nullptr);
        wglDeleteContext(state.alternate);
    }
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
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeDrawPreview(JNIEnv*,jclass) {
    // Test-only bridge to the existing original first-party native GL1.1
    // menu renderer. Never call on a game process or without eight accepted
    // WGL owner samples, and never create or activate a context here.
    if(!probe || !state.rc || !state.dc ||
       probe->invalid() || probe->active() || probe->accepted()!=8 ||
       wglGetCurrentContext()!=state.rc ||
       wglGetCurrentDC()!=state.dc)return JNI_FALSE;
    GLint beforeViewport[4]{},afterViewport[4]{};
    GLint beforeMode=0,afterMode=0,beforeAttrib=0,afterAttrib=0;
    GLint beforeClient=0,afterClient=0;
    GLfloat beforeColor[4]{},afterColor[4]{};
    glGetIntegerv(GL_VIEWPORT,beforeViewport);
    glGetIntegerv(GL_MATRIX_MODE,&beforeMode);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&beforeAttrib);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH,&beforeClient);
    glGetFloatv(GL_CURRENT_COLOR,beforeColor);
    slinkywinky::WglMenuOverlayHost host;
    if(!host.open(state.rc,state.dc,640,480))return JNI_FALSE;
    slinkywinky::MenuState menu;
    const bool drawn=host.draw(menu);
    const bool closed=host.close();
    glGetIntegerv(GL_VIEWPORT,afterViewport);
    glGetIntegerv(GL_MATRIX_MODE,&afterMode);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&afterAttrib);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH,&afterClient);
    glGetFloatv(GL_CURRENT_COLOR,afterColor);
    if(!drawn || !closed || beforeMode!=afterMode ||
       beforeAttrib!=afterAttrib || beforeClient!=afterClient)return JNI_FALSE;
    for(unsigned i=0;i<4;i++) {
        if(beforeViewport[i]!=afterViewport[i] ||
           std::fabs(beforeColor[i]-afterColor[i])>0.00001f)
            return JNI_FALSE;
    }
    return (wglGetCurrentContext()==state.rc &&
            wglGetCurrentDC()==state.dc)?JNI_TRUE:JNI_FALSE;
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
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeUseAlternate(JNIEnv*,jclass) {
    if(!state.rc || !state.dc || state.alternate ||
       wglGetCurrentContext()!=state.rc)return JNI_FALSE;
    state.alternate=wglCreateContext(state.dc);
    if(!state.alternate)return JNI_FALSE;
    if(!wglMakeCurrent(state.dc,state.alternate)){
        wglDeleteContext(state.alternate);
        state.alternate=nullptr;
        return JNI_FALSE;
    }
    glViewport(0,0,640,480);
    return JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R76WglJvmSmoke_nativeReturnOriginal(JNIEnv*,jclass) {
    if(!state.alternate || !state.rc || !state.dc ||
       wglGetCurrentContext()!=state.alternate)return JNI_FALSE;
    if(!wglMakeCurrent(state.dc,state.rc))return JNI_FALSE;
    const BOOL deleted=wglDeleteContext(state.alternate);
    state.alternate=nullptr;
    return deleted?JNI_TRUE:JNI_FALSE;
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
