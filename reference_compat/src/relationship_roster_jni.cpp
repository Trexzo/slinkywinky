// R282: Java8 immutable roster -> R281 native AntiBot-only proposals.
// No Minecraft entity, input, RelationshipService or packet writes.
#include "slinkywinky/roster_readonly_event0.hpp"
#include <jni.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace slinkywinky::reference_compat;
struct Session {
    const std::thread::id owner=std::this_thread::get_id();
    RosterReadOnlyEvent0 host{};
};
std::mutex mutex;
std::map<jlong,std::unique_ptr<Session>> sessions;
jlong next_id=1;
static_assert(sizeof(jlong)==sizeof(std::uint64_t),"JNI 64-bit long required");
std::uint64_t bits(jlong raw) noexcept {
    std::uint64_t result=0;
    std::memcpy(&result,&raw,sizeof(result));
    return result;
}
jlong rawbits(std::uint64_t n) noexcept {
    jlong result=0;
    std::memcpy(&result,&n,sizeof(result));
    return result;
}
void refuse(JNIEnv* env,const char* type,const char* reason) {
    if(env->ExceptionCheck())return;
    const jclass klass=env->FindClass(type);
    if(klass)env->ThrowNew(klass,reason);
}
bool bool01(jint v) noexcept {return v==0 || v==1;}
bool read_jstring(JNIEnv* env,jstring str,std::u16string& out) {
    if(!str)return false;
    const jsize length=env->GetStringLength(str);
    if(env->ExceptionCheck() || length<0 || length>2048)return false;
    const jchar* chars=env->GetStringChars(str,nullptr);
    if(!chars)return false;
    out.assign(reinterpret_cast<const char16_t*>(chars),
               reinterpret_cast<const char16_t*>(chars)+length);
    env->ReleaseStringChars(str,chars);
    return !env->ExceptionCheck();
}
} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_dev_slinkywinky_RelationshipRosterNativePreview_nativeCreate(
    JNIEnv* env,jclass) {
    try {
        std::lock_guard<std::mutex> lock(mutex);
        if(next_id<=0 || next_id==std::numeric_limits<jlong>::max()) {
            refuse(env,"java/lang/IllegalStateException","Roster sessions exhausted");
            return 0;
        }
        std::unique_ptr<Session> candidate(new Session());
        if(!candidate->host.set_enabled(true,false)) {
            refuse(env,"java/lang/IllegalStateException","Anti Bot preview unavailable");
            return 0;
        }
        const jlong id=next_id++;
        sessions.emplace(id,std::move(candidate));
        return id;
    }catch(...) {
        refuse(env,"java/lang/IllegalStateException","Native roster creation refused");
        return 0;
    }
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_dev_slinkywinky_RelationshipRosterNativePreview_nativeObserve(
    JNIEnv* env,jclass,jlong handle,jlongArray owner,jlongArray ids,
    jobjectArray names,jdoubleArray coords,jintArray facts,
    jint mask,jint uuid_mode,jfloat min_age) {
    if(!owner || !ids || !names || !coords || !facts ||
       env->GetArrayLength(owner)!=6 ||
       (mask&~15)!=0 || uuid_mode<0 || uuid_mode>2 ||
       !std::isfinite(min_age) || min_age<0) {
        refuse(env,"java/lang/IllegalArgumentException","Invalid roster JNI metadata");
        return nullptr;
    }
    const jsize count=env->GetArrayLength(names);
    if(count<0 || count>256 ||
       env->GetArrayLength(ids)!=2*count ||
       env->GetArrayLength(coords)!=4*count ||
       env->GetArrayLength(facts)!=4*count) {
        refuse(env,"java/lang/IllegalArgumentException","Invalid roster JNI dimensions");
        return nullptr;
    }
    try {
        jlong meta[6]={};
        env->GetLongArrayRegion(owner,0,6,meta);
        if(env->ExceptionCheck())return nullptr;
        if(meta[0]<=0 || meta[1]<=0 || meta[2]<=0 || meta[3]<0 ||
           meta[0]==meta[1]) {
            refuse(env,"java/lang/IllegalArgumentException","Invalid owner tokens");
            return nullptr;
        }
        std::vector<jlong> uuids(static_cast<std::size_t>(2*count));
        std::vector<jdouble> motion(static_cast<std::size_t>(4*count));
        std::vector<jint> detail(static_cast<std::size_t>(4*count));
        if(count>0) {
            env->GetLongArrayRegion(ids,0,2*count,uuids.data());
            env->GetDoubleArrayRegion(coords,0,4*count,motion.data());
            env->GetIntArrayRegion(facts,0,4*count,detail.data());
        }
        if(env->ExceptionCheck())return nullptr;
        RosterSourceSnapshot source;
        source.world_token=static_cast<std::uint64_t>(meta[0]);
        source.owner_token=static_cast<std::uint64_t>(meta[1]);
        source.generation=static_cast<std::uint64_t>(meta[2]);
        source.tick=static_cast<std::uint64_t>(meta[3]);
        source.owner_uuid={bits(meta[4]),bits(meta[5])};
        source.remote.reserve(static_cast<std::size_t>(count));
        for(jsize i=0;i<count;++i) {
            const std::size_t c=static_cast<std::size_t>(i);
            for(int j=1;j<4;++j)
                if(!bool01(detail[4*c+static_cast<std::size_t>(j)])) {
                    refuse(env,"java/lang/IllegalArgumentException",
                        "Invalid roster boolean observations");
                    return nullptr;
                }
            RosterSourcePlayer remote;
            remote.bot.uuid={bits(uuids[2*c]),bits(uuids[2*c+1])};
            jstring java_name=static_cast<jstring>(
                env->GetObjectArrayElement(names,i));
            if(env->ExceptionCheck())return nullptr;
            const bool name_ok=read_jstring(env,java_name,remote.bot.name);
            if(java_name)env->DeleteLocalRef(java_name);
            if(!name_ok) {
                refuse(env,"java/lang/IllegalArgumentException",
                    "Missing or oversized roster name");
                return nullptr;
            }
            remote.teams.raw_name=remote.bot.name;
            remote.bot.previous_x=motion[4*c];remote.bot.previous_z=motion[4*c+1];
            remote.bot.x=motion[4*c+2];remote.bot.z=motion[4*c+3];
            remote.bot.ticks_existed=detail[4*c];
            remote.bot.sleeping=detail[4*c+1]!=0;
            remote.bot.invisible_to_owner=detail[4*c+2]!=0;
            remote.bot.has_armor=detail[4*c+3]!=0;
            source.remote.push_back(std::move(remote));
        }
        AntiBotSettings anti;
        anti.always_stationary=(mask&1)!=0;
        anti.sleeping=(mask&2)!=0;
        anti.always_invis=(mask&4)!=0;
        anti.invalid_username=(mask&8)!=0;
        anti.invalid_uuid_mode=uuid_mode;
        anti.min_age=min_age;
        // Allocate before committing native state, so OOM never advances it.
        jlongArray output=env->NewLongArray(6+3*count);
        if(!output)return nullptr;
        std::vector<jlong> encoded(static_cast<std::size_t>(6+3*count),0);
        std::lock_guard<std::mutex> lock(mutex);
        const auto it=sessions.find(handle);
        if(it==sessions.end() ||
           it->second->owner!=std::this_thread::get_id()) {
            refuse(env,"java/lang/IllegalStateException","Foreign roster JNI session");
            return nullptr;
        }
        const auto proposal=it->second->host.observe(
            source,anti,TeamsSettings{});
        if(!proposal || !proposal->antibot_evaluated ||
           proposal->teams_evaluated ||
           proposal->remote.size()!=source.remote.size()) {
            refuse(env,"java/lang/IllegalStateException","Native roster rejected receipt");
            return nullptr;
        }
        encoded[0]=1;
        for(int i=0;i<4;++i)encoded[1+static_cast<std::size_t>(i)]=meta[i];
        encoded[5]=count;
        for(std::size_t i=0;i<proposal->remote.size();++i) {
            encoded[6+3*i]=rawbits(proposal->remote[i].uuid.msb);
            encoded[6+3*i+1]=rawbits(proposal->remote[i].uuid.lsb);
            encoded[6+3*i+2]=proposal->remote[i].antibot_rejected?2:0;
        }
        env->SetLongArrayRegion(output,0,6+3*count,encoded.data());
        if(env->ExceptionCheck())return nullptr;
        return output;
    }catch(...) {
        refuse(env,"java/lang/IllegalStateException","Native roster JNI processing failed");
        return nullptr;
    }
}
extern "C" JNIEXPORT void JNICALL
Java_dev_slinkywinky_RelationshipRosterNativePreview_nativeClose(
    JNIEnv* env,jclass,jlong handle) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it=sessions.find(handle);
    if(it==sessions.end() ||
       it->second->owner!=std::this_thread::get_id() ||
       !it->second->host.close()) {
        refuse(env,"java/lang/IllegalStateException","Roster JNI close not owned");
        return;
    }
    sessions.erase(it);
}
