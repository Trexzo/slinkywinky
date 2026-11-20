// R253: Windows kernel process birth/handle + real Java8 Attach receipts
// + R252 source recovery + actual PR2 SprintObservationPreview.
// Disposable targets only. No Minecraft or gameplay input.
#ifdef _WIN32
// PR2's clutch planner has a local variable named 'far'; windows.h
// defines a legacy 'far' macro, so parse the actual PR2 headers first.
#include "slinkywinky/sprint_observation_preview.hpp"
#include "slinkywinky/windows_jvm_kernel_exit_witness.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace slinkywinky;
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    std::string("R253 line ")+std::to_string(__LINE__)+": " #x);}while(false)

static SprintObservedTick observation(std::uint64_t gen,std::uint64_t stamp){
    SprintObservedTick tick;
    tick.observation.generation=gen;
    tick.observation.time_ms=stamp;
    tick.observation.flags=3u;
    tick.observation.xyz={0,0,0,0,0,0};
    tick.observation.angles={0,0};
    tick.update.player_present=true;
    tick.update.player_sprinting=true;
    tick.movement.player_present=true;
    tick.movement.forward=-0.2f;
    tick.movement.strafe=0.1f;
    tick.movement.speed=0.1f;
    tick.movement.using_item=true;
    tick.jump_delay=JumpDelayInput{true,10};
    tick.no_slow=NoSlowInput{true,true,261,0.1f,-0.2f,{}};
    return tick;
}
static JvmLeaseIdentity require_receipt(const std::string& line){
    auto parsed=parse_r252_fixture_receipt(line);
    CHECK(parsed);return *parsed;
}
int main(int argc,char** argv){
    try{
        if(argc!=2)return 2;
        const auto oldId=require_receipt(argv[1]);
        auto oldPin=WindowsJvmKernelPin::open(oldId);
        CHECK(oldPin && oldPin->live() && oldPin->os_birth_filetime());
        // Tampering with an OS-visible birth or PID must fail, even if
        // the Java8 challenge string is otherwise well formed.
        auto forged=oldId;forged.jvm_start_ms+=60000ULL;
        CHECK(!WindowsJvmKernelPin::open(forged));
        forged=oldId;forged.pid=0;
        CHECK(!WindowsJvmKernelPin::open(forged));
        std::cout<<"R253_OLD_JVM_KERNEL_HANDLE_PINNED"<<std::endl;
        // The parent Python orchestrator now exits ONLY its owned
        // disposable old JVM and starts its owned replacement JVM.
        std::string newLine;
        CHECK(static_cast<bool>(std::getline(std::cin,newLine)));
        const auto newId=require_receipt(newLine);
        auto newPin=WindowsJvmKernelPin::open(newId);
        CHECK(newPin && newPin->live() && oldPin->exited());
        CHECK(oldId.pid!=newId.pid ||
              oldPin->os_birth_filetime()!=newPin->os_birth_filetime());
        CHECK(oldPin->matches(oldId) && newPin->matches(newId));

        SprintObservationPreview oldNative;
        NativeModuleSnapshotAdapter oldAdapter;
        ModuleArrayListFeed oldFeed;
        GuiModuleControlHandoff oldGui;
        CHECK(oldNative.observe(observation(70u,1000u)).admitted);
        CHECK(oldAdapter.publish_on_owner(oldNative,oldFeed,40u,true));
        CHECK(oldNative.request_module({"No Slow",true}));
        CHECK(oldNative.control_state("No Slow")->active_in_world);
        oldGui.retire();
        CHECK(oldAdapter.retire_owner(oldFeed));
        CHECK(oldNative.shutdown() && oldNative.stopped());

        SprintObservationPreview replacement;
        NativeModuleSnapshotAdapter newAdapter;
        ModuleArrayListFeed newFeed;
        CHECK(replacement.observe(observation(71u,1001u)).admitted);
        WindowsJvmKernelExitWitness witness(*oldPin,*newPin);
        JvmWitnessedNativeRecovery recovery;
        auto tampered=newId;tampered.pid=oldId.pid;
        CHECK(!recovery.admit_on_owner(witness,oldId,tampered,
              oldNative,oldAdapter,oldFeed,oldGui,
              replacement,newAdapter,newFeed,41u,true));
        CHECK(!newFeed.latest() && !recovery.poisoned());
        CHECK(recovery.admit_on_owner(witness,oldId,newId,
              oldNative,oldAdapter,oldFeed,oldGui,
              replacement,newAdapter,newFeed,41u,true));
        CHECK(recovery.used() && !recovery.poisoned());
        CHECK(newFeed.latest() && newFeed.latest()->world_generation==71u &&
              newFeed.latest()->source_sequence==41u);
        CHECK(!newFeed.latest()->modules[1].requested_enabled);
        CHECK(replacement.request_module({"No Slow",true}));
        CHECK(replacement.observe(observation(71u,1002u)).no_slow);
        CHECK(!recovery.admit_on_owner(witness,oldId,newId,
              oldNative,oldAdapter,oldFeed,oldGui,
              replacement,newAdapter,newFeed,42u,true));
        CHECK(replacement.shutdown());
        CHECK(newAdapter.retire_owner(newFeed));
        std::cout<<"R253_PINNED_OLD_EXIT_NEW_LIVE_REAL_PR2_RECOVERY_PASS"<<std::endl;
        // Parent exits its new JVM after acknowledgement. Its birth
        // is never transferable to a recycled PID.
        std::cout<<"R253_PROBE_FINISHED"<<std::endl;
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"R253_PROBE_FAILURE "<<e.what()<<std::endl;
        return 3;
    }
}
#endif
