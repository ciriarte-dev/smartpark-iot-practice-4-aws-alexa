#pragma once
#include "Parking.h"
#include "BarrierServo.h"

// Tamano de la LCD; un config.h local puede redefinirlo antes de este punto.
#ifndef LCD_COLUMNS
#define LCD_COLUMNS 16
#endif
#ifndef LCD_ROWS
#define LCD_ROWS 2
#endif
static_assert(LCD_ROWS == 2, "El contenido de la LCD esta disenado para 2 filas");
static_assert(LCD_COLUMNS >= 16 && LCD_COLUMNS <= 40, "Los textos de la LCD necesitan 16 columnas");

class ParkingDisplay {
public:
    void begin();
    void reinitialize(uint32_t now);
    uint8_t probe();
    bool initialized() const { return init_ == Init::Ready; }
    void update(uint32_t now, const Parking::State& parking, const BarrierServo& barrier);
private:
    enum class Init : uint8_t { Wait, WakeOne, WakeTwo, WakeThree, FourBit, Configure, EntryMode, Ready };
    Init init_ = Init::Wait;
    uint32_t lastStep_ = 0;
    uint8_t nextRow_ = 0;
    char displayed_[LCD_ROWS][LCD_COLUMNS + 1]{};
    bool prepareExpander();
    bool nibble(uint8_t value, bool data = false);
    bool byte(uint8_t value, bool data = false);
};
