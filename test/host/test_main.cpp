#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "UltrasonicSensor.h"
#include "BarrierServo.h"
#include "SerialCommands.h"
#include "ParkingDisplay.h"
#include <Wire.h>
void setup();
void loop();
// Pantalla HD44780 reconstruida a partir de las tramas I2C (un nibble por trama, RS en P0).
struct LcdScreen {
    std::string rows[LCD_ROWS];
    bool valid = true; // Solo direcciones 0x00/0x40 y ninguna fila de mas de LCD_COLUMNS.
};

static LcdScreen lcdScreen() {
    constexpr size_t initFrames = 4 + 2 * 5; // 3, 3, 3, 2 y luego 0x28 0x08 0x06 0x0c 0x01.
    LcdScreen screen;
    for (auto& row : screen.rows) row.assign(LCD_COLUMNS, ' ');
    const auto& frames = Wire.transmissions;
    auto isData = [&](size_t i) { return (frames[i][0] & 1) != 0; };
    auto nibble = [&](size_t i) { return static_cast<uint8_t>(frames[i][0] >> 4); };
    int row = -1;
    size_t column = 0;
    for (size_t i = 0; i < frames.size();) {
        size_t end = i;
        while (end < frames.size() && isData(end) == isData(i)) ++end;
        if (!isData(i)) {
            size_t count = end - i;
            if (count >= initFrames) {
                for (auto& text : screen.rows) text.assign(LCD_COLUMNS, ' ');
                row = -1;
                i += initFrames;
                count -= initFrames;
            }
            if (count == 2) {
                const uint8_t command = static_cast<uint8_t>((nibble(i) << 4) | nibble(i + 1));
                row = command == 0x80 ? 0 : (command == 0xc0 ? 1 : -1);
                column = 0;
                if (row < 0) screen.valid = false;
            } else if (count != 0) {
                screen.valid = false;
            }
        } else {
            if ((end - i) % 2 || row < 0) screen.valid = false;
            for (size_t j = i; row >= 0 && j + 1 < end; j += 2) {
                if (column >= LCD_COLUMNS) {
                    screen.valid = false;
                    break;
                }
                screen.rows[row][column++] = static_cast<char>((nibble(j) << 4) | nibble(j + 1));
            }
        }
        i = end;
    }
    return screen;
}

static std::string padded(const std::string& text) {
    assert(text.size() <= LCD_COLUMNS);
    return text + std::string(LCD_COLUMNS - text.size(), ' ');
}

static void assertRows(const std::string& first, const std::string& second) {
    const auto screen = lcdScreen();
    assert(screen.valid && screen.rows[0] == padded(first) && screen.rows[1] == padded(second));
}

static void testParkingAndDisplay() {
    Fake::now = 0;
    Parking parking;
    parking.begin();
    ParkingDisplay display;
    display.begin();
    BarrierServo barrier;
    barrier.begin();
    const auto writes = Fake::writes;
    assert(Wire.sda == LCD_SDA_PIN && Wire.scl == LCD_SCL_PIN);
    assert(!parking.state().full() && parking.state().available == 0);
    for (const auto& pin : SLOT_PINS) {
        assert(Fake::modes[pin.ir] == INPUT);
        assert(Fake::levels[pin.red] == LOW && Fake::levels[pin.green] == LOW);
        Fake::levels[pin.ir] = !IR_OCCUPIED_LEVEL;
    }
    auto advance = [&](uint32_t duration) {
        for (uint32_t elapsed = 0; elapsed < duration; elapsed += IR_SAMPLE_MS) {
            Fake::now += IR_SAMPLE_MS;
            parking.update(Fake::now);
            display.update(Fake::now, parking.state(), barrier);
        }
    };
    // Fila 1 sin aviso: modo en las ventanas pares de 2 s y barrera en las impares.
    constexpr uint32_t alternateMs = 2000;
    auto modeWindow = [&] { return (Fake::now / alternateMs) % 2 == 0; };
    auto startWindow = [&](bool mode) {
        while (modeWindow() == mode) advance(IR_SAMPLE_MS);
        while (modeWindow() != mode) advance(IR_SAMPLE_MS);
        advance(3 * IR_SAMPLE_MS); // Una fila por llamada: ambas filas quedan escritas.
    };
    advance(IR_DEBOUNCE_DEFAULT_MS);
    assert(parking.state().available == 0);
    advance(IR_SAMPLE_MS);
    assert(parking.state().available == 4 && parking.state().occupied == 0);
    startWindow(true);
    assertRows("Libres:4/4 Oc:0", "Modo: manual");
    while ((Fake::now + 2 * IR_SAMPLE_MS) % alternateMs != 0) advance(IR_SAMPLE_MS);
    assertRows("Libres:4/4 Oc:0", "Modo: manual"); // 20 ms antes del cambio de ventana.
    advance(4 * IR_SAMPLE_MS);
    assertRows("Libres:4/4 Oc:0", "Barrera: ---"); // Servo sin posicion estimada.
    for (uint8_t i = 0; i < Parking::capacity; ++i) {
        const auto& pin = SLOT_PINS[i];
        assert(Fake::levels[pin.green] == HIGH && Fake::levels[pin.red] == LOW);
        for (unsigned j = 0; j < 6; ++j) {
            Fake::levels[pin.ir] = IR_OCCUPIED_LEVEL; advance(IR_SAMPLE_MS);
            Fake::levels[pin.ir] = !IR_OCCUPIED_LEVEL; advance(IR_SAMPLE_MS);
        }
        assert(parking.state().occupied == i);
        Fake::levels[pin.ir] = IR_OCCUPIED_LEVEL;
        advance(IR_DEBOUNCE_DEFAULT_MS + IR_SAMPLE_MS);
        assert(parking.state().occupied == i + 1 && parking.state().available == 3 - i);
        assert(Fake::levels[pin.red] == HIGH && Fake::levels[pin.green] == LOW);
    }
    assert(parking.state().full() && !parking.state().openingBlocked());
    // Lleno en manual: el aviso reemplaza la alternancia en ambas ventanas.
    startWindow(true);
    assertRows("Libres:0/4 Oc:4", "PARQUEO LLENO");
    startWindow(false);
    assertRows("Libres:0/4 Oc:4", "PARQUEO LLENO");
    parking.configure(Parking::Mode::Automatic, IR_DEBOUNCE_MIN_MS);
    advance(100);
    assert(parking.state().openingBlocked());
    // Bloqueo > lleno.
    assert(Wire.address == LCD_I2C_ADDRESS);
    startWindow(true);
    assertRows("Libres:0/4 Oc:4", "APERTURA BLOQ.");
    startWindow(false);
    assertRows("Libres:0/4 Oc:4", "APERTURA BLOQ.");
    Wire.connected = false;
    Fake::levels[SLOT_PINS[0].ir] = !IR_OCCUPIED_LEVEL;
    advance(IR_DEBOUNCE_MIN_MS + IR_SAMPLE_MS);
    assert(!parking.state().full() && !parking.state().openingBlocked());
    assert(Fake::levels[SLOT_PINS[0].green] == HIGH && Fake::writes == writes);
    Wire.connected = true;
    Wire.transmissions.clear();
    advance(250);
    // Recuperacion I2C: reinicializa y reescribe ambas filas.
    const auto recovered = lcdScreen();
    assert(recovered.valid && recovered.rows[0] == padded("Libres:1/4 Oc:3"));
    assert(recovered.rows[1] == padded(modeWindow() ? "Modo: automatico" : "Barrera: ---"));
    startWindow(true);
    assertRows("Libres:1/4 Oc:3", "Modo: automatico");
    // Estado de la barrera en la ventana impar.
    assert(barrier.open() && barrier.moving());
    startWindow(false);
    assertRows("Libres:1/4 Oc:3", "Barrera: mueve");
    Fake::now += SERVO_TRAVEL_MS;
    barrier.update();
    startWindow(false);
    assertRows("Libres:1/4 Oc:3", "Barrera: abierta");
    assert(barrier.close());
    Fake::now += SERVO_TRAVEL_MS;
    barrier.update();
    startWindow(false);
    assertRows("Libres:1/4 Oc:3", "Barrera: cerrada");
    Fake::levels[SLOT_PINS[0].ir] = 2;
    advance(IR_SAMPLE_MS);
    assert(parking.state().available == 0 && parking.state().openingBlocked());
    assert(Fake::levels[SLOT_PINS[0].red] == LOW && Fake::levels[SLOT_PINS[0].green] == LOW);
    // Plaza desconocida: bloqueo sin parqueo lleno.
    assert(!parking.state().full());
    advance(3 * IR_SAMPLE_MS);
    assertRows("Libres:0/4 Oc:3", "APERTURA BLOQ.");
    Wire.transmissions.clear();
    std::cout << "PASS: cuatro sensores IR, rebotes, conteos, LEDs y LCD 16x2 (filas, prioridad bloqueo > lleno > alternancia de 2 s, maximo 16 columnas y recuperacion I2C)\n";
}

int main() {
    using R=UltrasonicSensor::ReadingStatus;
    using P=UltrasonicSensor::Presence;
    UltrasonicSensor sensor;
    assert(sensor.measure().status==R::NotMeasured);
    sensor.begin();
    assert(Fake::modes[TRIG_PIN]==OUTPUT && Fake::modes[ECHO_PIN]==INPUT);
    assert(Fake::levels[TRIG_PIN]==LOW && sensor.presence()==P::Unknown);
    Fake::echo=700; // 12.005 cm
    assert(sensor.measure().status==R::Valid);
    assert(std::fabs(sensor.lastMeasurement().distanceCm-12.005f)<0.01f);
    assert(sensor.presence()==P::Present && Fake::timeout==ECHO_TIMEOUT_US);
    Fake::echo=960; sensor.measure(); assert(sensor.presence()==P::Present); // 16.46 cm
    Fake::echo=1200; sensor.measure(); assert(sensor.presence()==P::Absent); // 20.58 cm
    Fake::echo=960; sensor.measure(); assert(sensor.presence()==P::Absent);
    Fake::echo=0; assert(sensor.measure().status==R::Timeout && sensor.presence()==P::Unknown);
    Fake::echo=1; assert(sensor.measure().status==R::OutOfRange && sensor.presence()==P::Unknown);
    Fake::echo=25000; assert(sensor.measure().status==R::OutOfRange);
    Fake::echo=700; sensor.measure(); assert(sensor.presence()==P::Present);

    BarrierServo barrier;
    assert(!barrier.open() && !barrier.positionEstimated());
    assert(barrier.begin() && !barrier.moving() && Fake::writes == 0);
    Fake::attachSuccess=false; assert(!barrier.close());
    Fake::attachSuccess=true; assert(barrier.close() && barrier.moving());
    assert(Fake::pulseUs==500 && Fake::frequency==50);
    assert(Fake::servoPin==SERVO_PIN && Fake::minUs==500 && Fake::maxUs==2400);
    auto writes=Fake::writes; assert(!barrier.close() && Fake::writes==writes);
    Fake::now+=SERVO_TRAVEL_MS-1; barrier.update(); assert(barrier.moving());
    ++Fake::now; barrier.update(); assert(barrier.positionEstimated());
    assert(barrier.open() && Fake::pulseUs==1450);
    writes=Fake::writes; assert(!barrier.open() && Fake::writes==writes);
    assert(barrier.close() && barrier.commandedState()==BarrierServo::State::Closed);
    Fake::now=std::numeric_limits<uint32_t>::max()-100;
    barrier.open(); Fake::now+=SERVO_TRAVEL_MS; barrier.update();
    assert(barrier.positionEstimated());

    using C=SerialCommands::Command;
    SerialCommands parser;
    auto parse=[&](const std::string& s) { C last=C::None; for(char c:s) { auto r=parser.feed(c); if(r!=C::None) last=r; } return last; };
    assert(parse(" op")==C::None && parse("en \r\n")==C::Open);
    assert(parse("CLOSE\n")==C::Close && parse("STATUS\n")==C::Status);
    assert(parse("DISTANCE\n")==C::Distance && parse("help\n")==C::Help);
    assert(parse("INVALID\n")==C::Invalid && parse(" \n")==C::None);
    assert(parse(std::string(100,'x')+"OPEN\n")==C::Invalid);
    assert(parse("OPEN\n")==C::Open);

    testParkingAndDisplay();
    Fake::now=0; writes=Fake::writes; setup();
    assert(Fake::writes==writes);
    const auto triggerCount=Fake::triggers;
    Serial.output.clear(); Serial.inject("OPEN\n"); loop();
    assert(Fake::pulseUs==1450);
    assert(Fake::triggers==triggerCount); // No medicion antes del intervalo.
    Fake::now=500; Fake::echo=0; Serial.inject("CLOSE\nSTATUS\nDISTANCE\nHELP\n"); loop();
    assert(Fake::triggers==triggerCount+1 && Fake::pulseUs==500);
    assert(Serial.output.find("timeout")!=std::string::npos);
    // Sensor detecta, pero nunca abre la barrera.
    Fake::now=1000; Fake::echo=700; loop(); assert(Fake::pulseUs==500);
    Serial.inject(std::string(1000,'x')+"\nOPEN\n"); Fake::now=1500;
    loop(); assert(Serial.available()>0); // Presupuesto Serial respeta la tarea periodica.
    assert(Fake::triggers==triggerCount+3);
    while(Serial.available()) loop();
    assert(Fake::pulseUs==1450);
    Serial.inject("STATUS\n"); loop();
    assert(Serial.output.find("Barrera ordenada: ABIERTA")!=std::string::npos);
    const auto& slot1 = SLOT_PINS[0];
    auto diagnose = [&] {
        Serial.output.clear();
        Serial.inject("slot1\n");
        loop();
    };
    Fake::levels[slot1.ir] = !IR_OCCUPIED_LEVEL;
    auto settleSlot1 = [&] {
        for (unsigned elapsed = 0; elapsed <= IR_DEBOUNCE_DEFAULT_MS; elapsed += IR_SAMPLE_MS) {
            Fake::now += IR_SAMPLE_MS;
            loop();
        }
    };
    settleSlot1();
    diagnose();
    assert(Serial.output.find("estado=LIBRE") != std::string::npos);
    assert(Fake::levels[slot1.red] == LOW && Fake::levels[slot1.green] == HIGH);
    assert(Serial.output.find("inicializada; responde") != std::string::npos);
    Fake::levels[slot1.ir] = IR_OCCUPIED_LEVEL;
    diagnose(); // Raw reading changes before debounce.
    assert(Serial.output.find("IR GPIO " + std::to_string(slot1.ir) + "=" +
                              std::to_string(IR_OCCUPIED_LEVEL)) != std::string::npos);
    assert(Serial.output.find("estado=LIBRE") != std::string::npos);
    settleSlot1();
    Wire.connected = false;
    diagnose();
    assert(Serial.output.find("estado=OCUPADO") != std::string::npos);
    assert(Fake::levels[slot1.red] == HIGH && Fake::levels[slot1.green] == LOW);
    assert(Serial.output.find("error I2C") != std::string::npos);
    Wire.connected = true;
    std::cout << "PASS: sensor, histeresis, servo, parser e integracion del loop (simulados)\n";
}
