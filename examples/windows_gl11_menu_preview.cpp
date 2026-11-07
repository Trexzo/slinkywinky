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
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
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
using slinkywinky::recovered_widget_visible_count;
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
std::filesystem::path piercingPreferences; // separate sandbox only
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
std::filesystem::path piercing_preview_file() {
    const auto profilePath=profile_file();
    if(profilePath.empty())return {};
    return profilePath.parent_path()/L"piercing-preview-only.conf";
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
        const auto count=recovered_widget_visible_count(
            slinkywinky::official_module_catalog()[*focused].name,
            static_cast<int>(menu.aim_preview_mode()),
            static_cast<int>(menu.target_preview_mode()));
        if(*focused==11) {
            std::wostringstream control;
            control<<L" | Piercing sandbox: "
                   <<(menu.piercing_selected()+1u)
                   <<L"/8 (F9 select, F10 change, F11 previous)"
                   <<L" | F12 save / Shift+F12 load";
            label+=control.str();
        }
        if(*focused==0) {
            static constexpr const wchar_t* modes[]{
                L"Regular",L"Linear",L"Lock-on",L"Silent"
            };
            label+=L" | Aim: ";
            label+=modes[menu.aim_preview_mode()];
            label+=menu.target_preview_mode()==0?
                L" | Target: Single":L" | Target: Switch";
            label+=L" (F7/F8)";
        }
        if(count>5) {
            std::wostringstream paging;
            paging<<L" | Settings "<<menu.settings_offset(*focused)/5+1
                  <<L"/"<<(count+4)/5<<L" (Page Up/Down)";
            label+=paging.str();
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
    const unsigned clientWidth=menu.appearance().wide?1500u:900u;
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
            if(menu.unload_confirmation_preview())return 0;
            if(menu.close_preview_dropdown()) {
                request_paint(hwnd); // Never scroll an underlying card too.
                return 0;
            }
            if(!preview_focused(hwnd) || GetFocus()!=hwnd)return 0;
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
            // R118 standalone PREVIEW ONLY: wheel over the expanded
            // controls moves five-record pages rather than the module list.
            // The hit rectangle comes from the exact same geometry as WGL.
            const auto snapshot=mouse_frame(hwnd);
            if(!snapshot)return 0; // No verified GL owner frame, no wheel input.
            const auto expandedTarget=delta?
                slinkywinky::settings_page_wheel_target(*snapshot,mode,
                    mouse.x,mouse.y):std::nullopt;
            if(expandedTarget) {
                if(navigation.focus_visible_row(*expandedTarget))
                    navigation.apply(delta>0?
                        slinkywinky::MenuNavAction::PreviousSettingsPage:
                        slinkywinky::MenuNavAction::NextSettingsPage);
                // A wheel over a settings pane never scrolls modules,
                // even when already at the first/last settings page.
            }
            if(!expandedTarget && delta &&
               profile.scroll_by(delta>0?-1:1)) {
                const auto page=menu.page();
                if(!page.empty())navigation.focus_visible_row(page.front().catalog_index);
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_KILLFOCUS:
            cancel_mouse(hwnd);
            menu.close_preview_dropdown();
            menu.close_unload_confirmation_preview();
            return 0;
        case WM_ACTIVATEAPP:
            if(!wp) {
                cancel_mouse(hwnd);
                menu.close_preview_dropdown();
                menu.close_unload_confirmation_preview();
            }
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
            // A click outside an open popup closes it, never dispatching
            // to an underlying module/tab in the same mouse gesture.
            if(frame->dropdown_preview &&
               !slinkywinky::hit_test_menu(
                    *frame,panel_mode(),x,y)) {
                menu.close_preview_dropdown();
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 0;
            }
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
                if(hit->kind==MenuHitKind::ModalClose ||
                   hit->kind==MenuHitKind::ModalContinue)
                    menu.close_unload_confirmation_preview();
                else if(hit->kind==MenuHitKind::UnloadTab)
                    menu.open_unload_confirmation_preview();
                else if(hit->kind==MenuHitKind::DropdownOption) {
                    // Only a verified visible popup option can mutate the
                    // inert GUI preview, and the popup closes on selection.
                    menu.select_preview_dropdown_option(
                        static_cast<unsigned>(hit->control_index));
                } else if(hit->kind==MenuHitKind::Tab)
                    navigation.select_tab(hit->tab);
                else if(navigation.focus_visible_row(hit->catalog_index)) {
                    if(hit->kind==MenuHitKind::Expand)
                        navigation.apply(slinkywinky::MenuNavAction::ExpandFocused);
                    else if(hit->kind==MenuHitKind::Toggle)
                        navigation.apply(slinkywinky::MenuNavAction::ToggleFocusedPreview);
                    else if(hit->kind==MenuHitKind::RecoveredBooleanPreview)
                        menu.toggle_recovered_boolean_preview(
                            hit->catalog_index,hit->control_index);
                    else if(hit->kind==MenuHitKind::RecoveredChoiceControl)
                        menu.open_recovered_choice_preview(
                            hit->catalog_index,hit->control_index);
                    else if(hit->kind==MenuHitKind::RecoveredSliderPreview &&
                            frame) {
                        // Commit only a validated same-widget mouse-up.
                        // The mouse-gesture gate already checked foreground,
                        // owner thread, full press/release and stale frame.
                        const auto panel=slinkywinky::menu_panel_geometry(
                            frame->viewport_width,frame->viewport_height,
                            frame->appearance.wide,panel_mode());
                        if(panel) {
                            const auto track=slinkywinky::
                                menu_numeric_track_geometry(
                                    *frame,panel->logical_width(),
                                    panel->logical_height(),
                                    hit->catalog_index,hit->control_index);
                            if(track) {
                                const auto& all=slinkywinky::
                                    recovered_widget_catalog();
                                const float logicalX=(
                                    static_cast<float>(
                                        static_cast<short>(LOWORD(lp)))-
                                    static_cast<float>(panel->left))/
                                    panel->scale;
                                const auto value=slinkywinky::
                                    menu_slider_value_at_x(
                                        all[hit->control_index],*track,logicalX);
                                if(value)
                                    menu.set_recovered_slider_preview(
                                        hit->catalog_index,
                                        hit->control_index,*value);
                            }
                        }
                    }
                    else if(hit->kind==MenuHitKind::RecoveredRangePreview &&
                            frame && hit->sub_index<=1u) {
                        // R133: selection of lower or upper range knob is
                        // frozen at pointer-down by MenuMouseGesture.
                        // Releasing near the other knob cannot change it.
                        const auto panel=slinkywinky::menu_panel_geometry(
                            frame->viewport_width,frame->viewport_height,
                            frame->appearance.wide,panel_mode());
                        if(panel) {
                            const auto track=slinkywinky::
                                menu_numeric_track_geometry(
                                    *frame,panel->logical_width(),
                                    panel->logical_height(),
                                    hit->catalog_index,hit->control_index);
                            if(track) {
                                const auto& all=slinkywinky::
                                    recovered_widget_catalog();
                                const float logicalX=(
                                    static_cast<float>(
                                        static_cast<short>(LOWORD(lp)))-
                                    static_cast<float>(panel->left))/
                                    panel->scale;
                                const auto value=slinkywinky::
                                    menu_numeric_value_at_x(
                                        all[hit->control_index],*track,logicalX);
                                if(value)
                                    menu.set_recovered_range_preview(
                                        hit->catalog_index,
                                        hit->control_index,
                                        hit->sub_index,*value);
                            }
                        }
                    }
                    else if(hit->kind==MenuHitKind::PiercingControl &&
                            hit->catalog_index==11) {
                        if(hit->control_index==0u)
                            menu.open_preview_dropdown(
                                slinkywinky::MenuDropdownKind::PiercingSort,0u);
                        else if(menu.select_piercing_control(hit->control_index))
                            menu.activate_piercing_selection();
                    } else if(hit->kind==MenuHitKind::AimModeControl &&
                            hit->catalog_index==0 && hit->control_index==0)
                        menu.open_preview_dropdown(
                            slinkywinky::MenuDropdownKind::AimMode,0u);
                    else if(hit->kind==MenuHitKind::TargetModeControl &&
                            hit->catalog_index==0)
                        menu.open_preview_dropdown(
                            slinkywinky::MenuDropdownKind::TargetMode,
                            hit->control_index);
                    // A click outside the actual control remains inert.
                }
            }
            refresh_caption(hwnd);
            request_paint(hwnd);
            return 0;
        }
        case WM_KEYDOWN:{
            cancel_mouse(hwnd); // Keyboard can change tab/page/appearance.
            // Never act on a spoofed/late key message while this demo
            // window has lost its exact focus or foreground ownership.
            if(!preview_focused(hwnd) || GetFocus()!=hwnd)return 0;
            if(menu.unload_confirmation_preview()) {
                if(wp==VK_ESCAPE || wp==VK_RETURN) {
                    menu.close_unload_confirmation_preview();
                    request_paint(hwnd);
                }
                return 0; // Modal never passes keys to other UI actions.
            }
            if(wp==VK_ESCAPE) {
                if(menu.close_preview_dropdown()) {
                    request_paint(hwnd);
                    return 0;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            // Separate Windows preview ONLY; never hooks Minecraft.
            // Cycle locally selected Aim (F7) / Target (F8) view modes.
            if(wp==VK_F7 || wp==VK_F8) {
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0)
                    navigation.apply(wp==VK_F7?
                        slinkywinky::MenuNavAction::NextAimPreviewMode:
                        slinkywinky::MenuNavAction::NextTargetPreviewMode);
                refresh_caption(hwnd);
                request_paint(hwnd);
                return 0;
            }
            // R124: actual first-party editing of only the isolated
            // Piercing sandbox. No game key hook, JNI or module action.
            if(wp==VK_F9 || wp==VK_F10 || wp==VK_F11) {
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0)
                    navigation.apply(wp==VK_F9?
                        slinkywinky::MenuNavAction::NextPiercingControl:
                        (wp==VK_F11?
                        slinkywinky::MenuNavAction::PreviousPiercingControl:
                        slinkywinky::MenuNavAction::ActivatePiercingControl));
                refresh_caption(hwnd);
                request_paint(hwnd);
                return 0;
            }
            if(wp==VK_F12) {
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0 &&
                   !piercingPreferences.empty()) {
                    if(GetKeyState(VK_SHIFT)&0x8000) {
                        slinkywinky::PiercingPreview loaded{};
                        if(slinkywinky::load_piercing_preview(
                                piercingPreferences,loaded))
                            menu.adopt_piercing_preview(loaded);
                    } else
                        slinkywinky::save_piercing_preview(
                            piercingPreferences,menu.piercing_preview());
                }
                refresh_caption(hwnd);
                request_paint(hwnd);
                return 0;
            }
            // Native Windows key codes, distinct from LWJGL2's 201/209.
            // Page keys only navigate the first-party read-only preview.
            // Autorepeat cannot race two page changes for one physical hold.
            if(wp==VK_PRIOR || wp==VK_NEXT) {
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0)
                    navigation.apply(wp==VK_PRIOR?
                        slinkywinky::MenuNavAction::PreviousSettingsPage:
                        slinkywinky::MenuNavAction::NextSettingsPage);
                refresh_caption(hwnd);
                request_paint(hwnd);
                return 0;
            }
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
                menu.set_accent(menu.appearance().accent_rgb==0xF35D12u
                    ?0x779ADBu:0xF35D12u);persist();
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
    // Independent, strictly-validated GUI sandbox file, not ClientSettings.
    piercingPreferences=piercing_preview_file();
    if(!piercingPreferences.empty()) {
        slinkywinky::PiercingPreview saved{};
        if(slinkywinky::load_piercing_preview(piercingPreferences,saved))
            menu.adopt_piercing_preview(saved);
    }
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
    // R115 source-derived startup size, not the pre-reconstruction 676/844.
    RECT rect{0,0,menu.appearance().wide?1500:900,630};
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
