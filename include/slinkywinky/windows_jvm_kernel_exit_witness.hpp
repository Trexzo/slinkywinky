#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <processthreadsapi.h>
#include <securitybaseapi.h>
#include "slinkywinky/jvm_witnessed_native_recovery.hpp"
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace slinkywinky {

// R253: The process HANDLE pins a *kernel process object* through exit;
// unlike a bare PID it cannot silently become a recycled process.
class WindowsJvmKernelPin final {
public:
    WindowsJvmKernelPin(const WindowsJvmKernelPin&)=delete;
    WindowsJvmKernelPin& operator=(const WindowsJvmKernelPin&)=delete;
    WindowsJvmKernelPin(WindowsJvmKernelPin&& other) noexcept
        : handle_(other.handle_),identity_(std::move(other.identity_)),
          os_birth_(other.os_birth_) {other.handle_=nullptr;}
    WindowsJvmKernelPin& operator=(WindowsJvmKernelPin&&)=delete;
    ~WindowsJvmKernelPin(){if(handle_)CloseHandle(handle_);}

    static std::optional<WindowsJvmKernelPin> open(
            const JvmLeaseIdentity& identity) {
        if(!identity.well_formed() ||
           identity.pid>static_cast<std::uint64_t>(0xffffffffUL))
            return std::nullopt;
        HANDLE process=OpenProcess(
            SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,
            static_cast<DWORD>(identity.pid));
        if(!process)return std::nullopt;
        const auto refuse=[&]() -> std::optional<WindowsJvmKernelPin> {
            CloseHandle(process);
            return std::nullopt;
        };
        if(WaitForSingleObject(process,0)!=WAIT_TIMEOUT)return refuse();
        FILETIME birth{},exited{},kernel{},user{};
        if(!GetProcessTimes(process,&birth,&exited,&kernel,&user))
            return refuse();
        const std::uint64_t ticks=
            (static_cast<std::uint64_t>(birth.dwHighDateTime)<<32)|
            static_cast<std::uint64_t>(birth.dwLowDateTime);
        constexpr std::uint64_t unixOffset=116444736000000000ULL;
        if(ticks<=unixOffset)return refuse();
        const std::uint64_t birth_ms=(ticks-unixOffset)/10000ULL;
        // RuntimeMXBean startTime must follow OS creation reasonably
        // closely; tolerate OS/JVM clock rounding and slow class startup.
        const auto vm_start=identity.jvm_start_ms;
        const auto gap=birth_ms>vm_start?birth_ms-vm_start:vm_start-birth_ms;
        if(gap>30000ULL)return refuse();
        wchar_t path[32768]{};
        DWORD chars=static_cast<DWORD>(sizeof(path)/sizeof(path[0]));
        if(!QueryFullProcessImageNameW(process,0,path,&chars) || !chars)
            return refuse();
        const std::wstring image(path,chars);
        const auto slash=image.find_last_of(L"\\/");
        std::wstring leaf=image.substr(
            slash==std::wstring::npos?0:slash+1);
        std::transform(leaf.begin(),leaf.end(),leaf.begin(),
                       [](wchar_t c){return std::towlower(c);});
        if(leaf!=L"java.exe" && leaf!=L"javaw.exe")return refuse();
        if(!same_user(process))return refuse();
        return WindowsJvmKernelPin(process,identity,ticks);
    }

    bool matches(const JvmLeaseIdentity& identity) const noexcept {
        return handle_ && identity_==identity;
    }
    bool live() const noexcept {
        return handle_ && WaitForSingleObject(handle_,0)==WAIT_TIMEOUT;
    }
    bool exited() const noexcept {
        return handle_ && WaitForSingleObject(handle_,0)==WAIT_OBJECT_0;
    }
    std::uint64_t os_birth_filetime() const noexcept {return os_birth_;}
private:
    WindowsJvmKernelPin(HANDLE handle,JvmLeaseIdentity identity,
                        std::uint64_t os_birth)
        :handle_(handle),identity_(std::move(identity)),
         os_birth_(os_birth){}
    static bool same_user(HANDLE target) {
        HANDLE theirs=nullptr,ours=nullptr;
        bool allowed=false;
        if(!OpenProcessToken(target,TOKEN_QUERY,&theirs))return false;
        if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&ours)){
            DWORD a=0,b=0;
            GetTokenInformation(theirs,TokenUser,nullptr,0,&a);
            GetTokenInformation(ours,TokenUser,nullptr,0,&b);
            if(a && b && a<65536 && b<65536){
                std::vector<BYTE> one(a),two(b);
                if(GetTokenInformation(theirs,TokenUser,one.data(),a,&a) &&
                   GetTokenInformation(ours,TokenUser,two.data(),b,&b)){
                    const auto* x=reinterpret_cast<const TOKEN_USER*>(one.data());
                    const auto* y=reinterpret_cast<const TOKEN_USER*>(two.data());
                    allowed=EqualSid(x->User.Sid,y->User.Sid)!=FALSE;
                }
            }
            CloseHandle(ours);
        }
        CloseHandle(theirs);
        return allowed;
    }
    HANDLE handle_=nullptr;
    const JvmLeaseIdentity identity_;
    std::uint64_t os_birth_=0;
};

// R253 only supports old JVM EXIT and distinct new JVM incarnation.
// It deliberately refuses same-JVM epoch rotation: native owner quiescence
// is not enough to prove an active JVM lease was retired.
class WindowsJvmKernelExitWitness final {
public:
    WindowsJvmKernelExitWitness(const WindowsJvmKernelPin& previous,
                                const WindowsJvmKernelPin& replacement)
        :old_(previous),next_(replacement){}
    template<typename Callback>
    bool with_exclusive_lease(const JvmLeaseIdentity& old_id,
                              const JvmLeaseIdentity& next_id,
                              Callback callback){
        if(used_ || !old_.matches(old_id) || !next_.matches(next_id) ||
           old_id.pid==next_id.pid || !old_.exited() || !next_.live())
            return false;
        used_=true; // no second callback, even after callback refusal
        if(!callback())return false;
        // R252 quarantines a published state if this after-check fails.
        return old_.exited() && next_.live();
    }
private:
    const WindowsJvmKernelPin& old_;
    const WindowsJvmKernelPin& next_;
    bool used_=false;
};

} // namespace slinkywinky
#endif
