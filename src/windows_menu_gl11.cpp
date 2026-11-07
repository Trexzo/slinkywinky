#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_gl11.hpp"
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_bitmap_font.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_unload_modal_geometry.hpp"
#include "slinkywinky/menu_visual_copy.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/menu_numeric_tooltip.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/windows_menu_system_font.hpp"
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <string>

namespace slinkywinky {
namespace {
bool stack_available(GLenum depth, GLenum capacity,
                     GLint minimum_depth) noexcept {
    GLint current=0, max=0;
    glGetIntegerv(depth,&current);
    glGetIntegerv(capacity,&max);
    return current>=minimum_depth && max>current;
}

// First-party opaque geometry and tiny ASCII bitmaps only. No textures,
// shaders, VBO, font files, framebuffer binding or game input.
void quad(float x,float y,float w,float h) noexcept {
    glBegin(GL_QUADS);
    glVertex2f(x,y);
    glVertex2f(x+w,y);
    glVertex2f(x+w,y+h);
    glVertex2f(x,y+h);
    glEnd();
}
// R149: visual-only nested opacity. Its RAII owner restores the factor
// even on an early render failure. GL state remains isolated by R67.
thread_local float visual_alpha=1.0f;
struct ScopedVisualAlpha {
    float previous;
    explicit ScopedVisualAlpha(float factor) noexcept :
        previous(visual_alpha) {
        visual_alpha*=std::clamp(factor,0.0f,1.0f);
    }
    ~ScopedVisualAlpha() noexcept {visual_alpha=previous;}
};
void rgba(std::uint32_t c,GLubyte alpha=255) noexcept {
    const auto adjusted=static_cast<GLubyte>(
        std::clamp(std::lround(static_cast<float>(alpha)*visual_alpha),
                   0l,255l));
    glColor4ub(static_cast<GLubyte>((c>>16)&255),
               static_cast<GLubyte>((c>>8)&255),
               static_cast<GLubyte>(c&255),adjusted);
}
void rgb(std::uint32_t c) noexcept {
    rgba(c);
}

// R113: rounded, antialiased-free GL1.1 geometry. The radius is clamped
// to prevent invalid vertices at narrow panels or tiny controls.
void round_rect(float x,float y,float w,float h,float radius) noexcept {
    if(w<=0 || h<=0)return;
    const float r=std::max(0.0f,std::min(radius,std::min(w,h)*0.5f));
    if(r<=0.01f){quad(x,y,w,h);return;}
    constexpr float pi=3.14159265358979323846f;
    constexpr int steps=6;
    const float cx[4]={x+w-r,x+w-r,x+r,x+r};
    const float cy[4]={y+r,y+h-r,y+h-r,y+r};
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x+w*0.5f,y+h*0.5f);
    for(int corner=0;corner<4;++corner) {
        const float start=static_cast<float>(corner-1)*0.5f*pi;
        for(int step=0;step<=steps;++step) {
            const float a=start+static_cast<float>(step)*(0.5f*pi/steps);
            glVertex2f(cx[corner]+r*std::cos(a),
                       cy[corner]+r*std::sin(a));
        }
    }
    // Close the fan along its flat top. Without this final point the wedge
    // between the top corners was omitted, producing a diagonal cutout.
    glVertex2f(x+w-r,y);
    glEnd();
}
// Legacy glyphs remain a deterministic fallback if a Windows font isn't
// available; the default is grayscale Segoe UI supplied by the local OS.
void text(std::string_view label,float x,float y,
          float fontPx,float maxWidth) noexcept {
    if(draw_menu_system_text(label,x,y,fontPx,maxWidth))return;
    const float pixelScale=fontPx/9.0f;
    if(pixelScale<=0.0f || maxWidth<5.0f*pixelScale)return;
    const float advance=6.0f*pixelScale;
    const float glyphWidth=5.0f*pixelScale;
    const auto limit=std::min<std::size_t>(label.size(),48);
    glBegin(GL_QUADS);
    for(std::size_t i=0;i<limit;++i) {
        const float offset=static_cast<float>(i)*advance;
        if(offset+glyphWidth>maxWidth)break;
        const auto rows=menu_ascii_glyph(label[i]);
        for(unsigned row=0;row<7;++row) {
            for(unsigned col=0;col<5;++col) {
                if((rows[row]&(1u<<(4-col)))==0u)continue;
                const float left=x+offset+static_cast<float>(col)*pixelScale;
                const float top=y+static_cast<float>(row)*pixelScale;
                glVertex2f(left,top);
                glVertex2f(left+pixelScale,top);
                glVertex2f(left+pixelScale,top+pixelScale);
                glVertex2f(left,top+pixelScale);
            }
        }
    }
    glEnd();
}

// Four independent, hand-drawn glyphs for the read-only profile preview.
// Not buttons: hit testing and keyboard navigation NEVER act on these.
void profile_action_glyph(
        unsigned kind,float x,float y,bool disabled=false) noexcept {
    rgba(kind==3?0xe74a50:0x9e9ea2,
         disabled?128u:255u);
    if(kind==0 || kind==1) {
        // Save/upload and load/download: tray, stem and arrow head.
        quad(x+2.0f,y+12.0f,12.0f,2.0f);
        quad(x+2.0f,y+9.0f,2.0f,4.0f);
        quad(x+12.0f,y+9.0f,2.0f,4.0f);
        quad(x+7.0f,y+3.0f,2.0f,8.0f);
        glBegin(GL_TRIANGLES);
        if(kind==0) {
            glVertex2f(x+3.0f,y+7.0f);
            glVertex2f(x+13.0f,y+7.0f);
            glVertex2f(x+8.0f,y+1.0f);
        } else {
            glVertex2f(x+3.0f,y+7.0f);
            glVertex2f(x+13.0f,y+7.0f);
            glVertex2f(x+8.0f,y+12.0f);
        }
        glEnd();
    } else if(kind==2) {
        // Folder with tab: its shape is fully first-party geometry.
        round_rect(x+1.0f,y+5.0f,15.0f,10.0f,2.0f);
        quad(x+2.0f,y+3.0f,7.0f,4.0f);
    } else {
        // Trash icon intentionally red and visually inert.
        round_rect(x+3.0f,y+5.0f,10.0f,10.0f,2.0f);
        quad(x+2.0f,y+3.0f,12.0f,2.0f);
        quad(x+6.0f,y+1.0f,4.0f,2.0f);
        rgba(0x161617,disabled?128u:255u);
        quad(x+6.0f,y+7.0f,1.0f,6.0f);
        quad(x+9.0f,y+7.0f,1.0f,6.0f);
    }
}

// R116: read-only representations of all eight recovered candidate widget
// kinds. They use first-party fixed-function geometry, never callbacks,
// filesystem actions, mouse capture, or gameplay state.
unsigned hex_digit(char c) noexcept {
    if(c>='0'&&c<='9')return static_cast<unsigned>(c-'0');
    if(c>='a'&&c<='f')return static_cast<unsigned>(c-'a'+10);
    if(c>='A'&&c<='F')return static_cast<unsigned>(c-'A'+10);
    return 0;
}
std::uint32_t preview_swatch(const char* s) noexcept {
    if(!s || s[0]!='#')return 0x707070u;
    std::uint32_t value=0;
    for(unsigned i=1;i<=6;++i) {
        if(!s[i])return 0x707070u;
        value=(value<<4)|hex_digit(s[i]);
    }
    return value;
}
void draw_recovered_widget(const RecoveredWidget& widget,
                           float x,float y,float cardWidth,
                           std::uint32_t accent,
                           std::string_view dropdownOverride={},
                           std::string_view multiOverride={},
                           std::optional<std::uint32_t> colorOverride={},
                           std::string_view textOverride={},
                           bool textFocused=false,
                           float booleanHover=0.0f)
                           noexcept {
    const auto column=menu_setting_column(x,cardWidth);
    const float controlX=column.x;
    const float controlW=column.width;
    const float labelW=std::max(14.0f,controlX-x-14.0f);
    rgb(0xc9c9c9);
    // R139: source HTML renders Button as the entire row, without a
    // duplicate left-side setting label or an OS/game action.
    if(widget.kind!=RecoveredWidgetKind::Button)
        text(widget.label,x+8.0f,
             menu_control_center_text_y(y,20.0f,15.0f),15.0f,labelW);
    const float controlY=y+2.0f;
    switch(widget.kind) {
        case RecoveredWidgetKind::Boolean: {
            const float bx=menu_setting_switch_x(column);
            rgb(widget.example_enabled?accent:
                menu_control_hover_tint(0x404040u,booleanHover));
            round_rect(bx,controlY,40.0f,20.0f,10.0f);
            rgb(0x141414);
            round_rect(bx+(widget.example_enabled?26.0f:6.0f),
                       controlY+6.0f,8.0f,8.0f,4.0f);
            break;
        }
        case RecoveredWidgetKind::Slider:
        case RecoveredWidgetKind::Range: {
            const float minV=widget.min_value,maxV=widget.max_value;
            const float denominator=maxV>minV?maxV-minV:1.0f;
            const float a=std::max(0.0f,std::min(1.0f,
                (widget.example_value-minV)/denominator));
            rgb(0x404040u);
            round_rect(controlX,controlY+8.0f,controlW,4.0f,2.0f);
            rgb(accent);
            if(widget.kind==RecoveredWidgetKind::Range) {
                const float b=std::max(0.0f,std::min(1.0f,
                    (widget.example_other-minV)/denominator));
                const float lo=std::min(a,b),hi=std::max(a,b);
                round_rect(controlX+controlW*lo,controlY+8.0f,
                           std::max(0.01f,controlW*(hi-lo)),4.0f,2.0f);
                // Source NanoVG knob is r6: 12px diameter.
                round_rect(controlX+controlW*b-6.0f,
                           controlY+4.0f,12.0f,12.0f,6.0f);
            } else
                round_rect(controlX,controlY+8.0f,
                           std::max(0.01f,controlW*a),4.0f,2.0f);
            round_rect(controlX+controlW*a-6.0f,
                       controlY+4.0f,12.0f,12.0f,6.0f);
            break;
        }
        case RecoveredWidgetKind::Dropdown:
        case RecoveredWidgetKind::MultiSelect:
        case RecoveredWidgetKind::Text: {
            rgb(widget.kind==RecoveredWidgetKind::Text && textFocused?
                accent:0x404040u);
            round_rect(controlX,controlY,controlW,20.0f,10.0f);
            rgb(0x141414u);
            round_rect(controlX+2.0f,controlY+2.0f,
                       controlW-4.0f,16.0f,8.0f);
            std::string_view label;
            if(widget.kind==RecoveredWidgetKind::Dropdown) {
                label=dropdownOverride.empty()?
                      std::string_view(widget.options):dropdownOverride;
                label=label.substr(0,label.find('|'));
            } else if(widget.kind==RecoveredWidgetKind::MultiSelect) {
                label=multiOverride.empty()?"Select options":multiOverride;
            } else label=textOverride.empty()?
                    std::string_view(widget.auxiliary):textOverride;
            rgb(widget.kind==RecoveredWidgetKind::Text &&
                !textOverride.empty()?0xDDDDDDu:0x999999u);
            text(label,controlX+6.0f,
                 menu_control_center_text_y(controlY,20.0f,15.0f),
                 15.0f,controlW-12.0f);
            if(widget.kind==RecoveredWidgetKind::Text && textFocused) {
                const float measured=textOverride.empty()?0.0f:
                    measure_menu_system_text(textOverride,15.0f);
                const float xCaret=controlX+6.0f+
                    std::clamp(measured,0.0f,
                               std::max(0.0f,controlW-16.0f));
                rgb(accent);
                quad(xCaret,controlY+3.0f,1.0f,14.0f);
            }
            break;
        }
        case RecoveredWidgetKind::Color: {
            rgb(colorOverride.value_or(preview_swatch(widget.auxiliary)));
            round_rect(menu_setting_switch_x(column),controlY,
                       40.0f,20.0f,10.0f);
            break;
        }
        case RecoveredWidgetKind::Button: {
            // Recovered clickgui.tpl.html: row with a single full-width
            // .btn, 20px tall, radius 8, #404040, centered 15px label.
            // No source onclick exists for either Friends button.
            const float bx=x+8.0f;
            const float bw=std::max(16.0f,cardWidth-16.0f);
            rgb(0x404040u);
            round_rect(bx,controlY,bw,20.0f,8.0f);
            rgb(0xdddddd);
            const std::string_view label(widget.label);
            const float measured=measure_menu_system_text(label,15.0f);
            const float estimated=measured>0.0f?measured:
                static_cast<float>(label.size())*7.5f;
            const float tx=bx+std::max(8.0f,(bw-estimated)*0.5f);
            text(label,tx,
                 menu_control_center_text_y(controlY,20.0f,15.0f),
                 15.0f,std::max(2.0f,bx+bw-tx-8.0f));
            break;
        }
    }
}

/**
 * GL1.1 server/client attributes and the three fixed-function matrices.
 * No OpenGL call in the constructor may be made without the current WGL
 * context being checked by the outer function.
 */
class ScopedFixedFunctionState {
public:
    ScopedFixedFunctionState() noexcept {
        GLint current=0;
        glGetIntegerv(GL_MATRIX_MODE,&current);
        original_mode_=static_cast<GLenum>(current);
        if(original_mode_!=GL_MODELVIEW &&
           original_mode_!=GL_PROJECTION &&
           original_mode_!=GL_TEXTURE)
            return;
        // GL attribute stacks begin at depth 0; matrix stacks begin at 1.
        // Confusing these rejected a valid fresh WGL context.
        if(!stack_available(GL_ATTRIB_STACK_DEPTH,GL_MAX_ATTRIB_STACK_DEPTH,0) ||
           !stack_available(GL_CLIENT_ATTRIB_STACK_DEPTH,
                            GL_MAX_CLIENT_ATTRIB_STACK_DEPTH,0) ||
           !stack_available(GL_MODELVIEW_STACK_DEPTH,GL_MAX_MODELVIEW_STACK_DEPTH,1) ||
           !stack_available(GL_PROJECTION_STACK_DEPTH,GL_MAX_PROJECTION_STACK_DEPTH,1) ||
           !stack_available(GL_TEXTURE_STACK_DEPTH,GL_MAX_TEXTURE_STACK_DEPTH,1))
            return;
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        server_=true;
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        client_=true;
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glMatrixMode(original_mode_);
        ready_=true;
    }
    bool ready() const noexcept {return ready_;}
    ~ScopedFixedFunctionState() noexcept {
        if(!server_)return;
        if(ready_) {
            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(original_mode_);
        }
        if(client_)glPopClientAttrib();
        glPopAttrib();
    }
    ScopedFixedFunctionState(const ScopedFixedFunctionState&)=delete;
    ScopedFixedFunctionState& operator=(const ScopedFixedFunctionState&)=delete;
private:
    GLenum original_mode_=GL_MODELVIEW;
    bool server_=false;
    bool client_=false;
    bool ready_=false;
};
}

bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode) noexcept {
    return draw_menu_gl11(frame,expected_context,expected_dc,mode,std::nullopt);
}
bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode,
                    std::optional<std::size_t> focused_catalog_index) noexcept {
    // Never use a context inferred only from a PID. Require exact equality.
    if((mode!=MenuGL11Mode::StandalonePreview &&
        mode!=MenuGL11Mode::BoundedOverlay &&
        mode!=MenuGL11Mode::CompactOverlay) || !expected_context || !expected_dc ||
       wglGetCurrentContext()!=expected_context ||
       wglGetCurrentDC()!=expected_dc ||
       frame.epoch==0 || frame.sequence==0 ||
       frame.viewport_width<320 || frame.viewport_width>8192 ||
       frame.viewport_height<240 || frame.viewport_height>8192 ||
       frame.visible.size()>8 ||
       frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload ||
       (frame.is_placeholder != (frame.tab==MenuTab::Profiles)))
        return false;
    for(const auto& item:frame.visible)
        if(item.catalog_index>=official_module_catalog().size() ||
           official_module_catalog()[item.catalog_index].tab!=frame.tab ||
           item.title!=official_module_catalog()[item.catalog_index].name)
            return false;
    if(frame.is_placeholder && !frame.visible.empty())return false;
    if(focused_catalog_index) {
        bool inVisiblePage=false;
        for(const auto& row:frame.visible)
            if(row.catalog_index==*focused_catalog_index)
                inVisiblePage=true;
        if(!inVisiblePage)return false; // No stale or forged focus index.
    }
    if(frame.parameters.size()>frame.visible.size())return false;
    for(std::size_t i=0;i<frame.parameters.size();++i) {
        const auto& parameter=frame.parameters[i];
        if(parameter.caption.empty() || parameter.caption.size()>48 ||
           !module_has_profile_setting(parameter.catalog_index) ||
           (parameter.numeric &&
              (!std::isfinite(parameter.value) ||
               !std::isfinite(parameter.minimum) ||
               !std::isfinite(parameter.maximum) ||
               parameter.maximum<=parameter.minimum ||
               parameter.value<parameter.minimum ||
               parameter.value>parameter.maximum)))
            return false;
        for(const unsigned char c:parameter.caption)
            if(c<32 || c>126)return false; // ASCII first-party bitmap only.
        bool matched=false;
        for(const auto& item:frame.visible)
            if(item.catalog_index==parameter.catalog_index && item.expanded)
                matched=true;
        if(!matched)return false;
        for(std::size_t j=0;j<i;++j)
            if(frame.parameters[j].catalog_index==parameter.catalog_index)
                return false;
    }
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(frame.viewport_width) ||
       viewport[3]!=static_cast<GLint>(frame.viewport_height))
        return false;
    const auto geometryMode=mode==MenuGL11Mode::StandalonePreview?
        MenuPanelMode::StandalonePreview:
        (mode==MenuGL11Mode::BoundedOverlay?
         MenuPanelMode::BoundedOverlay:MenuPanelMode::CompactOverlay);
    const auto panel=menu_panel_geometry(frame.viewport_width,frame.viewport_height,
        frame.appearance.wide,geometryMode);
    if(!panel || !menu_settings_offsets_valid(frame) ||
       !menu_profile_preview_valid(
            frame,panel->logical_width(),panel->logical_height()) ||
       (frame.visual_motion && !menu_visual_motion_valid(
            *frame.visual_motion,frame.tab,panel->logical_height())) ||
       (frame.control_motion && !menu_control_motion_valid(
            *frame.control_motion)))
        return false;
    ScopedFixedFunctionState state;
    if(!state.ready())return false;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);

    const bool overlay=mode!=MenuGL11Mode::StandalonePreview;
    // R113: font antialiasing needs blending even in standalone previews.
    // Every modified GL flag, matrix and current color is independently
    // restored by ScopedFixedFunctionState.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const unsigned paneWidth=panel->width;
    const unsigned paneHeight=panel->height;
    const unsigned paneLeft=panel->left;
    const unsigned paneTop=panel->top;
    if(overlay) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(paneLeft),
                  static_cast<GLint>(frame.viewport_height-paneTop-paneHeight),
                  static_cast<GLsizei>(paneWidth),
                  static_cast<GLsizei>(paneHeight));
    } else glDisable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,static_cast<GLdouble>(frame.viewport_width),
            static_cast<GLdouble>(frame.viewport_height),0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(static_cast<GLfloat>(paneLeft),
                 static_cast<GLfloat>(paneTop),0.0f);
    const float scale=panel->scale;
    if(scale!=1.0f)glScalef(scale,scale,1.0f);
    const float w=static_cast<float>(paneWidth)/scale;
    const float h=static_cast<float>(paneHeight)/scale;
    // Apply source .animate 250ms opening translation/fade only to this
    // optional display frame. The original frame/geometry is unchanged.
    const float openY=frame.visual_motion?
        frame.visual_motion->opening_offset_y:0.0f;
    const float openAlpha=frame.visual_motion?
        frame.visual_motion->opening_opacity:1.0f;
    ScopedVisualAlpha openFade(openAlpha);
    if(openY!=0.0f)glTranslatef(0.0f,openY,0.0f);
    rgba(0x303030,overlay?static_cast<GLubyte>(192):static_cast<GLubyte>(255));
    quad(0,0,w,h);
    // Real Slinky uses a slim rounded navigation bar, rather than a
    // large centered logo/title and an orange underline.
    rgb(0x141414);
    round_rect(menu_nav_x,menu_nav_y,w-2*menu_nav_x,
               menu_nav_height,menu_nav_radius);
    const float cell=menu_tab_width(w);
    const auto selected=static_cast<unsigned>(frame.tab);
    const float pill=frame.visual_motion?
        frame.visual_motion->tab_position:static_cast<float>(selected);
    if(selected<6u) {
        rgb(0x404040);
        round_rect(menu_nav_x+pill*cell+
                       menu_nav_selected_inset,
                   menu_nav_y+menu_nav_selected_inset,
                   cell-2.0f*menu_nav_selected_inset,
                   menu_nav_height-2.0f*menu_nav_selected_inset,14.0f);
    }
    for(unsigned i=0;i<7;++i) {
        const auto tab=static_cast<MenuTab>(i);
        std::string_view label=menu_tab_name(tab);
        const float fontPx=16.0f;
        const float textWidth=measure_menu_system_text(label,fontPx);
        const float fallbackWidth=label.size()*fontPx*0.62f;
        const float measured=textWidth>0?textWidth:fallbackWidth;
        const float left=menu_nav_x+static_cast<float>(i)*cell+
                         std::max(2.0f,(cell-measured)*0.5f);
        rgb(i==6?0xdd4747:0xdddddd);
        text(label,left,menu_nav_y+11.0f,fontPx,cell-4.0f);
    }


    // Source #page animation runs only on presentation. Pointer dispatch
    // is withheld by the WGL owner until this translation settles.
    const float pageY=frame.visual_motion?
        frame.visual_motion->page_offset_y:0.0f;
    {
    ScopedVisualAlpha pageFade(frame.visual_motion?
        frame.visual_motion->page_opacity:1.0f);
    if(pageY!=0.0f)glTranslatef(0.0f,pageY,0.0f);
    if(frame.is_placeholder) {
        // R115 source layout: genuine 430px-target card grid. Real session
        // snapshots contain only the actual native Default card. No file
        // operations, fake saved paths, or OS input are connected here.
        for(std::size_t i=0;i<frame.profile_cards.size();++i) {
            const auto& card=frame.profile_cards[i];
            const auto rect=menu_profile_card_rect(frame,w,i);
            rgb(0x141414);
            round_rect(rect.x,rect.y,rect.width,rect.height,
                       menu_card_radius);
            const float nameX=rect.x+8.0f;
            const float iconX=rect.x+rect.width-124.0f;
            const float titleWidth=std::max(30.0f,iconX-nameX-45.0f);
            rgb(0xdddddd);
            text(card.name,nameX,rect.y+8.0f,16.0f,titleWidth);
            const auto chip=menu_profile_bind_rect(frame,w,h,i);
            if(chip) {
                const bool capturing=frame.profile_bind_capture_index==i;
                const bool bound=!card.bind.empty()&&card.bind!="NONE";
                rgba(capturing?0x999999u:
                     (bound?frame.appearance.accent_rgb:0x999999u),38);
                round_rect(chip->x,chip->y,chip->width,20.0f,5.0f);
                rgb(capturing?0x999999u:
                    (bound?frame.appearance.accent_rgb:0x999999u));
                const std::string_view label=capturing?"PRESS A KEY...":
                    (card.bind.empty()?"NONE":
                     std::string_view{card.bind});
                text(label,chip->x+6.0f,
                     chip->y+3.0f,12.0f,chip->width-12.0f);
            }
            if(!card.description.empty() && !frame.appearance.compact) {
                rgb(0x999999);
                text(card.description,nameX,rect.y+40.0f,14.0f,
                     rect.width-16.0f);
            }
            for(unsigned action=0;action<4;++action)
                profile_action_glyph(action,iconX+action*32.0f,
                                     rect.y+10.0f,
                                     i==0u && (action==1u || action==3u));
        }
        const auto nameInput=menu_profile_input_rect(frame,w,h);
        const auto create=menu_profile_create_rect(frame,w,h);
        if(nameInput && create) {
            // Source candidate: transparent half-opacity two-row card.
            const float cardX=nameInput->x-8.0f;
            const float cardY=nameInput->y-8.0f;
            rgba(0x141414,128);
            round_rect(cardX,cardY,nameInput->width+16.0f,68.0f,20.0f);
            rgb(frame.profile_name_focused?frame.appearance.accent_rgb:
                0x404040u);
            round_rect(nameInput->x,nameInput->y,
                       nameInput->width,nameInput->height,10.0f);
            rgb(0x202020);
            round_rect(nameInput->x+2.0f,nameInput->y+2.0f,
                       nameInput->width-4.0f,16.0f,8.0f);
            const std::string_view displayed=frame.profile_name_draft.empty()?
                (frame.profile_name_taken?
                 "Profile name is taken.":"Profile name"):
                std::string_view(frame.profile_name_draft);
            rgb(frame.profile_name_draft.empty()?0x999999u:0xdddddd);
            text(displayed,nameInput->x+8.0f,nameInput->y+2.0f,
                 15.0f,nameInput->width-16.0f);
            if(frame.profile_name_focused) {
                const float measured=frame.profile_name_draft.empty()?0.0f:
                    measure_menu_system_text(frame.profile_name_draft,15.0f);
                rgb(frame.appearance.accent_rgb);
                quad(nameInput->x+8.0f+
                     std::clamp(measured,0.0f,nameInput->width-22.0f),
                     nameInput->y+3.0f,1.0f,14.0f);
            }
            rgb(0xdddddd);
            constexpr std::string_view label="CREATE NEW PROFILE";
            const float textW=measure_menu_system_text(label,15.0f);
            text(label,create->x+
                 std::max(8.0f,(create->width-textW)*0.5f),
                 create->y+2.0f,15.0f,create->width-12.0f);
        }
    }

    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& item=frame.visible[i];
        const auto rect=menu_module_card_rect(frame,w,i);
        const float body=menu_row_body_height(frame);
        if(rect.y+rect.height>h-menu_panel_padding)continue;
        const float left=rect.x,right=rect.x+rect.width,y=rect.y;
        rgb(0x141414);
        round_rect(left,y,rect.width,rect.height,menu_card_radius);
        if(focused_catalog_index && item.catalog_index==*focused_catalog_index) {
            rgb(0x404040);
            round_rect(left,y,3.0f,rect.height,1.5f);
        }
        const bool supported=module_has_profile_setting(item.catalog_index);
        const bool hasReferenceSettings=recovered_widget_count(item.title)>0;
        const float titleFont=16.0f;
        const float nameX=left+menu_card_inner_padding;
        const float titleLimit=std::max(20.0f,rect.width-108.0f);
        rgb(0xdddddd);
        text(item.title,nameX,y+8.0f,titleFont,titleLimit);

        // R140: source-scoped key chip and independent HOLD badge.
        // This is the separate GL11 preview; game keybinding tables and
        // the Java8 Attach loader do not observe these values.
        const auto& bind=frame.bind_previews[item.catalog_index];
        const bool capturing=frame.bind_capture_index==
            item.catalog_index;
        const std::string keyText=capturing?"PRESS A KEY...":
            menu_bind_code_label(bind.code);
        const auto chips=menu_bind_chip_geometry(
            frame,w,h,i,item.catalog_index,keyText,capturing);
        if(chips) {
            rgba(capturing?0x999999:
                (bind.code?frame.appearance.accent_rgb:0x999999),38);
            round_rect(chips->key.x,chips->key.y,
                       chips->key.width,chips->key.height,5.0f);
            rgb(capturing?0x999999:
                (bind.code?frame.appearance.accent_rgb:0x999999));
            text(keyText,chips->key.x+6.0f,
                 chips->key.y+3.0f,12.0f,chips->key.width-12.0f);
            // The original chip appears on row hover or when HOLD is on.
            // First-party preview shows a bound HOLD chip persistently
            // until mouse-hover reporting is wired in a later visual gate.
            if(bind.code &&
               (bind.hold || frame.bind_hover_row==item.catalog_index)) {
                rgba(bind.hold?0xdddddd:0x999999,38);
                round_rect(chips->hold.x,chips->hold.y,
                           chips->hold.width,chips->hold.height,5.0f);
                rgb(bind.hold?0xdddddd:0x999999);
                text("HOLD",chips->hold.x+6.0f,
                     chips->hold.y+3.0f,12.0f,
                     chips->hold.width-12.0f);
            }
        }

        // WGL style mirrors source even for disabled/nonimplemented
        // modules. Mouse hit testing admits *only* native profile toggles.
        const bool hasSwitch=item.title!="GUI" &&
            item.title!="Weapons" && item.title!="Friends" &&
            item.title!="Teams";
        if(hasSwitch) {
            const bool active=supported&&item.preview_enabled;
            const float toggleX=right-menu_card_inner_padding-
                                menu_switch_width;
            const float toggleY=y+menu_card_inner_padding;
            rgb(active?frame.appearance.accent_rgb:0x404040);
            round_rect(toggleX,toggleY,40.0f,20.0f,10.0f);
            rgb(0x141414);
            round_rect(toggleX+(active?26.0f:6.0f),
                       toggleY+6.0f,8.0f,8.0f,4.0f);
        }
        if(!frame.appearance.compact) {
            const float expX=left+20.0f,expY=y+40.0f;
            rgb((supported||hasReferenceSettings)?0x404040:0x343434);
            round_rect(expX,expY,20.0f,20.0f,5.0f);
            rgb((supported||hasReferenceSettings)?0xdddddd:0x777777);
            quad(expX+6.0f,expY+9.0f,8.0f,2.0f);
            if(!item.expanded)
                quad(expX+9.0f,expY+6.0f,2.0f,8.0f);
            rgb(0x999999);
            text(menu_preview_description(item.catalog_index),
                 left+46.0f,y+42.0f,14.0f,
                 std::max(10.0f,rect.width-54.0f));
        } else {
            const float expX=left+8.0f,expY=y+8.0f;
            rgb((supported||hasReferenceSettings)?0x404040:0x343434);
            round_rect(expX,expY,20.0f,20.0f,5.0f);
            rgb((supported||hasReferenceSettings)?0xdddddd:0x777777);
            quad(expX+6.0f,expY+9.0f,8.0f,2.0f);
            if(!item.expanded)
                quad(expX+9.0f,expY+6.0f,2.0f,8.0f);
        }
        if(item.expanded) {
            // R123: selected read-only Aim mode chooses visible controls.
            // 385 source callsites remain available in the forensic catalog.
            std::array<const RecoveredWidget*,5> preview{};
            const std::size_t count=recovered_widget_visible_page(
                item.title.c_str(),item.settings_offset,
                static_cast<int>(frame.aim_preview_mode),
                static_cast<int>(frame.target_preview_mode),preview);
            const float settingY=y+body;
            rgb(0x999999);
            if(preview[0])
                text(preview[0]->group,left+8.0f,settingY+8.0f,
                     15.0f,rect.width-16.0f);
            else
                text("No recovered settings",left+8.0f,settingY+8.0f,
                     15.0f,rect.width-16.0f);
            rgb(0x777777);
            if(item.catalog_index==0 && frame.tab==MenuTab::Combat) {
                static constexpr const char* modes[]{
                    "REGULAR","LINEAR","LOCK-ON","SILENT"
                };
                char label[90]{};
                std::snprintf(label,sizeof(label),
                    "AIM PREVIEW: %s  TARGET: %s",
                    modes[frame.aim_preview_mode],
                    frame.target_preview_mode==0?"SINGLE":"SWITCH");
                text(label,left+8.0f,settingY+25.0f,11.0f,
                     rect.width-16.0f);
            } else if(item.catalog_index==11 &&
                      frame.tab==MenuTab::Combat) {
                text("LOCAL PIERCING PREVIEW ONLY  F9/F10/F11",
                     left+8.0f,settingY+25.0f,11.0f,rect.width-16.0f);
            } else
                text("REFERENCE PREVIEW - READ ONLY",left+8.0f,
                     settingY+25.0f,11.0f,rect.width-16.0f);
            float addedHeadings=0.0f;
            std::string_view lastGroup=preview[0]?
                std::string_view(preview[0]->group):std::string_view{};
            for(std::size_t j=0;j<preview.size() && preview[j];++j) {
                const std::string_view group(preview[j]->group);
                if(j>0 && group!=lastGroup) {
                    const float headingY=settingY+43.0f+
                        27.0f*static_cast<float>(j)+addedHeadings;
                    rgb(0x999999u);
                    text(group,left+8.0f,headingY,14.0f,rect.width-16.0f);
                    addedHeadings+=20.0f;
                    lastGroup=group;
                }
                RecoveredWidget display=*preview[j]; // never edits catalog
                // R130: show the exact pointer-indexed Boolean sandbox
                // value, never mutate the source fixture or ClientSettings.
                if(item.catalog_index!=11u &&
                   display.kind==RecoveredWidgetKind::Boolean) {
                    const auto& all=recovered_widget_catalog();
                    const auto slot=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(slot<all.size() &&
                       frame.recovered_boolean_overrides[slot])
                        display.example_enabled=
                            *frame.recovered_boolean_overrides[slot];
                }
                // R132: immutable typed numeric presentation override.
                // Original fixture entries and gameplay settings stay
                // unchanged; the GL bar/knob read a release-quantized value.
                if(display.kind==RecoveredWidgetKind::Slider) {
                    const auto& all=recovered_widget_catalog();
                    const auto slot=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(slot<all.size() &&
                       frame.recovered_slider_overrides[slot])
                        display.example_value=
                            *frame.recovered_slider_overrides[slot];
                    if(slot<all.size() && frame.numeric_drag_preview &&
                       frame.numeric_drag_preview->catalog_index==
                           item.catalog_index &&
                       frame.numeric_drag_preview->widget_index==slot)
                        display.example_value=frame.numeric_drag_preview->lower;
                }
                if(display.kind==RecoveredWidgetKind::Range) {
                    const auto& all=recovered_widget_catalog();
                    const auto slot=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(slot<all.size() &&
                       frame.recovered_range_overrides[slot]) {
                        const auto& range=*frame.recovered_range_overrides[slot];
                        display.example_value=range.lower;
                        display.example_other=range.upper;
                    }
                    if(slot<all.size() && frame.numeric_drag_preview &&
                       frame.numeric_drag_preview->catalog_index==
                           item.catalog_index &&
                       frame.numeric_drag_preview->widget_index==slot) {
                        display.example_value=frame.numeric_drag_preview->lower;
                        display.example_other=frame.numeric_drag_preview->upper;
                    }
                }
                std::string_view selectedChoice;
                if(item.catalog_index==0 && frame.tab==MenuTab::Combat &&
                   display.kind==RecoveredWidgetKind::Dropdown &&
                   std::string_view(display.label)=="Mode") {
                    if(std::string_view(display.group)=="Aim") {
                        static constexpr const char* modes[]{
                            "Regular","Linear","Lock-on","Silent"
                        };
                        selectedChoice=modes[frame.aim_preview_mode];
                    } else if(std::string_view(display.group)=="Target") {
                        selectedChoice=frame.target_preview_mode==0?
                            "Single":"Switch";
                    }
                }
                if(item.catalog_index==11 &&
                   frame.tab==MenuTab::Combat) {
                    const auto control=item.settings_offset+j;
                    if(control==0)selectedChoice=
                        frame.piercing_preview.sort==0?
                            "Hurt time":"Health";
                    else if(control<=7)
                        display.example_enabled=
                            frame.piercing_preview.enabled[control-1];
                    if(control==frame.piercing_selected) {
                        rgb(frame.appearance.accent_rgb);
                        quad(left+2.0f,
                             settingY+47.0f+
                               27.0f*static_cast<float>(j)+addedHeadings,
                             3.0f,13.0f);
                    }
                }
                if(display.kind==RecoveredWidgetKind::Dropdown &&
                   selectedChoice.empty()) {
                    const auto& all=recovered_widget_catalog();
                    const auto widgetIndex=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(widgetIndex<all.size()) {
                        const auto count=menu_source_choice_count(widgetIndex);
                        const float example=display.example_value;
                        if(count && std::isfinite(example) &&
                           example>=0.0f &&
                           example<static_cast<float>(count)) {
                            const auto selected=
                                frame.recovered_choice_overrides[widgetIndex]
                                .value_or(static_cast<unsigned>(example));
                            if(selected<count)
                                selectedChoice=menu_source_choice_label(
                                    widgetIndex,selected);
                        }
                    }
                }
                // R134: compact, source-listed selection summary.
                // Source option labels are held in a local string until
                // the immediate, non-retained GL11 draw call finishes.
                std::string multiCaption;
                if(display.kind==RecoveredWidgetKind::MultiSelect) {
                    const auto& all=recovered_widget_catalog();
                    const auto slot=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(slot<all.size()) {
                        const unsigned n=menu_source_multi_count(slot);
                        const std::uint32_t mask=
                            frame.recovered_multi_overrides[slot].value_or(0u);
                        for(unsigned k=0;k<n;++k) {
                            if(!(mask&(1u<<k)))continue;
                            const auto option=menu_source_multi_label(slot,k);
                            if(!multiCaption.empty())multiCaption+=", ";
                            if(multiCaption.size()+option.size()>120u) {
                                multiCaption+="...";break;
                            }
                            multiCaption.append(option.data(),option.size());
                        }
                    }
                }
                std::optional<std::uint32_t> colorOverride;
                if(display.kind==RecoveredWidgetKind::Color) {
                    const auto& all=recovered_widget_catalog();
                    const auto widgetIndex=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(widgetIndex<all.size())
                        colorOverride=frame.recovered_color_overrides[
                            widgetIndex];
                }
                std::string_view textOverride;
                bool textFocused=false;
                if(display.kind==RecoveredWidgetKind::Text) {
                    const auto& all=recovered_widget_catalog();
                    const auto widgetIndex=static_cast<std::size_t>(
                        preview[j]-all.data());
                    if(widgetIndex<all.size()) {
                        const auto it=std::find_if(
                            frame.recovered_text_overrides.begin(),
                            frame.recovered_text_overrides.end(),
                            [widgetIndex](const MenuRenderTextValue& entry){
                                return entry.widget_index==widgetIndex;});
                        if(it!=frame.recovered_text_overrides.end())
                            textOverride=it->value;
                        textFocused=frame.focused_text_widget==widgetIndex;
                    }
                }
                const auto visualWidgetIndex=static_cast<std::size_t>(
                    preview[j]-recovered_widget_catalog().data());
                const float hover=
                    frame.control_motion &&
                    frame.control_motion->hovered_boolean==visualWidgetIndex?
                    frame.control_motion->switch_hover_opacity:0.0f;
                draw_recovered_widget(display,left,
                    settingY+43.0f+27.0f*static_cast<float>(j)+addedHeadings,
                    rect.width,frame.appearance.accent_rgb,selectedChoice,
                    multiCaption,colorOverride,textOverride,textFocused,hover);
            }
            if(count>5) {
                char pagination[70]{};
                const std::size_t shown=std::min<std::size_t>(
                    5u,count-item.settings_offset);
                const std::size_t page=item.settings_offset/5u+1u;
                const std::size_t pages=(count+4u)/5u;
                std::snprintf(pagination,sizeof(pagination),
                    "PAGE %zu / %zu    CONTROLS %zu - %zu / %zu",
                    page,pages,item.settings_offset+1u,
                    item.settings_offset+shown,count);
                rgb(0x999999u);
                text(pagination,left+8.0f,
                     settingY+47.0f+27.0f*static_cast<float>(shown)+
                     addedHeadings,
                     12.0f,rect.width-16.0f);
            }
        }
    }
    // R127: draw the source-shaped dropdown *after* cards so it floats
    // above later columns. This is a preview-only, immutable frame surface.
    if(frame.dropdown_preview) {
        ScopedVisualAlpha popupFade(frame.control_motion?
            frame.control_motion->popup_opacity:1.0f);
        const auto popup=menu_dropdown_popup_geometry(frame,w,h);
        if(!popup)return false;
        const float height=popup->row_height*popup->option_count;
        rgb(0x404040);
        round_rect(popup->x,popup->y,popup->width,height,10.0f);
        rgb(0x141414);
        round_rect(popup->x+2.0f,popup->y+2.0f,
                   popup->width-4.0f,height-4.0f,8.0f);
        for(unsigned i=0;i<popup->option_count;++i) {
            const float y=popup->y+popup->row_height*i;
            if(i>0u) {
                rgba(0x404040,128);
                quad(popup->x+2.0f,y,popup->width-4.0f,1.0f);
            }
            const unsigned absolute=popup->first_option+i;
            const auto label=menu_dropdown_option_label(
                frame.dropdown_preview->kind,absolute,
                frame.dropdown_preview->widget_index);
            const float measured=measure_menu_system_text(label,15.0f);
            const float fallback=static_cast<float>(label.size())*8.0f;
            const float left=popup->x+std::max(
                4.0f,(popup->width-(measured>0?measured:fallback))*0.5f);
            const bool selected=frame.dropdown_preview->kind==
                MenuDropdownKind::RecoveredMulti?
                (popup->selected_mask&(1u<<absolute))!=0u:
                absolute==popup->selected;
            rgb(selected?frame.appearance.accent_rgb:0x999999);
            text(label,left,
                 menu_control_center_text_y(y,popup->row_height,15.0f),
                 15.0f,popup->width-8.0f);
        }
        if(popup->total_options>popup->option_count) {
            // A small, source-neutral scrollbar for oversized recovered
            // lists; wheel moves this viewport, never the module page.
            const float track=height-12.0f;
            const float thumb=std::max(10.0f,track*
                static_cast<float>(popup->option_count)/
                static_cast<float>(popup->total_options));
            const unsigned maxOffset=
                popup->total_options-popup->option_count;
            const float ratio=maxOffset?
                static_cast<float>(popup->first_option)/maxOffset:0.0f;
            rgb(0x505050);
            round_rect(popup->x+popup->width-6.0f,
                       popup->y+6.0f,3.0f,track,1.5f);
            rgb(frame.appearance.accent_rgb);
            round_rect(popup->x+popup->width-6.0f,
                       popup->y+6.0f+(track-thumb)*ratio,
                       3.0f,thumb,1.5f);
        }
    }
    // R128: the source-backed seventh tab displays only an inert modal.
    // The commercial client's unload operation is deliberately NOT wired.
    // R147: display candidate-source numeric tooltip above the active
    // visible knob (range uses endpoint midpoint). The reference's global
    // tooltip is pointer-events:none and drawn above cards. No game state.
    if((frame.numeric_hover || frame.numeric_drag_preview) &&
       !frame.dropdown_preview) {
        const auto tip=menu_numeric_tooltip_geometry(frame,w,h);
        if(tip) {
            rgb(0x404040u);
            round_rect(tip->x,tip->y,tip->width,tip->height,5.0f);
            rgb(0xdddddd);
            text(tip->label,tip->x+5.0f,tip->y+2.0f,14.0f,
                 tip->width-10.0f);
        }
    }
    // R145: source tooltip: 20px, radius 5, #404040, 5px padding,
    // 14px text, 28px above the hovered profile icon. The same geometry
    // validates hover ownership for both enabled and protected Default icons.
    if(frame.profile_icon_hover && !frame.profile_action_preview) {
        const auto tip=menu_profile_tooltip_rect(frame,w,h);
        if(tip) {
            const auto hover=*frame.profile_icon_hover;
            const auto label=menu_profile_icon_tooltip(
                hover.profile_index,hover.action);
            rgb(0x404040);
            round_rect(tip->x,tip->y,tip->width,tip->height,5.0f);
            rgb(0xdddddd);
            text(label,tip->x+5.0f,tip->y+2.0f,14.0f,
                 tip->width-10.0f);
        }
    }
    if(pageY!=0.0f)glTranslatef(0.0f,-pageY,0.0f);
    } // page fade ends; confirmation dialog lives above the moving page
    if(frame.profile_action_preview) {
        const auto modal=menu_profile_modal_geometry(frame,w,h);
        if(!modal)return false;
        rgba(0x303030,190);
        quad(0.0f,0.0f,w,h);
        rgb(0x141414);
        round_rect(modal->x,modal->y,modal->width,modal->height,20.0f);
        rgb(0xdddddd);
        text("Are you sure?",modal->x+8.0f,modal->y+10.0f,
             16.0f,modal->width-50.0f);
        rgb(0x999999);
        const auto question=menu_profile_action_question(frame);
        text(question,modal->x+8.0f,modal->y+42.0f,
             15.0f,modal->width-16.0f);
        rgb(0x404040);
        round_rect(modal->continue_x,modal->continue_y,
                   modal->continue_width,20.0f,8.0f);
        rgb(0xdddddd);
        const std::string_view label="Continue";
        const float textWidth=measure_menu_system_text(label,15.0f);
        text(label,modal->continue_x+
             std::max(8.0f,(modal->continue_width-textWidth)*0.5f),
             modal->continue_y+3.0f,15.0f,
             modal->continue_width-16.0f);
        rgb(0x999999);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(modal->close_x+4.0f,modal->close_y+4.0f);
        glVertex2f(modal->close_x+16.0f,modal->close_y+16.0f);
        glVertex2f(modal->close_x+16.0f,modal->close_y+4.0f);
        glVertex2f(modal->close_x+4.0f,modal->close_y+16.0f);
        glEnd();
    }
    if(frame.unload_confirmation_preview) {
        const auto modal=menu_unload_modal_geometry(frame,w,h);
        if(!modal)return false;
        rgba(0x303030,190);
        quad(0.0f,0.0f,w,h);
        rgb(0x141414);
        round_rect(modal->x,modal->y,modal->width,modal->height,20.0f);
        rgb(0xdddddd);
        text("Are you sure?",modal->x+8.0f,modal->y+10.0f,
             16.0f,modal->width-50.0f);
        rgb(0x999999);
        text("Unload Slinky?",modal->x+8.0f,modal->y+42.0f,
             15.0f,modal->width-16.0f);
        rgb(0x404040);
        round_rect(modal->continue_x,modal->continue_y,
                   modal->continue_width,20.0f,8.0f);
        rgb(0xdddddd);
        const std::string_view action="Continue (preview only)";
        const float measured=measure_menu_system_text(action,15.0f);
        text(action,modal->continue_x+
            std::max(8.0f,(modal->continue_width-measured)*0.5f),
            modal->continue_y+3.0f,15.0f,
            modal->continue_width-16.0f);
        // The reference uses a 2px gray cross at the right of the title.
        rgb(0x999999);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(modal->close_x+4.0f,modal->close_y+4.0f);
        glVertex2f(modal->close_x+16.0f,modal->close_y+16.0f);
        glVertex2f(modal->close_x+16.0f,modal->close_y+4.0f);
        glVertex2f(modal->close_x+4.0f,modal->close_y+16.0f);
        glEnd();
    }
    // We intentionally avoid altering GL_VIEWPORT, FBO, shader, texture
    // selection or app-managed caches; state restoration is GL1.1 only.
    return true;
}
} // namespace slinkywinky
