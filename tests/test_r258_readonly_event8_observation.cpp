#include "slinkywinky/r258_readonly_event8_observation.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <string>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x)) throw std::runtime_error( \
    "R258 native line "+std::to_string(__LINE__)+": " #x);} while(false)

int main(int argc,char**argv){
    const char* fixture[]={
        "R258_EVENT8_OBS 1 2 7 100",
        "R258_EVENT8_OBS 2 2 3 101",
        "R258_EVENT8_OBS 3 3 0 102",
        "R258_EVENT8_OBS 4 4 3 103"
    };
    R258ReadOnlyEvent8Gate guard;
    for(int i=0;i<4;++i){
        std::string record=argc==5?argv[i+1]:fixture[i];
        const auto parsed=parse_r258_event8_observation(record);
        CHECK(parsed && parsed->sequence==static_cast<std::uint64_t>(i+1));
        const unsigned expectedFlags[]={7,3,0,3};
        CHECK(parsed->flags==expectedFlags[i]);
        CHECK(!parsed->complete_gameplay_evidence());
        CHECK(guard.accept(record));
        CHECK(guard.accepted()==static_cast<std::uint64_t>(i+1));
        CHECK(!guard.may_evaluate_reach() &&
              !guard.may_evaluate_hitboxes() &&
              !guard.may_evaluate_ghost_hand());
        CHECK(!guard.accept(record)); // replay sequence
    }
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS 5 5 2 104"));
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS 5 5 4 104"));
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS 5 5 8 104"));
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS -5 5 3 104"));
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS 5 5 3 104 trailing"));
    CHECK(!parse_r258_event8_observation(
        "R258_EVENT8_OBS 5 5 3 18446744073709551616"));
    CHECK(!guard.accept("R258_EVENT8_OBS 5 4 0 104")); // generation not advanced on disconnect
    CHECK(!guard.accept("R258_EVENT8_OBS 5 5 3 1")); // clock regressed
    bool foreign=false;
    std::thread other([&]{
        foreign=guard.accept("R258_EVENT8_OBS 5 5 3 104") ||
                guard.retire();
    });
    other.join();
    CHECK(!foreign && guard.accepted()==4);
    CHECK(guard.accept("R258_EVENT8_OBS 5 5 3 104"));
    CHECK(!guard.may_evaluate_reach());
    CHECK(guard.retire());
    CHECK(!guard.accept("R258_EVENT8_OBS 6 6 3 105"));
    std::cout<<"R258_SOURCE_FRAME_GENERATION_REPLAY_OWNER_FENCE_PASS\n";
    std::cout<<"R258_INCOMPLETE_GAMEPLAY_INPUT_NEVER_DISPATCHES_MODULES_PASS\n";
    if(argc==5)
       std::cout<<"R258_REAL_JAVA8_OBSERVED_FRAME_NATIVE_ABI_PASS\n";
    else std::cout<<"R258_NATIVE_READ_ONLY_PARSER_NEGATIVES_PASS\n";
    return 0;
}
