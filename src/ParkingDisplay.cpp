#include "ParkingDisplay.h"
#include "config.h"
#include <Wire.h>
#include <stdio.h>
#include <string.h>

// Fila 1 sin aviso: alterna entre el modo y la barrera.
constexpr uint32_t LCD_ALTERNATE_MS = 2000;

void ParkingDisplay::begin() {
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN, 100000);
    Wire.setTimeOut(5);
    reinitialize(millis());
}

void ParkingDisplay::reinitialize(uint32_t now) {
    init_ = Init::Wait;
    lastStep_ = now;
    nextRow_ = 0;
    memset(displayed_, 0, sizeof(displayed_));
}

uint8_t ParkingDisplay::probe() {
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    return Wire.endTransmission();
}

bool ParkingDisplay::prepareExpander() {
    // El PCF8574 arranca con sus salidas altas. Fijar E, RS y R/W en bajo
    // antes del primer pulso y dejar que termine cualquier instruccion previa.
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    Wire.write(0x08);
    return Wire.endTransmission() == 0;
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
            ok = prepareExpander();
            break;
        case Init::WakeOne:
        case Init::WakeTwo:
        case Init::WakeThree:
            ok = nibble(3);
            break;
        case Init::FourBit:
            ok = nibble(2);
            break;
        case Init::Configure:
            ok = byte(0x28) && byte(0x0c) && byte(0x01);
            break;
        case Init::EntryMode:
            // La espera de 5 ms entre pasos deja terminar Clear Display.
            ok = byte(0x06);
            break;
        case Init::Ready: {
            // Holgura para tres %u de 10 digitos: snprintf nunca trunca; la fila se recorta despues.
            char text[48];
            const char* barrierState = !barrier.initialized() ? "---" :
                (barrier.moving() ? "mueve" :
                    (barrier.commandedState() == BarrierServo::State::Open ? "abierta" : "cerrada"));
            if (nextRow_ == 0) {
                snprintf(text, sizeof(text), "Libres:%u/%u Oc:%u", unsigned(parking.available),
                         unsigned(Parking::capacity), unsigned(parking.occupied));
            } else if (parking.openingBlocked()) {
                snprintf(text, sizeof(text), "%s", "APERTURA BLOQ.");
            } else if (parking.full()) {
                snprintf(text, sizeof(text), "%s", "PARQUEO LLENO");
            } else if ((now / LCD_ALTERNATE_MS) % 2 == 0) {
                snprintf(text, sizeof(text), "Modo: %s", parkingMode(parking.mode));
            } else {
                snprintf(text, sizeof(text), "Barrera: %s", barrierState);
            }
            char line[LCD_COLUMNS + 1];
            size_t length = strlen(text);
            if (length > LCD_COLUMNS) length = LCD_COLUMNS;
            memcpy(line, text, length);
            memset(line + length, ' ', LCD_COLUMNS - length);
            line[LCD_COLUMNS] = '\0';
            if (!memcmp(line, displayed_[nextRow_], sizeof(line))) {
                nextRow_ = (nextRow_ + 1) % LCD_ROWS;
                return;
            }
            // Direcciones DDRAM del HD44780 para 2 filas.
            constexpr uint8_t addresses[] = {0x00, 0x40};
            ok = byte(0x80 | addresses[nextRow_]);
            for (uint8_t i = 0; ok && i < LCD_COLUMNS; ++i) ok = byte(line[i], true);
            if (ok) {
                memcpy(displayed_[nextRow_], line, sizeof(line));
                nextRow_ = (nextRow_ + 1) % LCD_ROWS;
                return;
            }
            break;
        }
    }
    if (ok) init_ = static_cast<Init>(static_cast<uint8_t>(init_) + 1);
    else reinitialize(now);
}
