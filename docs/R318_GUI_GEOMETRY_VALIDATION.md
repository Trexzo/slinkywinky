# R318 — profile and modal geometry validation

Base a76df2dac3564ad274930f4e329f72394e5bc24c. Profile preview and modal geometry previously lacked finite-dimension gates. Direct callers now reject non-finite and non-positive dimensions, and unload confirmation geometry rejects non-finite dimensions before calculating its rectangle. Containment rejects invalid and overflowing rectangles. Icon hit admission shares the visual rectangle implementation; Default Save/Delete remain disallowed. Tooltip clamp bounds are ordered explicitly.

Extended the existing profile tooltip CTest with invalid-dimension, invalid-rectangle, edge, modal and visual/action identity assertions in all three panel modes. These are first-party UI robustness changes only, not additional original gameplay reconstruction. No live input routing or external actions introduced. R317 CI was still running when this commit was prepared; R318 tests have not been run locally. No module status promotion.
