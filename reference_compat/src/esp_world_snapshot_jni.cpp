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
        s.chest.tick(true,chestSelf,cs.range,cs,tiles);
        s.item.sample(true,itemSelf,is,{},drops);
        s.players.update(true,playerSelf,ps,players);
        jlong payload[11]{1,meta[0],meta[1],meta[2],meta[3],nt,nd,np,
            static_cast<jlong>(s.chest.rows().size()),
            static_cast<jlong>(s.item.rows().size()),
            static_cast<jlong>(s.players.rows().size())};
        env->SetLongArrayRegion(out,0,11,payload);
        if(env->ExceptionCheck())return nullptr;
        s.observed=true;s.world=meta[0];s.player=meta[1];
        s.generation=meta[2];s.tick=meta[3];
        return out;
    }catch(...){fail(env,"R301 native ESP observation failed");return nullptr;}
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
