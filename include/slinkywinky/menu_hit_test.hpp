#pragma once
#include "slinkywinky/menu_render.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

namespace slinkywinky {

// R97: render-neutral mirror of the three opt-in GL11 panel modes.
// Not an OS input handler. The native game JNI does NOT call this interface.
enum class MenuPanelMode : std::uint8_t {
    StandalonePreview, BoundedOverlay, CompactOverlay
};

struct MenuPanelGeometry {
    unsigned left=0, top=0, width=0, height=0;
    float scale=1.0f;
    float logical_width() const noexcept {
        return static_cast<float>(width)/scale;
    }
    float logical_height() const noexcept {
        return static_cast<float>(height)/scale;
    }
};

// Exactly the drawing panel used by the first-party GL1.1 renderer.
// Refuse invalid dimensions and unknown modes before any arithmetic/GL calls.
std::optional<MenuPanelGeometry> menu_panel_geometry(
    unsigned viewport_width,unsigned viewport_height,bool wide,
    MenuPanelMode mode) noexcept;

enum class MenuHitKind : std::uint8_t {
    Tab, RowBody, Expand, Toggle, PiercingControl,
    AimModeControl, TargetModeControl, // R126: preview-only dropdowns
    DropdownOption, // R127: selected option in verified popup
    UnloadTab, ModalClose, ModalContinue, // R128: inert modal preview
    RecoveredBooleanPreview, // R130: catalog-indexed inert Boolean switch
    RecoveredChoiceControl, // R131: source-global dropdown widget
    RecoveredMultiControl, // R134: source-global multiselect widget
    RecoveredColorSwatch, // R136: independent 24-bit RGB preview
    ColorPopupHsl, ColorPopupRecent, ColorPopupClose,
    ColorPopupHexField,
    // R200: modal-only HSL bar, recent swatch and commit/cancel
    RecoveredTextField, // R137: local source-indexed UTF-8 edit field
    BindChip, BindHold, // R140: preview-only module key and HOLD widgets
    ProfileNameField, ProfileCreate, ProfileAction, // R142-R143
    ProfileBindChip, // R144: local profile key capture
    ProfileModalClose, ProfileModalContinue, // local confirmation only
    RecoveredSliderPreview, // R132: source-indexed inert drag-on-release
    RecoveredRangePreview // R133: dual-handle guarded preview
};

// Catalog index is meaningful only for non-Tab hits. No raw input pointers,
// Windows hooks, game keybindings or client actions are retained.
struct MenuHit {
    MenuHitKind kind=MenuHitKind::RowBody;
    MenuTab tab=MenuTab::Combat;
    std::size_t catalog_index=0;
    std::size_t control_index=0; // R125 visible Piercing callsite, 0..7
    unsigned sub_index=0; // R133: held numeric range handle, 0 or 1
};

// Client coordinates are top-left physical pixels, precisely like
// WM_LBUTTONDOWN. Only a valid, fully contained visible draw target is
// hittable. Unsupported rows may be focused, but never enabled by the hit.
std::optional<MenuHit> hit_test_menu(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int client_x,int client_y) noexcept;

// R118: identifies only a *fully visible* expanded settings pane that has
// multiple read-only preview pages. Does NOT perform scrolling, mouse capture,
// gameplay actions or Win32 input processing. Same source geometry as WGL.
std::optional<std::size_t> settings_page_wheel_target(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int client_x,int client_y) noexcept;

} // namespace slinkywinky
