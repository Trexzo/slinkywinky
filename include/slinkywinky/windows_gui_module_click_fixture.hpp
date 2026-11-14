#pragma once
#ifdef _WIN32
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/gui_module_control_handoff.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"

namespace slinkywinky {
// R211: separately owned, synthetic-only menu-exclusive pointer transaction.
// This is not a Win32/LWJGL hook and cannot grant live Minecraft ownership.
class GuiModuleClickTransaction final {
public:
    GuiModuleClickTransaction() noexcept;
    GuiModuleClickTransaction(const GuiModuleClickTransaction&)=delete;
    GuiModuleClickTransaction& operator=(const GuiModuleClickTransaction&)=delete;
    bool fixture_event(RetainedPointerPhase phase,
        WglRetainedMenuPreview& menu,MenuInputHandoffLedger& ledger,
        MenuKeyboardRouter& router,GuiModuleControlHandoff& handoff,
        const ModuleArrayListFeed& source,HGLRC context,HDC dc,
        unsigned width,unsigned height,std::uint64_t epoch,
        int x,int y,std::uint64_t monotonic_ms,bool foreground_verified);
    void revoke() noexcept;
    bool pending() const noexcept;
private:
    bool owner() const noexcept;
    MenuMouseGesture gesture_;
    std::thread::id owner_{};
    bool pending_=false;
    std::uint64_t epoch_=0;
    std::uint64_t painted_frame_=0;
    std::uint64_t source_sequence_=0;
    std::uint64_t source_generation_=0;
    std::uint64_t down_ms_=0;
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
};
} // namespace slinkywinky
#endif
