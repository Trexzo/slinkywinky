#include "slinkywinky/weapons_tick_cache.hpp"
#include <cstdio>
#include <cstdlib>
using namespace slinkywinky::reference_compat;
#define CHECK(condition) do { if(!(condition)){\
 std::fprintf(stderr,"R262 Weapons cache line %d: %s\n",__LINE__,#condition);\
 std::exit(1);}} while(0)
static WeaponsTickObservation observation(int tick=1) {
    WeaponsTickObservation x;
    x.world_token=42;x.player_token=88;
    x.player_ticks_existed=tick;
    x.current_slot=2;
    x.inventory.slots[2]={true,276,1u};
    x.settings.swords=true;
    x.settings.sharpness=true;
    return x;
}
int main() {
    WeaponsTickCache cache;
    auto snap=observation(0);
    auto r=cache.on_query(snap);
    CHECK(r.admitted && !r.refreshed && !r.cached_allowed &&
          r.cached_enchant_groups==0 && r.cached_tick==0);
    snap.player_ticks_existed=1;
    r=cache.on_query(snap);
    CHECK(r.admitted && r.refreshed && r.cached_allowed &&
          r.cached_enchant_groups==1 && r.cached_tick==1);
    // Source intentionally keeps same-tick item and settings cached.
    snap.inventory.slots[2]={};
    snap.settings.swords=false;
    snap.settings.sharpness=false;
    r=cache.on_query(snap);
    CHECK(r.admitted && !r.refreshed && r.cached_allowed &&
          r.cached_enchant_groups==1 && r.cached_tick==1);
    snap.player_ticks_existed=2;
    r=cache.on_query(snap);
    CHECK(r.admitted && r.refreshed && !r.cached_allowed &&
          r.cached_enchant_groups==0);
    snap.settings.fists=true;
    r=cache.on_query(snap);
    CHECK(r.admitted && !r.refreshed && !r.cached_allowed);
    snap.player_ticks_existed=3;
    r=cache.on_query(snap);
    CHECK(r.admitted && r.refreshed && r.cached_allowed);
    // A new player's same-numbered tick cannot inherit cached state.
    snap.player_token=99;
    r=cache.on_query(snap);
    CHECK(!r.admitted && cache.key()==0);
    r=cache.on_query(snap);
    CHECK(r.admitted && r.refreshed && r.cached_allowed);
    // Tick regression in one owner refuses and invalidates the session.
    snap.player_ticks_existed=1;
    CHECK(!cache.on_query(snap).admitted && cache.key()==0);
    snap=observation();
    snap.world_token=0;
    CHECK(!cache.on_query(snap).admitted);
    snap=observation();
    snap.player_ticks_existed=-1;
    CHECK(!cache.on_query(snap).admitted);
    snap=observation();
    snap.current_slot=9;
    CHECK(!cache.on_query(snap).admitted);
    snap=observation();
    snap.inventory.slots[2].enchant_groups=8;
    CHECK(!cache.on_query(snap).admitted);
    snap=observation();
    snap.inventory.slots[2].identifier=-3;
    CHECK(!cache.on_query(snap).admitted);
    snap=observation();
    snap.settings.extra_item_identifiers={-1};
    CHECK(!cache.on_query(snap).admitted);
    cache.reset();
    CHECK(cache.key()==0 && cache.on_query(observation(0)).admitted);
    std::puts("R262_WEAPONS_PER_PLAYER_TICK_CACHE_SOURCE_PARITY_PASS");
}
