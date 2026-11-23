#ifdef SMARTPARK_LCD_DIAGNOSTICO
#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

namespace {
hd44780_I2Cexp lcd(0x27);

void checkRow(uint8_t row, const char* text) {
    const size_t length = strlen(text);
    if (lcd.setCursor(0, row) != 0 || lcd.print(text) != length || lcd.setCursor(0, row) != 0) {
        Serial.printf("Fila %u: error de escritura o posicionamiento\n", unsigned(row + 1));
        return;
    }
    for (size_t column = 0; column < length; ++column) {
        const int value = lcd.read();
        if (value != text[column]) {
            Serial.printf("Fila %u, columna %u: esperado=%d leido=%d\n",
                          unsigned(row + 1), unsigned(column + 1), int(text[column]), value);
            return;
        }
    }
    Serial.printf("Fila %u: lectura de memoria OK\n", unsigned(row + 1));
}
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22, 100000);
    Serial.println("\nDiagnostico LCD 16x2, direccion 0x27");
    Wire.beginTransmission(0x27);
    const int probe = Wire.endTransmission();
    Serial.printf("Respuesta del adaptador I2C: %d\n", probe);
    if (probe != 0) return;

    const int lcdStatus = lcd.begin(16, 2);
    Serial.printf("Inicializacion HD44780 con deteccion del mapa: %d\n", lcdStatus);
    if (lcdStatus != 0) return;

    checkRow(0, "SMARTPARK LCD OK");
    checkRow(1, "0123456789 ABCDE");
}

void loop() {
    delay(100);
}
#endif
