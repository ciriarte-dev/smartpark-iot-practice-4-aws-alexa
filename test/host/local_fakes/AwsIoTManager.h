#pragma once
#include "BarrierServo.h"
#include "Parking.h"
#include "UltrasonicSensor.h"
// Integracion local: no inicia red en las pruebas del loop.
class AwsIoTManager {
public:
    bool begin(Parking&) { return true; }
    void applyPending(BarrierServo&, Parking&, bool) {}
    void tick(const BarrierServo&, const UltrasonicSensor&, const Parking&) {}
    void noteSerialCommand(bool, bool, bool = true, bool = false) {}
    void observeBarrier(const BarrierServo&) {}
    void printStatus() {}
};
