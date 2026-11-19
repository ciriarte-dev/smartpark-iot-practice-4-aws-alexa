#pragma once
#include "config.h"

class Parking {
public:
    static constexpr uint8_t capacity = PARKING_CAPACITY;
    enum class Slot : uint8_t { Unknown, Free, Occupied };
    enum class Mode : uint8_t { Manual, Automatic };
    struct State {
        Slot slots[capacity]{};
        Mode mode = Mode::Manual;
        uint16_t debounceMs = 120;
        uint8_t occupied = 0, available = 0;
        bool full() const { return occupied == capacity; }
        bool openingBlocked() const { return mode == Mode::Automatic && available == 0; }
    };
    void begin();
    void update(uint32_t now);
    void configure(Mode mode, uint16_t debounceMs);
    const State& state() const { return state_; }
private:
    State state_{};
    Slot candidate_[capacity]{};
    uint32_t candidateSince_[capacity]{};
    uint32_t sampledAt_ = 0;
};

inline const char* parkingMode(Parking::Mode mode) {
    return mode == Parking::Mode::Automatic ? "automatico" : "manual";
}
