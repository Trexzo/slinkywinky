#include "slinkywinky/observation.hpp"
#include <stdexcept>
#define CHECK(condition) do { if (!(condition)) throw std::runtime_error("failed: " #condition); } while (false)
#include <cmath>
#include <limits>
#include <thread>

using slinkywinky::ObservationFrame;
using slinkywinky::ObservationLedger;

int main() {
    ObservationLedger ledger;
    ObservationFrame f{};
    f.time_ms=1000;
    f.generation=1;
    CHECK(ledger.can_accept(f));
    ledger.commit(f);
    auto status=ledger.snapshot();
    CHECK(status && status->valid_ticks==1 && status->flags==0 &&
           status->session_transitions==0);
    f.time_ms=1050; f.generation=2; f.flags=3;
    f.xyz={1,2,3,4,6,3};
    f.angles={40,-8};
    CHECK(ledger.can_accept(f));
    ledger.commit(f);
    status=ledger.snapshot();
    CHECK(status->valid_ticks==2 && status->session_transitions==1 &&
           status->eye_y==2 && status->yaw==40 && status->pitch==-8);
    f.time_ms=1100; f.flags=7;
    CHECK(ledger.can_accept(f));
    ledger.commit(f);
    status=ledger.snapshot();
    CHECK(status->valid_ticks==3 && status->flags==7 &&
           std::abs(status->target_distance-5.0)<1e-9);
    const auto count=status->valid_ticks;
    const auto transitions=status->session_transitions;
    ObservationFrame bad=f;
    bad.time_ms=1099;
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.generation=1;
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.flags=8;
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.flags=2;
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.flags=4;
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.flags=3; bad.xyz[3]=std::numeric_limits<double>::quiet_NaN();
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.xyz[0]=std::numeric_limits<double>::infinity();
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.angles[1]=std::numeric_limits<float>::infinity();
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.xyz={-1e308,2,3,1e308,4,5};
    CHECK(!ledger.can_accept(bad));
    bad=f; bad.flags=1; // World/player identity must change generation.
    CHECK(!ledger.can_accept(bad));
    std::thread wrong([&] {
        CHECK(!ledger.can_accept(f));
        CHECK(!ledger.snapshot().has_value());
    });
    wrong.join();
    status=ledger.snapshot();
    CHECK(status->valid_ticks==count &&
           status->session_transitions==transitions);
    f.flags=0; f.generation=3; f.time_ms=1200;
    CHECK(ledger.can_accept(f));
    ledger.commit(f);
    status=ledger.snapshot();
    CHECK(status->valid_ticks==4 && status->session_transitions==2 &&
           status->target_distance==0.0);
    f.flags=3; f.generation=4; f.time_ms=1250;
    CHECK(ledger.can_accept(f));
    ledger.commit(f);
    status=ledger.snapshot();
    CHECK(status->valid_ticks==5 && status->session_transitions==3);
    return 0;
}
