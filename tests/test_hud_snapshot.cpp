#include "slinkywinky/hud_snapshot.hpp"
#include "slinkywinky/hud.hpp"
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#define CHECK(c) do {if(!(c))throw std::runtime_error("snapshot test failed: " #c);} while(false)

using slinkywinky::HudSnapshotMailbox;
using slinkywinky::ObservationStatus;
int main() {
    HudSnapshotMailbox mailbox;
    CHECK(!mailbox.read().has_value());
    ObservationStatus s{};
    s.valid_ticks=1;s.generation=1;s.time_ms=1000;
    CHECK(mailbox.publish(s));
    auto latest=mailbox.read();
    CHECK(latest && latest->sequence==1 &&
          latest->status.valid_ticks==1 && latest->status.flags==0);
    CHECK(!mailbox.publish(s)); // No duplicate or rolled-back native tick.
    s.valid_ticks=2;s.generation=2;s.time_ms=1050;s.flags=3;
    s.eye_y=65.62;s.yaw=42;s.pitch=-8;
    CHECK(mailbox.publish(s));
    s.valid_ticks=3;s.time_ms=1100;s.flags=7;s.target_distance=3.75;
    CHECK(mailbox.publish(s));
    latest=mailbox.read();
    CHECK(latest && latest->sequence==3 &&
          latest->status.flags==7 && latest->status.target_distance==3.75);
    auto panel=slinkywinky::make_hud_panel(latest->status);
    CHECK(panel.has_data && panel.has_ray_target &&
          panel.state=="RAY TARGET OBSERVED");
    CHECK(panel.rows.back().value=="3.75 blocks");

    ObservationStatus bad=s;
    bad.flags=2;CHECK(!mailbox.publish(bad));
    bad=s;bad.flags=4;CHECK(!mailbox.publish(bad));
    bad=s;bad.target_distance=std::numeric_limits<double>::quiet_NaN();
    CHECK(!mailbox.publish(bad));
    bad=s;bad.time_ms=100;CHECK(!mailbox.publish(bad));
    bad=s;bad.generation=1;CHECK(!mailbox.publish(bad));
    CHECK(mailbox.read()->sequence==3); // Invalid input never replaces a good snapshot.

    std::atomic<bool> go{true};
    std::atomic<bool> broken{false};
    std::atomic<int> reads{0};
    std::vector<std::thread> readers;
    for(int i=0;i<4;++i) {
        readers.emplace_back([&]{
            std::uint64_t previous=0;
            while(go.load()) {
                auto copy=mailbox.read();
                if(!copy) {broken=true;break;}
                if(copy->sequence<previous ||
                   copy->status.valid_ticks!=copy->sequence ||
                   (copy->status.flags==7 && copy->status.target_distance<=0))
                    broken=true;
                previous=copy->sequence;
                ++reads;
            }
        });
    }
    std::atomic<bool> foreignPublish{true};
    std::thread foreign([&]{ foreignPublish=mailbox.publish(s); });
    foreign.join();
    CHECK(!foreignPublish.load());
    for(int i=4;i<=3000;++i) {
        s.valid_ticks=static_cast<std::uint64_t>(i);
        s.time_ms=1100+static_cast<std::uint64_t>(i-3)*50;
        s.generation=2;
        CHECK(mailbox.publish(s));
    }
    go=false;
    for(auto& reader:readers) reader.join();
    CHECK(!broken.load());
    CHECK(reads.load()>0);
    latest=mailbox.read();
    CHECK(latest && latest->sequence==3000 &&
          latest->status.valid_ticks==3000);
    CHECK(mailbox.revoke());
    CHECK(!mailbox.read().has_value());
    CHECK(!mailbox.revoke());
    CHECK(!mailbox.publish(s));
    std::thread after([&]{CHECK(!mailbox.read().has_value());});
    after.join();
    return 0;
}
