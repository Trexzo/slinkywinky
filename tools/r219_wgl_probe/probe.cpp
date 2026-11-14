// R219: read-only diagnostic of the R67 disposable WGL pixel-format request.
// No game window, launcher, injector, input, or account is accessed.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <array>
#include <cstdio>
namespace {
constexpr wchar_t cls[]=L"SlinkyWinky-R219-WGL-Format";
LRESULT CALLBACK wndproc(HWND h,UINT m,WPARAM w,LPARAM l){
    return DefWindowProcW(h,m,w,l);
}
struct Fixture {
    HINSTANCE inst=GetModuleHandleW(nullptr);
    HWND hwnd=nullptr;
    HDC dc=nullptr;
    HGLRC gl=nullptr;
    bool registered=false;
    ~Fixture(){
        wglMakeCurrent(nullptr,nullptr);
        if(gl)wglDeleteContext(gl);
        if(dc&&hwnd)ReleaseDC(hwnd,dc);
        if(hwnd)DestroyWindow(hwnd);
        if(registered)UnregisterClassW(cls,inst);
    }
};
int fail(const char* reason) {
    std::fprintf(stderr,"R219_WGL_PROBE_FAILED=%s\n",reason);
    return 1;
}
}
int main(){
    Fixture f;
    WNDCLASSW wc{};
    wc.style=CS_OWNDC;wc.lpfnWndProc=wndproc;
    wc.hInstance=f.inst;wc.lpszClassName=cls;
    if(!RegisterClassW(&wc))return fail("register");
    f.registered=true;
    f.hwnd=CreateWindowW(cls,L"R219 disposable WGL",WS_POPUP,
                         0,0,676,630,nullptr,nullptr,f.inst,nullptr);
    if(!f.hwnd)return fail("window");
    f.dc=GetDC(f.hwnd);
    if(!f.dc)return fail("dc");
    PIXELFORMATDESCRIPTOR requested{};
    requested.nSize=sizeof(requested);
    requested.nVersion=1;
    requested.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    requested.iPixelType=PFD_TYPE_RGBA;
    requested.cColorBits=24;
    requested.cDepthBits=16;
    requested.iLayerType=PFD_MAIN_PLANE;
    const int pf=ChoosePixelFormat(f.dc,&requested);
    if(!pf)return fail("choose-format");
    PIXELFORMATDESCRIPTOR actual{};
    if(!DescribePixelFormat(f.dc,pf,sizeof(actual),&actual))
        return fail("describe-format");
    if(!SetPixelFormat(f.dc,pf,&requested))return fail("set-format");
    f.gl=wglCreateContext(f.dc);
    if(!f.gl||!wglMakeCurrent(f.dc,f.gl))return fail("context");
    GLint red=-1,green=-1,blue=-1,alpha=-1,depth=-1;
    glGetIntegerv(GL_RED_BITS,&red);
    glGetIntegerv(GL_GREEN_BITS,&green);
    glGetIntegerv(GL_BLUE_BITS,&blue);
    glGetIntegerv(GL_ALPHA_BITS,&alpha);
    glGetIntegerv(GL_DEPTH_BITS,&depth);
    if(glGetError()!=GL_NO_ERROR)return fail("query-bits");
    glViewport(0,0,676,630);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    glOrtho(0,676,630,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDrawBuffer(GL_BACK);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    glColor3ub(0xF3,0x5D,0x12);
    glBegin(GL_QUADS);
    glVertex2f(665,17);glVertex2f(669,17);
    glVertex2f(669,20);glVertex2f(665,20);
    glEnd();
    std::array<GLubyte,3> rgb{};
    glReadBuffer(GL_BACK);
    glReadPixels(666,612,1,1,GL_RGB,GL_UNSIGNED_BYTE,rgb.data());
    if(glGetError()!=GL_NO_ERROR)return fail("sample");
    std::printf("R219_WGL_PFD_INDEX=%d COLOR_BITS=%u RGB_BITS=%u,%u,%u ALPHA=%u DEPTH=%u\n",
        pf,unsigned(actual.cColorBits),unsigned(actual.cRedBits),
        unsigned(actual.cGreenBits),unsigned(actual.cBlueBits),
        unsigned(actual.cAlphaBits),unsigned(actual.cDepthBits));
    std::printf("R219_WGL_GL_BITS=%d,%d,%d,%d DEPTH=%d\n",
        red,green,blue,alpha,depth);
    std::printf("R219_WGL_REQUESTED_ACCENT=F35D12 DRAWN_ACCENT=%02X%02X%02X\n",
        unsigned(rgb[0]),unsigned(rgb[1]),unsigned(rgb[2]));
    return 0;
}
