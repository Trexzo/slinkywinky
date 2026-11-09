#include "slinkywinky/original_window_focus_trace.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using slinkywinky::OriginalWindowFocusTrace;
using slinkywinky::OriginalWindowFocusState;
namespace {
void check(bool yes,const char* why) {
    if(!yes)throw std::runtime_error(why);
}
void expect(const OriginalWindowFocusTrace& trace,
            std::array<std::uint64_t,6> value) {
    check(trace.counters()==value,"R183 wrong OS focus chronology");
    const auto v=trace.counters();
    check(v[0]+v[1]+v[2]==trace.samples(),"R183 sample conservation");
    check(v[4]<=v[3] && v[5]<=v[0],
          "R183 impossible focus restoration count");
}
}
int main() {
    using F=OriginalWindowFocusState;
    try {
        OriginalWindowFocusTrace valid;
        expect(valid,{{0,0,0,0,0,0}});
        check(valid.observe(F::Background,false,false),"Initial background");
        check(valid.observe(F::Foreground,true,true),"Initial foreground");
        expect(valid,{{1,1,0,0,0,0}});
        check(valid.observe(F::Background,false,false),"First loss");
        expect(valid,{{1,2,0,1,0,0}});
        check(valid.observe(F::Foreground,true,false),"Regain alone");
        expect(valid,{{2,2,0,1,1,0}});
        check(valid.observe(F::Foreground,true,true),"Recovered exact source");
        expect(valid,{{3,2,0,1,1,1}});
        check(valid.observe(F::Background,false,false),"Second loss");
        check(valid.observe(F::Foreground,true,true),"Second recovery exact");
        expect(valid,{{4,3,0,2,2,2}});

        // Missing pointer in a known foreground window = REFUSED,
        // never a valid background or a successful restoration.
        OriginalWindowFocusTrace failedPointer;
        failedPointer.observe(F::Foreground,true,true);
        failedPointer.observe(F::Foreground,false,false);
        failedPointer.observe(F::Foreground,true,true);
        expect(failedPointer,{{2,0,1,0,0,0}});

        // A refused/ambiguous window identity breaks the focus epoch.
        OriginalWindowFocusTrace invalidWindow;
        invalidWindow.observe(F::Foreground,true,true);
        invalidWindow.observe(F::Refused,false,false);
        invalidWindow.observe(F::Background,false,false);
        invalidWindow.observe(F::Foreground,true,true);
        expect(invalidWindow,{{2,1,1,0,0,0}});
        invalidWindow.observe(F::Background,false,false);
        invalidWindow.observe(F::Foreground,true,true);
        expect(invalidWindow,{{3,2,1,1,1,1}});

        // Background with a returned foreground pointer is a race or
        // inconsistent audit. It cannot silently be treated as a loss.
        OriginalWindowFocusTrace mismatch;
        mismatch.observe(F::Foreground,true,false);
        mismatch.observe(F::Background,true,false);
        mismatch.observe(F::Foreground,true,true);
        expect(mismatch,{{2,0,1,0,0,0}});
        mismatch.observe(F::Foreground,true,false);
        mismatch.observe(F::Refused,true,true);
        mismatch.observe(F::Foreground,true,true);
        expect(mismatch,{{4,0,2,0,0,0}});

        // No production menu pointer hook, synthetic clicks or OS focus
        // changes are exercised by this pure cross-platform fixture.
        std::cout<<"R183_EXACT_OS_FOCUS_TRI_STATE_NEGATIVES_PASS\n";
        std::cout<<"R183_REFUSALS_CANNOT_FABRICATE_REACQUISITION_PASS\n";
        std::cout<<"R183_NATIVE_CLICK_DISPATCH_REMAINS_DISABLED\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"R183_FAIL "<<error.what()<<"\n";
        return EXIT_FAILURE;
    }
}
