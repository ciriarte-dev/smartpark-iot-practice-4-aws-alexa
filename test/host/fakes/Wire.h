#pragma once
#include <cstdint>
#include <vector>

struct FakeWire {
    bool connected = true;
    uint8_t sda = 0, scl = 0, address = 0;
    std::vector<std::vector<uint8_t>> transmissions;
    std::vector<uint8_t> current;
    void begin(uint8_t data, uint8_t clock, uint32_t) { sda = data; scl = clock; }
    void setTimeOut(uint16_t) {}
    void beginTransmission(uint8_t target) { address = target; current.clear(); }
    void write(uint8_t value) { current.push_back(value); }
    uint8_t endTransmission() {
        if (!connected) return 2;
        transmissions.push_back(current);
        return 0;
    }
};
inline FakeWire Wire;
