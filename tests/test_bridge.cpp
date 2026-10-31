#include "slinkywinky/bridge.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+": " #x); } while(false)
int main() {
  const uint32_t seed[4]={1,2,3,4};
  SWContext* ctx=nullptr;
  CHECK(sw_create(seed,&ctx)==0); CHECK(ctx!=nullptr);
  CHECK(sw_create(seed,&ctx)==-1);
  SWFrame frame{};frame.abi_version=SW_ABI_VERSION;
  frame.time_ms=100;frame.generation=1;frame.world_valid=1;frame.weapon=1;
  frame.target_present=1;frame.target_visible=1;frame.target_alive=1;
  frame.target_x=2;
  SWSettings settings{};settings.abi_version=SW_ABI_VERSION;
  settings.clicker=1; settings.cps=12;settings.aim=1;settings.aim_range=4;
  settings.aim_hspeed=15;settings.aim_vspeed=15;settings.aim_fov=90;
  SWOutput out{};
  CHECK(sw_step(ctx,&frame,&settings,&out)==0);
  CHECK(out.abi_version==SW_ABI_VERSION && out.attempt_attack && out.has_rotation);
  frame.abi_version=0;
  CHECK(sw_step(ctx,&frame,&settings,&out)==-3);
  frame.abi_version=SW_ABI_VERSION;
  int cross=-10;
  std::thread other([&]{cross=sw_step(ctx,&frame,&settings,&out);});other.join();
  CHECK(cross==-4);
  CHECK(sw_disable(ctx,0,0,&out)==0);
  CHECK(sw_destroy(&ctx)==0 && ctx==nullptr);
  std::cout<<"PASS portable bridge ABI, explicit owner thread, disable, lifecycle\n";
}
