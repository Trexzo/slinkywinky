// R68 Windows desktop OpenGL preview. NOT a game hook or injector.
// Uses first-party GL1.1 text and geometry with zero gameplay actions.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace {
using slinkywinky::MenuTab;
slinkywinky::MenuState menu;
slinkywinky::MenuProfileBridge profile(menu);
slinkywinky::MenuNavigation navigation(menu,profile);
slinkywinky::MenuRenderSession session;
slinkywinky::WglMenuOverlayHost overlayHost;
constexpr wchar_t title[]=L"SlinkyWinky GL1.1 Menu Preview - Offline";
constexpr wchar_t className[]=L"SlinkyWinkyGL11Preview";
HDC device=nullptr;
HGLRC graphics=nullptr;
HWND window=nullptr;
std::filesystem::path preferences;
std::filesystem::path profilePreferences;
bool overlayDemo=false; // Standalone-only demonstration, never attached to Minecraft.

std::filesystem::path settings_file() {
    wchar_t buffer[32768]{};
    const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",buffer,32768);
    if(len==0 || len>=32768)return {};
    return std::filesystem::path(buffer)/L"SlinkyWinky"/L"menu-gl-preview.conf";
}
std::filesystem::path profile_file() {
    wchar_t buffer[32768]{};
    const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",buffer,32768);
    if(len==0 || len>=32768)return {};
    return std::filesystem::path(buffer)/L"SlinkyWinky"/
           L"module-profile-preview.conf";
}
void persist() {
    if(!preferences.empty())
        slinkywinky::save_menu_appearance(preferences,menu.appearance());
}
void refresh_caption(HWND hwnd) {
    std::wstring label=overlayDemo?
        L"SlinkyWinky GL1.1 Menu - Bounded Overlay Demo (NO GAME)":title;
    const auto focused=navigation.visible_focus();
    if(focused && menu.expanded(*focused)) {
        const auto parameter=profile.primary(*focused);
        if(parameter) {
            std::wostringstream formatted;
            formatted<<L" | "<<std::wstring(parameter->name,
                parameter->name+std::char_traits<char>::length(parameter->name))
                <<L" "<<std::fixed
                <<std::setprecision(parameter->integral?0:2)
                <<parameter->value<<L"  (+/-)";
            label+=formatted.str();
        }
    }
    SetWindowTextW(hwnd,label.c_str());
}
void request_paint(HWND hwnd){InvalidateRect(hwnd,nullptr,FALSE);}
bool create_context(HWND hwnd) {
    device=GetDC(hwnd);
    if(!device)return false;
    PIXELFORMATDESCRIPTOR desc{};
    desc.nSize=sizeof(desc);desc.nVersion=1;
    desc.dwFlags=PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    desc.iPixelType=PFD_TYPE_RGBA;
    desc.cColorBits=24;desc.cDepthBits=16;
    desc.iLayerType=PFD_MAIN_PLANE;
    int format=ChoosePixelFormat(device,&desc);
    if(!format || !SetPixelFormat(device,format,&desc))return false;
    graphics=wglCreateContext(device);
    if(!graphics || !wglMakeCurrent(device,graphics))return false;
    RECT client{};GetClientRect(hwnd,&client);
    const unsigned w=static_cast<unsigned>(client.right);
    const unsigned h=static_cast<unsigned>(client.bottom);
    if(w<320 || h<240)return false;
    glViewport(0,0,static_cast<GLsizei>(w),static_cast<GLsizei>(h));
    return session.attach(1,w,h);
}
void close_context(HWND hwnd) {
    // Window proc always runs on the same UI thread that owns the WGL context.
    overlayHost.close();
    session.detach();
    if(wglGetCurrentContext()==graphics)wglMakeCurrent(nullptr,nullptr);
    if(graphics){wglDeleteContext(graphics);graphics=nullptr;}
    if(device){ReleaseDC(hwnd,device);device=nullptr;}
}
void adjust_client(HWND hwnd) {
    const unsigned clientWidth=menu.appearance().wide?844u:676u;
    RECT rectangle{0,0,static_cast<LONG>(clientWidth),630};
    const DWORD style=static_cast<DWORD>(GetWindowLongPtrW(hwnd,GWL_STYLE));
    const DWORD extended=static_cast<DWORD>(GetWindowLongPtrW(hwnd,GWL_EXSTYLE));
    AdjustWindowRectEx(&rectangle,style,FALSE,extended);
    SetWindowPos(hwnd,nullptr,0,0,rectangle.right-rectangle.left,
                 rectangle.bottom-rectangle.top,SWP_NOMOVE|SWP_NOZORDER);
}
LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
        case WM_CREATE:
            return create_context(hwnd)?0:-1;
        case WM_SIZE: {
            if(!graphics || !device)return 0;
            const unsigned width=LOWORD(lp),height=HIWORD(lp);
            if(width<320u || height<240u || width>8192u || height>8192u)
                return 0;
            if(wglGetCurrentContext()!=graphics &&
               !wglMakeCurrent(device,graphics))return 0;
            glViewport(0,0,static_cast<GLsizei>(width),
                       static_cast<GLsizei>(height));
            session.resize(width,height);
            if(overlayDemo)overlayHost.resize(width,height);
            request_paint(hwnd);
            return 0;
        }
        case WM_ERASEBKGND:return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd,&paint);
            RECT client{};GetClientRect(hwnd,&client);
            if(graphics && device && wglGetCurrentContext()==graphics &&
               client.right>=320 && client.bottom>=240) {
                // The menu render session holds no Windows handles and never
                // acquires the GL context from an unknown thread.
                session.resize(static_cast<unsigned>(client.right),
                               static_cast<unsigned>(client.bottom));
                // Only the standalone demo application clears this test
                // background. The bounded host itself never clears anything.
                if(overlayDemo) {
                    glClearColor(0.10f,0.38f,0.69f,1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    if(overlayHost.draw(menu,&profile,
                            slinkywinky::MenuGL11Mode::BoundedOverlay,
                            &navigation))SwapBuffers(device);
                } else {
                    const auto frame=session.next(menu,&profile,&navigation);
                    if(frame && slinkywinky::draw_menu_gl11(
                        *frame,graphics,device,
                        slinkywinky::MenuGL11Mode::StandalonePreview,
                        navigation.visible_focus()))
                        SwapBuffers(device);
                }
            }
            EndPaint(hwnd,&paint);
            return 0;
        }
        case WM_MOUSEWHEEL:{
            if(profile.scroll_by(GET_WHEEL_DELTA_WPARAM(wp)>0?-1:1)) {
                const auto page=menu.page();
                if(!page.empty())navigation.focus_visible_row(page.front().catalog_index);
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_LBUTTONDOWN:{
            RECT client{};GetClientRect(hwnd,&client);
            const int width=client.right;
            const int x=static_cast<short>(LOWORD(lp));
            const int y=static_cast<short>(HIWORD(lp));
            if(y>=78 && y<120 && x>=18 && x<width-18) {
                const int tab=(x-18)*7/(width-36);
                if(tab>=0 && tab<6) {
                    navigation.select_tab(static_cast<MenuTab>(tab));
                }
            } else if(y>=153 && y<545 && menu.tab()!=MenuTab::Profiles){
                const auto rows=menu.page();
                const int index=(y-153)/49;
                if(index>=0 && static_cast<std::size_t>(index)<rows.size()) {
                    if(navigation.focus_visible_row(rows[index].catalog_index)) {
                        if(x>=width-92 && x<=width-19)
                            navigation.apply(slinkywinky::MenuNavAction::ToggleFocusedPreview);
                        else if(x>=width-145 && x<width-92)
                            navigation.apply(slinkywinky::MenuNavAction::ExpandFocused);
                    }
                }
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_KEYDOWN:{
            if(wp==VK_ESCAPE){DestroyWindow(hwnd);return 0;}
            // Preview-only first-party keyboard navigation. This WndProc is
            // a separate Windows demo and is NEVER installed into Minecraft.
            if(wp>=L'1' && wp<=L'6')
                navigation.select_tab(static_cast<MenuTab>(wp-L'1'));
            if(wp==VK_LEFT)
                navigation.apply(slinkywinky::MenuNavAction::PreviousTab);
            if(wp==VK_RIGHT || wp==VK_TAB)
                navigation.apply(slinkywinky::MenuNavAction::NextTab);
            if(wp==VK_UP)
                navigation.apply(slinkywinky::MenuNavAction::PreviousRow);
            if(wp==VK_DOWN)
                navigation.apply(slinkywinky::MenuNavAction::NextRow);
            if(wp==VK_RETURN)
                navigation.apply(slinkywinky::MenuNavAction::ExpandFocused);
            if(wp==VK_SPACE)
                navigation.apply(slinkywinky::MenuNavAction::ToggleFocusedPreview);
            if(wp==VK_OEM_PLUS || wp==VK_ADD)
                navigation.apply(slinkywinky::MenuNavAction::IncreaseSetting);
            if(wp==VK_OEM_MINUS || wp==VK_SUBTRACT)
                navigation.apply(slinkywinky::MenuNavAction::DecreaseSetting);
            // The only effect of P/L is saving/loading a local native
            // configuration document. No module action is dispatched to MC.
            if(wp==L'P' && !profilePreferences.empty())
                profile.save(profilePreferences);
            if(wp==L'L' && !profilePreferences.empty())
                profile.load(profilePreferences);
            if(wp==L'O'){
                if(overlayDemo) {
                    overlayHost.close();
                    overlayDemo=false;
                } else if(graphics && device &&
                          wglGetCurrentContext()==graphics) {
                    RECT bounds{};GetClientRect(hwnd,&bounds);
                    overlayDemo=overlayHost.open(graphics,device,
                        static_cast<unsigned>(bounds.right),
                        static_cast<unsigned>(bounds.bottom));
                }
                refresh_caption(hwnd);
            }
            if(wp==L'C'){
                menu.set_compact(!menu.appearance().compact);persist();
            }
            if(wp==L'G'){
                menu.set_accent(menu.appearance().accent_rgb==0xE88C46u
                    ?0x779ADBu:0xE88C46u);persist();
            }
            if(wp==L'W'){
                menu.set_wide(!menu.appearance().wide);
                persist();
                adjust_client(hwnd);
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_DESTROY:
            close_context(hwnd);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR arguments,int show) {
    // Refuse any external process argument: this is never a loader or injector.
    if(arguments && *arguments)return 3;
    preferences=settings_file();
    profilePreferences=profile_file();
    if(!profilePreferences.empty())profile.load(profilePreferences);
    slinkywinky::MenuAppearance stored{};
    if(!preferences.empty() &&
       slinkywinky::load_menu_appearance(preferences,stored))
        menu.set_appearance(stored);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);
    wc.lpfnWndProc=window_proc;
    wc.style=CS_OWNDC;
    wc.hInstance=instance;
    wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
    wc.lpszClassName=className;
    if(!RegisterClassExW(&wc))return 1;
    RECT rect{0,0,menu.appearance().wide?844:676,630};
    AdjustWindowRectEx(&rect,WS_OVERLAPPEDWINDOW,FALSE,0);
    window=CreateWindowExW(0,className,title,WS_OVERLAPPEDWINDOW,
         140,100,rect.right-rect.left,rect.bottom-rect.top,
         nullptr,nullptr,instance,nullptr);
    if(!window) {
        if(graphics || device)close_context(window);
        UnregisterClassW(className,instance);
        return 2;
    }
    ShowWindow(window,show==SW_HIDE?SW_SHOWNORMAL:show);
    UpdateWindow(window);
    MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0){
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    UnregisterClassW(className,instance);
    return static_cast<int>(message.wParam);
}
