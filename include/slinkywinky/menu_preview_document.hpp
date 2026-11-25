#pragma once
#include "slinkywinky/menu.hpp"
#include <variant>

namespace slinkywinky {
// Typed sparse overrides for the recovered presentation catalog. The
// document has no runtime module flags, active key bindings or ClientSettings.
struct PreviewChoiceValue { std::uint32_t ordinal=0; };
struct PreviewColorValue { std::uint32_t rgb=0; };
struct PreviewMultiValue { std::uint32_t mask=0; };
using PreviewWidgetValue=std::variant<bool,float,PreviewChoiceValue,
    RecoveredRangePreview,PreviewColorValue,PreviewMultiValue,std::string>;
struct PreviewWidgetRecord {
    std::size_t widget_index=0;
    PreviewWidgetValue value=false;
};
struct MenuPreviewDocument {
    bool compact=false,wide=false;
    std::uint32_t accent_rgb=0xF35D12u;
    float scale=1.0f;
    unsigned aim_mode=0,target_mode=0;
    PiercingPreview piercing{};
    std::vector<MenuLocalProfile> profiles{
        {"Default","Contains the default configuration.",""}
    };
    std::vector<PreviewWidgetRecord> values;
};
constexpr std::size_t menu_preview_document_limit=65536u;
std::uint64_t menu_preview_catalog_fingerprint();
bool menu_preview_document_valid(const MenuPreviewDocument& doc) noexcept;
MenuPreviewDocument capture_local_preview(const MenuState& menu);
std::optional<std::string> encode_menu_preview_document(const MenuPreviewDocument& doc);
std::optional<MenuPreviewDocument> decode_menu_preview_document(std::string_view data);
bool save_menu_preview_document(const std::filesystem::path& path,
                                const MenuPreviewDocument& doc) noexcept;
std::optional<MenuPreviewDocument> load_menu_preview_document(
    const std::filesystem::path& path) noexcept;
} // namespace slinkywinky
