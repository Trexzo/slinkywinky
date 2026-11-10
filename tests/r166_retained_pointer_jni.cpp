// R166: DISPOSABLE Windows JNI test DLL, not linked into the agent or
// slinkywinky_jni.dll. No original Minecraft process, hooks or input.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <jni.h>
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_numeric_tooltip.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_settings_vertical.hpp"
#include <array>
#include "slinkywinky/recovered_settings.hpp"
#include <string_view>
#include <utility>
#include "slinkywinky/menu_profile.hpp"
#include <cmath>
#include <cstdint>
#include <memory>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr unsigned kWidth=676u,kHeight=630u;
constexpr wchar_t kWindowClass[]=L"SlinkyWinky-R166-Disposable-JNI-WGL";
std::mutex gate;
struct DisposableSession final {
    HWND hwnd=nullptr;
    HDC dc=nullptr;
    HGLRC gl=nullptr;
    bool registered=false;
    std::thread::id owner=std::this_thread::get_id();
    std::unique_ptr<slinkywinky::WglRetainedMenuPreview> ui;
    bool isOwner() const {return owner==std::this_thread::get_id();}
    bool valid() const {
        return isOwner() && hwnd && dc && gl && ui && ui->active() &&
               wglGetCurrentContext()==gl && wglGetCurrentDC()==dc;
    }
    bool open() {
        HINSTANCE instance=GetModuleHandleW(nullptr);
        WNDCLASSW wc{};
        wc.style=CS_OWNDC;
        wc.lpfnWndProc=DefWindowProcW;
        wc.hInstance=instance;
        wc.lpszClassName=kWindowClass;
        registered=RegisterClassW(&wc)!=0;
        if(!registered)return false;
        hwnd=CreateWindowW(kWindowClass,L"R166 isolated Java 8 JNI fixture",
                           WS_POPUP,0,0,static_cast<int>(kWidth),
                           static_cast<int>(kHeight),nullptr,nullptr,
                           instance,nullptr);
        if(!hwnd)return false;
        dc=GetDC(hwnd);
        if(!dc)return false;
        PIXELFORMATDESCRIPTOR descriptor{};
        descriptor.nSize=sizeof(descriptor);
        descriptor.nVersion=1;
        descriptor.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
        descriptor.iPixelType=PFD_TYPE_RGBA;
        descriptor.cColorBits=24;
        descriptor.cDepthBits=16;
        descriptor.iLayerType=PFD_MAIN_PLANE;
        const int format=ChoosePixelFormat(dc,&descriptor);
        if(!format || !SetPixelFormat(dc,format,&descriptor))return false;
        gl=wglCreateContext(dc);
        if(!gl || !wglMakeCurrent(dc,gl))return false;
        glViewport(0,0,kWidth,kHeight);
        ui=std::make_unique<slinkywinky::WglRetainedMenuPreview>();
        return ui->draw(gl,dc,kWidth,kHeight) && glGetError()==GL_NO_ERROR;
    }
    bool close() {
        if(!isOwner())return false;
        const bool menuOk=!ui || !ui->active() || ui->close();
        ui.reset();
        wglMakeCurrent(nullptr,nullptr);
        if(gl)wglDeleteContext(gl);
        gl=nullptr;
        if(dc && hwnd)ReleaseDC(hwnd,dc);
        dc=nullptr;
        if(hwnd)DestroyWindow(hwnd);
        hwnd=nullptr;
        if(registered)UnregisterClassW(kWindowClass,GetModuleHandleW(nullptr));
        registered=false;
        return menuOk;
    }
    // This source-validated target is the same first module toggle reached
    // by tests/test_windows_retained_pointer.cpp, in physical CLIENT pixels.
    std::optional<std::pair<int,int>> firstToggle() const {
        if(!valid())return std::nullopt;
        slinkywinky::MenuState mirror;
        slinkywinky::MenuProfileBridge profile(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(166u,kWidth,kHeight))return std::nullopt;
        const auto frame=frames.next(mirror,&profile);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f)return std::nullopt;
        const auto rect=slinkywinky::menu_module_card_rect(
            *frame,panel->logical_width(),0u);
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround((rect.x+rect.width-30.0f)*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround((rect.y+17.0f)*panel->scale));
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        frames.detach();
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::Toggle ||
           hit->catalog_index!=0u)return std::nullopt;
        return std::pair<int,int>{x,y};
    }
};
std::unique_ptr<DisposableSession> session;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_begin(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(session)return JNI_FALSE;
    try {
        auto created=std::make_unique<DisposableSession>();
        if(!created->open()) {
            created->close();
            return JNI_FALSE;
        }
        session=std::move(created);
        return JNI_TRUE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_toggleTarget(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto hit=session->firstToggle();
        if(!hit)return nullptr;
        const jint data[2]={hit->first,hit->second};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,data);
        return result;
    }catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_pointer(
        JNIEnv*,jclass,jint phase,jint x,jint y,jlong epoch,
        jboolean exclusive,jboolean foreground,jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() || phase<0 || phase>3 ||
       epoch<=0 || width<320 || height<240 || width>8192 ||
       height>8192)return JNI_FALSE;
    try {
        return session->ui->pointer(
            static_cast<slinkywinky::RetainedPointerPhase>(phase),
            session->gl,session->dc,
            static_cast<unsigned>(width),static_cast<unsigned>(height),
            x,y,static_cast<std::uint64_t>(epoch),
            exclusive==JNI_TRUE,foreground==JNI_TRUE)?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jstring JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_profile(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto snapshot=session->ui->profile_snapshot(
            session->gl,session->dc,kWidth,kHeight);
        return snapshot?env->NewStringUTF(snapshot->c_str()):nullptr;
    }catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_redraw(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid())return JNI_FALSE;
    try {
        return session->ui->draw(session->gl,session->dc,kWidth,kHeight) &&
               glGetError()==GL_NO_ERROR?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R166RetainedPointerJniSmoke_end(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->isOwner())return JNI_FALSE;
    try {
        const bool closed=session->close();
        session.reset();
        return closed?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}

// R167: the *same* disposable WGL JNI library now exercises an actual
// recovered slider -> validated C++ profile path. No real game input owner,
// live Java entry point, Minecraft package, or gameplay action is involved.
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R167RecoveredSliderJniSmoke_navigate(
        JNIEnv*,jclass,jint action) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() || (action!=2 && action!=4))
        return JNI_FALSE;
    try {
        return session->ui->apply(
            static_cast<slinkywinky::MenuNavAction>(action))?
            JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R167RecoveredSliderJniSmoke_sliderTarget(
        JNIEnv* env,jclass,jint fractionPermille) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       fractionPermille<0 || fractionPermille>=1000 ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u))
        return nullptr;
    try {
        // Recreate ONLY the reference hit-test geometry. The input release
        // is handled by session->ui's real R159 source-indexed pointer().
        slinkywinky::MenuState mirror;
        if(!mirror.toggle_expanded(1u))return nullptr;
        slinkywinky::MenuProfileBridge mirrorProfile(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(167u,kWidth,kHeight))return nullptr;
        const auto frame=frames.next(mirror,&mirrorProfile);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f)return nullptr;
        const auto& source=slinkywinky::recovered_widget_catalog();
        std::size_t index=source.size();
        for(std::size_t i=0;i<source.size();++i) {
            const auto& w=source[i];
            if(std::string_view(w.module)=="Auto Block" &&
               std::string_view(w.group)=="Main" &&
               std::string_view(w.label)=="Range" &&
               w.kind==slinkywinky::RecoveredWidgetKind::Slider) {
                index=i;break;
            }
        }
        if(index==source.size())return nullptr;
        const auto track=slinkywinky::menu_numeric_track_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            1u,index);
        if(!track)return nullptr;
        const float fraction=static_cast<float>(fractionPermille)/1000.0f;
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround((track->x+track->width*fraction)*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround((track->y+track->height*0.5f)*panel->scale));
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::RecoveredSliderPreview ||
           hit->catalog_index!=1u || hit->control_index!=index ||
           x<0 || x>=static_cast<int>(kWidth) || y<0 ||
           y>=static_cast<int>(kHeight))return nullptr;
        const jint coords[2]={x,y};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,coords);
        return result;
    }catch(...) {return nullptr;}
}

// R168: original recovered Auto Block / Conditions / Damaged Boolean, page
// offset 10 of 12 source controls. JNI exports are DISPOSABLE test-only.
// A read-only source-page receipt guarantees the mirror hit-test cannot be
// used as a substitute for the actual retained menu's current page.
extern "C" JNIEXPORT jint JNICALL
Java_dev_slinkywinky_attach_R168RecoveredBooleanJniSmoke_pageOffset(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u))
        return -1;
    const auto source=session->ui->settings_page(1u);
    return source && *source<=10u?static_cast<jint>(*source):-1;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R168RecoveredBooleanJniSmoke_page(
        JNIEnv*,jclass,jint action) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() || (action!=8 && action!=9) ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u) ||
       !session->ui->settings_page(1u))
        return JNI_FALSE;
    try {
        return session->ui->apply(
            static_cast<slinkywinky::MenuNavAction>(action))?
            JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R168RecoveredBooleanJniSmoke_booleanTarget(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u) ||
       session->ui->settings_page(1u)!=std::optional<std::size_t>(10u))
        return nullptr;
    try {
        slinkywinky::MenuState mirror;
        if(!mirror.toggle_expanded(1u) ||
           !mirror.change_settings_page(1u,1) ||
           !mirror.change_settings_page(1u,1) ||
           mirror.settings_offset(1u)!=10u)return nullptr;
        slinkywinky::MenuProfileBridge local(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(168u,kWidth,kHeight))return nullptr;
        const auto frame=frames.next(mirror,&local);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f)return nullptr;
        const auto& catalog=slinkywinky::recovered_widget_catalog();
        std::size_t target=catalog.size();
        for(std::size_t i=0;i<catalog.size();++i) {
            const auto& w=catalog[i];
            if(std::string_view(w.module)=="Auto Block" &&
               std::string_view(w.group)=="Conditions" &&
               std::string_view(w.label)=="Damaged" &&
               w.kind==slinkywinky::RecoveredWidgetKind::Boolean) {
                target=i;break;
            }
        }
        if(target==catalog.size())return nullptr;
        std::size_t row=frame->visible.size();
        for(std::size_t i=0;i<frame->visible.size();++i)
            if(frame->visible[i].catalog_index==1u) {
                row=i;break;
            }
        if(row==frame->visible.size())return nullptr;
        std::array<const slinkywinky::RecoveredWidget*,5> widgets{};
        const auto& item=frame->visible[row];
        if(item.settings_offset!=10u || !item.expanded ||
           !slinkywinky::recovered_widget_visible_page(
               item.title.c_str(),item.settings_offset,
               static_cast<int>(frame->aim_preview_mode),
               static_cast<int>(frame->target_preview_mode),widgets))
            return nullptr;
        std::size_t localWidget=widgets.size();
        for(std::size_t i=0;i<widgets.size();++i)
            if(widgets[i]==&catalog[target]) {
                localWidget=i;break;
            }
        if(localWidget==widgets.size())return nullptr;
        const auto rect=slinkywinky::menu_module_card_rect(
            *frame,panel->logical_width(),row);
        const auto col=slinkywinky::menu_setting_column(rect.x,rect.width);
        const float logicalX=slinkywinky::menu_setting_switch_x(col)+20.0f;
        const float logicalY=rect.y+
            slinkywinky::menu_row_body_height(*frame)+
            slinkywinky::menu_settings_control_offset(widgets,localWidget)+
            10.0f;
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround(logicalX*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround(logicalY*panel->scale));
        if(x<0 || y<0 || x>=static_cast<int>(kWidth) ||
           y>=static_cast<int>(kHeight))return nullptr;
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::RecoveredBooleanPreview ||
           hit->catalog_index!=1u || hit->control_index!=target)
            return nullptr;
        const jint coords[2]={x,y};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,coords);
        return result;
    }catch(...) {return nullptr;}
}

// R169: DISPOSABLE Java 8 -> first-party retained GUI dropdown endpoints.
// All input still comes from Java test fixtures, not a game or OS hook.
// The real C++ menu owns dropdown state and validates every click/option.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R169DropdownJniSmoke_modeReceipt(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto selected=session->ui->aim_preview_mode();
    if(!selected || *selected>3u)return nullptr;
    const jint items[2]={
        static_cast<jint>(*selected),
        session->ui->preview_dropdown_open()?1:0
    };
    jintArray result=env->NewIntArray(2);
    if(result)env->SetIntArrayRegion(result,0,2,items);
    return result;
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R169DropdownJniSmoke_aimControlTarget(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       session->ui->focused_row()!=std::optional<std::size_t>(0u) ||
       session->ui->settings_page(0u)!=std::optional<std::size_t>(0u) ||
       session->ui->preview_dropdown_open())return nullptr;
    try {
        slinkywinky::MenuState mirror;
        if(!mirror.toggle_expanded(0u))return nullptr;
        slinkywinky::MenuProfileBridge profile(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(169u,kWidth,kHeight))return nullptr;
        const auto frame=frames.next(mirror,&profile);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f ||
           frame->visible.empty() ||
           frame->visible[0].catalog_index!=0u ||
           frame->visible[0].settings_offset!=0u)return nullptr;
        std::array<const slinkywinky::RecoveredWidget*,5> widgets{};
        if(!slinkywinky::recovered_widget_visible_page(
            "Aim Assist",0u,
            static_cast<int>(frame->aim_preview_mode),
            static_cast<int>(frame->target_preview_mode),widgets) ||
           !widgets[0] ||
           std::string_view(widgets[0]->label)!="Mode" ||
           std::string_view(widgets[0]->group)!="Aim" ||
           widgets[0]->kind!=slinkywinky::RecoveredWidgetKind::Dropdown)
            return nullptr;
        const auto rect=slinkywinky::menu_module_card_rect(
            *frame,panel->logical_width(),0u);
        const auto col=slinkywinky::menu_setting_column(
            rect.x,rect.width);
        const float controlY=rect.y+
            slinkywinky::menu_row_body_height(*frame)+
            slinkywinky::menu_settings_control_offset(widgets,0u)+10.0f;
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround((col.x+col.width*0.5f)*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround(controlY*panel->scale));
        if(x<0 || y<0 || x>=static_cast<int>(kWidth) ||
           y>=static_cast<int>(kHeight))return nullptr;
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::AimModeControl ||
           hit->catalog_index!=0u || hit->control_index!=0u)
            return nullptr;
        const jint xy[2]={x,y};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,xy);
        return result;
    }catch(...) {return nullptr;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R169DropdownJniSmoke_aimOptionTarget(
        JNIEnv* env,jclass,jint option) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       !session->ui->preview_dropdown_open() ||
       session->ui->focused_row()!=std::optional<std::size_t>(0u) ||
       session->ui->settings_page(0u)!=std::optional<std::size_t>(0u) ||
       option<0 || option>=4)return nullptr;
    try {
        slinkywinky::MenuState mirror;
        if(!mirror.toggle_expanded(0u) ||
           !mirror.open_preview_dropdown(
                slinkywinky::MenuDropdownKind::AimMode,0u))
            return nullptr;
        slinkywinky::MenuProfileBridge profile(mirror);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(170u,kWidth,kHeight))return nullptr;
        const auto frame=frames.next(mirror,&profile);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel)return nullptr;
        const auto popup=slinkywinky::menu_dropdown_popup_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        if(!popup || popup->option_count!=4u ||
           popup->first_option!=0u || popup->row_height<=0.0f)
            return nullptr;
        const float lx=popup->x+popup->width*0.5f;
        const float ly=popup->y+popup->row_height*
            (static_cast<float>(option)+0.5f);
        const int x=static_cast<int>(panel->left)+static_cast<int>(
            std::lround(lx*panel->scale));
        const int y=static_cast<int>(panel->top)+static_cast<int>(
            std::lround(ly*panel->scale));
        const auto hit=slinkywinky::hit_test_menu(
            *frame,slinkywinky::MenuPanelMode::CompactOverlay,x,y);
        if(!hit || hit->kind!=slinkywinky::MenuHitKind::DropdownOption ||
           hit->catalog_index!=0u ||
           hit->control_index!=static_cast<std::size_t>(option))
            return nullptr;
        const jint xy[2]={x,y};
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,xy);
        return result;
    }catch(...) {return nullptr;}
}

// R175: disposable JNI entry points exercising the SAME production C++
// owner-bound passive hover model; no original game mouse interception.
// The source-recovered coordinates are provided by R167 sliderTarget.
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R175RetainedHoverJniSmoke_observeHover(
        JNIEnv*,jclass,jint x,jint y,jboolean foreground,
        jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() ||
       width<320 || height<240 || width>8192 || height>8192)
        return JNI_FALSE;
    try {
        return session->ui->observe_hover(
            session->gl,session->dc,
            static_cast<unsigned>(width),static_cast<unsigned>(height),
            x,y,foreground==JNI_TRUE,true)?JNI_TRUE:JNI_FALSE; // disposable fixture: synthetic release
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R175RetainedHoverJniSmoke_hoverReceipt(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto hit=session->ui->observed_numeric_hover();
        if(!hit || hit->catalog_index>67u ||
           hit->widget_index>=slinkywinky::recovered_widget_catalog().size())
            return nullptr;
        const jint record[2]={
            static_cast<jint>(hit->catalog_index),
            static_cast<jint>(hit->widget_index)
        };
        jintArray result=env->NewIntArray(2);
        if(result)env->SetIntArrayRegion(result,0,2,record);
        return result;
    }catch(...) {return nullptr;}
}

// R176: read-only evidence from the same source-retained WGL hover owner.
// Test DLL only. No original game or game input can enter this endpoint.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R175RetainedHoverJniSmoke_hoverEvidence(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    try {
        const auto data=session->ui->hover_evidence(
            session->gl,session->dc,kWidth,kHeight);
        if(!data)return nullptr;
        jint value[5]={};
        for(std::size_t i=0;i<5;i++) {
            if((*data)[i]>static_cast<std::uint64_t>(
                    std::numeric_limits<jint>::max()))return nullptr;
            value[i]=static_cast<jint>((*data)[i]);
        }
        jintArray result=env->NewIntArray(5);
        if(result)env->SetIntArrayRegion(result,0,5,value);
        return result;
    }catch(...) {return nullptr;}
}

// R177: isolated REAL GL11 framebuffer readback, bounded to the
// source-identified recovered Auto Block / Main / Range tooltip rectangle.
// Never linked into the production JNI library or Java instrumentation.
extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_attach_R177RetainedTooltipPixelsSmoke_pixelDigest(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u) ||
       session->ui->settings_page(1u)!=std::optional<std::size_t>(0u))
        return static_cast<jlong>(-1);
    try {
        slinkywinky::MenuState mirror;
        if(!mirror.toggle_expanded(1u))return static_cast<jlong>(-1);
        slinkywinky::MenuProfileBridge local(mirror);
        const auto& source=slinkywinky::recovered_widget_catalog();
        std::size_t index=source.size();
        for(std::size_t i=0;i<source.size();++i) {
            const auto& w=source[i];
            if(std::string_view(w.module)=="Auto Block" &&
               std::string_view(w.group)=="Main" &&
               std::string_view(w.label)=="Range" &&
               w.kind==slinkywinky::RecoveredWidgetKind::Slider) {
                index=i;break;
            }
        }
        if(index==source.size() ||
           !mirror.set_numeric_hover(
               slinkywinky::MenuNumericHover{1u,index}))
            return static_cast<jlong>(-1);
        slinkywinky::MenuRenderSession frames;
        if(!frames.attach(177u,kWidth,kHeight))return static_cast<jlong>(-1);
        const auto frame=frames.next(mirror,&local);
        const auto panel=slinkywinky::menu_panel_geometry(
            kWidth,kHeight,false,slinkywinky::MenuPanelMode::CompactOverlay);
        if(!frame || !panel || panel->scale<=0.0f)return static_cast<jlong>(-1);
        const auto rect=slinkywinky::menu_numeric_tooltip_geometry(
            *frame,panel->logical_width(),panel->logical_height());
        if(!rect || rect->width<12.0f || rect->height!=20.0f)
            return static_cast<jlong>(-1);
        const int clientX=static_cast<int>(
            std::floor(panel->left+rect->x*panel->scale));
        const int clientY=static_cast<int>(
            std::floor(panel->top+rect->y*panel->scale));
        const int pixelsW=static_cast<int>(
            std::ceil(rect->width*panel->scale));
        const int pixelsH=static_cast<int>(
            std::ceil(rect->height*panel->scale));
        if(clientX<0 || clientY<0 || pixelsW<8 || pixelsH<8 ||
           clientX+pixelsW>static_cast<int>(kWidth) ||
           clientY+pixelsH>static_cast<int>(kHeight))
            return static_cast<jlong>(-1);
        // Re-establish one deterministic framebuffer for each render.
        // The only possible pixel difference in the bounded source tooltip
        // location is the first-party GL11 menu's passive hover state.
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        if(!session->ui->draw(session->gl,session->dc,kWidth,kHeight))
            return static_cast<jlong>(-1);
        glFinish();
        GLint readBuffer=0;
        glGetIntegerv(GL_READ_BUFFER,&readBuffer);
        if(readBuffer<=0)return static_cast<jlong>(-1);
        glReadBuffer(GL_BACK);
        std::vector<unsigned char> pixels(
            static_cast<std::size_t>(pixelsW)*
            static_cast<std::size_t>(pixelsH)*4u);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(clientX,
                     static_cast<int>(kHeight)-clientY-pixelsH,
                     pixelsW,pixelsH,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        const GLenum error=glGetError();
        glReadBuffer(static_cast<GLenum>(readBuffer));
        if(error!=GL_NO_ERROR || glGetError()!=GL_NO_ERROR)
            return static_cast<jlong>(-1);
        std::uint64_t hash=14695981039346656037ull;
        for(auto item:pixels){hash^=item;hash*=1099511628211ull;}
        return static_cast<jlong>(hash);
    }catch(...){return static_cast<jlong>(-1);}
}

// R179: test-only JNI evidence from the actual first-party retained native
// source target. No desktop cursor movement, game attach, or click action.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R179ExactSourceTargetJniSmoke_target(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto target=session->ui->source_hover_target(
        session->gl,session->dc,kWidth,kHeight);
    if(!target)return nullptr;
    const jint data[6]={target->left,target->top,target->right,
                        target->bottom,target->center_x,target->center_y};
    jintArray out=env->NewIntArray(6);
    if(out)env->SetIntArrayRegion(out,0,6,data);
    return out;
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R179ExactSourceTargetJniSmoke_evidence(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto e=session->ui->source_hover_evidence(
        session->gl,session->dc,kWidth,kHeight);
    if(!e)return nullptr;
    for(const auto count:*e)
        if(count>static_cast<std::uint64_t>(
                     std::numeric_limits<jint>::max()))return nullptr;
    const jint data[3]={
        static_cast<jint>((*e)[0]),static_cast<jint>((*e)[1]),
        static_cast<jint>((*e)[2])};
    jintArray out=env->NewIntArray(3);
    if(out)env->SetIntArrayRegion(out,0,3,data);
    return out;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R179ExactSourceTargetJniSmoke_nextTab(
        JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() ||
       session->ui->focused_row()!=std::optional<std::size_t>(1u) ||
       !session->ui->settings_page(1u))return JNI_FALSE;
    return session->ui->apply(slinkywinky::MenuNavAction::NextTab)?
           JNI_TRUE:JNI_FALSE;
}
// R182: DISPOSABLE JNI only. The first-party native model receives a
// test-supplied foreground flag; these counters are not hardware evidence.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R182FocusCycleJniSmoke_targetCenter(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto target=session->ui->source_hover_target(
        session->gl,session->dc,kWidth,kHeight);
    if(!target)return nullptr;
    const jint data[2]={target->center_x,target->center_y};
    jintArray result=env->NewIntArray(2);
    if(result)env->SetIntArrayRegion(result,0,2,data);
    return result;
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R182FocusCycleJniSmoke_focusEvidence(
        JNIEnv* env,jclass,jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       width<320 || height<240 || width>8192 || height>8192)
        return nullptr;
    const auto evidence=session->ui->focus_cycle_evidence(
        session->gl,session->dc,
        static_cast<unsigned>(width),static_cast<unsigned>(height));
    if(!evidence)return nullptr;
    for(const auto count:*evidence)
        if(count>static_cast<std::uint64_t>(
                     std::numeric_limits<jint>::max()))return nullptr;
    const jint data[3]={static_cast<jint>((*evidence)[0]),
                        static_cast<jint>((*evidence)[1]),
                        static_cast<jint>((*evidence)[2])};
    jintArray out=env->NewIntArray(3);
    if(out)env->SetIntArrayRegion(out,0,3,data);
    return out;
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R182FocusCycleJniSmoke_sourceEvidence(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto evidence=session->ui->source_hover_evidence(
        session->gl,session->dc,kWidth,kHeight);
    if(!evidence)return nullptr;
    for(const auto count:*evidence)
        if(count>static_cast<std::uint64_t>(
                     std::numeric_limits<jint>::max()))return nullptr;
    const jint data[3]={static_cast<jint>((*evidence)[0]),
                        static_cast<jint>((*evidence)[1]),
                        static_cast<jint>((*evidence)[2])};
    jintArray out=env->NewIntArray(3);
    if(out)env->SetIntArrayRegion(out,0,3,data);
    return out;
}

// R183: test-only exact OS focus sequence injected as synthetic status data.
// The JNI DLL is disposable and NEVER part of the Minecraft injector.
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R183ExactWindowFocusJniSmoke_target(
        JNIEnv* env,jclass) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid())return nullptr;
    const auto target=session->ui->source_hover_target(
        session->gl,session->dc,kWidth,kHeight);
    if(!target)return nullptr;
    const jint data[2]={target->center_x,target->center_y};
    jintArray out=env->NewIntArray(2);
    if(out)env->SetIntArrayRegion(out,0,2,data);
    return out;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_dev_slinkywinky_attach_R183ExactWindowFocusJniSmoke_sample(
        JNIEnv*,jclass,jint status,jboolean pointerValid,
        jint x,jint y,jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!session || !session->valid() ||
       status<-1 || status>1 ||
       width!=static_cast<jint>(kWidth) ||
       height!=static_cast<jint>(kHeight))return JNI_FALSE;
    try {
        const bool physical=pointerValid==JNI_TRUE;
        if(!session->ui->observe_hover(
               session->gl,session->dc,kWidth,kHeight,x,y,physical))
            return JNI_FALSE;
        return session->ui->observe_original_window_focus(
            session->gl,session->dc,kWidth,kHeight,
            static_cast<slinkywinky::OriginalWindowFocusState>(status),
            physical)?JNI_TRUE:JNI_FALSE;
    }catch(...) {return JNI_FALSE;}
}
extern "C" JNIEXPORT jintArray JNICALL
Java_dev_slinkywinky_attach_R183ExactWindowFocusJniSmoke_evidence(
        JNIEnv* env,jclass,jint width,jint height) {
    std::lock_guard<std::mutex> lock(gate);
    if(!env || !session || !session->valid() ||
       width<320 || height<240 || width>8192 || height>8192)
        return nullptr;
    const auto evidence=session->ui->original_window_focus_evidence(
        session->gl,session->dc,
        static_cast<unsigned>(width),static_cast<unsigned>(height));
    if(!evidence)return nullptr;
    for(const auto count:*evidence)
        if(count>static_cast<std::uint64_t>(
                     std::numeric_limits<jint>::max()))return nullptr;
    jint data[6]{};
    for(std::size_t i=0;i<6u;i++)data[i]=static_cast<jint>((*evidence)[i]);
    jintArray result=env->NewIntArray(6);
    if(result)env->SetIntArrayRegion(result,0,6,data);
    return result;
}
