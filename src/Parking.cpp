#include "Parking.h"
#include "config.h" 

static_assert(Parking::capacity >= 1 && Parking::capacity <= sizeof(SLOT_PINS) / sizeof(SLOT_PINS[0]), "Mapa de plazas incompleto");

void Parking::begin() {
    state_.debounceMs = IR_DEBOUNCE_DEFAULT_MS;
    for (uint8_t i = 0; i < capacity; ++i) {
        const auto& pins = SLOT_PINS[i];
        pinMode(pins.ir, INPUT);
        digitalWrite(pins.red, LOW);
        digitalWrite(pins.green, LOW);
        pinMode(pins.red, OUTPUT);
        pinMode(pins.green, OUTPUT);
    }
}

void Parking::configure(Mode mode, uint16_t debounceMs) {
    state_.mode = mode;
    if (debounceMs != state_.debounceMs) {
        state_.debounceMs = debounceMs;
        // Start a new stability window after a configuration change.
        for (auto& candidate : candidate_) candidate = Slot::Unknown;
    }
}

void Parking::update(uint32_t now) {
    if (now - sampledAt_ < IR_SAMPLE_MS) return;
    const bool interrupted = now - sampledAt_ > IR_SAMPLE_MS * 3;
    sampledAt_ = now;
    for (uint8_t i = 0; i < capacity; ++i) {
        const auto& pins = SLOT_PINS[i];
        const int value = digitalRead(pins.ir);
        const Slot reading = value != LOW && value != HIGH ? Slot::Unknown :
            (value == IR_OCCUPIED_LEVEL ? Slot::Occupied : Slot::Free);
        if (interrupted || reading != candidate_[i]) {
            candidate_[i] = reading;
            candidateSince_[i] = now;
        }
        auto& state = state_.slots[i];
        if (reading == state || (reading != Slot::Unknown && now - candidateSince_[i] < state_.debounceMs)) continue;
        state_.occupied -= state == Slot::Occupied;
        state_.available -= state == Slot::Free;
        state = reading;
        state_.occupied += state == Slot::Occupied;
        state_.available += state == Slot::Free;
        digitalWrite(pins.red, LOW);
        digitalWrite(pins.green, LOW);
        if (state == Slot::Occupied) digitalWrite(pins.red, HIGH);
        if (state == Slot::Free) digitalWrite(pins.green, HIGH);
    }
}
