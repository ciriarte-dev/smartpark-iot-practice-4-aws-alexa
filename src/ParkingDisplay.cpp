#include "ParkingDisplay.h"
#include "config.h"
#include <Wire.h>
#include <stdio.h>
#include <string.h>

void ParkingDisplay::begin() {
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN, 100000);
    Wire.setTimeOut(5);
    lastStep_ = millis();
}

uint8_t ParkingDisplay::probe() {
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    return Wire.endTransmission();
}

bool ParkingDisplay::nibble(uint8_t value, bool data) {
    // PCF8574: P0=RS, P1=RW (0), P2=EN, P3=luz, P4..7=D4..7.
    const uint8_t pins = (value << 4) | 0x08 | (data ? 1 : 0);
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    Wire.write(pins);
    Wire.write(pins | 0x04);
    Wire.write(pins);
    return Wire.endTransmission() == 0;
}

bool ParkingDisplay::byte(uint8_t value, bool data) {
    return nibble(value >> 4, data) && nibble(value & 0x0f, data);
}

void ParkingDisplay::update(uint32_t now, const Parking::State& parking, const BarrierServo& barrier) {
    if (now - lastStep_ < (init_ == Init::Wait ? 100u : 5u)) return;
    lastStep_ = now;
    bool ok = false;
    switch (init_) {
        case Init::Wait:
        case Init::WakeTwo:
        case Init::WakeThree:
            ok = nibble(3);
            break;
        case Init::FourBit:
            ok = nibble(2);
            break;
        case Init::Configure:
            ok = byte(0x28) && byte(0x08) && byte(0x06) && byte(0x0c) && byte(0x01);
            break;
        case Init::Ready: {
            char line[21];
            const char* barrierState = !barrier.initialized() ? "sin estimar" :
                (barrier.moving() ? "moviendo" :
                    (barrier.commandedState() == BarrierServo::State::Open ? "abierta*" : "cerrada*"));
            switch (nextRow_) {
                case 0: snprintf(line, sizeof(line), "Ocup:%u Libres:%u/%u", unsigned(parking.occupied), unsigned(parking.available), unsigned(Parking::capacity)); break;
                case 1: snprintf(line, sizeof(line), "Modo: %s", parkingMode(parking.mode)); break;
                case 2: snprintf(line, sizeof(line), "Barrera:%s", barrierState); break;
                default: snprintf(line, sizeof(line), "%s", parking.full() ? "PARQUEO LLENO" :
                    (parking.openingBlocked() ? "APERTURA BLOQUEADA" : "SmartPark")); break;
            }
            const size_t length = strlen(line);
            memset(line + length, ' ', 20 - length);
            line[20] = '\0';
            if (!memcmp(line, displayed_[nextRow_], sizeof(line))) {
                nextRow_ = (nextRow_ + 1) % 4;
                return;
            }
            constexpr uint8_t addresses[] = {0x00, 0x40, 0x14, 0x54};
            ok = byte(0x80 | addresses[nextRow_]);
            for (uint8_t i = 0; ok && i < 20; ++i) ok = byte(line[i], true);
            if (ok) {
                memcpy(displayed_[nextRow_], line, sizeof(line));
                nextRow_ = (nextRow_ + 1) % 4;
                return;
            }
            break;
        }
    }
    if (ok) init_ = static_cast<Init>(static_cast<uint8_t>(init_) + 1);
    else {
        init_ = Init::Wait;
        memset(displayed_, 0, sizeof(displayed_));
    }
}
