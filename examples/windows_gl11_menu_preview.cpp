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
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
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
slinkywinky::MenuMouseGesture mouseGesture;
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
slinkywinky::MenuGL11Mode overlayMode=slinkywinky::MenuGL11Mode::BoundedOverlay;

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
        (overlayMode==slinkywinky::MenuGL11Mode::CompactOverlay?
         L"SlinkyWinky GL1.1 Menu - Compact Overlay Demo (NO GAME)":
         L"SlinkyWinky GL1.1 Menu - Bounded Overlay Demo (NO GAME)"):
        title;
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
slinkywinky::MenuPanelMode panel_mode() noexcept {
    return !overlayDemo?slinkywinky::MenuPanelMode::StandalonePreview:
           (overlayMode==slinkywinky::MenuGL11Mode::CompactOverlay?
            slinkywinky::MenuPanelMode::CompactOverlay:
            slinkywinky::MenuPanelMode::BoundedOverlay);
}
bool preview_focused(HWND hwnd) {
    return hwnd && GetForegroundWindow()==hwnd &&
           IsWindowVisible(hwnd) && !IsIconic(hwnd);
}
std::optional<slinkywinky::MenuRenderFrame> mouse_frame(HWND hwnd) {
    if(!preview_focused(hwnd) || !graphics || !device ||
       wglGetCurrentContext()!=graphics)return std::nullopt;
    RECT client{};if(!GetClientRect(hwnd,&client) ||
       client.right<320 || client.bottom<240 ||
       client.right>8192 || client.bottom>8192)return std::nullopt;
    auto frame=session.next(menu,&profile,&navigation);
    if(!frame || frame->viewport_width!=static_cast<unsigned>(client.right) ||
       frame->viewport_height!=static_cast<unsigned>(client.bottom))
        return std::nullopt;
    return frame;
}
void cancel_mouse(HWND hwnd) {
    mouseGesture.cancel();
    // Capture is solely owned by this separate preview window. A new
    // Minecraft client/window is never discovered or touched.
    if(GetCapture()==hwnd)ReleaseCapture();
}
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
    cancel_mouse(hwnd);
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
            // Match Windows actual double-click timeout for this preview.
            mouseGesture.set_double_click_window(GetDoubleClickTime());
            return create_context(hwnd)?0:-1;
        case WM_SIZE: {
            cancel_mouse(hwnd); // Resize invalidates pressed coordinates.
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
                            overlayMode,&navigation))SwapBuffers(device);
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
            cancel_mouse(hwnd); // Scrolling invalidates pressed row.
            RECT client{};GetClientRect(hwnd,&client);
            const auto mode=panel_mode();
            const auto panel=slinkywinky::menu_panel_geometry(
                static_cast<unsigned>(client.right),
                static_cast<unsigned>(client.bottom),
                menu.appearance().wide,mode);
            POINT mouse{static_cast<short>(LOWORD(lp)),
                        static_cast<short>(HIWORD(lp))};
            if(!panel || !ScreenToClient(hwnd,&mouse) ||
               mouse.x<static_cast<LONG>(panel->left) ||
               mouse.y<static_cast<LONG>(panel->top) ||
               mouse.x>=static_cast<LONG>(panel->left+panel->width) ||
               mouse.y>=static_cast<LONG>(panel->top+panel->height))
                return 0; // Wheel outside the painted panel is inert.
            const auto delta=GET_WHEEL_DELTA_WPARAM(wp);
            if(delta && profile.scroll_by(delta>0?-1:1)) {
                const auto page=menu.page();
                if(!page.empty())navigation.focus_visible_row(page.front().catalog_index);
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_KILLFOCUS:
            cancel_mouse(hwnd);
            return 0;
        case WM_ACTIVATEAPP:
            if(!wp)cancel_mouse(hwnd);
            return 0;
        case WM_CANCELMODE:
            cancel_mouse(hwnd);
            return 0;
        case WM_SETTINGCHANGE:
            cancel_mouse(hwnd);
            mouseGesture.set_double_click_window(GetDoubleClickTime());
            return 0;
        case WM_CAPTURECHANGED:
            mouseGesture.cancel(); // Capture was taken by another window.
            return 0;
        case WM_LBUTTONDBLCLK:
            // Explicit OS-recognized double-click: the first committed
            // action is enough. Never start a second gesture on this down.
            cancel_mouse(hwnd);
            return 0;
        case WM_LBUTTONDOWN:{
            // R98 never changes state on DOWN. It only stores a bounded,
            // immutable owner-thread render snapshot for later comparison.
            if(GetCapture() && GetCapture()!=hwnd)return 0;
            const auto frame=mouse_frame(hwnd);
            if(!frame) {cancel_mouse(hwnd);return 0;}
            const int x=static_cast<short>(LOWORD(lp));
            const int y=static_cast<short>(HIWORD(lp));
            if(mouseGesture.press(*frame,panel_mode(),x,y,
                                  GetTickCount64(),preview_focused(hwnd))) {
                SetCapture(hwnd);
                if(GetCapture()!=hwnd)mouseGesture.cancel();
            }
            return 0;
        }
        case WM_MOUSEMOVE:{
            if(!mouseGesture.pending())return 0;
            const auto frame=mouse_frame(hwnd);
            if(!frame || GetCapture()!=hwnd ||
               !mouseGesture.move(*frame,panel_mode(),
                                  static_cast<short>(LOWORD(lp)),
                                  static_cast<short>(HIWORD(lp)),
                                  preview_focused(hwnd))) {
                cancel_mouse(hwnd); // Leaving the target is terminal.
            }
            return 0;
        }
        case WM_LBUTTONUP:{
            const auto frame=mouse_frame(hwnd);
            std::optional<slinkywinky::MenuHit> hit;
            if(frame && GetCapture()==hwnd)
                hit=mouseGesture.release(
                    *frame,panel_mode(),
                    static_cast<short>(LOWORD(lp)),
                    static_cast<short>(HIWORD(lp)),
                    GetTickCount64(),preview_focused(hwnd));
            cancel_mouse(hwnd);
            if(hit) {
                using slinkywinky::MenuHitKind;
                if(hit->kind==MenuHitKind::Tab)
                    navigation.select_tab(hit->tab);
                else if(navigation.focus_visible_row(hit->catalog_index)) {
                    if(hit->kind==MenuHitKind::Expand)
                        navigation.apply(slinkywinky::MenuNavAction::ExpandFocused);
                    else if(hit->kind==MenuHitKind::Toggle)
                        navigation.apply(slinkywinky::MenuNavAction::ToggleFocusedPreview);
                    // Body clicks only set inert visual focus.
                }
            }
            refresh_caption(hwnd);
            request_paint(hwnd);
            return 0;
        }
        case WM_KEYDOWN:{
            cancel_mouse(hwnd); // Keyboard can change tab/page/appearance.
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
            if(wp==L'K'){
                // Switch bounded/compact only within this inert preview.
                overlayMode=(overlayMode==slinkywinky::MenuGL11Mode::CompactOverlay)?
                    slinkywinky::MenuGL11Mode::BoundedOverlay:
                    slinkywinky::MenuGL11Mode::CompactOverlay;
                refresh_caption(hwnd);
            }
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
    wc.style=CS_OWNDC|CS_DBLCLKS; // R98 explicit double-click path.
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
