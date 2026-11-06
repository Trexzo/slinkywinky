#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <GL/gl.h>
#include <limits>

namespace slinkywinky {
bool WglMenuOverlayHost::current() const noexcept {
    return session_.attached() && owner_==std::this_thread::get_id() &&
           context_!=nullptr && dc_!=nullptr &&
           wglGetCurrentContext()==context_ && wglGetCurrentDC()==dc_;
}
bool WglMenuOverlayHost::open(HGLRC ctx,HDC dc,unsigned w,unsigned h) noexcept {
    if(session_.attached() || ctx==nullptr || dc==nullptr ||
       wglGetCurrentContext()!=ctx || wglGetCurrentDC()!=dc ||
       last_epoch_==std::numeric_limits<std::uint64_t>::max())
        return false;
    // Do not mutate any GL state or increment the epoch on a failed admit.
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(w) ||
       viewport[3]!=static_cast<GLint>(h))
        return false;
    const auto next=last_epoch_+1;
    if(!session_.attach(next,w,h))return false;
    context_=ctx;
    dc_=dc;
    owner_=std::this_thread::get_id();
    width_=w;
    height_=h;
    last_epoch_=next;
    return true;
}
bool WglMenuOverlayHost::resize(unsigned w,unsigned h) noexcept {
    if(!current())return false;
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(w) ||
       viewport[3]!=static_cast<GLint>(h))
        return false;
    if(!session_.resize(w,h))return false;
    width_=w;
    height_=h;
    return true;
}
bool WglMenuOverlayHost::draw(const MenuState& menu,
                              const MenuProfileBridge* profile) {
    return draw(menu,profile,MenuGL11Mode::BoundedOverlay);
}
bool WglMenuOverlayHost::draw(const MenuState& menu,
                              const MenuProfileBridge* profile,MenuGL11Mode mode) {
    return draw(menu,profile,mode,nullptr);
}
bool WglMenuOverlayHost::draw(const MenuState& menu,
                              const MenuProfileBridge* profile,MenuGL11Mode mode,
                              const MenuNavigation* navigation) {
    if(!current())return false;
    // R70: refuse desynchronized host viewports BEFORE taking a numbered
    // frame snapshot. Context/size mismatches must not consume sequences.
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=static_cast<GLint>(width_) ||
       viewport[3]!=static_cast<GLint>(height_))
        return false;
    const auto frame=session_.next(menu,profile,navigation);
    if(!frame)return false;
    return draw_menu_gl11(*frame,context_,dc_,mode);
}
bool WglMenuOverlayHost::close() noexcept {
    // No WGL or OpenGL calls here: allows shutdown if the context was
    // revoked by its owner. Only the admitted render thread may close.
    if(!session_.attached() || owner_!=std::this_thread::get_id())
        return false;
    if(!session_.detach())return false;
    context_=nullptr;
    dc_=nullptr;
    width_=height_=0;
    owner_=std::thread::id{};
    return true;
}
} // namespace slinkywinky
