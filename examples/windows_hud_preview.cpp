// Read-only Win32 HUD preview: no process injection, game access or networking.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "slinkywinky/hud.hpp"
#include "slinkywinky/hud_snapshot.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <chrono>

namespace {
constexpr wchar_t class_name[]=L"SlinkyWinkyNativeHudPreview";
constexpr int width=444, height=406;
struct Preview { std::atomic<int> page{3}; bool compact=false,angles=true; };
Preview preview;
std::atomic<bool> producerRunning{true};
std::mutex mailboxAccess;
std::shared_ptr<slinkywinky::HudSnapshotMailbox> publishedMailbox;

// A fake game-thread producer for the OFFLINE preview only. The GDI thread
// never calls the state engine: it reads immutable, owner-published copies.
void synthetic_producer() {
    auto mailbox=std::make_shared<slinkywinky::HudSnapshotMailbox>();
    {
        std::lock_guard<std::mutex> guard(mailboxAccess);
        publishedMailbox=mailbox;
    }
    std::uint64_t tick=0, generation=1, transitions=0;
    std::uint32_t previousFlags=0;
    while(producerRunning.load()) {
        const int page=preview.page.load();
        const std::uint32_t flags=page<=1?0u:(page==2?3u:7u);
        if(((previousFlags^flags)&3u)!=0) {++generation;++transitions;}
        previousFlags=flags;
        slinkywinky::ObservationStatus sample{};
        sample.valid_ticks=++tick;
        sample.generation=generation;
        sample.session_transitions=transitions;
        sample.time_ms=1000+tick*50;
        sample.flags=flags;
        sample.eye_y=65.62;
        sample.yaw=42.0f;
        sample.pitch=-8.0f;
        if(flags==7)
            sample.target_distance=3.3+0.5*std::sin(static_cast<double>(tick)*0.05);
        if(!mailbox->publish(sample)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    mailbox->revoke();
    std::lock_guard<std::mutex> guard(mailboxAccess);
    publishedMailbox.reset();
}
std::wstring widen(const std::string& s) {return std::wstring(s.begin(),s.end());}
void fill(HDC dc,RECT r,COLORREF color) {
    HBRUSH brush=CreateSolidBrush(color);
    if(brush){FillRect(dc,&r,brush);DeleteObject(brush);}
}
void label(HDC dc,const std::wstring& text,RECT r,UINT format) {
    DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&r,format|DT_NOPREFIX);
}
HFONT make_font(int size,int weight){
    return CreateFontW(size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
}
slinkywinky::HudPanel model(){
    const slinkywinky::HudOptions options{preview.angles,true,preview.compact};
    if(preview.page.load()==0) return slinkywinky::make_hud_panel(std::nullopt,options);
    std::shared_ptr<slinkywinky::HudSnapshotMailbox> box;
    {
        std::lock_guard<std::mutex> guard(mailboxAccess);
        box=publishedMailbox;
    }
    if(!box) return slinkywinky::make_hud_panel(std::nullopt,options);
    const auto copied=box->read(); // UI thread consumes an immutable copy.
    return slinkywinky::make_hud_panel(
        copied ? std::optional<slinkywinky::ObservationStatus>(copied->status) :
                 std::nullopt,options);
}
void paint(HWND hwnd,HDC dc){
    RECT bounds{};GetClientRect(hwnd,&bounds);
    HDC memory=CreateCompatibleDC(dc);
    if(!memory)return;
    HBITMAP bitmap=CreateCompatibleBitmap(dc,bounds.right,bounds.bottom);
    if(!bitmap){DeleteDC(memory);return;}
    HGDIOBJ oldBitmap=SelectObject(memory,bitmap);
    fill(memory,bounds,RGB(16,19,26));
    fill(memory,{0,0,width,4},RGB(97,157,221));
    fill(memory,{0,63,width,64},RGB(48,53,67));
    SetBkMode(memory,TRANSPARENT);
    HFONT large=make_font(24,FW_SEMIBOLD),normal=make_font(16,FW_NORMAL);
    HFONT small=make_font(14,FW_NORMAL);
    HGDIOBJ oldFont=nullptr;
    if(large)oldFont=SelectObject(memory,large);
    auto panel=model();
    SetTextColor(memory,RGB(237,242,250));
    label(memory,widen(panel.title),{22,14,421,45},DT_SINGLELINE|DT_VCENTER);
    if(normal)SelectObject(memory,normal);
    SetTextColor(memory,RGB(144,159,178));
    label(memory,widen(panel.subtitle),{22,46,422,62},DT_SINGLELINE);
    if(small)SelectObject(memory,small);
    SetTextColor(memory,RGB(145,193,244));
    label(memory,widen(panel.state),{22,76,423,100},DT_SINGLELINE|DT_VCENTER);
    if(normal)SelectObject(memory,normal);
    int y=109,rowIndex=0;
    for(const auto& row:panel.rows) {
        if(y+30>355)break;
        fill(memory,{16,y,428,y+29},
             rowIndex%2==0?RGB(26,30,40):RGB(22,26,35));
        SetTextColor(memory,RGB(157,171,191));
        label(memory,widen(row.label),{26,y+3,192,y+26},
              DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
        SetTextColor(memory,RGB(239,243,250));
        label(memory,widen(row.value),{188,y+3,416,y+26},
              DT_RIGHT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
        ++rowIndex;y+=30;
    }
    fill(memory,{16,361,428,362},RGB(49,55,68));
    if(small)SelectObject(memory,small);
    SetTextColor(memory,RGB(140,151,170));
    label(memory,L"OFFLINE  |  1-4 states  |  C compact  |  A angles  |  Esc close",
          {18,371,426,397},DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    BitBlt(dc,0,0,bounds.right,bounds.bottom,memory,0,0,SRCCOPY);
    if(oldFont)SelectObject(memory,oldFont);
    if(large)DeleteObject(large);
    if(normal)DeleteObject(normal);
    if(small)DeleteObject(small);
    SelectObject(memory,oldBitmap);DeleteObject(bitmap);DeleteDC(memory);
}
LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    switch(message){
        case WM_CREATE: SetTimer(hwnd,1,50,nullptr);return 0;
        case WM_TIMER:
            if(wp==1)InvalidateRect(hwnd,nullptr,FALSE);return 0;
        case WM_KEYDOWN:
            if(wp==VK_ESCAPE){DestroyWindow(hwnd);return 0;}
            if(wp>=L'1'&&wp<=L'4')preview.page.store(static_cast<int>(wp-L'1'));
            if(wp==L'C')preview.compact=!preview.compact;
            if(wp==L'A')preview.angles=!preview.angles;
            InvalidateRect(hwnd,nullptr,FALSE);return 0;
        case WM_NCHITTEST:{
            LRESULT hit=DefWindowProcW(hwnd,message,wp,lp);
            return hit==HTCLIENT?HTCAPTION:hit;
        }
        case WM_ERASEBKGND:return 1;
        case WM_PAINT:{
            PAINTSTRUCT p{};HDC dc=BeginPaint(hwnd,&p);
            if(dc)paint(hwnd,dc);
            EndPaint(hwnd,&p);return 0;
        }
        case WM_DESTROY:KillTimer(hwnd,1);PostQuitMessage(0);return 0;
        default:return DefWindowProcW(hwnd,message,wp,lp);
    }
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
    WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);
    cls.hInstance=instance;cls.lpfnWndProc=procedure;
    cls.lpszClassName=class_name;cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
    if(!RegisterClassExW(&cls))return 1;
    const int x=(GetSystemMetrics(SM_CXSCREEN)-width)/2;
    const int y=(GetSystemMetrics(SM_CYSCREEN)-height)/2;
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_LAYERED,
        class_name,L"SlinkyWinky native HUD - offline preview",
        WS_POPUP,x,y,width,height,nullptr,nullptr,instance,nullptr);
    if(!hwnd)return 2;
    SetLayeredWindowAttributes(hwnd,RGB(0,0,0),248,LWA_ALPHA);
    std::thread producer(synthetic_producer);
    ShowWindow(hwnd,show==SW_HIDE?SW_SHOWNORMAL:show);
    UpdateWindow(hwnd);SetFocus(hwnd);
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    producerRunning.store(false);
    producer.join(); // No detached render/publication threads after window exit.
    return static_cast<int>(msg.wParam);
}
