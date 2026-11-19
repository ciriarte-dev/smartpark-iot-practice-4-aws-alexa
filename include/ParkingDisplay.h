#pragma once
#include "Parking.h"
#include "BarrierServo.h"

class ParkingDisplay {
public:
    void begin();
    uint8_t probe();
    bool initialized() const { return init_ == Init::Ready; }
    void update(uint32_t now, const Parking::State& parking, const BarrierServo& barrier);
private:
    enum class Init : uint8_t { Wait, WakeTwo, WakeThree, FourBit, Configure, Ready };
    Init init_ = Init::Wait;
    uint32_t lastStep_ = 0;
    uint8_t nextRow_ = 0;
    char displayed_[4][21]{};
    bool nibble(uint8_t value, bool data = false);
    bool byte(uint8_t value, bool data = false);
};
