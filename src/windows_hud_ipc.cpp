#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "slinkywinky/windows_hud_ipc.hpp"
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <string>

namespace slinkywinky {
namespace {
constexpr std::uint32_t magic=0x57534855u; // "WSHU"
constexpr std::uint32_t version=1;
constexpr ULONGLONG expiry_ms=1500;
struct HudWire {
    std::uint32_t tag, revision, pid, active;
    std::uint64_t beat_ms;
    double fields[10];
};
static_assert(sizeof(HudWire)==104, "Wire format must remain exact x64 ABI");
std::wstring name(const wchar_t* suffix,std::uint32_t pid) {
    return std::wstring(L"Local\\SlinkyWinky-HUD-v1-")+suffix+
           L"-"+std::to_wstring(pid);
}
bool owned_lock(HANDLE h) {
    return WaitForSingleObject(h,0)==WAIT_OBJECT_0;
}
bool plausible(const PublishedObservation& p) {
    const auto& s=p.status;
    return p.sequence!=0 && s.valid_ticks!=0 && s.generation!=0 &&
        (s.flags&~7u)==0 && !(s.flags&2u && !(s.flags&1u)) &&
        !(s.flags&4u && (s.flags&3u)!=3u) &&
        std::isfinite(s.eye_y) && std::isfinite(s.yaw) &&
        std::isfinite(s.pitch) && std::isfinite(s.target_distance) &&
        s.target_distance>=0;
}
HudWire encode(const PublishedObservation& p,std::uint32_t pid) {
    const auto& s=p.status;
    return {magic,version,pid,1,GetTickCount64(),{
        static_cast<double>(s.valid_ticks),
        static_cast<double>(s.session_transitions),
        static_cast<double>(s.generation),
        static_cast<double>(s.flags),
        s.target_distance,s.yaw,s.pitch,
        static_cast<double>(s.time_ms),s.eye_y,
        static_cast<double>(p.sequence)}};
}
std::optional<PublishedObservation> decode(const HudWire& wire,std::uint32_t pid) {
    if(wire.tag!=magic || wire.revision!=version || wire.pid!=pid ||
       wire.active!=1 || wire.beat_ms==0) return std::nullopt;
    const auto now=GetTickCount64();
    if(now<wire.beat_ms || now-wire.beat_ms>expiry_ms) return std::nullopt;
    for(double v:wire.fields) if(!std::isfinite(v)) return std::nullopt;
    // Integer-valued counters and flags must be exact and bounded.
    for(int index:std::array<int,6>{0,1,2,3,7,9}) {
        double value=wire.fields[index];
        if(value<0 || value>9007199254740991.0 ||
           std::floor(value)!=value) return std::nullopt;
    }
    if(wire.fields[3]>7 || wire.fields[0]==0 ||
       wire.fields[2]==0 || wire.fields[9]==0 ||
       std::abs(wire.fields[5])>static_cast<double>(FLT_MAX) ||
       std::abs(wire.fields[6])>static_cast<double>(FLT_MAX))
        return std::nullopt;
    ObservationStatus s{};
    s.valid_ticks=static_cast<std::uint64_t>(wire.fields[0]);
    s.session_transitions=static_cast<std::uint64_t>(wire.fields[1]);
    s.generation=static_cast<std::uint64_t>(wire.fields[2]);
    s.flags=static_cast<std::uint32_t>(wire.fields[3]);
    s.target_distance=wire.fields[4];
    s.yaw=static_cast<float>(wire.fields[5]);
    s.pitch=static_cast<float>(wire.fields[6]);
    s.time_ms=static_cast<std::uint64_t>(wire.fields[7]);
    s.eye_y=wire.fields[8];
    PublishedObservation snapshot{s,static_cast<std::uint64_t>(wire.fields[9])};
    return plausible(snapshot) ? std::optional<PublishedObservation>(snapshot) :
                                 std::nullopt;
}
}
HudIpcPublisher::~HudIpcPublisher() noexcept {revoke();}
bool HudIpcPublisher::open(std::uint32_t pid) noexcept {
    if(mapping_||mutex_||memory_||pid==0 || pid!=GetCurrentProcessId())
        return false;
    const auto mutexName=name(L"Lock",pid);
    HANDLE lock=CreateMutexW(nullptr,FALSE,mutexName.c_str());
    if(!lock) return false;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(lock);return false;}
    const auto mappingName=name(L"Data",pid);
    HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,
                                 0,static_cast<DWORD>(sizeof(HudWire)),
                                 mappingName.c_str());
    if(!map) {CloseHandle(lock);return false;}
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
        CloseHandle(map);CloseHandle(lock);return false;
    }
    void* view=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(HudWire));
    if(!view) {CloseHandle(map);CloseHandle(lock);return false;}
    HudWire empty{magic,version,pid,0,0,{}};
    std::memcpy(view,&empty,sizeof(empty));
    pid_=pid;mutex_=lock;mapping_=map;memory_=view;
    return true;
}
bool HudIpcPublisher::publish(const PublishedObservation& snapshot) noexcept {
    if(!memory_ || !plausible(snapshot))return false;
    HANDLE lock=static_cast<HANDLE>(mutex_);
    if(!owned_lock(lock))return false; // Do not block the client tick.
    const auto wire=encode(snapshot,pid_);
    std::memcpy(memory_,&wire,sizeof(wire));
    ReleaseMutex(lock);
    return true;
}
void HudIpcPublisher::revoke() noexcept {
    if(memory_) {
        HANDLE lock=static_cast<HANDLE>(mutex_);
        if(owned_lock(lock)) {
            auto* wire=static_cast<HudWire*>(memory_);
            wire->active=0;wire->beat_ms=0;
            ReleaseMutex(lock);
        }
        UnmapViewOfFile(memory_);memory_=nullptr;
    }
    if(mapping_){CloseHandle(static_cast<HANDLE>(mapping_));mapping_=nullptr;}
    if(mutex_){CloseHandle(static_cast<HANDLE>(mutex_));mutex_=nullptr;}
    pid_=0;
}
HudIpcReader::~HudIpcReader() noexcept {close();}
bool HudIpcReader::connect(std::uint32_t pid) noexcept {
    if(mapping_ || mutex_ || memory_ || pid==0)return false;
    HANDLE lock=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,
                            name(L"Lock",pid).c_str());
    if(!lock)return false;
    HANDLE map=OpenFileMappingW(FILE_MAP_READ,FALSE,name(L"Data",pid).c_str());
    if(!map){CloseHandle(lock);return false;}
    const void* view=MapViewOfFile(map,FILE_MAP_READ,0,0,sizeof(HudWire));
    if(!view){CloseHandle(map);CloseHandle(lock);return false;}
    pid_=pid;mutex_=lock;mapping_=map;memory_=view;
    return true;
}
std::optional<PublishedObservation> HudIpcReader::read() noexcept {
    if(!memory_)return std::nullopt;
    HANDLE lock=static_cast<HANDLE>(mutex_);
    if(!owned_lock(lock))return std::nullopt;
    HudWire copy{};
    std::memcpy(&copy,memory_,sizeof(copy));
    ReleaseMutex(lock);
    return decode(copy,pid_);
}
void HudIpcReader::close() noexcept {
    if(memory_){UnmapViewOfFile(memory_);memory_=nullptr;}
    if(mapping_){CloseHandle(static_cast<HANDLE>(mapping_));mapping_=nullptr;}
    if(mutex_){CloseHandle(static_cast<HANDLE>(mutex_));mutex_=nullptr;}
    pid_=0;
}
} // namespace slinkywinky
#endif
