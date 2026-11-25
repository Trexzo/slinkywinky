#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace slinkywinky::prefs_file_io {

// A bounded read also checks the actual stream length, not a stale file_size.
inline bool read(const std::filesystem::path& path, std::size_t limit,
                 std::string& output) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::string contents;
    char buffer[256];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
        const auto count = static_cast<std::size_t>(input.gcount());
        if (count > limit - contents.size()) return false;
        contents.append(buffer, count);
    }
    if (input.bad() || !input.eof()) return false;
    output.swap(contents);
    return true;
}

class ScratchDirectory {
    std::filesystem::path path_;
public:
    ScratchDirectory() = default;
    ScratchDirectory(const ScratchDirectory&) = delete;
    ScratchDirectory& operator=(const ScratchDirectory&) = delete;
    ~ScratchDirectory() noexcept {
        if (path_.empty()) return;
        try {
            std::error_code ignored;
            // Remove only the file and directory we created, never recurse.
            std::filesystem::remove(path_ / "contents", ignored);
            std::filesystem::remove(path_, ignored);
        } catch (...) {}
    }
    bool create(const std::filesystem::path& parent) {
        static std::atomic<unsigned long long> sequence{0};
        std::random_device random;
        for (unsigned attempt = 0; attempt < 32; ++attempt) {
            auto candidate = parent / (".slinkywinky-prefs-" +
                std::to_string(random()) + "-" +
                std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                path_.swap(candidate);
                return true;
            }
            if (error && error != std::errc::file_exists) return false;
        }
        return false;
    }
    std::filesystem::path file() const { return path_ / "contents"; }
};

// Same-filesystem replacement. A failed write/close/rename leaves the
// existing destination in place. No crash-durability or hostile-directory
// guarantee; the destination directory must be trusted by the caller.
inline bool replace(const std::filesystem::path& path,
                    const std::string& contents) {
    if (path.empty() || contents.empty()) return false;
    const auto parent = path.parent_path().empty()
        ? std::filesystem::path(".") : path.parent_path();
    std::filesystem::create_directories(parent);
    ScratchDirectory scratch;
    if (!scratch.create(parent)) return false;
    const auto temp = scratch.file();
    {
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        if (!output) return false;
        output.close();
        if (!output) return false;
    }
#ifdef _WIN32
    return MoveFileExW(temp.c_str(), path.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::filesystem::rename(temp, path);
    return true;
#endif
}

} // namespace slinkywinky::prefs_file_io
