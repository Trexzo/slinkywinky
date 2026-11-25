#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace slinkywinky::reference_compat {

// R310: independently implemented, offline-only notification queue.
// Source settings must be supplied explicitly; no factory-default claim.
struct NotificationSettings {
    bool hide_ingame;
    bool hide_hold_notifs;
    bool hide_bind_only;
    bool show_combat;
    bool show_move;
    bool show_block;
    bool show_visual;
    bool show_utility;
};

enum class NotificationKind { profile, info, error };

struct NotificationMessage {
    NotificationKind kind;
    std::string text;
    float age_seconds;
};

class NotificationsController {
    struct Batch {
        std::string last;
        std::uint32_t count = 0;
    };
    std::deque<NotificationMessage> messages_;
    Batch enabled_, disabled_;
    bool hint_seen_ = false;
    static constexpr std::size_t capacity_ = 10;

    static std::string bounded(std::string text) {
        // Source formatting buffer capacity is 0x65 including terminator.
        const auto nul = text.find('\0');
        if (nul != std::string::npos) text.resize(nul);
        if (text.size() > 100) text.resize(100);
        return text;
    }

    void post(NotificationKind kind, std::string text) {
        messages_.push_back({kind, bounded(std::move(text)), 0.0f});
        if (messages_.size() > capacity_) messages_.pop_front();
    }

    static bool visible(const NotificationSettings& s, unsigned category,
                        unsigned flags, bool hidden) noexcept {
        if ((flags & 0x10u) != 0) return false;
        if ((flags & 2u) != 0 && s.hide_hold_notifs) return false;
        if (hidden && s.hide_bind_only) return false;
        switch (category) {
        case 0: return s.show_combat;
        case 1: return s.show_move;
        case 2: return s.show_visual;
        case 3: return s.show_block;
        case 4: return s.show_utility;
        default: return true; // recovered fallback
        }
    }

    void flush(Batch& batch, const char* verb) {
        if (batch.count == 0) return;
        post(NotificationKind::info, std::string(verb) + " " +
             (batch.count == 1 ? batch.last :
              std::to_string(batch.count) + " mods"));
        batch = Batch{};
    }

public:
    // Missing name is the source Default label. Unknown statuses rejected,
    // rather than using the candidate's unchecked two-entry string table.
    void profile(unsigned status, const std::string* name) {
        if (status == 2) return;
        if (status > 2) throw std::invalid_argument("notification profile status");
        post(NotificationKind::profile,
             std::string(status == 0 ? "Loaded profile " : "Saved profile ") +
             (name ? bounded(*name) : "Default"));
    }

    void error(int code, const std::string* message) {
        post(NotificationKind::error, message ? bounded(*message) :
             "Error code " + std::to_string(code));
    }

    void friend_changed(bool added) {
        post(NotificationKind::info, added ? "Friend added" : "Friend removed");
    }

    // The C++ candidate uses a special descriptor pointer comparison. Model
    // that distinction explicitly; do not infer identity from a display name.
    void special_descriptor_toggle(bool on, const std::string& key_label) {
        if (!on) {
            messages_.clear();
            return;
        }
        if (!hint_seen_) {
            post(NotificationKind::info, "Press " + bounded(key_label) +
                 " to open GUI");
            hint_seen_ = true;
        }
    }

    void module_toggle(const NotificationSettings& settings, std::string name,
                       unsigned category, unsigned flags, bool hidden, bool on) {
        if (!visible(settings, category, flags, hidden)) return;
        auto& batch = on ? enabled_ : disabled_;
        if (batch.count == std::numeric_limits<std::uint32_t>::max())
            throw std::overflow_error("notification batch overflow");
        batch.last = bounded(std::move(name));
        ++batch.count;
    }

    // Explicit owner lifecycle boundary for the standalone diagnostic model.
    void reset_session() noexcept {
        messages_.clear();
        enabled_ = Batch{};
        disabled_ = Batch{};
        hint_seen_ = false;
    }

    // Source ages messages AFTER expiring old ones and posting batches.
    // bit 0: redraw; bit 1: transition animation. Preserve incoming flags.
    unsigned tick(const NotificationSettings& settings, bool gui_open,
                  float delta_seconds, unsigned incoming_flags = 0) {
        if (!std::isfinite(delta_seconds) || delta_seconds < 0.0f)
            throw std::invalid_argument("invalid notification frame delta");
        if (settings.hide_ingame && !gui_open) {
            messages_.clear();
            enabled_ = Batch{};
            disabled_ = Batch{};
            return incoming_flags;
        }
        while (!messages_.empty() && messages_.front().age_seconds > 5.0f)
            messages_.pop_front();
        flush(enabled_, "Enabled");
        flush(disabled_, "Disabled");
        if (messages_.empty()) return incoming_flags;
        bool animating = false;
        for (auto& message : messages_) {
            // Bound finite overflow; an already expired item is removed next tick.
            const double age = static_cast<double>(message.age_seconds) +
                               static_cast<double>(delta_seconds);
            message.age_seconds = static_cast<float>(
                age > std::numeric_limits<float>::max() ?
                std::numeric_limits<float>::max() : age);
            const double actual_age = message.age_seconds;
            animating |= actual_age < 0.2 || actual_age > 4.8;
        }
        return incoming_flags | (animating ? 3u : 1u);
    }

    std::vector<NotificationMessage> snapshot() const {
        return {messages_.begin(), messages_.end()};
    }
};

} // namespace slinkywinky::reference_compat
