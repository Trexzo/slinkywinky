#pragma once
#include "slinkywinky/combat.hpp"
#include "slinkywinky/movement.hpp"
#include <filesystem>
#include <string>

namespace slinkywinky {
struct ClientSettings {
    unsigned version = 1;
    CombatSettings combat;
    VelocitySettings velocity;
    SprintSettings sprint;
    bool backtrack_enabled = false;
    unsigned backtrack_ms = 100;
};

// Parser is strict: invalid versions, duplicates, ranges and non-finite numbers
// are errors, not partially applied settings. Unknown keys are ignored to
// support forward-compatible profiles.
struct SettingsResult {
    ClientSettings settings;
    bool valid = false;
    std::string error;
};
SettingsResult parse_settings(const std::string& document);
std::string serialize_settings(const ClientSettings& settings);
SettingsResult load_settings(const std::filesystem::path& path);
bool save_settings(const std::filesystem::path& path, const ClientSettings& settings,
                   std::string* error = nullptr);
} // namespace slinkywinky
