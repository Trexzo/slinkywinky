#include "slinkywinky/chams_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x)){\
 std::fprintf(stderr,"R295 Chams %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(0)
struct Spy {
    std::vector<std::string> calls;
    float factor=0,units=0;
    bool throw_enable=false;
    void enable(int what){
        calls.push_back("enable:"+std::to_string(what));
        if(throw_enable)throw std::runtime_error("begin");
    }
    void offset(float x,float y){
        calls.push_back("offset");factor=x;units=y;
    }
    void disable(int what){
        calls.push_back("disable:"+std::to_string(what));
    }
};
int main() {
    static_assert(ChamsController::relationship_mask(false)==2u,
                  "source bot-only filter");
    static_assert(ChamsController::relationship_mask(true)==7u,
                  "source friend-or-bot filter");
    ChamsInput p;
    CHECK(!ChamsController::eligible(p));
    p.module_enabled=true;
    CHECK(ChamsController::eligible(p));
    p.relationship_flags=1u; // friend bit but hide friendlies false
    CHECK(ChamsController::eligible(p));
    p.relationship_flags=2u; // bot always hidden by Chams source
    CHECK(!ChamsController::eligible(p));
    p.relationship_flags=4u; // alternate friend bit
    CHECK(ChamsController::eligible(p));
    p.hide_friendlies=true;
    CHECK(!ChamsController::eligible(p));
    p.relationship_flags=1u;
    CHECK(!ChamsController::eligible(p));
    p.relationship_flags=2u;
    CHECK(!ChamsController::eligible(p));
    p.relationship_flags=7u;
    CHECK(!ChamsController::eligible(p));
    p.relationship_flags=8u;
    CHECK(ChamsController::eligible(p)); // unrelated bits preserved
    Spy spy;
    bool ran=false;
    const bool selected=ChamsController::render(
        p,spy,-1.25f,-2.5f,[&] {
            spy.calls.push_back("render");ran=true;
        });
    CHECK(selected && ran && spy.calls.size()==4u &&
          spy.calls[0]=="enable:32823" &&
          spy.calls[1]=="offset" &&
          spy.calls[2]=="render" &&
          spy.calls[3]=="disable:32823");
    CHECK(spy.factor==-1.25f && spy.units==-2.5f);
    p.relationship_flags=2u;
    spy.calls.clear();ran=false;
    CHECK(!ChamsController::render(p,spy,1.0f,2.0f,[&] {
        spy.calls.push_back("render");ran=true;
    }));
    CHECK(ran && spy.calls.size()==1u && spy.calls[0]=="render");
    p.relationship_flags=0u;
    spy.calls.clear();
    bool caught=false;
    try {
        ChamsController::render(p,spy,1.0f,2.0f,[&] {
            spy.calls.push_back("render");
            throw std::runtime_error("original renderer fault");
        });
    } catch(const std::runtime_error&) {caught=true;}
    CHECK(caught && spy.calls.size()==4u &&
          spy.calls[2]=="render" &&
          spy.calls[3]=="disable:32823");
    // Source lifecycle: begin() sits outside try, so a Graphics.enable
    // failure does not enter the finally/end block.
    spy.calls.clear();spy.throw_enable=true;caught=false;ran=false;
    try {
        ChamsController::render(p,spy,1.0f,2.0f,[&]{ran=true;});
    }catch(const std::runtime_error&) {caught=true;}
    CHECK(caught && !ran && spy.calls.size()==1u &&
          spy.calls[0]=="enable:32823");
    std::puts("R295_CHAMS_SOURCE_GL_BEGIN_END_AND_RELATIONSHIPS_PASS");
}
