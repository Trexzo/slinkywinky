#pragma once
#ifdef _WIN32
namespace slinkywinky {
/**
 * R95: purely deterministic edge state for the native Insert hotkey.
 *
 * No key reading or Windows input side effects. A focused key-up sample is
 * mandatory after admission or after every foreground loss. Consequently
 * holding Insert in another window and switching to the game cannot toggle.
 * This object is owner-thread confined by the caller's WGL admission.
 */
class VisibilityInsertEdge final {
public:
    bool sample(bool owner_foreground,bool insert_down) noexcept {
        if(!owner_foreground) {
            armed_=false;
            last_down_=true;
            return false;
        }
        if(!armed_) {
            if(!insert_down) {
                armed_=true;
                last_down_=false;
            } else {
                last_down_=true;
            }
            return false;
        }
        const bool rising=insert_down && !last_down_;
        last_down_=insert_down;
        return rising;
    }
    void reset() noexcept {
        armed_=false;
        last_down_=true;
    }
    bool armed() const noexcept {return armed_;}
private:
    bool armed_=false;
    bool last_down_=true;
};
} // namespace slinkywinky
#endif
