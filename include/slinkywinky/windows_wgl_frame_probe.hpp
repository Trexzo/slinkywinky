#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <mutex>
#include <thread>

namespace slinkywinky {
/** R76: passive WGL current-context observer (no rendering or state changes).
 * All operations are thread-safe; the owning thread must call close().
 */
class WglFrameContextProbe final {
public:
    enum class Result : int { Refused=0, Accepted=1, LimitReached=2 };
    explicit WglFrameContextProbe(unsigned limit=8) noexcept;
    bool open() noexcept;
    Result sample() noexcept;
    bool close() noexcept;
    unsigned accepted() const noexcept;
    bool invalid() const noexcept;
    bool active() const noexcept;
    WglFrameContextProbe(const WglFrameContextProbe&)=delete;
    WglFrameContextProbe& operator=(const WglFrameContextProbe&)=delete;
private:
    bool current_locked() const noexcept;
    bool viewport_locked() const noexcept;
    mutable std::mutex lock_;
    const unsigned limit_;
    unsigned accepted_=0;
    bool active_=false;
    bool invalid_=false;
    std::thread::id owner_{};
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
    int viewport_width_=0;
    int viewport_height_=0;
};
} // namespace slinkywinky
#endif
