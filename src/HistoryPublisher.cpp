#include "HistoryPublisher.h"
#include <ArduinoJson.h>
#include <esp_system.h>
#include <sys/time.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr char topic[] = "smartpark/" THING_NAME "/history/v1";
constexpr size_t payloadCapacity = 512;
// PubSubClient reserves five fixed-header bytes plus the two-byte topic length.
static_assert(payloadCapacity - 1 + sizeof(topic) - 1 + 7 <= MQTT_BUFFER_SIZE,
              "El buffer MQTT debe admitir el paquete historico completo");
constexpr const char* slotNames[] = {"desconocido", "libre", "ocupado"};
constexpr const char* slotKeys[] = {"1", "2", "3", "4"};
constexpr const char* positions[] = {"desconocida", "cerrada", "abierta"};
constexpr const char* origins[] = {"", "legado", "voz", "nube", "edge"};
constexpr const char* eventTypes[] = {"barrier_changed", "occupancy_changed", "sample"};
}

void HistoryPublisher::begin() {
    queues_[static_cast<size_t>(Kind::Barrier)] = xQueueCreate(HISTORY_BARRIER_QUEUE, sizeof(Event));
    queues_[static_cast<size_t>(Kind::Slot)] = xQueueCreate(HISTORY_SLOT_QUEUE, sizeof(Event));
    queues_[static_cast<size_t>(Kind::Sample)] = xQueueCreate(1, sizeof(Event));
}

void HistoryPublisher::enableClock() {
    if (clockReady_.load()) return;
    for (auto& word : bootId_) word = esp_random();
    clockReady_.store(true);
}

void HistoryPublisher::enqueue(Kind kind, Event& event) {
    timeval now{};
    if (!clockReady_.load() || gettimeofday(&now, nullptr) != 0 || now.tv_sec < AWS_MIN_VALID_EPOCH) {
        ++withoutClock_;
        return;
    }
    event.occurredAt = static_cast<uint64_t>(now.tv_sec) * 1000 + now.tv_usec / 1000;
    event.sequence = ++sequence_;
    const auto queue = queues_[static_cast<size_t>(kind)];
    if (!queue || xQueueSend(queue, &event, 0) != pdTRUE) ++overflow_;
}

void HistoryPublisher::observeParking(const Parking::State& parking, const UltrasonicSensor& sensor, uint32_t now) {
    const bool sample = clockReady_.load() && (!sampled_ || now - sampledAt_ >= HISTORY_SAMPLE_INTERVAL_MS);
    bool changed = false;
    for (uint8_t i = 0; i < Parking::capacity; ++i) changed |= observed_[i] != parking.slots[i];
    if (!sample && !changed) return;

    Event event;
    event.parking = parking;
    const auto& reading = sensor.lastMeasurement();
    event.distanceValid = reading.status == UltrasonicSensor::ReadingStatus::Valid &&
                          std::isfinite(reading.distanceCm) && reading.distanceCm >= 2 && reading.distanceCm <= 400;
    if (event.distanceValid) {
        event.distance = std::round(reading.distanceCm * 10) / 10;
        if (sensor.presence() != UltrasonicSensor::Presence::Unknown)
            event.presence = sensor.presence() == UltrasonicSensor::Presence::Present;
    }
    for (uint8_t i = 0; i < Parking::capacity; ++i) {
        if (observed_[i] == parking.slots[i]) continue;
        event.slot = i + 1;
        event.previousSlot = observed_[i];
        observed_[i] = parking.slots[i];
        enqueue(Kind::Slot, event);
    }
    if (sample) {
        sampled_ = true;
        sampledAt_ = now;
        enqueue(Kind::Sample, event);
    }
}

void HistoryPublisher::motionStarted(Origin origin, const char* requestId) {
    motionOrigin_ = origin;
    snprintf(motionRequestId_, sizeof(motionRequestId_), "%s", requestId);
}

void HistoryPublisher::observeBarrier(const BarrierServo& barrier) {
    if (motionOrigin_ == Origin::None || !barrier.positionEstimated()) return;
    const auto current = barrier.commandedState() == BarrierServo::State::Open ? Position::Open : Position::Closed;
    if (current != position_) {
        Event event;
        event.previous = position_;
        event.current = current;
        event.origin = motionOrigin_;
        memcpy(event.requestId, motionRequestId_, sizeof(event.requestId));
        enqueue(Kind::Barrier, event);
        position_ = current;
    }
    motionOrigin_ = Origin::None;
}

bool HistoryPublisher::encode(Kind kind, const Event& event, char* output, size_t capacity) const {
    char id[50];
    snprintf(id, sizeof(id), "%08lx%08lx%08lx%08lx-%016llx",
             static_cast<unsigned long>(bootId_[0]), static_cast<unsigned long>(bootId_[1]),
             static_cast<unsigned long>(bootId_[2]), static_cast<unsigned long>(bootId_[3]),
             static_cast<unsigned long long>(event.sequence));
    StaticJsonDocument<1024> doc;
    doc["eventId"] = id;
    doc["occurredAt"] = event.occurredAt;
    doc["eventType"] = eventTypes[static_cast<size_t>(kind)];
    doc["origin"] = origins[static_cast<size_t>(event.origin)];
    auto data = doc.createNestedObject("data");
    if (kind == Kind::Barrier) {
        data["anterior"] = positions[static_cast<size_t>(event.previous)];
        data["actual"] = positions[static_cast<size_t>(event.current)];
        data["posicion_estimada"] = true;
        if (*event.requestId) data["requestId"] = event.requestId;
    } else {
        auto slots = data.createNestedObject("slots");
        for (uint8_t i = 0; i < Parking::capacity; ++i)
            slots[slotKeys[i]] = slotNames[static_cast<size_t>(event.parking.slots[i])];
        data["capacidad"] = Parking::capacity;
        data["ocupados"] = event.parking.occupied;
        data["disponibles"] = event.parking.available;
        data["modo"] = parkingMode(event.parking.mode);
        if (event.distanceValid) data["distancia_cm"] = event.distance;
        else data["distancia_cm"] = nullptr;
        if (event.presence >= 0) data["vehiculo_detectado"] = event.presence == 1;
        else data["vehiculo_detectado"] = nullptr;
        if (kind == Kind::Slot) {
            data["slot"] = event.slot;
            data["estado_anterior"] = slotNames[static_cast<size_t>(event.previousSlot)];
        }
    }
    if (doc.overflowed() || measureJson(doc) >= capacity) return false;
    serializeJson(doc, output, capacity);
    return true;
}

void HistoryPublisher::publish(PubSubClient& mqtt, uint32_t now) {
    if (!mqtt.connected() || (attempted_ && now - attemptedAt_ < HISTORY_PUBLISH_INTERVAL_MS)) return;
    Event event;
    for (size_t lane = 0; lane < laneCount; ++lane) {
        const auto queue = queues_[lane];
        if (!queue || xQueuePeek(queue, &event, 0) != pdTRUE) continue;
        attempted_ = true;
        char payload[payloadCapacity];
        if (!encode(static_cast<Kind>(lane), event, payload, sizeof(payload))) {
            ++discarded_;
        } else if (mqtt.publish(topic, payload)) {
            ++written_;
        } else {
            ++failures_;
            attemptedAt_ = millis();
            if (++attempts_[lane] < HISTORY_MAX_ATTEMPTS) return;
            ++discarded_;
        }
        attemptedAt_ = millis();
        xQueueReceive(queue, &event, 0);
        attempts_[lane] = 0;
        return;
    }
}

HistoryPublisher::Stats HistoryPublisher::stats() const {
    return {written_, overflow_.load(), withoutClock_.load(), failures_, discarded_};
}
