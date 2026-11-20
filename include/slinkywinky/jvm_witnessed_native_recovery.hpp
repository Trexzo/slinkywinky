#pragma once

#include "slinkywinky/native_module_recovery_fence.hpp"
#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace slinkywinky {

// R252. VALUE-ONLY R251 fixture identity, not a trusted OS attestation.
// The challenge is not a secret once exported via fixture stdout.
struct JvmLeaseIdentity {
    std::uint64_t pid=0;
    std::uint64_t jvm_start_ms=0;
    std::uint64_t lease_epoch=0;
    std::string challenge_hex;

    bool well_formed() const noexcept {
        if(!pid || !jvm_start_ms || !lease_epoch ||
           challenge_hex.size()!=32)return false;
        bool any_nonzero=false;
        for(char c:challenge_hex){
            if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))
                return false;
            if(c!='0')any_nonzero=true;
        }
        return any_nonzero;
    }
    bool operator==(const JvmLeaseIdentity& rhs) const noexcept {
        return pid==rhs.pid && jvm_start_ms==rhs.jvm_start_ms &&
               lease_epoch==rhs.lease_epoch &&
               challenge_hex==rhs.challenge_hex;
    }
};

// Exact 5-token cross-language fixture format. Strictly reject overflowing
// integers, extra fields, upper-case/zero nonce and malformed whitespace.
inline std::optional<JvmLeaseIdentity> parse_r252_fixture_receipt(
        std::string_view line) {
    constexpr std::string_view prefix="R252_LEASE_RECEIPT";
    std::array<std::string_view,5> parts{};
    std::size_t pos=0;
    for(std::size_t i=0;i<parts.size();++i){
        if(pos>=line.size())return std::nullopt;
        auto next=line.find(' ',pos);
        if(i==parts.size()-1 && next!=std::string_view::npos)
            return std::nullopt;
        if(next==pos)return std::nullopt;
        parts[i]=line.substr(pos,next==std::string_view::npos?
            line.size()-pos:next-pos);
        if(i<parts.size()-1 && next==std::string_view::npos)
            return std::nullopt;
        pos=next==std::string_view::npos?line.size():next+1;
    }
    if(parts[0]!=prefix)return std::nullopt;
    auto parse_uint=[](std::string_view value,
                       std::uint64_t& result)->bool {
        if(value.empty())return false;
        for(char c:value)if(c<'0'||c>'9')return false;
        const auto from=std::from_chars(
            value.data(),value.data()+value.size(),result);
        return from.ec==std::errc{} &&
               from.ptr==value.data()+value.size();
    };
    JvmLeaseIdentity out;
    if(!parse_uint(parts[1],out.pid) ||
       !parse_uint(parts[2],out.jvm_start_ms) ||
       !parse_uint(parts[3],out.lease_epoch))
        return std::nullopt;
    out.challenge_hex=std::string(parts[4]);
    if(!out.well_formed())return std::nullopt;
    return out;
}

// The WITNESS must hold an exclusive live attachment across invocation of
// callback and return bool; an identity string alone cannot authorize
// recovery. A future trusted OS/JVM implementation MUST supply this witness.
// A test fixture witness supplies controlled evidence in R252 regression.
class JvmWitnessedNativeRecovery final {
public:
    JvmWitnessedNativeRecovery()
      : owner_(std::this_thread::get_id()) {}
    JvmWitnessedNativeRecovery(const JvmWitnessedNativeRecovery&)=delete;
    JvmWitnessedNativeRecovery& operator=(
        const JvmWitnessedNativeRecovery&)=delete;

    template<typename NativeOwner,typename ExclusiveLeaseWitness>
    bool admit_on_owner(
        ExclusiveLeaseWitness& witness,
        const JvmLeaseIdentity& old_identity,
        const JvmLeaseIdentity& next_identity,
        const NativeOwner& old_native,
        const NativeModuleSnapshotAdapter& old_adapter,
        const ModuleArrayListFeed& old_feed,
        const GuiModuleControlHandoff& old_gui,
        const NativeOwner& next_native,
        NativeModuleSnapshotAdapter& next_adapter,
        ModuleArrayListFeed& next_feed,
        std::uint64_t next_sequence,bool world_ready) {
        if(std::this_thread::get_id()!=owner_ || used_ || poisoned_ ||
           !old_identity.well_formed() ||
           !next_identity.well_formed() ||
           old_identity==next_identity ||
           old_identity.challenge_hex==next_identity.challenge_hex)
            return false;

        // A lease can rotate in the same JVM only when the R251 epoch
        // actually increases. A distinct JVM incarnation can start
        // again at epoch=1; the native world/sequence guards remain.
        const bool same_jvm=
            old_identity.pid==next_identity.pid &&
            old_identity.jvm_start_ms==next_identity.jvm_start_ms;
        if(same_jvm && next_identity.lease_epoch<=
                       old_identity.lease_epoch)return false;
        if(old_identity.pid==next_identity.pid &&
           old_identity.jvm_start_ms>next_identity.jvm_start_ms)
            return false;  // a reused PID cannot travel backwards in time

        bool called=false,published=false;
        try {
            const bool witnessed=witness.with_exclusive_lease(
                old_identity,next_identity,[&]() -> bool {
                    if(called)return false;  // one recovery attempt per lease
                    called=true;
                    published=NativeModuleRecoveryFence::admit_after_quiescence(
                        old_native,old_adapter,old_feed,old_gui,
                        next_native,next_adapter,next_feed,
                        next_sequence,world_ready);
                    return published;
                });
            if(!witnessed || !called || !published){
                if(called){
                    // Witness revoked midway or gave an ambiguous result.
                    // A source snapshot may already have been published:
                    // clear it and never retry using these objects.
                    poisoned_=true;
                    next_adapter.retire_owner(next_feed);
                }
                return false;
            }
            used_=true;
            return true;
        } catch(...) {
            if(called){
                poisoned_=true;
                next_adapter.retire_owner(next_feed);
            }
            return false;
        }
    }

    bool used() const noexcept {return used_;}
    bool poisoned() const noexcept {return poisoned_;}
private:
    const std::thread::id owner_;
    bool used_=false;
    bool poisoned_=false;
};

} // namespace slinkywinky
