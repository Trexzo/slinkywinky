#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "slinkywinky/windows_hud_ipc.hpp"
#include <stdexcept>
#include <cwchar>
#include <string>
#define CHECK(c) do { if(!(c)) throw std::runtime_error("IPC test failed: " #c); } while(false)
using namespace slinkywinky;

int reader_child(DWORD pid,bool javaOrigin) {
    HudIpcReader reader;
    if(!reader.connect(pid))return 2;
    auto status=reader.read();
    if(!status)return 3;
    if(javaOrigin) {
        // Java8 smoke publisher may have refreshed its safe observation
        // while this process was being scheduled. Accept only the exact
        // bounded first-party sequence/time relationship, never stale data.
        const auto seq=status->sequence;
        if(seq<1 || seq>161 || status->status.valid_ticks!=seq ||
           status->status.generation!=1 ||
           status->status.time_ms!=1000u+50u*(seq-1u))
            return 3;
        return status->status.flags==0 &&
               status->status.target_distance==0 ? 0 : 4;
    }
    // Original standalone fixture remains pinned to one exact observation.
    if(status->sequence!=1 || status->status.valid_ticks!=1)return 3;
    return status->status.target_distance==3.25 ? 0 : 5;
}
int wmain(int argc,wchar_t** argv) {
    if(argc==3 && (std::wstring(argv[1])==L"--reader" ||
                   std::wstring(argv[1])==L"--java-reader"))
        return reader_child(static_cast<DWORD>(std::wcstoul(argv[2],nullptr,10)),
                            std::wstring(argv[1])==L"--java-reader");
    CHECK(argc==1);
    const auto pid=GetCurrentProcessId();
    HudIpcReader reader;
    CHECK(!reader.connect(pid)); // Not yet opted in.
    HudIpcPublisher publisher;
    CHECK(!publisher.open(pid+1)); // Wrong process ID cannot publish.
    CHECK(publisher.open(pid));
    HudIpcPublisher competitor;
    CHECK(!competitor.open(pid));
    CHECK(reader.connect(pid));
    CHECK(!reader.read()); // No initial status.
    ObservationStatus status{};
    status.valid_ticks=1;
    status.generation=1;
    status.time_ms=1000;
    status.flags=7;
    status.eye_y=65.62;
    status.yaw=42;
    status.pitch=-8;
    status.target_distance=3.25;
    CHECK(publisher.publish({status,1}));
    auto view=reader.read();
    CHECK(view && view->sequence==1 &&
          view->status.target_distance==3.25);

    wchar_t executable[MAX_PATH]{};
    CHECK(GetModuleFileNameW(nullptr,executable,MAX_PATH)>0);
    std::wstring command=L"\""+std::wstring(executable)+
                         L"\" --reader "+std::to_wstring(pid);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);
    PROCESS_INFORMATION child{};
    CHECK(CreateProcessW(nullptr,&command[0],nullptr,nullptr,FALSE,
                          CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child));
    CHECK(WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0);
    DWORD exitCode=999;
    CHECK(GetExitCodeProcess(child.hProcess,&exitCode));
    CloseHandle(child.hThread);CloseHandle(child.hProcess);
    CHECK(exitCode==0); // Real cross-process read validated.

    status.valid_ticks=2;
    status.time_ms=1050;
    status.flags=3;
    status.target_distance=0;
    CHECK(publisher.publish({status,2}));
    view=reader.read();
    CHECK(view && view->sequence==2 &&
          view->status.flags==3 && view->status.target_distance==0);
    publisher.revoke();
    CHECK(!reader.read());
    CHECK(!publisher.publish({status,3}));
    reader.close();
    return 0;
}
