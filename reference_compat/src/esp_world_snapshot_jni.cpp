// R301: strict owner-bound R300 Java8 -> original-source R297-R299
// first-party C++ offline ESP controllers. Count-only; no GL or game writes.
#include "slinkywinky/chest_esp_controller.hpp"
#include "slinkywinky/item_esp_controller.hpp"
#include "slinkywinky/player_esp_controller.hpp"
#include <jni.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace {
using namespace slinkywinky::reference_compat;
struct Session {
    const std::thread::id thread=std::this_thread::get_id();
    bool observed=false;
    jlong world=0,player=0,generation=0,tick=-1;
    ChestEspController chest;
    ItemEspController item;
    PlayerEspController players;
    // R305 opt-in only. Legacy R301-R304 JNI ABI and eager observe remain.
    bool phased=false;
    // 0 staged, 1 Chest event0 complete, 2 event18 complete,
    // 6 world render, 7 overlay, 26 world unload; 3 initial.
    int source_phase=3;
    std::vector<ChestEspTile> staged_tiles;
    std::vector<ItemEspEntity> staged_drops;
    std::vector<PlayerEspEntity> staged_players;
    ChestEspPosition staged_self{};
    ChestEspSettings staged_chest{};
    ItemEspSettings staged_item{};
    PlayerEspSettings staged_player{};
};
std::mutex lock;
std::map<jlong,std::unique_ptr<Session>> sessions;
jlong nextHandle=1;
void fail(JNIEnv* env,const char* message){
    if(env->ExceptionCheck())return;
    jclass type=env->FindClass("java/lang/IllegalStateException");
    if(type)env->ThrowNew(type,message);
}
bool finite(double x){return std::isfinite(x);}
bool boolean(jint x){return x==0||x==1;}
bool dimensions(JNIEnv* env,jlongArray ids,jdoubleArray self,
                jdoubleArray tilep,jintArray tileids,
                jdoubleArray dropp,jintArray dropfacts,
                jdoubleArray playerp,jfloatArray health,
                jintArray playerfacts,jfloatArray radii){
    if(!ids||!self||!tilep||!tileids||!dropp||!dropfacts||
       !playerp||!health||!playerfacts||!radii)return false;
    const jsize nt=env->GetArrayLength(tileids);
    const jsize nd=env->GetArrayLength(dropfacts)/5;
    const jsize np=env->GetArrayLength(playerfacts)/3;
    return env->GetArrayLength(ids)==4&&env->GetArrayLength(self)==3&&
        env->GetArrayLength(radii)==3&&
        nt>=0&&nt<=4096&&nd>=0&&nd<=8192&&np>=0&&np<=512&&
        env->GetArrayLength(tilep)==nt*3&&
        env->GetArrayLength(dropfacts)==nd*5&&
        env->GetArrayLength(dropp)==nd*6&&
        env->GetArrayLength(playerfacts)==np*3&&
        env->GetArrayLength(playerp)==np*6&&
        env->GetArrayLength(health)==np*3;
}
template<typename T> bool finiteVector(const std::vector<T>& input){
    for(const auto x:input)if(!std::isfinite(x))return false;
    return true;
}
} // namespace
extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeCreate(JNIEnv* env,jclass){
    try{
        std::lock_guard<std::mutex> guard(lock);
        if(nextHandle<=0||nextHandle==std::numeric_limits<jlong>::max()){
            fail(env,"R301 handle overflow");return 0;
        }
        const jlong id=nextHandle++;
        sessions.emplace(id,std::unique_ptr<Session>(new Session()));
        return id;
    }catch(...){fail(env,"R301 session creation failed");return 0;}
}
// R305 optional semantic phase routing, never enabled by prior R301-R304 JARs.
extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeEnableSourcePhases(
    JNIEnv* env,jclass,jlong handle){
    std::lock_guard<std::mutex> guard(lock);
    const auto it=sessions.find(handle);
    if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()||
       it->second->observed||it->second->phased){
        fail(env,"R305 phase upgrade denied");return;
    }
    it->second->phased=true;
    it->second->source_phase=3;
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeObserve(
    JNIEnv* env,jclass,jlong handle,jlongArray ids,jdoubleArray self,
    jdoubleArray tilep,jintArray tileids,jdoubleArray dropp,
    jintArray dropfacts,jdoubleArray playerp,jfloatArray health,
    jintArray playerfacts,jfloatArray radii){
    if(!dimensions(env,ids,self,tilep,tileids,dropp,dropfacts,
                   playerp,health,playerfacts,radii)){
        fail(env,"R301 invalid dimensions");return nullptr;
    }
    try{
        const jsize nt=env->GetArrayLength(tileids);
        const jsize nd=env->GetArrayLength(dropfacts)/5;
        const jsize np=env->GetArrayLength(playerfacts)/3;
        jlong meta[4]{};jdouble location[3]{};jfloat ranges[3]{};
        env->GetLongArrayRegion(ids,0,4,meta);
        env->GetDoubleArrayRegion(self,0,3,location);
        env->GetFloatArrayRegion(radii,0,3,ranges);
        if(env->ExceptionCheck())return nullptr;
        if(meta[0]<=0||meta[1]<=0||meta[0]==meta[1]||
           meta[2]<=0||meta[3]<0||!finiteVector(std::vector<jdouble>(
           location,location+3))||
           !finiteVector(std::vector<jfloat>(ranges,ranges+3))||
           ranges[0]<0||ranges[1]<0||ranges[2]<0||
           ranges[0]>256||ranges[1]>256||ranges[2]>256){
            fail(env,"R301 invalid owner or radii");return nullptr;
        }
        std::vector<jdouble> tp(static_cast<std::size_t>(3*nt));
        std::vector<jint> ti(static_cast<std::size_t>(nt));
        std::vector<jdouble> dp(static_cast<std::size_t>(6*nd));
        std::vector<jint> di(static_cast<std::size_t>(5*nd));
        std::vector<jdouble> pp(static_cast<std::size_t>(6*np));
        std::vector<jfloat> ph(static_cast<std::size_t>(3*np));
        std::vector<jint> pi(static_cast<std::size_t>(3*np));
        if(nt){env->GetDoubleArrayRegion(tilep,0,3*nt,tp.data());
               env->GetIntArrayRegion(tileids,0,nt,ti.data());}
        if(nd){env->GetDoubleArrayRegion(dropp,0,6*nd,dp.data());
               env->GetIntArrayRegion(dropfacts,0,5*nd,di.data());}
        if(np){env->GetDoubleArrayRegion(playerp,0,6*np,pp.data());
               env->GetFloatArrayRegion(health,0,3*np,ph.data());
               env->GetIntArrayRegion(playerfacts,0,3*np,pi.data());}
        if(env->ExceptionCheck())return nullptr;
        if(!finiteVector(tp)||!finiteVector(dp)||!finiteVector(pp)||
           !finiteVector(ph)){
            fail(env,"R301 invalid coordinates or health");return nullptr;
        }
        std::vector<ChestEspTile> tiles;tiles.reserve(static_cast<std::size_t>(nt));
        for(jsize i=0;i<nt;i++){
            const std::size_t k=static_cast<std::size_t>(i);
            if(ti[k]<0){fail(env,"R301 invalid block ID");return nullptr;}
            tiles.push_back({{tp[3*k],tp[3*k+1],tp[3*k+2]},ti[k]});
        }
        std::vector<ItemEspEntity> drops;drops.reserve(static_cast<std::size_t>(nd));
        for(jsize i=0;i<nd;i++){
            const std::size_t k=static_cast<std::size_t>(i);
            if(di[5*k]<-1||di[5*k+1]<0||
               !boolean(di[5*k+2])||!boolean(di[5*k+3])||
               !boolean(di[5*k+4])||
               (di[5*k+4]&&(!di[5*k+3]||di[5*k]<0))){
                fail(env,"R301 invalid item");return nullptr;
            }
            ItemEspEntity e;
            e.previous={dp[6*k],dp[6*k+1],dp[6*k+2]};
            e.position={dp[6*k+3],dp[6*k+4],dp[6*k+5]};
            e.item_id=di[5*k];e.stack_size=di[5*k+1];
            e.dead=di[5*k+2]!=0;e.stack_present=di[5*k+3]!=0;
            e.item_present=di[5*k+4]!=0;
            drops.push_back(e);
        }
        std::vector<PlayerEspEntity> players;players.reserve(static_cast<std::size_t>(np));
        int selves=0;
        for(jsize i=0;i<np;i++){
            const std::size_t k=static_cast<std::size_t>(i);
            if(!boolean(pi[3*k])||!boolean(pi[3*k+1])||pi[3*k+2]<0){
                fail(env,"R301 invalid player");return nullptr;
            }
            PlayerEspEntity e;
            e.previous={pp[6*k],pp[6*k+1],pp[6*k+2]};
            e.position={pp[6*k+3],pp[6*k+4],pp[6*k+5]};
            e.is_self=pi[3*k]!=0;e.dead=pi[3*k+1]!=0;
            e.health=ph[3*k];e.max_health=ph[3*k+1];
            e.absorption=ph[3*k+2];e.hurt_time=pi[3*k+2];
            e.name_color="unknown";e.relationship_flags=0;
            if(e.is_self)++selves;
            players.push_back(e);
        }
        if(selves!=1){fail(env,"R301 missing/duplicate self");return nullptr;}
        jlongArray out=env->NewLongArray(11);
        if(!out)return nullptr;
        std::lock_guard<std::mutex> guard(lock);
        auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()){
            fail(env,"R301 foreign session");return nullptr;
        }
        Session& s=*it->second;
        if(s.observed){
            if(meta[2]<s.generation||
               (meta[2]==s.generation&&
                (meta[0]!=s.world||meta[1]!=s.player||meta[3]<=s.tick))||
               (meta[2]>s.generation&&
                (meta[0]==s.world||meta[1]==s.player))){
                fail(env,"R301 stale, replayed, or incoherent provenance");
                return nullptr;
            }
        }
        ChestEspSettings cs;
        cs.range=ranges[0];cs.render_chest=true;cs.render_ender_chest=true;
        cs.render_trapped_chest=true;cs.render_beacon=true;
        ItemEspSettings is;is.range=ranges[1];is.highlight_all=true;is.show_count=true;
        PlayerEspSettings ps;ps.range=ranges[2];ps.color_mode=1;
        const ChestEspPosition chestSelf{location[0],location[1],location[2]};
        const ItemEspPosition itemSelf{location[0],location[1],location[2]};
        const PlayerEspPosition playerSelf{location[0],location[1],location[2]};
        jlong chestCount=-1,itemCount=-1,playerCount=-1;
        if(s.phased){
            if(s.source_phase==0||s.source_phase==1){
                fail(env,"R305 incomplete source events before next tick");
                return nullptr;
            }
            if(s.observed&&(meta[0]!=s.world||meta[1]!=s.player||
                            meta[2]!=s.generation)){
                // Event26-equivalent invalidation on world replacement.
                s.chest.unload();s.item.clear_event26();s.players.world_unload();
            }
            s.staged_tiles=std::move(tiles);
            s.staged_drops=std::move(drops);
            s.staged_players=std::move(players);
            s.staged_self=chestSelf;
            s.staged_chest=cs;
            s.staged_item=is;
            s.staged_player=ps;
            s.source_phase=0;
        }else{
            s.chest.tick(true,chestSelf,cs.range,cs,tiles);
            s.item.sample(true,itemSelf,is,{},drops);
            s.players.update(true,playerSelf,ps,players);
            chestCount=static_cast<jlong>(s.chest.rows().size());
            itemCount=static_cast<jlong>(s.item.rows().size());
            playerCount=static_cast<jlong>(s.players.rows().size());
        }
        jlong payload[11]{1,meta[0],meta[1],meta[2],meta[3],nt,nd,np,
            chestCount,itemCount,playerCount};
        env->SetLongArrayRegion(out,0,11,payload);
        if(env->ExceptionCheck())return nullptr;
        s.observed=true;s.world=meta[0];s.player=meta[1];
        s.generation=meta[2];s.tick=meta[3];
        return out;
    }catch(...){fail(env,"R301 native ESP observation failed");return nullptr;}
}
// R305: actual first-party C++ controller event-0/event-18 dispatch,
// plus render 6/7 admission and event26 invalidation. All optional.
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeSourceEvent(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,jint type){
    if(!identity||env->GetArrayLength(identity)!=4){
        fail(env,"R305 invalid source event receipt");return nullptr;
    }
    jlong id[4]{};
    env->GetLongArrayRegion(identity,0,4,id);
    if(env->ExceptionCheck())return nullptr;
    if(id[0]<=0||id[1]<=0||id[0]==id[1]||id[2]<=0||id[3]<0){
        fail(env,"R305 invalid source event identity");return nullptr;
    }
    std::lock_guard<std::mutex> guard(lock);
    const auto it=sessions.find(handle);
    if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()||
       !it->second->phased){
        fail(env,"R305 source event wrong owner");return nullptr;
    }
    Session& s=*it->second;
    if(!s.observed||id[0]!=s.world||id[1]!=s.player||
       id[2]!=s.generation||id[3]!=s.tick){
        fail(env,"R305 stale source event");return nullptr;
    }
    if(type==0 && s.source_phase==0){
        s.chest.tick(true,s.staged_self,s.staged_chest.range,
                     s.staged_chest,s.staged_tiles);
        s.staged_tiles.clear();
        s.source_phase=1;
    }else if(type==18 && s.source_phase==1){
        const ItemEspPosition self{s.staged_self.x,s.staged_self.y,
                                   s.staged_self.z};
        const PlayerEspPosition pself{s.staged_self.x,s.staged_self.y,
                                     s.staged_self.z};
        s.item.sample(true,self,s.staged_item,{},s.staged_drops);
        s.players.update(true,pself,s.staged_player,s.staged_players);
        s.staged_drops.clear();s.staged_players.clear();
        s.source_phase=2;
    }else if(type==6 && (s.source_phase==2||
                          s.source_phase==6||s.source_phase==7)){
        s.source_phase=6;
    }else if(type==7 && (s.source_phase==6||s.source_phase==7)){
        s.source_phase=7;
    }else if(type==26){
        s.chest.unload();s.item.clear_event26();s.players.world_unload();
        s.staged_tiles.clear();s.staged_drops.clear();s.staged_players.clear();
        s.source_phase=26;
    }else{
        fail(env,"R305 source event order/duplicate refused");return nullptr;
    }
    jlongArray out=env->NewLongArray(3);
    if(!out)return nullptr;
    jlong payload[3]{
        static_cast<jlong>(s.chest.rows().size()),
        s.source_phase==1 ? -1 : static_cast<jlong>(s.item.rows().size()),
        s.source_phase==1 ? -1 : static_cast<jlong>(s.players.rows().size())
    };
    env->SetLongArrayRegion(out,0,3,payload);
    return env->ExceptionCheck()?nullptr:out;
}
extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeClose(
    JNIEnv* env,jclass,jlong handle){
    std::lock_guard<std::mutex> guard(lock);
    auto it=sessions.find(handle);
    if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()){
        fail(env,"R301 close not owned");return;
    }
    sessions.erase(it);
}


// R303: source-faithful ChestEspController.plan(...) row projection.
// A 3D draw plan alone does not invoke GL or certify visual fidelity.
// Reuses the same thread-bound R301 session and exact most recent owner IDs.
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeChestPlan(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,jdoubleArray camera){
    if(!identity||!camera||env->GetArrayLength(identity)!=4||
       env->GetArrayLength(camera)!=3){
        fail(env,"R303 invalid draw metadata");return nullptr;
    }
    try{
        jlong id[4]{};jdouble xyz[3]{};
        env->GetLongArrayRegion(identity,0,4,id);
        env->GetDoubleArrayRegion(camera,0,3,xyz);
        if(env->ExceptionCheck())return nullptr;
        if(id[0]<=0||id[1]<=0||id[0]==id[1]||id[2]<=0||id[3]<0||
           !std::isfinite(xyz[0])||!std::isfinite(xyz[1])||!std::isfinite(xyz[2])){
            fail(env,"R303 malformed camera/owner");return nullptr;
        }
        std::lock_guard<std::mutex> guard(lock);
        const auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()){
            fail(env,"R303 unowned native draw plan");return nullptr;
        }
        const Session& s=*it->second;
        if(!s.observed||id[0]!=s.world||id[1]!=s.player||
           id[2]!=s.generation||id[3]!=s.tick||
           (s.phased&&s.source_phase!=6&&s.source_phase!=7)){
            fail(env,"R303/R305 stale or out-of-phase render receipt");return nullptr;
        }
        ChestEspSettings settings;
        settings.line_width=1.0f;settings.filled=false;
        const auto plan=s.chest.plan({xyz[0],xyz[1],xyz[2]},settings);
        const std::size_t rows=plan.draws.size();
        if(rows>ChestEspController::kSourceRowCap){
            fail(env,"R303 source draw row cap breached");return nullptr;
        }
        std::vector<jdouble> payload(5+5*rows);
        payload[0]=1.0;payload[1]=static_cast<double>(rows);
        payload[2]=plan.scale;payload[3]=plan.filled_alpha;
        payload[4]=plan.line_width;
        for(std::size_t i=0;i<rows;i++){
            const auto& draw=plan.draws[i];
            const std::size_t base=5+5*i;
            for(int axis=0;axis<3;axis++){
                if(!std::isfinite(draw.translation[static_cast<std::size_t>(axis)])){
                    fail(env,"R303 nonfinite draw translation");return nullptr;
                }
                payload[base+static_cast<std::size_t>(axis)]=
                    draw.translation[static_cast<std::size_t>(axis)];
            }
            if(!ChestEspController::kind(draw.block_id)){
                fail(env,"R303 invalid source color kind");return nullptr;
            }
            payload[base+3]=draw.block_id;
            payload[base+4]=draw.filled?1.0:0.0;
        }
        jdoubleArray output=env->NewDoubleArray(static_cast<jsize>(payload.size()));
        if(!output)return nullptr;
        env->SetDoubleArrayRegion(output,0,static_cast<jsize>(payload.size()),
                                  payload.data());
        return env->ExceptionCheck()?nullptr:output;
    }catch(...){
        fail(env,"R303 native chest draw plan failed");return nullptr;
    }
}


// R304: source-order Entity ESP render *data* from the existing actual
// ItemEspController and PlayerEspController, never synthetic replacements.
// [version, itemRows, playerRows], 5 doubles per item
// [cameraX,Y,Z,itemId,mergedQuantity], 6 per player
// [cameraX,Y,Z,sourceHealth,sourceAbsorption,hurtTime].
// Only the latest owner/session receipt is eligible and render interpolation
// does not mutate any world or C++ source controller state.
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativeEntityPlan(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,
    jdoubleArray camera,jdouble partialTicks){
    if(!identity||!camera||env->GetArrayLength(identity)!=4||
       env->GetArrayLength(camera)!=3){
        fail(env,"R304 invalid entity draw metadata");return nullptr;
    }
    try{
        jlong id[4]{};jdouble xyz[3]{};
        env->GetLongArrayRegion(identity,0,4,id);
        env->GetDoubleArrayRegion(camera,0,3,xyz);
        if(env->ExceptionCheck())return nullptr;
        if(id[0]<=0||id[1]<=0||id[0]==id[1]||id[2]<=0||id[3]<0||
           !std::isfinite(partialTicks)||partialTicks<0||partialTicks>1||
           !std::isfinite(xyz[0])||!std::isfinite(xyz[1])||
           !std::isfinite(xyz[2])){
            fail(env,"R304 invalid owner/camera/interpolation");return nullptr;
        }
        std::lock_guard<std::mutex> guard(lock);
        const auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()){
            fail(env,"R304 unowned render call");return nullptr;
        }
        const Session& s=*it->second;
        if(!s.observed||id[0]!=s.world||id[1]!=s.player||
           id[2]!=s.generation||id[3]!=s.tick||
           (s.phased&&s.source_phase!=6&&s.source_phase!=7)){
            fail(env,"R304/R305 stale or out-of-phase entity rows");return nullptr;
        }
        const auto& items=s.item.rows();
        const auto& players=s.players.rows();
        if(items.size()>ItemEspController::kSourceRowCap||
           players.size()>PlayerEspController::kSourceRowCap){
            fail(env,"R304 source controller cap");return nullptr;
        }
        std::vector<jdouble> data(3+5*items.size()+6*players.size());
        data[0]=1;data[1]=static_cast<double>(items.size());
        data[2]=static_cast<double>(players.size());
        const auto at=[&](double previous,double current,int axis)->double {
            return previous+(current-previous)*partialTicks-xyz[axis];
        };
        for(std::size_t i=0;i<items.size();i++){
            const auto& row=items[i];
            const std::size_t k=3+5*i;
            data[k]=at(row.previous.x,row.position.x,0);
            data[k+1]=at(row.previous.y,row.position.y,1);
            data[k+2]=at(row.previous.z,row.position.z,2);
            data[k+3]=row.item_id;data[k+4]=row.quantity;
            if(!std::isfinite(data[k])||!std::isfinite(data[k+1])||
               !std::isfinite(data[k+2])||row.item_id<0||
               row.quantity<0||row.quantity>99){
                fail(env,"R304 invalid native item plan");return nullptr;
            }
        }
        const std::size_t off=3+5*items.size();
        for(std::size_t i=0;i<players.size();i++){
            const auto& row=players[i];
            const std::size_t k=off+6*i;
            data[k]=at(row.previous.x,row.position.x,0);
            data[k+1]=at(row.previous.y,row.position.y,1);
            data[k+2]=at(row.previous.z,row.position.z,2);
            data[k+3]=row.health;
            data[k+4]=row.absorption;
            data[k+5]=row.hurt;
            if(!std::isfinite(data[k])||!std::isfinite(data[k+1])||
               !std::isfinite(data[k+2])||
               !std::isfinite(row.health)||row.health<0||row.health>1||
               !std::isfinite(row.absorption)||row.absorption<0||
               row.absorption>1||row.hurt<0){
                fail(env,"R304 invalid native player plan");return nullptr;
            }
        }
        jdoubleArray result=env->NewDoubleArray(static_cast<jsize>(data.size()));
        if(!result)return nullptr;
        env->SetDoubleArrayRegion(result,0,static_cast<jsize>(data.size()),
                                  data.data());
        return env->ExceptionCheck()?nullptr:result;
    }catch(...){
        fail(env,"R304 native source entity draw failure");return nullptr;
    }
}


// R306: strictly event-7 source PlayerESP rows for projection by the Forge
// screen overlay. This method does not project, draw, or call the game.
// [version=1, rowCount, each: cameraRelativeX/Y/Z, sourceHealth,
// sourceAbsorption, hurtTime]. Uses only R299 C++ controller row ordering.
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_dev_slinkywinky_EspNativeReadOnlyPreview_nativePlayerScreenRows(
    JNIEnv* env,jclass,jlong handle,jlongArray identity,
    jdoubleArray camera,jdouble partialTicks){
    if(!identity||!camera||env->GetArrayLength(identity)!=4||
       env->GetArrayLength(camera)!=3){
        fail(env,"R306 malformed screen metadata");return nullptr;
    }
    try{
        jlong id[4]{};jdouble xyz[3]{};
        env->GetLongArrayRegion(identity,0,4,id);
        env->GetDoubleArrayRegion(camera,0,3,xyz);
        if(env->ExceptionCheck())return nullptr;
        if(id[0]<=0||id[1]<=0||id[0]==id[1]||id[2]<=0||id[3]<0||
           !std::isfinite(partialTicks)||partialTicks<0||partialTicks>1||
           !std::isfinite(xyz[0])||!std::isfinite(xyz[1])||
           !std::isfinite(xyz[2])){
            fail(env,"R306 malformed screen camera/owner");return nullptr;
        }
        std::lock_guard<std::mutex> guard(lock);
        const auto it=sessions.find(handle);
        if(it==sessions.end()||it->second->thread!=std::this_thread::get_id()){
            fail(env,"R306 foreign owner");return nullptr;
        }
        const Session& s=*it->second;
        if(!s.phased||!s.observed||s.source_phase!=7||
           id[0]!=s.world||id[1]!=s.player||
           id[2]!=s.generation||id[3]!=s.tick||
           !s.players.screen_mode(s.staged_player,true)){
            fail(env,"R306 event7, owner or source mode not admitted");return nullptr;
        }
        const auto& rows=s.players.rows();
        if(rows.size()>PlayerEspController::kSourceRowCap){
            fail(env,"R306 player cap exceeded");return nullptr;
        }
        std::vector<jdouble> output(2+6*rows.size());
        output[0]=1;output[1]=static_cast<jdouble>(rows.size());
        for(std::size_t i=0;i<rows.size();i++){
            const auto& row=rows[i];
            const std::size_t k=2+6*i;
            const auto interp=[&](double a,double b,double o)->double {
                return a+(b-a)*partialTicks-o;
            };
            output[k]=interp(row.previous.x,row.position.x,xyz[0]);
            output[k+1]=interp(row.previous.y,row.position.y,xyz[1]);
            output[k+2]=interp(row.previous.z,row.position.z,xyz[2]);
            output[k+3]=row.health;
            output[k+4]=row.absorption;
            output[k+5]=row.hurt;
            if(!std::isfinite(output[k])||!std::isfinite(output[k+1])||
               !std::isfinite(output[k+2])||
               !std::isfinite(row.health)||row.health<0||row.health>1||
               !std::isfinite(row.absorption)||row.absorption<0||
               row.absorption>1||row.hurt<0){
                fail(env,"R306 native nonfinite/out-of-range screen row");return nullptr;
            }
        }
        jdoubleArray result=env->NewDoubleArray(static_cast<jsize>(output.size()));
        if(!result)return nullptr;
        env->SetDoubleArrayRegion(result,0,static_cast<jsize>(output.size()),
                                  output.data());
        return env->ExceptionCheck()?nullptr:result;
    }catch(...){
        fail(env,"R306 player source screen rows failed");return nullptr;
    }
}
