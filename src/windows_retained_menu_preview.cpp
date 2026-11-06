#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include <GL/gl.h>
#include <limits>

namespace slinkywinky {
WglRetainedMenuPreview::WglRetainedMenuPreview()
    : profile_(menu_),navigation_(menu_,profile_),
      owner_(std::this_thread::get_id()) {}
bool WglRetainedMenuPreview::owner_thread() const noexcept {
    return owner_==std::this_thread::get_id();
}
bool WglRetainedMenuPreview::draw(
    HGLRC context,HDC dc,unsigned width,unsigned height) {
    if(!owner_thread() || !context || !dc ||
       width<320 || width>8192 || height<240 || height>8192 ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       drawn_frames_==std::numeric_limits<std::uint64_t>::max())
        return false;
    if(active_ && (context_!=context || dc_!=dc ||
                   width_!=width || height_!=height))return false;
    // First observed frame pins a single original OpenGL render epoch.
    // A changed GL context/viewport requires an explicit new session.
    WglMenuOverlayHost host;
    if(!host.open(context,dc,width,height))return false;
    bool rendered=false;
    try {
        rendered=host.draw(menu_,&profile_,MenuGL11Mode::CompactOverlay,
                           &navigation_);
    } catch(...) {rendered=false;}
    const bool closed=host.close(); // same admitted graphics owner
    if(!rendered || !closed)return false;
    if(!active_) {
        context_=context;
        dc_=dc;
        width_=width;
        height_=height;
        active_=true;
    }
    drawn_frames_++;
    return true;
}
bool WglRetainedMenuPreview::apply(MenuNavAction action) {
    if(!active_ || !owner_thread() ||
       accepted_actions_==std::numeric_limits<std::uint64_t>::max() ||
       refused_actions_==std::numeric_limits<std::uint64_t>::max())
        return false;
    if(!navigation_.apply(action)) {
        refused_actions_++;
        return false;
    }
    accepted_actions_++;
    return true;
}
bool WglRetainedMenuPreview::close() noexcept {
    if(!owner_thread() || !active_)return false;
    // Every frame uses a stack-local WGL host that is already closed.
    // No WGL or Java operations occur during shutdown.
    context_=nullptr;
    dc_=nullptr;
    width_=height_=0;
    active_=false;
    return true;
}
std::optional<std::size_t> WglRetainedMenuPreview::focused_row() const {
    return owner_thread() && active_?navigation_.visible_focus():std::nullopt;
}
bool WglRetainedMenuPreview::preview_enabled(std::size_t index) const noexcept {
    return owner_thread() && active_ && profile_.enabled(index);
}
} // namespace slinkywinky
