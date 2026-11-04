#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_wgl_frame_probe.hpp"
#include <GL/gl.h>

namespace slinkywinky {
WglFrameContextProbe::WglFrameContextProbe(unsigned limit) noexcept:limit_(limit){}

bool WglFrameContextProbe::current_locked() const noexcept {
    // Read-only WGL checks, no context activation, handoff, or MakeCurrent.
    return std::this_thread::get_id()==owner_ &&
           context_!=nullptr && dc_!=nullptr &&
           wglGetCurrentContext()==context_ &&
           wglGetCurrentDC()==dc_;
}
bool WglFrameContextProbe::viewport_locked() const noexcept {
    if(!current_locked())return false;
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    return viewport[0]==0 && viewport[1]==0 &&
           viewport[2]==viewport_width_ &&
           viewport[3]==viewport_height_;
}
bool WglFrameContextProbe::open() noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    if(active_ || invalid_ || accepted_!=0 || limit_==0 || limit_>8)
        return false;
    HGLRC context=wglGetCurrentContext();
    HDC dc=wglGetCurrentDC();
    if(context==nullptr || dc==nullptr)return false;
    // No GL call until a current context and DC are confirmed.
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]<320 || viewport[2]>8192 ||
       viewport[3]<240 || viewport[3]>8192)
        return false;
    context_=context;
    dc_=dc;
    viewport_width_=viewport[2];
    viewport_height_=viewport[3];
    owner_=std::this_thread::get_id();
    active_=true;
    return true;
}
WglFrameContextProbe::Result WglFrameContextProbe::sample() noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    if(!active_ || invalid_)return Result::Refused;
    // A wrong thread/current context/viewport permanently invalidates this
    // epoch; it cannot be recovered by making a context current again.
    if(!viewport_locked()){
        invalid_=true;
        active_=false;
        return Result::Refused;
    }
    ++accepted_;
    if(accepted_>=limit_){
        active_=false;
        return Result::LimitReached;
    }
    return Result::Accepted;
}
bool WglFrameContextProbe::close() noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    // The owner may close after context loss, without making any GL calls.
    if(owner_==std::thread::id{} ||
       owner_!=std::this_thread::get_id())return false;
    active_=false;
    return !invalid_;
}
unsigned WglFrameContextProbe::accepted() const noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    return accepted_;
}
bool WglFrameContextProbe::invalid() const noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    return invalid_;
}
bool WglFrameContextProbe::active() const noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    return active_;
}
} // namespace slinkywinky
#endif
