#include "AwsIoTManager.h"
#include "config.h"
#include <WiFi.h>
#include <cassert>
#include <iostream>
#include <ctime>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <set>
struct AwsIoTManagerTest {
    static void receive(AwsIoTManager& a,const char* topic,const std::string& body) {
        a.receive(const_cast<char*>(topic),reinterpret_cast<uint8_t*>(const_cast<char*>(body.data())),body.size());
    }
    static void legacy(AwsIoTManager& a, unsigned version, const char* target, uint32_t age) {
        const auto stamp = static_cast<uint32_t>(time(nullptr)) - age;
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":" + std::to_string(version) +
            ",\"state\":{\"barrera\":\"" + target + "\"},\"metadata\":{\"barrera\":{\"timestamp\":" +
            std::to_string(stamp) + "}}}");
    }
    static void ack(AwsIoTManager& a) { receive(a,TOPIC_SHADOW_UPDATE_ACCEPTED,std::string("{\"clientToken\":\"")+a.token_+"\"}"); }
    static void init(AwsIoTManager& a) {
        Parking parking; assert(a.begin(parking)); a.storageOK_=true; a.timeReady_=true;
        a.retryDelay_=WIFI_RETRY_INTERVAL_MS;
        assert(a.refreshDay()); WiFi.state=WL_CONNECTED;
    }
    static void addMetadata(JsonObjectConst state, JsonObject target, uint32_t timestamp) {
        for (const auto entry : state) {
            if (entry.value().is<JsonObjectConst>())
                addMetadata(entry.value().as<JsonObjectConst>(), target.createNestedObject(entry.key()), timestamp);
            else target.createNestedObject(entry.key())["timestamp"] = timestamp;
        }
    }
    static void testParkingShadow() {
        Preferences::other.clear();
        Preferences::disk.clear();
        AwsIoTManager a;
        Parking parking;
        parking.begin();
        assert(a.begin(parking));
        a.synced_ = true;
        a.timeReady_ = true;
        a.mqtt_.online = true;
        BarrierServo barrier;
        barrier.begin();
        UltrasonicSensor sensor;
        sensor.begin();
        const auto writes = Fake::writes;
        const auto now = static_cast<uint32_t>(time(nullptr));
        const auto modern = [now](const char* id, const char* objective, unsigned order) {
            return std::string("{\"id\":\"") + id + "\",\"objetivo\":\"" + objective +
                "\",\"origen\":\"voz\",\"solicitada_ms\":" + std::to_string(uint64_t{now} * 1000 + order) +
                ",\"vence_s\":" + std::to_string(now + 29) + "}";
        };
        const char* longId = "orden-12345678901234567890123456";
        const auto open = modern(longId, "abierta", 1);
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":1,\"state\":{\"modo\":\"automatico\","
            "\"parametros_auto\":{\"antirrebote_ir_ms\":50},\"orden_barrera\":" + open + "}}");
        a.applyPending(barrier, parking, false);
        assert(parking.state().mode == Parking::Mode::Automatic && parking.state().debounceMs == 50);
        assert(parking.state().openingBlocked() && Fake::writes == writes);
        assert(a.mainOrder_.outcome == AwsIoTManager::Outcome::Blocked && a.mainOrder_.origin == AwsIoTManager::Origin::Voice);
        a.tick(barrier, sensor, parking);
        a.step();
        assert(a.inflight_);
        const auto blockedReport = a.mqtt_.bodies.back();
        assert(blockedReport.find("\"resultado\":\"bloqueada\"") != std::string::npos);
        assert(blockedReport.find("\"barrera\":null") != std::string::npos);
        ack(a);

        for (auto pins : SLOT_PINS) Fake::levels[pins.ir] = IR_OCCUPIED_LEVEL;
        for (unsigned i = 0; i <= 6; ++i) { Fake::now += IR_SAMPLE_MS; parking.update(Fake::now); }
        assert(parking.state().full());
        legacy(a, 2, "abierta", 1); // older than the consumed V2 order
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == writes);
        Fake::levels[SLOT_PINS[0].ir] = !IR_OCCUPIED_LEVEL;
        for (unsigned i = 0; i <= 6; ++i) { Fake::now += IR_SAMPLE_MS; parking.update(Fake::now); }
        assert(parking.state().available == 1);
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":3,\"state\":{\"orden_barrera\":" + open + "}}");
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == writes);
        legacy(a, 4, "abierta", AWS_ORDER_TTL_S + 1);
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == writes);

        const auto freshOpen = modern("otra-apertura", "abierta", 2);
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":5,\"state\":{\"orden_barrera\":" + freshOpen + "}}");
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":6,\"state\":{\"modo\":\"manual\"}}");
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == writes + 1 && a.mainOrder_.outcome == AwsIoTManager::Outcome::Accepted);
        assert(parking.state().mode == Parking::Mode::Manual);
        const auto close = modern("orden-cierre", "cerrada", 3);
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":7,\"state\":{\"orden_barrera\":" + close + "}}");
        a.disconnect();
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == writes + 1); // Disconnect invalidates unexecuted commands.

        AwsIoTManager rebooted;
        Parking restored;
        restored.begin();
        assert(rebooted.begin(restored));
        assert(restored.state().mode == Parking::Mode::Manual && restored.state().debounceMs == 50);
        rebooted.getPending_ = true;
        strcpy(rebooted.token_, "reconnect");
        receive(rebooted, TOPIC_SHADOW_GET_ACCEPTED,
            "{\"clientToken\":\"reconnect\",\"version\":8,\"state\":{\"desired\":{\"orden_barrera\":" + close + "}}}");
        rebooted.applyPending(barrier, restored, false);
        assert(Fake::writes == writes + 1); // NVS consumes a motion before it reaches the servo.

        // Measure exact payloads serialized by the firmware and representative AWS responses.
        a.current_.ready = true;
        a.current_.valid = true;
        a.current_.distance = 400;
        a.current_.presence = 0;
        a.stablePresence_ = -1;
        a.current_.order = a.mainOrder_;
        strcpy(a.current_.order.id, longId);
        a.current_.parking = parking.state();
        a.current_.parking.mode = Parking::Mode::Automatic;
        a.current_.parking.debounceMs = IR_DEBOUNCE_MAX_MS;
        const auto before = a.mqtt_.bodies.size();
        a.report(ShadowBudget::Control);
        assert(a.mqtt_.bodies.size() == before + 1);
        const auto& report = a.mqtt_.bodies.back();
        DynamicJsonDocument outgoing(4096);
        assert(!deserializeJson(outgoing, report));
        const auto r = outgoing["state"]["reported"].as<JsonObjectConst>();
        assert(r["slots"]["1"] == "libre" && r["slots"]["4"] == "ocupado");
        assert(r["ocupados"] == 3 && r["disponibles"] == 1 && r["capacidad"] == 4);
        assert(r["modo"] == "automatico" && r["parametros_auto"]["antirrebote_ir_ms"] == IR_DEBOUNCE_MAX_MS);

        assert(r["distancia_cm"].as<float>() == 400.0f);
        assert(r["vehiculo_detectado"].is<bool>() && !r["vehiculo_detectado"].as<bool>());

        DynamicJsonDocument response(12288);
        response["clientToken"] = "00000000-0000-4000-8000-000000000000";
        response["timestamp"] = 1999999999;
        response["version"] = 4294967295u;
        auto state = response.createNestedObject("state");
        state["reported"] = r;
        auto desired = state.createNestedObject("desired");
        desired["barrera"] = "abierta";
        desired["modo"] = "manual";
        desired.createNestedObject("parametros_auto")["antirrebote_ir_ms"] = 999;
        DynamicJsonDocument request(1024);
        assert(!deserializeJson(request, open));
        desired["orden_barrera"] = request.as<JsonObjectConst>();
        state["delta"] = desired;
        auto metadata = response.createNestedObject("metadata");
        addMetadata(r, metadata.createNestedObject("reported"), 1999999999);
        addMetadata(desired, metadata.createNestedObject("desired"), 1999999999);
        addMetadata(desired, metadata.createNestedObject("delta"), 1999999999);
        assert(!response.overflowed());
        const size_t getSize = measureJson(response);
        std::string getJson;
        serializeJson(response, getJson);
        DynamicJsonDocument delta(4096);
        delta["clientToken"] = response["clientToken"];
        delta["timestamp"] = response["timestamp"];
        delta["version"] = response["version"];
        delta["state"] = desired;
        delta["metadata"] = metadata["desired"];
        assert(!delta.overflowed());
        const size_t deltaSize = measureJson(delta);
        // UPDATE accepted has reported and its metadata, without GET's additional delta.
        assert(!deserializeJson(response, getJson.c_str()));
        response["state"].remove("delta");
        response["metadata"].remove("delta");
        const size_t acceptedSize = measureJson(response);
        const auto packet = [](size_t json, const char* topic) { return json + strlen(topic) + 9; };
        std::cout << "Shadow JSON reporte/GET/delta/accepted: " << report.size() << "/" <<
            getSize << "/" << deltaSize << "/" << acceptedSize << "; MQTT GET: " <<
            packet(getSize, TOPIC_SHADOW_GET_ACCEPTED) << " bytes\n";
        assert(packet(report.size(), TOPIC_SHADOW_UPDATE) <= MQTT_BUFFER_SIZE);
        assert(packet(deltaSize, TOPIC_SHADOW_DELTA) <= MQTT_BUFFER_SIZE);
        assert(packet(getSize, TOPIC_SHADOW_GET_ACCEPTED) <= MQTT_BUFFER_SIZE);
        assert(packet(acceptedSize, TOPIC_SHADOW_UPDATE_ACCEPTED) <= MQTT_BUFFER_SIZE);
        rebooted.getPending_ = true;
        strcpy(rebooted.token_, "00000000-0000-4000-8000-000000000000");
        receive(rebooted, TOPIC_SHADOW_GET_ACCEPTED, getJson);
        assert(rebooted.synced_ && !rebooted.getPending_ && rebooted.version_ == 4294967295u);

        UltrasonicSensor ultrasonic;
        ultrasonic.begin();
        Fake::echo = 580;
        assert(ultrasonic.measure().status == UltrasonicSensor::ReadingStatus::Valid);
        a.tick(barrier, ultrasonic, parking);
        assert(xQueueReceive(a.snapshots_, &a.current_, 0) == pdTRUE);
        a.report(ShadowBudget::Control);
        assert(!deserializeJson(outgoing, a.mqtt_.bodies.back()));
        assert(outgoing["state"]["reported"]["distancia_cm"].as<float>() == 9.9f);
        assert(outgoing["state"]["reported"]["vehiculo_detectado"] == true);

        Fake::echo = 0;
        assert(ultrasonic.measure().status == UltrasonicSensor::ReadingStatus::Timeout);
        a.tick(barrier, ultrasonic, parking);
        assert(xQueueReceive(a.snapshots_, &a.current_, 0) == pdTRUE);
        a.report(ShadowBudget::Control);
        assert(!deserializeJson(outgoing, a.mqtt_.bodies.back()));
        const auto invalid = outgoing["state"]["reported"].as<JsonObjectConst>();
        assert(invalid.containsKey("distancia_cm") && invalid["distancia_cm"].isNull());
        assert(invalid.containsKey("vehiculo_detectado") && invalid["vehiculo_detectado"].isNull());
        assert(invalid["capacidad"] == 4);
    }

    static void testLegacyParking() {
        Preferences::other.clear();
        Preferences::disk.clear();
        AwsIoTManager a;
        Parking parking;
        parking.begin();
        assert(a.begin(parking));
        a.synced_ = true;
        BarrierServo barrier;
        barrier.begin();
        const auto initialWrites = Fake::writes;
        for (auto pins : SLOT_PINS) Fake::levels[pins.ir] = IR_OCCUPIED_LEVEL;
        auto settle = [&] {
            for (unsigned elapsed = 0; elapsed <= IR_DEBOUNCE_DEFAULT_MS; elapsed += IR_SAMPLE_MS) {
                Fake::now += IR_SAMPLE_MS;
                parking.update(Fake::now);
            }
        };
        settle();
        assert(parking.state().full());
        receive(a, TOPIC_SHADOW_DELTA, "{\"version\":1,\"state\":{\"modo\":\"automatico\"}}");
        legacy(a, 2, "abierta", 4);
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites && a.mainOrder_.outcome == AwsIoTManager::Outcome::Blocked);
        Fake::levels[SLOT_PINS[0].ir] = !IR_OCCUPIED_LEVEL;
        settle();
        assert(parking.state().available == 1);
        a.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites);

        AwsIoTManager rebooted;
        assert(rebooted.begin(parking) && parking.state().mode == Parking::Mode::Automatic);
        rebooted.synced_ = true;
        legacy(rebooted, 3, "abierta", 4);
        rebooted.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites);
        legacy(rebooted, 4, "abierta", 3);
        rebooted.applyPending(barrier, parking, true); // An explicit Serial order wins this loop.
        assert(Fake::writes == initialWrites);
        rebooted.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites);
        legacy(rebooted, 5, "abierta", 2);
        rebooted.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites + 1);
        Fake::levels[SLOT_PINS[0].ir] = IR_OCCUPIED_LEVEL;
        settle();
        assert(parking.state().full() && Fake::writes == initialWrites + 1);
        legacy(rebooted, 6, "cerrada", 2);
        rebooted.applyPending(barrier, parking, false);
        assert(Fake::writes == initialWrites + 2); // Explicit close is legal when full.
        receive(rebooted, TOPIC_SHADOW_DELTA, "{\"version\":7,\"state\":{\"modo\":\"manual\","
            "\"parametros_auto\":{\"antirrebote_ir_ms\":2001}}}");
        rebooted.applyPending(barrier, parking, false);
        assert(parking.state().mode == Parking::Mode::Automatic);
        receive(rebooted, TOPIC_SHADOW_DELTA, "{\"version\":8,\"state\":{\"modo\":\"manual\"}}");
        legacy(rebooted, 9, "abierta", 1);
        rebooted.applyPending(barrier, parking, false);
        assert(parking.state().full() && parking.state().mode == Parking::Mode::Manual);
        assert(Fake::writes == initialWrites + 3);
        std::cout << "PASS: modos, lleno, prioridad Serial y deduplicacion V1/V2 tras reconexion o reinicio\n";
    }

    static void testHistoryPublisher() {
        Preferences::other.clear();
        Preferences::disk.clear();
        AwsIoTManager a;
        Parking parking;
        parking.begin();
        assert(a.begin(parking));
        UltrasonicSensor sensor;
        sensor.begin();
        BarrierServo barrier;
        barrier.begin();
        Fake::now = 1000;
        auto settleSlots = [&] {
            for (unsigned elapsed = 0; elapsed <= IR_DEBOUNCE_DEFAULT_MS; elapsed += IR_SAMPLE_MS) {
                Fake::now += IR_SAMPLE_MS;
                parking.update(Fake::now);
            }
            a.tick(barrier, sensor, parking);
        };
        const char* historyTopic = "smartpark/" THING_NAME "/history/v1";
        for (auto pins : SLOT_PINS) Fake::levels[pins.ir] = IR_OCCUPIED_LEVEL;
        settleSlots();
        assert(a.history_.stats().withoutClock == Parking::capacity);
        a.history_.enableClock();
        for (auto pins : SLOT_PINS) Fake::levels[pins.ir] = !IR_OCCUPIED_LEVEL;
        settleSlots(); // Four transitions and one initial sample, all queued.

        auto settleBarrier = [&] {
            Fake::now += SERVO_TRAVEL_MS;
            barrier.update();
            a.observeBarrier(barrier);
            a.observeBarrier(barrier); // No duplicate from the local observer.
        };
        a.synced_ = a.timeReady_ = a.mqtt_.online = true;
        WiFi.state = WL_CONNECTED;
        assert(barrier.open());
        a.noteSerialCommand(true, false, true, true);
        a.observeBarrier(barrier); // Still in travel.
        assert(!barrier.open());
        a.noteSerialCommand(false, true); // A later rejected request cannot replace the origin.
        settleBarrier();
        const auto now = static_cast<uint32_t>(time(nullptr));
        auto command = [&](unsigned version, const char* id, const char* target, const char* origin) {
            receive(a, TOPIC_SHADOW_DELTA, "{\"version\":" + std::to_string(version) +
                ",\"state\":{\"orden_barrera\":{\"id\":\"" + id + "\",\"objetivo\":\"" + target +
                "\",\"origen\":\"" + origin + "\",\"solicitada_ms\":" +
                std::to_string(uint64_t{now} * 1000 + version) + ",\"vence_s\":" +
                std::to_string(now + 29) + "}}}");
            a.applyPending(barrier, parking, false);
        };
        command(1, "voice-close", "cerrada", "voz");
        settleBarrier();
        command(2, "cloud-open", "abierta", "nube");
        settleBarrier();
        command(3, "cloud-no-move", "abierta", "nube");
        settleBarrier();

        auto reconnect = [&](unsigned version) {
            Fake::now = a.nextAttempt_;
            WiFi.state = WL_CONNECTED;
            a.step();
            assert(a.mqtt_.online && a.getPending_);
            receive(a, TOPIC_SHADOW_GET_ACCEPTED, "{\"clientToken\":\"" + std::string(a.token_) +
                "\",\"version\":" + std::to_string(version) + ",\"state\":{}}");
            assert(a.synced_ && !a.getPending_);
            // Keep the test focused on the independent history path while the Shadow is inhibited.
            a.rejectedBlocked_ = true;
        };
        WiFi.state = 0;
        a.mqtt_.online = false;
        a.step();
        assert(a.mqtt_.topics.empty());
        reconnect(4);
        a.mqtt_.publishOK = false;
        a.step();
        assert(a.mqtt_.topics.back() == historyTopic && a.history_.stats().failures == 1);
        const auto firstAttempt = a.mqtt_.bodies.back();
        const auto count = a.mqtt_.topics.size();
        Fake::now += HISTORY_PUBLISH_INTERVAL_MS - 1;
        a.step();
        assert(a.mqtt_.topics.size() == count);
        WiFi.state = 0;
        a.mqtt_.online = false;
        a.step();
        a.mqtt_.publishOK = true;
        reconnect(5);
        const auto budget = a.budget_.total();
        std::set<std::string> ids;
        size_t maxSize = 0;
        std::vector<std::string> payloads{firstAttempt};
        auto next = [&] {
            Fake::now += HISTORY_PUBLISH_INTERVAL_MS;
            a.step();
            assert(a.mqtt_.topics.back() == historyTopic);
            payloads.push_back(a.mqtt_.bodies.back());
            maxSize = std::max(maxSize, payloads.back().size());
            assert(maxSize + strlen(historyTopic) + 7 <= MQTT_BUFFER_SIZE);
            DynamicJsonDocument result(4096);
            assert(!deserializeJson(result, payloads.back()));
            assert(ids.insert(result["eventId"].as<std::string>()).second);
            return result;
        };
        auto edge = next();
        assert(payloads.back() == firstAttempt); // Bytes, ID and occurredAt survive a reconnect.
        assert(edge["eventType"] == "barrier_changed" && edge["origin"] == "edge");
        assert(edge["data"]["anterior"] == "desconocida" && edge["data"]["actual"] == "abierta");
        auto voice = next();
        assert(voice["eventType"] == "barrier_changed" && voice["origin"] == "voz");
        assert(voice["data"]["actual"] == "cerrada" && voice["data"]["requestId"] == "voice-close");
        auto cloud = next();
        assert(cloud["origin"] == "nube" && cloud["data"]["actual"] == "abierta");
        for (unsigned i = 1; i <= Parking::capacity; ++i) {
            auto slot = next();
            assert(slot["eventType"] == "occupancy_changed" && slot["data"]["slot"] == i);
            assert(slot["data"]["estado_anterior"] == "ocupado" && slot["data"]["disponibles"] == Parking::capacity);
        }
        auto sample = next();
        assert(sample["eventType"] == "sample" && sample["data"]["distancia_cm"].isNull());
        const auto drained = a.mqtt_.topics.size();
        Fake::now += HISTORY_PUBLISH_INTERVAL_MS;
        a.step();
        assert(a.mqtt_.topics.size() == drained && a.budget_.total() == budget);
        a.tick(barrier, sensor, parking);
        a.step();
        assert(a.mqtt_.topics.size() == drained); // Periodic snapshots are not retriggered by loop iterations.
        Fake::now += HISTORY_SAMPLE_INTERVAL_MS;
        Fake::echo = 700;
        sensor.measure();
        a.tick(barrier, sensor, parking);
        auto reading = next();
        assert(reading["eventType"] == "sample" && reading["data"]["vehiculo_detectado"] == true);
        assert(reading["data"]["distancia_cm"].as<float>() > 11.0f);

        // Saturate the slot lane and prove that a new barrier entry still gets through first.
        for (unsigned i = 0; i <= HISTORY_SLOT_QUEUE; ++i) {
            Fake::levels[SLOT_PINS[0].ir] = (i % 2 == 0) ? IR_OCCUPIED_LEVEL : !IR_OCCUPIED_LEVEL;
            settleSlots();
        }
        assert(a.history_.stats().overflow >= 1);
        const char* maxRequest = "12345678901234567890123456789012";
        command(6, maxRequest, "cerrada", "voz");
        settleBarrier();
        auto priority = next();
        assert(priority["data"]["requestId"] == maxRequest && priority["eventType"] == "barrier_changed");

        a.mqtt_.publishOK = false;
        for (unsigned i = 0; i < HISTORY_MAX_ATTEMPTS; ++i) {
            Fake::now += HISTORY_PUBLISH_INTERVAL_MS;
            a.step();
        }
        assert(a.history_.stats().failures == 1 + HISTORY_MAX_ATTEMPTS && a.history_.stats().discarded == 1);
        auto skipped = a.mqtt_.bodies.back();
        a.mqtt_.publishOK = true;
        auto subsequent = next();
        assert(subsequent["eventType"] == "occupancy_changed" && a.mqtt_.bodies.back() != skipped);
        if (const auto* path = std::getenv("SMARTPARK_HISTORY_FIXTURES")) {
            std::ofstream output(path);
            for (const auto& payload : payloads) output << payload << '\n';
            assert(output.good());
        }
        std::cout << "PASS: publicacion MQTT, reloj, prioridad, reconexion, reintentos, cuota independiente y desborde. "
                  << "Historico JSON max observado: " << maxSize << " B; MQTT: "
                  << maxSize + strlen(historyTopic) + 7 << " B\n";
    }

    static void run() {
        ShadowBudget b;
        assert(b.advance(100)); b.record.counts[ShadowBudget::Periodic]=800;
        assert(!b.allows(ShadowBudget::Periodic,1000,200));
        assert(b.allows(ShadowBudget::Recovery,1000,200));
        assert(b.allows(ShadowBudget::Control,1000,200));
        assert(!b.advance(99) && b.total()==800);
        assert(!b.advance(100) && b.total()==800);
        b.record.counts[ShadowBudget::Control]=200;
        assert(!b.allows(ShadowBudget::Control,1000,200));
        assert(b.advance(101) && b.total()==0);

        static_assert(SENSOR_INTERVAL_MS==500 && AWS_REPORT_INTERVAL_MS==30000);
        static_assert(AWS_MIN_PUBLISH_INTERVAL_MS==10000 && AWS_DISTANCE_CHANGE_THRESHOLD_CM==10);
        static_assert(AWS_MAX_SHADOW_UPDATES_PER_DAY==1000 && AWS_RESERVED_CONTROL_UPDATES==200);
        static_assert(AWS_DEMO_MAX_CONNECTED_MS==3600000 && SERVO_TRAVEL_MS==700);
        // Agotar cada solicitud de ambos tramos, sin desbordar el techo.
        ShadowBudget exhaustive; exhaustive.advance(100);
        for (unsigned i=0;i<800;++i) {
            assert(exhaustive.allows(ShadowBudget::Sensor,1000,200));
            ++exhaustive.record.counts[ShadowBudget::Sensor];
        }
        assert(!exhaustive.allows(ShadowBudget::Periodic,1000,200));
        for (unsigned i=0;i<200;++i) {
            auto kind=i%2 ? ShadowBudget::Control : ShadowBudget::Recovery;
            assert(exhaustive.allows(kind,1000,200)); ++exhaustive.record.counts[kind];
        }
        for (auto kind : {ShadowBudget::Periodic,ShadowBudget::Sensor,ShadowBudget::Control,ShadowBudget::Recovery})
            assert(!exhaustive.allows(kind,1000,200));
        Fake::now=10000;
        AwsIoTManager a; init(a);
        a.connectStep(Fake::now);
        assert(a.mqtt_.connects==1 && a.mqtt_.subscriptions.size()==5);
        assert(a.mqtt_.subscriptions[3] == TOPIC_SHADOW_UPDATE_ACCEPTED);
        assert(a.mqtt_.subscriptions[4] == TOPIC_SHADOW_UPDATE_REJECTED);
        assert(a.mqtt_.topics.size()==1 && a.mqtt_.topics[0]==TOPIC_SHADOW_GET);
        a.step(); a.step(); assert(a.mqtt_.topics.size()==1); // GET unico
        auto getToken=std::string(a.token_);
        receive(a,TOPIC_SHADOW_GET_ACCEPTED,"{\"clientToken\":\""+getToken+"\",\"version\":1,\"state\":{\"reported\":{\"barrera\":\"cerrada\",\"distancia_cm\":20,\"vehiculo_detectado\":false}}}");
        assert(a.synced_ && a.haveConfirmed_ && a.attempts_==0);
        a.current_.ready=true; a.current_.open=false; a.current_.valid=true; a.current_.distance=20;
        a.current_.presence=0; a.stablePresence_=0;
        a.step(); assert(a.mqtt_.topics.size()==1);
        legacy(a, 2, "abierta", 2);
        AwsIoTManager::Command cmd;
        assert(xQueueReceive(a.commands_,&cmd,0) && cmd.order.open);
        a.step(); assert(a.mqtt_.topics.size()==1); // Esperar ejecucion, no confirmar anticipadamente
        a.current_.applied=cmd.sequence; a.current_.open=true; a.current_.ready=false; a.current_.moving=true;
        a.step(); assert(a.mqtt_.topics.size()==1);
        a.current_.ready=true; a.current_.moving=false; a.step(); assert(a.inflight_ && a.mqtt_.topics.size()==2);
        assert(a.mqtt_.bodies.back().find("desired")==std::string::npos);
        ack(a); assert(a.confirmed_.open && !a.inflight_);
        legacy(a, 3, "abierta", 2);
        a.step(); assert(a.mqtt_.topics.size()==2 && !a.commands_->full);
        legacy(a, 2, "cerrada", 0);
        assert(!a.commands_->full); // version atrasada
        legacy(a, 4, "cerrada", 2);
        assert(xQueueReceive(a.commands_,&cmd,0) && !cmd.order.open);
        a.current_.applied=cmd.sequence; a.current_.open=false; a.step(); ack(a);
        assert(!a.confirmed_.open && a.budget_.record.counts[ShadowBudget::Control]==2);
        a.current_.distance=30; a.step(); assert(!a.inflight_);
        Fake::now+=AWS_MIN_PUBLISH_INTERVAL_MS-1; a.step(); assert(!a.inflight_);
        ++Fake::now;
        a.current_.distance=29.9f; a.step(); assert(!a.inflight_);
        a.current_.distance=30; a.step(); assert(a.inflight_); ack(a);
        assert(a.budget_.record.counts[ShadowBudget::Sensor]==1);
        Fake::now+=AWS_MIN_PUBLISH_INTERVAL_MS;
        a.current_.distance=6.2f;
        a.current_.presence=1; a.stablePresence_=1; a.stableCount_=3; a.step(); assert(a.inflight_); ack(a);
        const auto sensorCount = a.budget_.record.counts[ShadowBudget::Sensor];
        const auto acceptedCount = a.status_.accepted;
        Fake::now+=AWS_REPORT_INTERVAL_MS-1; a.step(); assert(!a.inflight_);
        ++Fake::now; a.step(); assert(a.inflight_);
        DynamicJsonDocument periodic(1024);
        assert(!deserializeJson(periodic, a.mqtt_.bodies.back()));
        const auto reported = periodic["state"]["reported"];
        assert(reported["distancia_cm"].as<float>() == 6.2f);
        assert(reported["vehiculo_detectado"].is<bool>() && reported["vehiculo_detectado"].as<bool>());
        assert(a.status_.accepted == acceptedCount);
        ack(a);
        assert(a.status_.accepted == acceptedCount + 1);
        assert(a.budget_.record.counts[ShadowBudget::Sensor] == sensorCount);
        assert(a.budget_.record.counts[ShadowBudget::Periodic]==1);
        auto persisted=Preferences::disk;
        ShadowBudgetRecord restored; std::memcpy(&restored,persisted.data(),sizeof(restored));
        assert(restored.counts[ShadowBudget::Control]==2 && restored.counts[ShadowBudget::Recovery]==1);
        AwsIoTManager restarted;
        assert(restarted.loadBudget());
        assert(restarted.budget_.total()==a.budget_.total());
        assert(restarted.budget_.record.day==a.budget_.record.day);
        Preferences::disk[0]=0;
        AwsIoTManager corrupt;
        assert(!corrupt.loadBudget());
        Preferences::disk=persisted;
        a.budget_.record.counts[ShadowBudget::Periodic]=800;
        assert(!a.reserve(ShadowBudget::Sensor));
        assert(a.reserve(ShadowBudget::Control));
        Preferences::writable=false;
        assert(!a.reserve(ShadowBudget::Control) && !a.storageOK_);
        Preferences::writable=true; a.storageOK_=true;
        a.budget_.record.counts[ShadowBudget::Periodic]=0;
        a.mqtt_.online=false; a.step(); auto connects=a.mqtt_.connects;
        a.step(); assert(a.mqtt_.connects==connects); // backoff
        Fake::now=a.nextAttempt_; a.step(); assert(a.mqtt_.connects==connects+1 && a.getPending_);
        // Desired pendiente en el GET tras reconectar; no repetir GET en loop.
        auto pendingToken=std::string(a.token_);
        receive(a,TOPIC_SHADOW_GET_ACCEPTED,"{\"clientToken\":\""+pendingToken+"\",\"version\":10,\"state\":{\"desired\":{\"barrera\":\"abierta\"},\"reported\":{\"barrera\":\"cerrada\"}}}");
        assert(!a.commands_->full); // GET legado sin timestamp: no es seguro reproducirlo.
        a.clockMs_=Fake::now; a.connectedMs_=AWS_DEMO_MAX_CONNECTED_MS-1; a.step();
        assert(!a.demoEnded_); ++Fake::now; a.step();
        assert(a.demoEnded_ && !a.mqtt_.online);
        Fake::now+=AWS_RETRY_MAX_MS; a.step(); assert(a.mqtt_.connects==connects+1);
        // Debounce por muestras, nunca por numero de iteraciones de loop.
        AwsIoTManager stable; init(stable);
        stable.mqtt_.online=true; stable.synced_=true; stable.haveConfirmed_=true;
        stable.confirmed_.ready=true; stable.confirmed_.valid=true; stable.confirmed_.distance=20;
        stable.confirmed_.presence=0; stable.stablePresence_=0;
        stable.lastReport_=Fake::now; stable.lastPublish_=Fake::now;
        AwsIoTManager::Snapshot snapshot;
        snapshot.ready=true; snapshot.valid=true; snapshot.distance=20; snapshot.presence=1;
        for (unsigned i=1;i<=AWS_PRESENCE_STABLE_SAMPLES;++i) {
            snapshot.sample=i*500; xQueueOverwrite(stable.snapshots_,&snapshot);
            stable.step(); stable.step();
            assert(stable.stablePresence_==(i<AWS_PRESENCE_STABLE_SAMPLES?0:1));
        }
        assert(stable.inflight_); // presencia prioritaria sin esperar 10 s
        auto count=stable.mqtt_.topics.size();
        receive(stable,TOPIC_SHADOW_UPDATE_ACCEPTED,"{\"clientToken\":\"ajeno\"}");
        assert(stable.inflight_);
        Fake::now+=AWS_SHADOW_RESPONSE_TIMEOUT_MS;
        stable.step(); assert(stable.mqtt_.topics.size()==count+1);
        ack(stable);
        // Control nuevo tiene prioridad sobre informe pendiente de acuse.
        Fake::now+=AWS_MIN_PUBLISH_INTERVAL_MS; stable.current_.distance=40; stable.step();
        assert(stable.inflight_); auto oldToken=std::string(stable.token_);
        stable.current_.open=true; stable.step();
        assert(stable.inflight_ && std::string(stable.token_)!=oldToken);
        assert(stable.sent_.open);
        assert(stable.mqtt_.bodies.back().find("\"barrera_posicion_estimada\":true")!=std::string::npos);
        receive(stable,TOPIC_SHADOW_UPDATE_ACCEPTED,"{\"clientToken\":\""+oldToken+"\"}");
        assert(stable.inflight_); ack(stable);
        // Presencia nueva desplaza distancia en vuelo, sin esperar acuse ni intervalo.
        Fake::now+=AWS_MIN_PUBLISH_INTERVAL_MS; stable.current_.distance=50; stable.step();
        oldToken=stable.token_; count=stable.mqtt_.topics.size();
        stable.current_.presence=0; stable.stablePresence_=0; stable.step();
        assert(stable.mqtt_.topics.size()==count+1 && stable.sent_.presence==0);
        assert(std::string(stable.token_)!=oldToken); ack(stable);
        // Presencia no puede gastar la reserva; control si puede.
        auto priorBudget=stable.budget_.record;
        stable.budget_.record=ShadowBudgetRecord{}; stable.budget_.record.day=priorBudget.day;
        stable.budget_.record.counts[ShadowBudget::Periodic]=800;
        stable.current_.presence=1; stable.stablePresence_=1; count=stable.mqtt_.topics.size(); stable.step();
        assert(stable.mqtt_.topics.size()==count && stable.budget_.total()==800);
        stable.controlPending_=true; stable.step();
        assert(stable.budget_.total()==801 && stable.inflightKind_==ShadowBudget::Control); ack(stable);
        stable.budget_.record=priorBudget;
        // Rechazo correlacionado suspende envios, sin martillar AWS.
        Fake::now+=AWS_MIN_PUBLISH_INTERVAL_MS; stable.current_.distance=60; stable.step();
        receive(stable,TOPIC_SHADOW_UPDATE_REJECTED,"{\"clientToken\":\""+std::string(stable.token_)+"\",\"code\":400}");
        assert(stable.rejectedBlocked_); count=stable.mqtt_.topics.size();
        Fake::now+=AWS_REPORT_INTERVAL_MS; stable.step(); assert(stable.mqtt_.topics.size()==count);
        stable.rejectedBlocked_=false;
        // Agotamiento de cuota no bloquea recibir comandos; no envia mas.
        stable.budget_.record.counts[ShadowBudget::Periodic]=AWS_MAX_SHADOW_UPDATES_PER_DAY;
        count=stable.mqtt_.topics.size();
        legacy(stable, 20, "cerrada", 0);
        BarrierServo physicalDouble; assert(physicalDouble.begin());
        Parking parkingDouble; parkingDouble.begin();
        UltrasonicSensor sensorDouble; sensorDouble.begin();
        stable.applyPending(physicalDouble, parkingDouble, false);
        stable.tick(physicalDouble,sensorDouble,parkingDouble);
        assert(physicalDouble.commandedState()==BarrierServo::State::Closed);
        Fake::now+=SERVO_TRAVEL_MS; physicalDouble.update(); stable.tick(physicalDouble,sensorDouble,parkingDouble);
        stable.step(); assert(stable.mqtt_.topics.size()==count);
        // Recuperacion puede usar reserva aun con 800 consumidas; WiFi tiene backoff.
        AwsIoTManager recovery; init(recovery);
        for (auto& value : recovery.budget_.record.counts) value = 0;
        recovery.budget_.record.counts[ShadowBudget::Periodic]=800;
        WiFi.state=0; recovery.connectStep(Fake::now);
        assert(recovery.mqtt_.connects==0);
        WiFi.state=WL_CONNECTED; Fake::now=recovery.nextAttempt_;
        recovery.connectStep(Fake::now);
        assert(recovery.mqtt_.connects==1 && recovery.getPending_);
        assert(recovery.budget_.total()==801);
        assert(!recovery.reserve(ShadowBudget::Periodic));
        // Sin hora y NVS no hay solicitudes; nunca se borra el registro.
        auto saved=Preferences::disk;
        recovery.timeReady_=false; assert(!recovery.reserve(ShadowBudget::Recovery));
        assert(Preferences::disk==saved);
        // Un registro del firmware previo con 172 solicitudes se conserva al actualizar.
        ShadowBudgetRecord legacy; legacy.day=a.budget_.record.day; legacy.counts[0]=172;
        Preferences::disk.resize(sizeof(legacy)); std::memcpy(Preferences::disk.data(),&legacy,sizeof(legacy));
        AwsIoTManager upgraded; assert(upgraded.loadBudget()); upgraded.timeReady_=true;
        assert(upgraded.refreshDay() && upgraded.budget_.total()==172);
        assert(upgraded.budget_.allows(ShadowBudget::Recovery,1000,200));
        // Limite de intentos consecutivos conserva modo local.
        stable.disconnect(); stable.attempts_=AWS_MAX_CONNECTION_ATTEMPTS;
        count=stable.mqtt_.connects; stable.connectStep(Fake::now); assert(stable.mqtt_.connects==count);
        std::cout<<"PASS: AWS con dobles: GET unico, comandos, espera, reported, deduplicacion, versiones, distancia, presencia, periodo, cuota/reserva/NVS, backoff y demo\n";
    }
};
int main() { AwsIoTManagerTest::run(); AwsIoTManagerTest::testParkingShadow(); AwsIoTManagerTest::testLegacyParking(); AwsIoTManagerTest::testHistoryPublisher(); }
