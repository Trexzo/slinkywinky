#pragma once
#ifdef _WIN32
#include "slinkywinky/hud_snapshot.hpp"
#include <cstdint>
#include <optional>

namespace slinkywinky {
// Opt-in, per-PID, local-user Windows file mapping. Only numeric, read-only
// observation data crosses the process boundary. No game pointers or commands.
class HudIpcPublisher {
public:
    HudIpcPublisher() noexcept = default;
    ~HudIpcPublisher() noexcept;
    HudIpcPublisher(const HudIpcPublisher&) = delete;
    HudIpcPublisher& operator=(const HudIpcPublisher&) = delete;
    bool open(std::uint32_t pid) noexcept;
    bool publish(const PublishedObservation& snapshot) noexcept;
    void revoke() noexcept;
private:
    void* mapping_ = nullptr;
    void* mutex_ = nullptr;
    void* memory_ = nullptr;
    std::uint32_t pid_ = 0;
};

class HudIpcReader {
public:
    HudIpcReader() noexcept = default;
    ~HudIpcReader() noexcept;
    HudIpcReader(const HudIpcReader&) = delete;
    HudIpcReader& operator=(const HudIpcReader&) = delete;
    bool connect(std::uint32_t pid) noexcept;
    std::optional<PublishedObservation> read() noexcept;
    void close() noexcept;
private:
    void* mapping_ = nullptr;
    void* mutex_ = nullptr;
    const void* memory_ = nullptr;
    std::uint32_t pid_ = 0;
};
} // namespace slinkywinky
#endif
