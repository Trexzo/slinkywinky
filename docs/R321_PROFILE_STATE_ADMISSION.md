# R321 — profile state admission and invalid tab rejection

Base 53aa540ed4060f929b44829116fb8bf25034c2d2. MenuTab has eight declared values; arbitrary underlying uint8 values beyond Count were formerly admitted by MenuState::select. Reject all values beyond Profiles before mutating state, and repeat the bound at the render publication and profile geometry boundary. Profile snapshots now reject module-only interactions, conflicting name focus and unload confirmation, contradictory duplicate-name state, and noncanonical space-padded card names. These checks reflect the existing first-party state producer rather than invented original commercial behavior.

Added tests for all 248 undeclared tab values and ten malformed profile-frame variants in each of the three supported panel modes. Existing valid profile interactions remain exercised. These changes are offline UI validation only; no native loader, live input or gameplay behavior changes.

R318 standalone-native-loader check failed at r160_java8_mouse_owner_no_game_hook: Held-release drain not completed; 109 of 110 tests passed. This historical failure is not classified as a flake or fixed by R321. Local execution not performed; CI pending. No module coverage promotion.
