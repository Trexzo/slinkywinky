// First-party, standalone Windows menu preview. No Minecraft process, Forge,
// injector operations, gameplay module state, input synthesis or networking.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "slinkywinky/menu.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace {
using slinkywinky::MenuTab;
slinkywinky::MenuState menu;
constexpr int baseWidth=676, wideWidth=844, panelHeight=630;
constexpr int navY=78, navH=42, rowStart=153, rowH=49, visibleRows=8;
constexpr wchar_t klass[]=L"SlinkyWinkyMenuPreview";
bool alternateAccent=false;
int windowWidth=baseWidth;

COLORREF color(std::uint32_t rgb) {
    return RGB((rgb>>16)&0xffu,(rgb>>8)&0xffu,rgb&0xffu);
}
std::wstring widen(const std::string& v) {
    return std::wstring(v.begin(),v.end()); // public catalog is ASCII
}
void rect(HDC dc,int x,int y,int w,int h,COLORREF shade) {
    RECT r{x,y,x+w,y+h}; HBRUSH brush=CreateSolidBrush(shade);
    if(brush){FillRect(dc,&r,brush);DeleteObject(brush);}
}
HFONT font(int size,int weight) {
    return CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
}
void draw(HDC dc,std::wstring s,int x,int y,int w,int h,COLORREF shade,
          UINT align=DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS) {
    RECT bounds{x,y,x+w,y+h};SetBkMode(dc,TRANSPARENT);SetTextColor(dc,shade);
    DrawTextW(dc,s.c_str(),static_cast<int>(s.length()),&bounds,
              align|DT_NOPREFIX);
}
int tab_x(int index,int width) {return 18+index*(width-36)/7;}
void repaint(HWND hwnd) {InvalidateRect(hwnd,nullptr,FALSE);}
void redraw(HWND hwnd,HDC target) {
    RECT client{};GetClientRect(hwnd,&client);
    HDC buffer=CreateCompatibleDC(target);
    if(!buffer)return;
    HBITMAP bitmap=CreateCompatibleBitmap(target,client.right,client.bottom);
    if(!bitmap){DeleteDC(buffer);return;}
    HGDIOBJ previousBitmap=SelectObject(buffer,bitmap);
    const COLORREF bg=RGB(22,22,23),panel=RGB(28,28,29),
                   subtle=RGB(37,37,38),muted=RGB(151,151,153),
                   bright=RGB(244,244,245);
    const COLORREF accent=color(menu.appearance().accent_rgb);
    const int w=client.right;
    rect(buffer,0,0,w,client.bottom,bg);
    rect(buffer,0,0,w,4,accent);
    HFONT heading=font(24,FW_SEMIBOLD),body=font(17,FW_NORMAL),
          small=font(13,FW_NORMAL),nameFont=font(16,FW_SEMIBOLD);
    HGDIOBJ prevFont=SelectObject(buffer,heading);
    draw(buffer,L"SLINKYWINKY",21,15,w-40,37,bright);
    SelectObject(buffer,small);
    draw(buffer,L"NATIVE MENU  /  STANDALONE VISUAL PREVIEW  /  NO GAME ACTIONS",
         24,51,w-48,19,muted);
    rect(buffer,18,navY,w-36,navH,RGB(16,16,17));
    for(int i=0;i<7;++i) {
        const auto tab=static_cast<MenuTab>(i);
        const int x=tab_x(i,w), cell=(w-36)/7;
        if(menu.tab()==tab) {
            rect(buffer,x,navY,cell,navH,RGB(49,43,37));
            rect(buffer,x+8,navY+navH-3,cell-16,3,accent);
        }
        draw(buffer,widen(slinkywinky::menu_tab_name(tab)),x+3,navY,
             cell-6,navH,tab==MenuTab::Unload?RGB(114,114,117):
             (menu.tab()==tab?bright:RGB(183,183,185)),
             DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
    SelectObject(buffer,body);
    draw(buffer,widen(slinkywinky::menu_tab_name(menu.tab())),25,125,
         w-160,24,bright);
    SelectObject(buffer,small);
    draw(buffer,menu.tab()==MenuTab::Profiles?L"Profiles not wired to gameplay":
         L"PREVIEW-ONLY SWITCHES",w-225,125,202,24,muted,
         DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
    const auto page=menu.page();
    if(menu.tab()==MenuTab::Profiles) {
        rect(buffer,18,163,w-36,113,panel);
        draw(buffer,L"Profile management is a separate future milestone.",
             33,172,w-66,32,bright);
        draw(buffer,L"GUI appearance will remain independent of saved combat profiles.",
             33,209,w-66,32,muted);
    } else {
        for(std::size_t i=0;i<page.size();++i) {
            int y=rowStart+static_cast<int>(i)*rowH;
            const auto& item=page[i];
            rect(buffer,18,y,w-36,rowH-5,i%2?subtle:panel);
            if(item.preview_enabled)rect(buffer,18,y,3,rowH-5,accent);
            SelectObject(buffer,nameFont);
            draw(buffer,widen(item.title),31,y+5,w-206,23,bright);
            SelectObject(buffer,small);
            if(!menu.appearance().compact)
                draw(buffer,item.expanded?L"Settings UI: not connected":
                     L"UI catalog only - no module implementation",
                     31,y+24,w-206,16,muted);
            // Dedicated + expansion and inert preview-switch affordances.
            draw(buffer,item.expanded?L"-":L"+",w-122,y+9,32,28,muted,
                 DT_CENTER|DT_SINGLELINE|DT_VCENTER);
            const int swX=w-76,swY=y+12;
            rect(buffer,swX,swY,45,22,
                 item.preview_enabled?accent:RGB(85,85,88));
            rect(buffer,swX+(item.preview_enabled?24:3),swY+3,17,16,
                 item.preview_enabled?RGB(255,255,255):RGB(202,202,204));
        }
    }
    rect(buffer,18,565,w-36,1,RGB(61,61,63));
    SelectObject(buffer,small);
    draw(buffer,L"1-6 tabs  |  Scroll wheel  |  C compact  |  W wide  |  G accent  |  Esc close",
         21,575,w-42,18,muted);
    draw(buffer,L"Preview switches never enable Minecraft modules. Unload is disabled here.",
         21,594,w-42,18,RGB(203,137,90));
    BitBlt(target,0,0,client.right,client.bottom,buffer,0,0,SRCCOPY);
    SelectObject(buffer,prevFont);
    DeleteObject(heading);DeleteObject(body);DeleteObject(small);
    DeleteObject(nameFont);
    SelectObject(buffer,previousBitmap);DeleteObject(bitmap);DeleteDC(buffer);
}
LRESULT CALLBACK wndproc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message){
        case WM_ERASEBKGND:return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);
            if(dc)redraw(hwnd,dc);EndPaint(hwnd,&ps);return 0;
        }
        case WM_MOUSEWHEEL:{
            const int sign=GET_WHEEL_DELTA_WPARAM(wp)>0?-1:1;
            menu.scroll_by(sign);repaint(hwnd);return 0;
        }
        case WM_LBUTTONDOWN:{
            const int x=static_cast<short>(LOWORD(lp));
            const int y=static_cast<short>(HIWORD(lp));
            if(y>=navY && y<navY+navH && x>=18 && x<windowWidth-18) {
                const int idx=(x-18)*7/(windowWidth-36);
                if(idx>=0 && idx<6)menu.select(static_cast<MenuTab>(idx));
            } else if(y>=rowStart && y<rowStart+8*rowH &&
                     menu.tab()!=MenuTab::Profiles) {
                const auto rows=menu.page();
                const int idx=(y-rowStart)/rowH;
                if(idx>=0 && static_cast<std::size_t>(idx)<rows.size()) {
                    if(x>=windowWidth-92 && x<=windowWidth-19)
                        menu.toggle_preview(rows[idx].catalog_index);
                    else if(x>=windowWidth-145 && x<windowWidth-92)
                        menu.toggle_expanded(rows[idx].catalog_index);
                }
            }
            repaint(hwnd);return 0;
        }
        case WM_KEYDOWN:{
            if(wp==VK_ESCAPE){DestroyWindow(hwnd);return 0;}
            if(wp>=L'1'&&wp<=L'6')menu.select(static_cast<MenuTab>(wp-L'1'));
            if(wp==L'C')menu.set_compact(!menu.appearance().compact);
            if(wp==L'G'){
                alternateAccent=!alternateAccent;
                menu.set_accent(alternateAccent?0x779ADB:0xE88C46);
            }
            if(wp==L'W'){
                menu.set_wide(!menu.appearance().wide);
                windowWidth=menu.appearance().wide?wideWidth:baseWidth;
                SetWindowPos(hwnd,nullptr,0,0,windowWidth,panelHeight,
                             SWP_NOMOVE|SWP_NOZORDER);
            }
            if(wp==VK_UP)menu.scroll_by(-1);
            if(wp==VK_DOWN)menu.scroll_by(1);
            repaint(hwnd);return 0;
        }
        case WM_DESTROY:PostQuitMessage(0);return 0;
        default:return DefWindowProcW(hwnd,message,wp,lp);
    }
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR args,int show) {
    if(args && args[0]!=L'\0')return 3; // No PIDs, injected objects, or commands.
    WNDCLASSEXW klassInfo{};klassInfo.cbSize=sizeof(klassInfo);
    klassInfo.lpfnWndProc=wndproc;klassInfo.hInstance=instance;
    klassInfo.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    klassInfo.lpszClassName=klass;
    if(!RegisterClassExW(&klassInfo))return 1;
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,klass,
         L"SlinkyWinky native menu preview - offline",WS_OVERLAPPED|WS_CAPTION|
         WS_SYSMENU,160,110,windowWidth,panelHeight,nullptr,nullptr,instance,nullptr);
    if(!hwnd)return 2;
    ShowWindow(hwnd,show==SW_HIDE?SW_SHOWNORMAL:show);UpdateWindow(hwnd);
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0) {
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
