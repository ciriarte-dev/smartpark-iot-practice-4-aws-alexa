#pragma once
#include <vector>
#include <deque>
#include <cstring>
struct FakeQueue {
    size_t length, size;
    std::deque<std::vector<unsigned char>> entries;
    bool full = false;
};
using QueueHandle_t = FakeQueue*;
inline QueueHandle_t xQueueCreate(unsigned length, size_t size) {
    return new FakeQueue{length, size, {}, false};
}
inline int xQueueSend(QueueHandle_t q, const void* data, unsigned) {
    if (q->full) return 0;
    const auto* bytes = static_cast<const unsigned char*>(data);
    q->entries.emplace_back(bytes, bytes + q->size);
    q->full = q->entries.size() == q->length;
    return 1;
}
inline int xQueueOverwrite(QueueHandle_t q, const void* data) {
    q->entries.clear();
    q->full = false;
    return xQueueSend(q, data, 0);
}
inline int xQueuePeek(QueueHandle_t q, void* data, unsigned) {
    if (q->entries.empty()) return 0;
    std::memcpy(data, q->entries.front().data(), q->size);
    return 1;
}
inline int xQueueReceive(QueueHandle_t q, void* data, unsigned wait) {
    if (!xQueuePeek(q, data, wait)) return 0;
    q->entries.pop_front();
    q->full = false;
    return 1;
}
