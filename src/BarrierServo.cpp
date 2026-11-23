#include "BarrierServo.h"
#include "config.h" //cambiar por config.example.h
namespace {
int pulseForAngle(int angle) {
    return SERVO_MIN_US + (SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
}
}
bool BarrierServo::begin() {
    prepared_ = true;
    return true;
}
bool BarrierServo::command(State state) {
    if (!prepared_ || (initialized_ && state == commanded_)) return false;
    if (!initialized_) {
        servo_.setPeriodHertz(SERVO_FREQUENCY_HZ);
        servo_.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
        initialized_ = servo_.attached();
        if (!initialized_) return false;
    }
    servo_.writeMicroseconds(pulseForAngle(state == State::Open ? BARRIER_OPEN_ANGLE : BARRIER_CLOSED_ANGLE));
    commanded_ = state;
    commandTime_ = millis();
    moving_ = true;
    return true;
}
bool BarrierServo::open() { return command(State::Open); }
bool BarrierServo::close() { return command(State::Closed); }
void BarrierServo::update() {
    if (moving_ && static_cast<uint32_t>(millis() - commandTime_) >= SERVO_TRAVEL_MS)
        moving_ = false;
}
