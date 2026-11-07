// R68 Windows desktop OpenGL preview. NOT a game hook or injector.
// Uses first-party GL1.1 text and geometry with zero gameplay actions.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <GL/gl.h>
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/menu_numeric_tooltip.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <cstdint>
#include <array>
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
std::uint16_t pendingTextHigh=0; // R137 UTF-16 surrogate pairing
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
// R148: sample a captured, source-validated numeric track in logical
// coordinates. The temporary value only affects the standalone GUI
// presentation and is not persisted until the mouse-up gate succeeds.
bool preview_numeric_drag_at(const slinkywinky::MenuRenderFrame& frame,
                             slinkywinky::MenuPanelMode mode,
                             const slinkywinky::MenuHit& hit,
                             int screenX) {
    const auto panel=slinkywinky::menu_panel_geometry(
        frame.viewport_width,frame.viewport_height,
        frame.appearance.wide,mode);
    if(!panel)return false;
    const auto track=slinkywinky::menu_numeric_track_geometry(
        frame,panel->logical_width(),panel->logical_height(),
        hit.catalog_index,hit.control_index);
    const auto& widgets=slinkywinky::recovered_widget_catalog();
    if(!track || hit.control_index>=widgets.size())return false;
    const float logicalX=(static_cast<float>(screenX)-
        static_cast<float>(panel->left))/panel->scale;
    const auto value=slinkywinky::menu_numeric_value_at_x(
        widgets[hit.control_index],*track,logicalX);
    return value && menu.move_numeric_drag_preview(*value);
}
void cancel_mouse(HWND hwnd) {
    mouseGesture.cancel();
    // Release transient drag values on every cancellation path. Invalid
    // releases and resize/focus-loss cannot commit these preview values.
    if(menu.cancel_numeric_drag_preview() && IsWindow(hwnd))
        request_paint(hwnd);
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
            if(menu.bind_capture_index() ||
               menu.profile_bind_capture_index())return 0;
            cancel_mouse(hwnd); // Scrolling invalidates pressed row.
            if(menu.unload_confirmation_preview())return 0;
            if(menu.dropdown_preview()) {
                // Source lists can contain up to 16 options. Wheel changes
                // only the popup's bounded six-row viewport, never the
                // underlying module list, and invalidates held gestures.
                if(preview_focused(hwnd) && GetFocus()==hwnd) {
                    const auto delta=GET_WHEEL_DELTA_WPARAM(wp);
                    if(delta)menu.scroll_preview_popup(delta>0?-1:1);
                }
                request_paint(hwnd);
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
            if(!expandedTarget && delta) {
                // R135: narrow one-column panels cannot show eight cards.
                // Keep a bounded scroll window small enough that the
                // final recovered module (including Teams) can be moved
                // near the top before expanding its controls.
                const bool oneColumn=slinkywinky::menu_module_columns(
                    panel->logical_width())==1u;
                const float height=panel->logical_height();
                const std::size_t visibleHint=!oneColumn?8u:
                    (height>=550.0f?3u:(height>=440.0f?2u:1u));
                if(profile.scroll_by_visible(delta>0?-1:1,visibleHint)) {
                    const auto page=menu.page();
                    if(!page.empty())
                        navigation.focus_visible_row(page.front().catalog_index);
                }
            }
            refresh_caption(hwnd);
            request_paint(hwnd);return 0;
        }
        case WM_KILLFOCUS:
            pendingTextHigh=0;
            menu.blur_recovered_text_preview();
            menu.blur_profile_name_preview();
            menu.cancel_profile_action_preview();
            menu.cancel_profile_bind_capture();
            menu.cancel_bind_capture();
            menu.set_bind_hover_row(std::nullopt);
            menu.set_profile_icon_hover(std::nullopt);
            menu.set_numeric_hover(std::nullopt);
            cancel_mouse(hwnd);
            menu.close_preview_dropdown();
            menu.close_unload_confirmation_preview();
            return 0;
        case WM_ACTIVATEAPP:
            if(!wp) {
                pendingTextHigh=0;
                menu.blur_recovered_text_preview();
                menu.blur_profile_name_preview();
                menu.cancel_profile_action_preview();
                menu.cancel_profile_bind_capture();
                menu.cancel_bind_capture();
                menu.set_bind_hover_row(std::nullopt);
                menu.set_profile_icon_hover(std::nullopt);
                menu.set_numeric_hover(std::nullopt);
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
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN: {
            // Reconstructed DOM button ordering: middle=MOUSE2,
            // right=MOUSE3 and XBUTTON1/2=MOUSE4/5.
            if(menu.profile_bind_capture_index()) {
                if(!preview_focused(hwnd) || GetFocus()!=hwnd)
                    menu.cancel_profile_bind_capture();
                else {
                    const std::uint16_t code=message==WM_MBUTTONDOWN?
                        slinkywinky::menu_bind_mouse2:
                        (message==WM_RBUTTONDOWN?
                         slinkywinky::menu_bind_mouse3:
                         (HIWORD(wp)==XBUTTON1?
                          slinkywinky::menu_bind_mouse4:
                          slinkywinky::menu_bind_mouse5));
                    menu.commit_profile_bind_capture(code);
                }
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 1;
            }
            if(menu.bind_capture_index()) {
                if(!preview_focused(hwnd) || GetFocus()!=hwnd)
                    menu.cancel_bind_capture();
                else {
                    const std::uint16_t code=message==WM_MBUTTONDOWN?
                        slinkywinky::menu_bind_mouse2:
                        (message==WM_RBUTTONDOWN?
                         slinkywinky::menu_bind_mouse3:
                         (HIWORD(wp)==XBUTTON1?
                          slinkywinky::menu_bind_mouse4:
                          slinkywinky::menu_bind_mouse5));
                    menu.commit_bind_capture(code);
                }
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 1;
            }
            break;
        }
        case WM_LBUTTONDOWN:{
            if(menu.profile_bind_capture_index()) {
                if(preview_focused(hwnd) && GetFocus()==hwnd)
                    menu.commit_profile_bind_capture(
                        slinkywinky::menu_bind_mouse1);
                else menu.cancel_profile_bind_capture();
                pendingTextHigh=0;
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 0;
            }
            // R140: source click-capture policy. A fresh left click outside
            // a waiting key chip clears it and never falls through to GUI
            // activation. Capture ownership is only this native window.
            if(menu.bind_capture_index()) {
                if(preview_focused(hwnd) && GetFocus()==hwnd)
                    menu.commit_bind_capture(0u);
                else menu.cancel_bind_capture();
                pendingTextHigh=0;
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 0;
            }
            // R98 never changes state on DOWN. It only stores a bounded,
            // immutable owner-thread render snapshot for later comparison.
            if(GetCapture() && GetCapture()!=hwnd)return 0;
            const auto frame=mouse_frame(hwnd);
            if(!frame) {cancel_mouse(hwnd);return 0;}
            const int x=static_cast<short>(LOWORD(lp));
            const int y=static_cast<short>(HIWORD(lp));
            // A background/label click blurs a focused text field. A
            // separate verified GUI target may still be activated on the
            // same mouse-up, where focus is cleared before dispatch.
            if(menu.profile_name_focused() &&
               !slinkywinky::hit_test_menu(*frame,panel_mode(),x,y)) {
                menu.blur_profile_name_preview();
                pendingTextHigh=0;
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 0;
            }
            if(menu.focused_text_widget() &&
               !slinkywinky::hit_test_menu(*frame,panel_mode(),x,y)) {
                menu.blur_recovered_text_preview();
                pendingTextHigh=0;
                cancel_mouse(hwnd);
                request_paint(hwnd);
                return 0;
            }
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
                if(GetCapture()!=hwnd) {
                    cancel_mouse(hwnd);
                    return 0;
                }
                const auto hit=slinkywinky::hit_test_menu(
                    *frame,panel_mode(),x,y);
                if(hit &&
                   (hit->kind==slinkywinky::MenuHitKind::RecoveredSliderPreview ||
                    hit->kind==slinkywinky::MenuHitKind::RecoveredRangePreview)) {
                    if(!menu.begin_numeric_drag_preview(
                          hit->catalog_index,hit->control_index,
                          hit->kind==slinkywinky::MenuHitKind::RecoveredRangePreview?
                            hit->sub_index:0u)) {
                        cancel_mouse(hwnd);
                        return 0;
                    }
                    preview_numeric_drag_at(*frame,panel_mode(),*hit,x);
                    request_paint(hwnd);
                }
            }
            return 0;
        }
        case WM_MOUSEMOVE:{
            if(!mouseGesture.pending()) {
                std::optional<std::size_t> row;
                std::optional<slinkywinky::MenuProfileIconHover> iconHover;
                std::optional<slinkywinky::MenuNumericHover> numericHover;
                const auto frame=mouse_frame(hwnd);
                if(frame && preview_focused(hwnd)) {
                    const auto panel=slinkywinky::menu_panel_geometry(
                        frame->viewport_width,frame->viewport_height,
                        frame->appearance.wide,panel_mode());
                    if(panel) {
                        const float x=(static_cast<float>(
                            static_cast<short>(LOWORD(lp)))-
                            static_cast<float>(panel->left))/panel->scale;
                        const float y=(static_cast<float>(
                            static_cast<short>(HIWORD(lp)))-
                            static_cast<float>(panel->top))/panel->scale;
                        if(frame->tab==MenuTab::Profiles &&
                           !frame->profile_action_preview &&
                           !frame->unload_confirmation_preview) {
                            for(std::size_t i=0;i<frame->profile_cards.size()
                                && !iconHover;++i) {
                                for(unsigned action=0;action<4u;++action) {
                                    const auto rect=
                                        slinkywinky::menu_profile_icon_visual_rect(
                                            *frame,panel->logical_width(),
                                            panel->logical_height(),i,action);
                                    if(rect && slinkywinky::menu_profile_contains(
                                        *rect,x,y)) {
                                        iconHover=slinkywinky::MenuProfileIconHover{
                                            i,action};
                                        break;
                                    }
                                }
                            }
                        }
                        // Recover the same source-indexed numeric widget that
                        // owns the pointer rectangle, not a loose label or
                        // off-page slider. The tooltip cannot itself accept
                        // input, capture a global pointer or modify gameplay.
                        if(frame->tab!=slinkywinky::MenuTab::Profiles &&
                           !frame->dropdown_preview &&
                           !frame->unload_confirmation_preview) {
                            const auto& all=slinkywinky::recovered_widget_catalog();
                            for(const auto& item:frame->visible) {
                                if(!item.expanded || numericHover)continue;
                                std::array<const slinkywinky::RecoveredWidget*,5>
                                    visible{};
                                if(!slinkywinky::recovered_widget_visible_page(
                                    item.title.c_str(),item.settings_offset,
                                    static_cast<int>(frame->aim_preview_mode),
                                    static_cast<int>(frame->target_preview_mode),
                                    visible))continue;
                                for(const auto* widget:visible) {
                                    if(!widget)break;
                                    if(widget->kind!=
                                           slinkywinky::RecoveredWidgetKind::Slider &&
                                       widget->kind!=
                                           slinkywinky::RecoveredWidgetKind::Range)
                                        continue;
                                    const auto index=static_cast<std::size_t>(
                                        widget-all.data());
                                    if(index>=all.size() ||
                                       slinkywinky::menu_numeric_format_for_widget(
                                           index).empty())continue;
                                    const auto track=
                                        slinkywinky::menu_numeric_track_geometry(
                                            *frame,panel->logical_width(),
                                            panel->logical_height(),
                                            item.catalog_index,index);
                                    if(track && x>=track->x &&
                                       x<track->x+track->width &&
                                       y>=track->y &&
                                       y<track->y+track->height) {
                                        numericHover=slinkywinky::MenuNumericHover{
                                            item.catalog_index,index};
                                        break;
                                    }
                                }
                            }
                        }
                        for(std::size_t i=0;i<frame->visible.size();++i) {
                            const auto rect=slinkywinky::menu_module_card_rect(
                                *frame,panel->logical_width(),i);
                            if(rect.y+rect.height>
                               panel->logical_height()-
                               slinkywinky::menu_panel_padding)
                                continue;
                            const auto index=frame->visible[i].catalog_index;
                            if(slinkywinky::menu_bind_supported_module(index) &&
                               x>=rect.x && x<rect.x+rect.width &&
                               y>=rect.y+8.0f && y<rect.y+28.0f) {
                                row=index;
                                break;
                            }
                        }
                    }
                }
                bool repaint=menu.set_bind_hover_row(row);
                if(menu.set_profile_icon_hover(iconHover))repaint=true;
                if(menu.set_numeric_hover(numericHover))repaint=true;
                if(repaint)request_paint(hwnd);
                TRACKMOUSEEVENT tracking{};
                tracking.cbSize=sizeof(tracking);
                tracking.dwFlags=TME_LEAVE;
                tracking.hwndTrack=hwnd;
                TrackMouseEvent(&tracking);
                return 0;
            }
            const auto frame=mouse_frame(hwnd);
            const int x=static_cast<short>(LOWORD(lp));
            const int y=static_cast<short>(HIWORD(lp));
            if(!frame || GetCapture()!=hwnd ||
               !mouseGesture.move(*frame,panel_mode(),x,y,
                                  preview_focused(hwnd))) {
                cancel_mouse(hwnd); // R98 target and ownership remain strict.
            } else if(const auto captured=menu.numeric_drag_preview()) {
                const slinkywinky::MenuHit source{
                    slinkywinky::MenuHitKind::RecoveredSliderPreview,
                    frame->tab,captured->catalog_index,
                    captured->widget_index,captured->handle};
                if(preview_numeric_drag_at(*frame,panel_mode(),source,x))
                    request_paint(hwnd);
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            // A captured drag owns the temporary tooltip until capture
            // ends; clearing hover now would invalidate its source frame.
            if(mouseGesture.pending())return 0;
            bool repaint=menu.set_bind_hover_row(std::nullopt);
            if(menu.set_profile_icon_hover(std::nullopt))repaint=true;
            if(menu.set_numeric_hover(std::nullopt))repaint=true;
            if(repaint)request_paint(hwnd);
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
                if(hit->kind==MenuHitKind::ProfileModalClose)
                    menu.cancel_profile_action_preview();
                else if(hit->kind==MenuHitKind::ProfileModalContinue)
                    menu.confirm_profile_action_preview();
                else if(hit->kind==MenuHitKind::ProfileBindChip) {
                    pendingTextHigh=0;
                    menu.begin_profile_bind_capture(hit->catalog_index);
                }
                else if(hit->kind==MenuHitKind::ProfileNameField) {
                    pendingTextHigh=0;
                    menu.focus_profile_name_preview();
                }
                else if(hit->kind==MenuHitKind::ProfileCreate) {
                    pendingTextHigh=0;
                    menu.create_profile_preview();
                }
                else if(hit->kind==MenuHitKind::ProfileAction) {
                    pendingTextHigh=0;
                    menu.begin_profile_action_preview(
                        hit->catalog_index,
                        static_cast<slinkywinky::MenuProfileAction>(
                            hit->control_index));
                }
                else if(hit->kind==MenuHitKind::ModalClose ||
                        hit->kind==MenuHitKind::ModalContinue)
                    menu.close_unload_confirmation_preview();
                else if(hit->kind==MenuHitKind::UnloadTab) {
                    menu.blur_recovered_text_preview();
                    pendingTextHigh=0;
                    menu.open_unload_confirmation_preview();
                }
                else if(hit->kind==MenuHitKind::DropdownOption) {
                    // Only a verified visible popup option can mutate the
                    // inert GUI preview, and the popup closes on selection.
                    menu.select_preview_dropdown_option(
                        static_cast<unsigned>(hit->control_index));
                } else if(hit->kind==MenuHitKind::Tab) {
                    menu.blur_profile_name_preview();
                    menu.blur_recovered_text_preview();
                    pendingTextHigh=0;
                    navigation.select_tab(hit->tab);
                } else if(navigation.focus_visible_row(hit->catalog_index)) {
                    if(hit->kind!=MenuHitKind::RecoveredTextField) {
                        menu.blur_recovered_text_preview();
                        pendingTextHigh=0;
                    }
                    if(hit->kind==MenuHitKind::BindChip)
                        menu.begin_bind_capture(hit->catalog_index);
                    else if(hit->kind==MenuHitKind::BindHold)
                        menu.toggle_bind_hold(hit->catalog_index);
                    else if(hit->kind==MenuHitKind::Expand)
                        navigation.apply(slinkywinky::MenuNavAction::ExpandFocused);
                    else if(hit->kind==MenuHitKind::Toggle)
                        navigation.apply(slinkywinky::MenuNavAction::ToggleFocusedPreview);
                    else if(hit->kind==MenuHitKind::RecoveredBooleanPreview)
                        menu.toggle_recovered_boolean_preview(
                            hit->catalog_index,hit->control_index);
                    else if(hit->kind==MenuHitKind::RecoveredChoiceControl)
                        menu.open_recovered_choice_preview(
                            hit->catalog_index,hit->control_index);
                    else if(hit->kind==MenuHitKind::RecoveredMultiControl)
                        menu.open_recovered_multi_preview(
                            hit->catalog_index,hit->control_index);
                    else if(hit->kind==MenuHitKind::RecoveredTextField) {
                        pendingTextHigh=0;
                        menu.focus_recovered_text_preview(
                            hit->catalog_index,hit->control_index);
                    }
                    else if(hit->kind==MenuHitKind::RecoveredColorSwatch &&
                            frame) {
                        // The source HTML candidate used an input type=color
                        // control. Use the stock Windows RGB chooser only in
                        // this separate GL11 preview; no game hook or packet.
                        const auto initial=slinkywinky::menu_source_widget_color(
                            hit->control_index);
                        if(initial) {
                            const std::uint32_t rgb=menu.recovered_color_override(
                                hit->control_index).value_or(*initial);
                            static std::array<COLORREF,16> customColors{};
                            CHOOSECOLORW picker{};
                            picker.lStructSize=sizeof(picker);
                            picker.hwndOwner=hwnd;
                            picker.rgbResult=RGB(
                                (rgb>>16u)&0xffu,
                                (rgb>>8u)&0xffu,rgb&0xffu);
                            picker.lpCustColors=customColors.data();
                            picker.Flags=CC_RGBINIT|CC_FULLOPEN;
                            // Native modal dialog: Cancel leaves preview
                            // state unchanged. Admission is checked again
                            // after the dialog has released the input owner.
                            if(ChooseColorW(&picker) && IsWindow(hwnd)) {
                                const COLORREF chosen=picker.rgbResult;
                                const std::uint32_t value=
                                    (static_cast<std::uint32_t>(
                                        GetRValue(chosen))<<16u) |
                                    (static_cast<std::uint32_t>(
                                        GetGValue(chosen))<<8u) |
                                    static_cast<std::uint32_t>(
                                        GetBValue(chosen));
                                menu.set_recovered_color_preview(
                                    hit->catalog_index,
                                    hit->control_index,value);
                            }
                        }
                    }
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
        case WM_CHAR: {
            if((!menu.focused_text_widget() &&
                 !menu.profile_name_focused()) ||
               !preview_focused(hwnd) || GetFocus()!=hwnd)
                return 0;
            const std::uint32_t unit=static_cast<std::uint16_t>(wp);
            std::uint32_t scalar=unit;
            if(unit>=0xD800u && unit<=0xDBFFu) {
                pendingTextHigh=static_cast<std::uint16_t>(unit);
                return 0;
            }
            if(unit>=0xDC00u && unit<=0xDFFFu) {
                if(pendingTextHigh<0xD800u ||
                   pendingTextHigh>0xDBFFu)return 0;
                scalar=0x10000u+
                    ((static_cast<std::uint32_t>(pendingTextHigh)-
                      0xD800u)<<10u)+(unit-0xDC00u);
                pendingTextHigh=0;
            } else pendingTextHigh=0;
            if(scalar<0x20u || scalar==0x7Fu ||
               (scalar>=0x80u && scalar<=0x9Fu) ||
               scalar>0x10FFFFu)return 0;
            std::string fragment;
            if(scalar<0x80u) {
                fragment.push_back(static_cast<char>(scalar));
            } else if(scalar<0x800u) {
                fragment.push_back(static_cast<char>(0xC0u|(scalar>>6u)));
                fragment.push_back(static_cast<char>(0x80u|(scalar&0x3Fu)));
            } else if(scalar<0x10000u) {
                fragment.push_back(static_cast<char>(0xE0u|(scalar>>12u)));
                fragment.push_back(static_cast<char>(
                    0x80u|((scalar>>6u)&0x3Fu)));
                fragment.push_back(static_cast<char>(0x80u|(scalar&0x3Fu)));
            } else {
                fragment.push_back(static_cast<char>(0xF0u|(scalar>>18u)));
                fragment.push_back(static_cast<char>(
                    0x80u|((scalar>>12u)&0x3Fu)));
                fragment.push_back(static_cast<char>(
                    0x80u|((scalar>>6u)&0x3Fu)));
                fragment.push_back(static_cast<char>(0x80u|(scalar&0x3Fu)));
            }
            if(menu.profile_name_focused()) {
                if(menu.append_profile_name_preview(fragment))
                    request_paint(hwnd);
            } else if(menu.append_recovered_text_preview(fragment))
                request_paint(hwnd);
            return 0;
        }
        case WM_KEYDOWN:{
            cancel_mouse(hwnd); // Keyboard can change tab/page/appearance.
            // Never act on a spoofed/late key message while this demo
            // window has lost its exact focus or foreground ownership.
            if(!preview_focused(hwnd) || GetFocus()!=hwnd)return 0;
            if(menu.profile_bind_capture_index()) {
                // The source's profile bind chip listens only while
                // selected; this separate window never registers global
                // hotkeys, handlers or actual Minecraft profile binds.
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0u) {
                    std::uint16_t code=static_cast<std::uint16_t>(wp);
                    if(wp==VK_ESCAPE)code=0u;
                    else if(wp==VK_SHIFT) {
                        const UINT scan=static_cast<UINT>(
                            (static_cast<std::uint32_t>(lp)>>16u)&0xFFu);
                        const UINT mapped=MapVirtualKeyW(
                            scan,MAPVK_VSC_TO_VK_EX);
                        code=mapped==VK_RSHIFT?0xA1u:0xA0u;
                    } else if(wp==VK_CONTROL)
                        code=(static_cast<std::uint32_t>(lp)&
                              (1u<<24u))?0xA3u:0xA2u;
                    else if(wp==VK_MENU)
                        code=(static_cast<std::uint32_t>(lp)&
                              (1u<<24u))?0xA5u:0xA4u;
                    menu.commit_profile_bind_capture(code);
                }
                request_paint(hwnd);
                return 0;
            }
            if(menu.bind_capture_index()) {
                // Only a fresh keydown is admitted. ESC clears; a loss of
                // foreground would cancel instead. The original app's
                // key input tables are not modified by this preview.
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0u) {
                    std::uint16_t code=static_cast<std::uint16_t>(wp);
                    if(wp==VK_ESCAPE)code=0u;
                    else if(wp==VK_SHIFT) {
                        const UINT scan=static_cast<UINT>(
                            (static_cast<std::uint32_t>(lp)>>16u)&0xFFu);
                        const UINT mapped=MapVirtualKeyW(
                            scan,MAPVK_VSC_TO_VK_EX);
                        code=mapped==VK_RSHIFT?0xA1u:0xA0u;
                    } else if(wp==VK_CONTROL)
                        code=(static_cast<std::uint32_t>(lp)&
                              (1u<<24u))?0xA3u:0xA2u;
                    else if(wp==VK_MENU)
                        code=(static_cast<std::uint32_t>(lp)&
                              (1u<<24u))?0xA5u:0xA4u;
                    menu.commit_bind_capture(code);
                }
                request_paint(hwnd);
                return 0;
            }
            if(menu.profile_action_preview()) {
                if(wp==VK_ESCAPE)menu.cancel_profile_action_preview();
                else if(wp==VK_RETURN)
                    menu.confirm_profile_action_preview();
                request_paint(hwnd);
                return 0; // modal owns all keyboard input
            }
            if(menu.profile_name_focused()) {
                // Source profile-name field only. Prevent normal game
                // shortcuts, navigation and bind capture while editing.
                pendingTextHigh=0;
                if(wp==VK_ESCAPE)menu.blur_profile_name_preview();
                else if(wp==VK_RETURN)menu.create_profile_preview();
                else if(wp==VK_BACK)menu.erase_profile_name_preview();
                request_paint(hwnd);
                return 0;
            }
            if(menu.focused_text_widget()) {
                // Focused inline editing owns keyboard events. Do not
                // dispatch typed module shortcuts through the text field.
                if(wp==VK_ESCAPE || wp==VK_RETURN) {
                    menu.blur_recovered_text_preview();
                    pendingTextHigh=0;
                    request_paint(hwnd);
                } else if(wp==VK_BACK) {
                    pendingTextHigh=0;
                    if(menu.erase_recovered_text_preview())
                        request_paint(hwnd);
                }
                return 0;
            }
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
            if(menu.dropdown_preview()) {
                // The open popup owns keyboard navigation too.
                // Never pass page/arrow/module hotkeys through it.
                if((static_cast<std::uint32_t>(lp)&(1u<<30))==0) {
                    if(wp==VK_UP || wp==VK_PRIOR)
                        menu.scroll_preview_popup(-1);
                    if(wp==VK_DOWN || wp==VK_NEXT)
                        menu.scroll_preview_popup(1);
                }
                request_paint(hwnd);
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
