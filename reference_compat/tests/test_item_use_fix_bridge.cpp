#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ck(bool b,const char* x,int n){if(!b)throw std::runtime_error(std::string("R185 ")+std::to_string(n)+" "+x);}
#define CHECK(x) ck(static_cast<bool>(x),#x,__LINE__)
static ExecutionFrame frame(unsigned event,bool hitting,bool skip=false,bool attack=false){
    ExecutionFrame f;
    f.event=event;f.world_present=true;f.player_present=true;
    f.item_use_fix=ItemUseFixTickPayload{{skip,hitting},{},0};
    if(attack){
        NoHitDelayInput hit;
        hit.attack_click=true;hit.jvm_ready=true;hit.left_click_counter=10;
        f.no_hit_delay=NoHitDelayTickPayload{hit,{},0};
    }
    return f;
}
int main(){
    ItemUseFixController p;
    CHECK(!p.after({false,true}).proposed_hitting_block);
    auto d=p.before({false,true});
    CHECK(d.proposed_hitting_block==false&&d.restore_pending);
    CHECK(!p.after({true,false}).proposed_hitting_block);
    CHECK(p.after({false,false}).proposed_hitting_block==true);
    CHECK(p.before({true,false}).restore_pending);
    CHECK(!p.before({false,false}).restore_pending);
    CHECK(!p.after({false,false}).proposed_hitting_block);
    p.before({false,true});p.reset();
    CHECK(!p.restore_pending());

    ModuleExecutionBridge b;
    auto fix=b.add({"Item Use Fix",4,{{14,0},{15,0}},false});
    auto hit=b.add({"No Hit Delay",4,{{14,0}},true});
    auto obs=b.add({"Priority Observer",2,{{14,100}},true});
    auto unbound=b.add({"Unbound",0,{{14,-100}},true});
    CHECK(fix==0&&hit==1&&obs==2&&unbound==3);
    unsigned calls=0;
    CHECK(b.bind(obs,14,[&](const Delivery& d,ExecutionFrame&){
        CHECK(d.recipient==obs&&d.event==14);++calls;
    }));
    CHECK(b.bind_no_hit_delay_from_catalog());
    CHECK(b.bind_item_use_fix_from_catalog());
    CHECK(!b.bind_item_use_fix(fix));
    const auto c=b.coverage();
    CHECK(c.registered_modules==4);
    CHECK(c.native_callback_subscriptions==5);
    CHECK(c.executable_callback_bindings==4);
    CHECK(c.unimplemented_callback_subscriptions==1);
    CHECK(c.implemented_per_event[14]==3&&c.implemented_per_event[15]==1);

    auto f=frame(14,true,false,true);
    CHECK(b.dispatch(f)&&f.item_use_fix->executions==0);
    CHECK(f.no_hit_delay->executions==1&&f.no_hit_delay->decision.clear_click_counter);
    CHECK((f.executed_modules==std::vector<std::size_t>{obs,hit}));
    CHECK(b.request(fix,true));
    CHECK(!b.apply(false,false));
    CHECK(b.apply(true,true));
    f=frame(14,true,false,true);
    CHECK(b.dispatch(f));
    CHECK(f.item_use_fix->executions==1);
    CHECK(f.item_use_fix->decision.proposed_hitting_block==false);
    CHECK(f.no_hit_delay->executions==1);
    CHECK(f.executed_modules.size()==3&&f.executed_modules.front()==obs);
    // Original qsort tie ordering for two priority-0 callbacks is unknown.
    CHECK((f.executed_modules[1]==fix||f.executed_modules[2]==fix));
    CHECK((f.executed_modules[1]==hit||f.executed_modules[2]==hit));
    f=frame(15,false);
    CHECK(b.dispatch(f)&&f.item_use_fix->executions==1);
    CHECK(f.item_use_fix->decision.proposed_hitting_block==true);
    CHECK((f.executed_modules==std::vector<std::size_t>{fix}));
    f=frame(14,false);
    CHECK(b.dispatch(f)&&!f.item_use_fix->decision.restore_pending);
    f=frame(15,false);
    CHECK(b.dispatch(f)&&!f.item_use_fix->decision.proposed_hitting_block);
    f=frame(14,true);CHECK(b.dispatch(f));
    f=frame(14,false,true);CHECK(b.dispatch(f));
    CHECK(f.item_use_fix->decision.restore_pending);
    f=frame(15,false,true);CHECK(b.dispatch(f));
    CHECK(!f.item_use_fix->decision.proposed_hitting_block);
    f=frame(15,false);CHECK(b.dispatch(f));
    CHECK(f.item_use_fix->decision.proposed_hitting_block==true);

    f=frame(14,true);f.world_present=false;CHECK(!b.dispatch(f));
    f.world_present=true;f.player_present=false;CHECK(!b.dispatch(f));
    f.player_present=true;f.event=13;CHECK(!b.dispatch(f));
    f.event=14;f.no_slow=NoSlowTickPayload{};CHECK(!b.dispatch(f));
    f.no_slow.reset();
    CHECK(b.request(fix,false)&&b.apply(true,true));
    f=frame(15,false);CHECK(b.dispatch(f)&&f.item_use_fix->executions==0);
    b.reset_item_use_fix_world_state();
    CHECK(b.request(fix,true)&&b.apply(true,true));
    f=frame(15,false);CHECK(b.dispatch(f));
    CHECK(!f.item_use_fix->decision.proposed_hitting_block);
    b.stop();CHECK(!b.dispatch(f)&&!b.bind_item_use_fix_from_catalog());

    ModuleExecutionBridge wrong_cat;
    auto id=wrong_cat.add({"Item Use Fix",0,{{14,0},{15,0}},true});
    CHECK(!wrong_cat.bind_item_use_fix(id));
    ModuleExecutionBridge wrong_slot;
    id=wrong_slot.add({"Item Use Fix",4,{{14,0},{16,0}},true});
    CHECK(!wrong_slot.bind_item_use_fix(id));
    ModuleExecutionBridge wrong_priority;
    id=wrong_priority.add({"Item Use Fix",4,{{14,0},{15,1}},true});
    CHECK(!wrong_priority.bind_item_use_fix(id));
    ModuleExecutionBridge only_before;
    id=only_before.add({"Item Use Fix",4,{{14,0}},true});
    CHECK(!only_before.bind_item_use_fix(id));
    CHECK(only_before.coverage().executable_callback_bindings==0);
    std::cout<<"R185 paired Item Use Fix PASS\n";
}
