#pragma once

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <PubSubClient.h>
#include "Parking.h"
#include "UltrasonicSensor.h"
#include "BarrierServo.h"

class HistoryPublisher {
public:
    enum class Origin : uint8_t { None, Legacy, Voice, Cloud, Edge };
    struct Stats {
        uint32_t written, overflow, withoutClock, failures, discarded;
    };

    void begin();
    // Network task, after Wi-Fi and the first UTC synchronization.
    void enableClock();
    // Observers and motion capture run in the local loop.
    void observeParking(const Parking::State& parking, const UltrasonicSensor& sensor, uint32_t now);
    void motionStarted(Origin origin, const char* requestId);
    void observeBarrier(const BarrierServo& barrier);
    // Only the network task publishes and removes entries.
    void publish(PubSubClient& mqtt, uint32_t now);
    Stats stats() const;

private:
    enum class Kind : uint8_t { Barrier, Slot, Sample, Count };
    enum class Position : uint8_t { Unknown, Closed, Open };
    struct Event {
        uint64_t occurredAt = 0, sequence = 0;
        Parking::State parking;
        float distance = 0;
        int8_t presence = -1;
        bool distanceValid = false;
        uint8_t slot = 0;
        Parking::Slot previousSlot = Parking::Slot::Unknown;
        Position previous = Position::Unknown, current = Position::Unknown;
        Origin origin = Origin::Edge;
        char requestId[33]{};
    };

    static constexpr size_t laneCount = static_cast<size_t>(Kind::Count);
    QueueHandle_t queues_[laneCount]{};
    uint8_t attempts_[laneCount]{};
    std::atomic<bool> clockReady_{false};
    uint32_t bootId_[4]{};
    uint64_t sequence_ = 0;
    Parking::Slot observed_[Parking::capacity]{};
    Position position_ = Position::Unknown;
    Origin motionOrigin_ = Origin::None;
    char motionRequestId_[33]{};
    bool sampled_ = false, attempted_ = false;
    uint32_t sampledAt_ = 0, attemptedAt_ = 0;
    std::atomic<uint32_t> overflow_{0}, withoutClock_{0};
    uint32_t written_ = 0, failures_ = 0, discarded_ = 0;

    void enqueue(Kind kind, Event& event);
    bool encode(Kind kind, const Event& event, char* output, size_t capacity) const;
};
