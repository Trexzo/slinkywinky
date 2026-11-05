#pragma once
#include "slinkywinky/menu.hpp"
#include "slinkywinky/settings.hpp"
#include <cstddef>
#include <filesystem>
#include <string>

namespace slinkywinky {

// First-party configuration binding for modules already modeled by the C++
// engine. This is NOT a gameplay/injection bridge and does not send input.
bool module_has_profile_setting(std::size_t catalog_index) noexcept;

// Owner-thread menu controller: only selected, visible, implemented settings
// may change. Unsupported catalog entries remain off and cannot be enabled.
// The resulting ClientSettings are saved for later, separately admitted
// runtime integration; this controller does not activate any game module.
class MenuProfileBridge {
public:
    explicit MenuProfileBridge(MenuState& menu) noexcept;
    bool select(MenuTab tab) noexcept;
    bool toggle(std::size_t catalog_index) noexcept;
    bool enabled(std::size_t catalog_index) const noexcept;
    bool adopt(const ClientSettings& settings);
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path, std::string* error=nullptr) const;
    const ClientSettings& settings() const noexcept { return settings_; }
private:
    bool* flag(std::size_t catalog_index) noexcept;
    const bool* flag(std::size_t catalog_index) const noexcept;
    void synchronize_visible() noexcept;
    MenuState& menu_;
    ClientSettings settings_{};
};
} // namespace slinkywinky
