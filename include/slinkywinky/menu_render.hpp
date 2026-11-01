#pragma once
#include "slinkywinky/menu.hpp"
#include <cstdint>
#include <optional>
#include <mutex>
#include <thread>
#include <vector>

namespace slinkywinky {
// R63: a first-party, renderer-neutral admission boundary.
// This never calls OpenGL, Minecraft, JNI, a loader, or gameplay input.
struct MenuRenderFrame {
    std::uint64_t epoch=0;
    std::uint64_t sequence=0;
    unsigned viewport_width=0;
    unsigned viewport_height=0;
    MenuTab tab=MenuTab::Combat;
    MenuAppearance appearance{};
    std::vector<MenuItem> visible;
    bool is_placeholder=false; // Profiles is informational; Unload not admitted.
};

class MenuRenderSession {
public:
    // Must be invoked on the actual graphics-context owner; epoch is unique
    // to the lifetime of that context, not a Minecraft game session.
    bool attach(std::uint64_t epoch,unsigned width,unsigned height) noexcept;
    bool resize(unsigned width,unsigned height) noexcept;
    std::optional<MenuRenderFrame> next(const MenuState& menu);
    // Cleanup is intentionally owner-thread only; a failed close leaves this
    // context locked until the host can perform the correct teardown.
    bool detach() noexcept;
    bool attached() const noexcept;
    std::uint64_t sequence() const noexcept;
private:
    static bool valid_viewport(unsigned width,unsigned height) noexcept;
    bool owner() const noexcept;
    mutable std::mutex mutex_;
    std::thread::id thread_{};
    std::uint64_t last_epoch_=0;
    bool active_=false;
    std::uint64_t epoch_=0;
    std::uint64_t sequence_=0;
    unsigned width_=0, height_=0;
};
} // namespace slinkywinky
