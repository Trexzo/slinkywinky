#include "slinkywinky/module_execution_bridge.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;

static void ck(bool ok,const char* reason,int line) {
    if(!ok) throw std::runtime_error(
        "R188 line "+std::to_string(line)+": "+reason);
}
#define CHECK(x) ck(static_cast<bool>(x),#x,__LINE__)

static ExecutionFrame late_tick(bool blindness, bool nausea, bool active,
                                bool include_use_delay = false) {
    ExecutionFrame f;
    f.event=kAntiDebuffEvent;f.world_present=true;f.player_present=true;
    f.anti_debuff=AntiDebuffTickPayload{{blindness,nausea,active},{},0};
    if(include_use_delay) {
        NoUseDelayTickPayload item;
        item.input.in_game=true;
        item.input.use_key_down=true;
        item.input.physical_use_held=true;
        item.input.use_ticks_left=3;
        f.no_use_delay=item;
    }
    return f;
}
int main() {
    AntiDebuffController policy;
    auto d=policy.evaluate({true,true,true});
    CHECK(d.proposed_blindness_duration==0);
    CHECK(d.proposed_time_in_portal==0.0f);
    CHECK(d.proposed_prev_time_in_portal==0.0f);
    d=policy.evaluate({true,false,true});
    CHECK(d.proposed_blindness_duration==0);
    CHECK(!d.proposed_time_in_portal&&!d.proposed_prev_time_in_portal);
    d=policy.evaluate({false,true,true});
    CHECK(!d.proposed_blindness_duration);
    CHECK(d.proposed_time_in_portal==0.0f);
    CHECK(d.proposed_prev_time_in_portal==0.0f);
    d=policy.evaluate({true,true,false});
    CHECK(!d.proposed_blindness_duration);
    CHECK(d.proposed_time_in_portal==0.0f);
    d=policy.evaluate({false,false,true});
    CHECK(!d.proposed_blindness_duration);
    CHECK(!d.proposed_time_in_portal);
    CHECK(!d.proposed_prev_time_in_portal);

    ModuleExecutionBridge bridge;
    // The order of the two equal-priority 0 handlers is not asserted:
    // reference qsort tie-ordering is not proven identical.
    const auto anti=bridge.add({"Anti Debuff",2,{{18,0}},false});
    const auto item=bridge.add({"No Use Delay",4,{{18,0}},false});
    const auto observer=bridge.add({"Synthetic Observer",2,{{18,100}},true});
    const auto unbound=bridge.add({"Not Implemented",4,{{18,-100}},true});
    CHECK(anti==0&&item==1&&observer==2&&unbound==3);
    unsigned observed=0;
    CHECK(bridge.bind(observer,18,[&](const Delivery& d,ExecutionFrame& f) {
        CHECK(d.event==18&&d.recipient==observer&&f.event==18);
        ++observed;
    }));
    CHECK(bridge.bind_anti_debuff_from_catalog());
    CHECK(!bridge.bind_anti_debuff(anti));
    CHECK(bridge.bind_no_use_delay_from_catalog());
    const auto c=bridge.coverage();
    CHECK(c.registered_modules==4);
    CHECK(c.native_callback_subscriptions==4);
    CHECK(c.modules_with_executable_bindings==3);
    CHECK(c.executable_callback_bindings==3);
    CHECK(c.unimplemented_callback_subscriptions==1);
    CHECK(c.implemented_per_event[18]==3);

    auto f=late_tick(true,true,true,true);
    CHECK(bridge.dispatch(f));
    CHECK(f.anti_debuff->executions==0);
    CHECK(f.no_use_delay->executions==0);
    CHECK((f.executed_modules==std::vector<std::size_t>{observer}));
    CHECK(observed==1);
    CHECK(bridge.request(anti,true)&&bridge.request(item,true));
    CHECK(!bridge.apply(false,false));
    CHECK(!bridge.apply(true,false));
    CHECK(!bridge.enabled(anti));
    CHECK(bridge.apply(true,true));

    f=late_tick(true,true,true,true);
    CHECK(bridge.dispatch(f));
    CHECK(f.anti_debuff->executions==1);
    CHECK(f.anti_debuff->decision.proposed_blindness_duration==0);
    CHECK(f.anti_debuff->decision.proposed_time_in_portal==0.0f);
    CHECK(f.anti_debuff->decision.proposed_prev_time_in_portal==0.0f);
    CHECK(f.no_use_delay->executions==1);
    CHECK(f.no_use_delay->decision.tracked_remaining_ticks==3);
    CHECK(f.executed_modules.size()==3);
    CHECK(f.executed_modules.front()==observer);
    CHECK(std::count(f.executed_modules.begin(),f.executed_modules.end(),anti)==1);
    CHECK(std::count(f.executed_modules.begin(),f.executed_modules.end(),item)==1);
    f=late_tick(true,false,false,true);
    CHECK(bridge.dispatch(f));
    CHECK(!f.anti_debuff->decision.proposed_blindness_duration);
    CHECK(!f.anti_debuff->decision.proposed_time_in_portal);
    CHECK(!f.anti_debuff->decision.proposed_prev_time_in_portal);
    CHECK(f.no_use_delay->executions==1);
    CHECK(f.no_use_delay->decision.tracked_remaining_ticks==2);

    f=late_tick(true,true,true,true);
    f.world_present=false;
    CHECK(!bridge.dispatch(f));
    f.world_present=true;f.player_present=false;
    CHECK(!bridge.dispatch(f));
    f.player_present=true;f.event=17;
    CHECK(!bridge.dispatch(f));
    f.event=18;
    f.no_hit_delay=NoHitDelayTickPayload{};
    CHECK(!bridge.dispatch(f));
    f.no_hit_delay.reset();

    CHECK(bridge.request(anti,false)&&bridge.apply(true,true));
    f=late_tick(true,true,true,true);
    CHECK(bridge.dispatch(f));
    CHECK(f.anti_debuff->executions==0);
    CHECK(f.no_use_delay->executions==1);
    CHECK(std::find(f.executed_modules.begin(),f.executed_modules.end(),anti)
          ==f.executed_modules.end());

    CHECK(bridge.request(item,false)&&bridge.request(anti,true));
    CHECK(bridge.apply(true,true));
    f=late_tick(false,true,true);
    CHECK(bridge.dispatch(f));
    CHECK(f.anti_debuff->executions==1);
    CHECK(!f.anti_debuff->decision.proposed_blindness_duration);
    CHECK(f.anti_debuff->decision.proposed_time_in_portal==0.0f);
    CHECK(f.anti_debuff->decision.proposed_prev_time_in_portal==0.0f);
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    CHECK(!bridge.bind_anti_debuff_from_catalog());

    ModuleExecutionBridge wrong_category;
    auto id=wrong_category.add({"Anti Debuff",4,{{18,0}},true});
    CHECK(!wrong_category.bind_anti_debuff(id));
    ModuleExecutionBridge wrong_event;
    id=wrong_event.add({"Anti Debuff",2,{{17,0}},true});
    CHECK(!wrong_event.bind_anti_debuff(id));
    ModuleExecutionBridge wrong_priority;
    id=wrong_priority.add({"Anti Debuff",2,{{18,1}},true});
    CHECK(!wrong_priority.bind_anti_debuff(id));

    std::cout<<"R188 Anti Debuff PASS: blindness, nausea, shared event-18 "
                "No Use Delay routing, admission, metadata and teardown\n";
}
