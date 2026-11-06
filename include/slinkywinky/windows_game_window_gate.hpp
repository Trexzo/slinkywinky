#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <cstdint>
#include <string>

namespace slinkywinky {
// R101: source-verified original-render-WGL *window identity* preflight.
// Read-only. This function never changes focus, hooks/subclasses any window,
// consumes keys/clicks, sends input, or enables gameplay features.
enum class GameWindowFocus : int {
    Refused=-1, ValidBackground=0, ValidForeground=1
};
inline GameWindowFocus audit_game_render_window(
    HGLRC expectedContext,HDC expectedDc,unsigned width,unsigned height) noexcept {
    if(!expectedContext || !expectedDc ||
       width<320 || width>8192 || height<240 || height>8192 ||
       wglGetCurrentContext()!=expectedContext ||
       wglGetCurrentDC()!=expectedDc)
        return GameWindowFocus::Refused;
    // Resolve from the EXACT current renderer's HDC rather than guessing
    // from a process ID, foreground title, or an unrelated owned window.
    const HWND child=WindowFromDC(expectedDc);
    if(!child || !IsWindow(child))return GameWindowFocus::Refused;
    const HWND root=GetAncestor(child,GA_ROOT);
    if(!root || root!=child || !IsWindow(root) ||
       !IsWindowVisible(root) || IsIconic(root) || !IsWindowEnabled(root))
        return GameWindowFocus::Refused;
    DWORD pid=0;
    const DWORD windowThread=GetWindowThreadProcessId(root,&pid);
    if(!windowThread || pid!=GetCurrentProcessId() ||
       windowThread!=GetCurrentThreadId())
        return GameWindowFocus::Refused;
    // Only the ORIGINAL vanilla 1.8.9 window title is admitted for this
    // test, never a generic game/Java title. This remains insufficient
    // authentication alone and relies on the upstream pinned Mojang SHA.
    wchar_t title[128]{};
    if(GetWindowTextW(root,title,128)<=0 ||
       std::wstring(title)!=L"Minecraft 1.8.9")
        return GameWindowFocus::Refused;
    RECT client{};
    if(!GetClientRect(root,&client) ||
       client.left!=0 || client.top!=0 ||
       static_cast<unsigned>(client.right)!=width ||
       static_cast<unsigned>(client.bottom)!=height)
        return GameWindowFocus::Refused;
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(width) ||
       viewport[3]!=static_cast<GLint>(height))
        return GameWindowFocus::Refused;

    // An extra same-title root is ambiguous even inside the same PID.
    // The host must not infer identity from the first enumerated candidate.
    struct EnumScope {DWORD pid;unsigned count;};
    EnumScope scope{pid,0};
    EnumWindows([](HWND hwnd,LPARAM parameter)->BOOL {
        auto* state=reinterpret_cast<EnumScope*>(parameter);
        DWORD owner=0;
        GetWindowThreadProcessId(hwnd,&owner);
        if(owner!=state->pid || !IsWindowVisible(hwnd) ||
           IsIconic(hwnd))return TRUE;
        wchar_t other[128]{};
        if(GetWindowTextW(hwnd,other,128)>0 &&
           std::wstring(other)==L"Minecraft 1.8.9")
            ++state->count;
        return state->count>1?FALSE:TRUE;
    },reinterpret_cast<LPARAM>(&scope));
    if(scope.count!=1)return GameWindowFocus::Refused;
    return GetForegroundWindow()==root ?
        GameWindowFocus::ValidForeground : GameWindowFocus::ValidBackground;
}
} // namespace slinkywinky
#endif
