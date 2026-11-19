#include <Arduino.h>
#include "config.h"
#include "UltrasonicSensor.h"
#include "BarrierServo.h"
#include "SerialCommands.h"
#include "AwsIoTManager.h"
#include "ParkingDisplay.h"
namespace {
UltrasonicSensor sensor;
BarrierServo barrier;
SerialCommands commands;
AwsIoTManager aws;
Parking parking;
ParkingDisplay display;
uint32_t lastAwsStatusMs = 0;
uint32_t lastSampleMs = 0;
void printBarrier() {
    if (!barrier.initialized()) { Serial.println("Barrera: sin posicion estimada"); return; }
    Serial.print("Barrera ordenada: ");
    Serial.println(barrier.commandedState() == BarrierServo::State::Open ? "ABIERTA" : "CERRADA");
    Serial.println(barrier.moving() ? "Movimiento: en curso (estimado)" : "Posicion: estimada por tiempo, sin confirmacion fisica");
}
void printDistance() {
    const auto& reading = sensor.lastMeasurement();
    switch (reading.status) {
    case UltrasonicSensor::ReadingStatus::Valid:
        Serial.print("Distancia: "); Serial.print(reading.distanceCm, 1); Serial.println(" cm"); break;
    case UltrasonicSensor::ReadingStatus::Timeout: Serial.println("Distancia: invalida (timeout)"); break;
    case UltrasonicSensor::ReadingStatus::OutOfRange: Serial.println("Distancia: invalida (fuera de rango)"); break;
    default: Serial.println("Distancia: sin medicion"); break;
    }
    if (reading.status != UltrasonicSensor::ReadingStatus::NotMeasured) {
        Serial.print("Edad lectura (ms): "); Serial.println(static_cast<uint32_t>(millis() - reading.timestampMs));
    }
    Serial.print("Vehiculo detectado: ");
    switch (sensor.presence()) {
    case UltrasonicSensor::Presence::Present: Serial.println("SI"); break;
    case UltrasonicSensor::Presence::Absent: Serial.println("NO"); break;
    default: Serial.println("DESCONOCIDO"); break;
    }
}
void printSlot1() {
    const auto& pins = SLOT_PINS[0];
    constexpr const char* names[] = {"DESCONOCIDO", "LIBRE", "OCUPADO"};
    const auto state = parking.state().slots[0];
    Serial.printf("Slot 1: IR GPIO %u=%d (activo=%u); estado=%s; rojo GPIO %u=%s; verde GPIO %u=%s\n",
        unsigned(pins.ir), digitalRead(pins.ir), unsigned(IR_OCCUPIED_LEVEL),
        names[static_cast<uint8_t>(state)], unsigned(pins.red),
        state == Parking::Slot::Occupied ? "ON" : "OFF", unsigned(pins.green),
        state == Parking::Slot::Free ? "ON" : "OFF");
    const auto error = display.probe();
    Serial.printf("LCD 0x%02X: %s (I2C=%u)\n", unsigned(LCD_I2C_ADDRESS),
        error ? "error I2C" : (display.initialized() ? "inicializada; responde" : "responde; inicializando"),
        unsigned(error));
}
void execute(SerialCommands::Command cmd) {
    using C = SerialCommands::Command;
    switch (cmd) {
    case C::Open:
    case C::Close: {
        const bool open = cmd == C::Open;
        if (open && parking.state().openingBlocked()) {
            aws.noteSerialCommand(open, true);
            Serial.println("Apertura bloqueada: sin plazas confirmadas");
        } else {
            const bool moved = open ? barrier.open() : barrier.close();
            aws.noteSerialCommand(open, false, barrier.initialized(), moved);
        }
        printBarrier();
        break;
    }
    case C::Status: printDistance(); printBarrier(); aws.printStatus(); break;
    case C::Distance: printDistance(); break;
    case C::Slot1: printSlot1(); break;
    case C::Help: Serial.println("Comandos: OPEN, CLOSE, STATUS, DISTANCE, SLOT1, HELP (terminar con CR o LF)"); break;
    case C::Invalid: Serial.println("Comando invalido o demasiado largo. Use HELP."); break;
    case C::None: break;
    }
}
}
void setup() {
    Serial.begin(SERIAL_BAUD);
    sensor.begin();
    barrier.begin();
    parking.begin();
    display.begin();
    if (!aws.begin(parking)) Serial.println("AWS: no se pudo iniciar tarea; modo local disponible");
    lastSampleMs = millis();
    Serial.println("=== SmartPark V2 - ESP32 ===");
    Serial.println("Sistema iniciado. HC-SR04: pines configurados, funcionamiento no verificado.");
    printBarrier();
    execute(SerialCommands::Command::Help);
}
void loop() {
    barrier.update();
    aws.observeBarrier(barrier);
    parking.update(millis());
    // One explicit local barrier request per loop, prioritizing the newest.
    using C = SerialCommands::Command;
    C serialBarrier = C::None;
    for (size_t i = 0; i < SERIAL_BYTES_PER_LOOP && Serial.available() > 0; ++i) {
        const auto command = commands.feed(static_cast<char>(Serial.read()));
        if (command == C::Open || command == C::Close) serialBarrier = command;
        else execute(command);
    }
    aws.applyPending(barrier, parking, serialBarrier != C::None);
    execute(serialBarrier);
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastSampleMs) >= SENSOR_INTERVAL_MS) {
        lastSampleMs = now;
        sensor.measure();
        printDistance();
    }
    aws.tick(barrier, sensor, parking);
    display.update(millis(), parking.state(), barrier);
    if (static_cast<uint32_t>(now-lastAwsStatusMs) >= AWS_STATUS_INTERVAL_MS) {
        lastAwsStatusMs = now; aws.printStatus();
    }
}
