#include "AwsIoTManager.h"
#include "config.h"
#include "secrets.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <time.h>
#include <math.h>
#include <string.h>

bool AwsIoTManager::begin(Parking& parking) {
    if (!loadBudget()) Serial.println("AWS: NVS invalida; envios bloqueados");
    remoteDebounceMs_ = IR_DEBOUNCE_DEFAULT_MS;
    uint16_t settings[2]{};
    if (storageOK_ && prefs_.isKey("parking") && prefs_.getBytesLength("parking") == sizeof(settings) &&
        prefs_.getBytes("parking", settings, sizeof(settings)) == sizeof(settings) &&
        settings[0] <= 1 && settings[1] >= IR_DEBOUNCE_MIN_MS && settings[1] <= IR_DEBOUNCE_MAX_MS) {
        remoteMode_ = settings[0] ? Parking::Mode::Automatic : Parking::Mode::Manual;
        remoteDebounceMs_ = settings[1];
    }
    parking.configure(remoteMode_, remoteDebounceMs_);
    snapshots_ = xQueueCreate(1, sizeof(Snapshot));
    commands_ = xQueueCreate(1, sizeof(Command));
    statuses_ = xQueueCreate(1, sizeof(Status));
    if (!snapshots_ || !commands_ || !statuses_) return false;
    history_.begin();
    return xTaskCreate(taskEntry, "smartpark-aws", 12288, this, 1, &task_) == pdPASS;
}
void AwsIoTManager::applyPending(BarrierServo& barrier, Parking& parking, bool serialPending) {
    if (!task_) return;
    Command cmd;
    if (xQueuePeek(commands_, &cmd, 0) == pdTRUE && cmd.sequence > mainApplied_) {
        if (cmd.configure) parking.configure(cmd.mode, cmd.debounceMs);
        if (cmd.generation == generation_.load()) {
            if (cmd.motionSequence > mainMotion_) {
                mainMotion_ = cmd.motionSequence;
                if (!serialPending) {
                    mainOrder_ = cmd.order;
                    const auto now = time(nullptr);
                    if (now < AWS_MIN_VALID_EPOCH || now >= cmd.expiresAt ||
                        cmd.expiresAt - now > AWS_ORDER_TTL_S + 5) mainOrder_.outcome = Outcome::Expired;
                    else if (cmd.order.open && parking.state().openingBlocked()) mainOrder_.outcome = Outcome::Blocked;
                    else {
                        const bool moved = cmd.order.open ? barrier.open() : barrier.close();
                        if (moved) history_.motionStarted(cmd.order.origin, cmd.order.id);
                        mainOrder_.outcome = barrier.initialized() ? Outcome::Accepted : Outcome::Unavailable;
                    }
                }
            }
        }
        mainApplied_ = cmd.sequence;
    }
}
void AwsIoTManager::tick(const BarrierServo& barrier, const UltrasonicSensor& sensor, const Parking& parking) {
    if (!task_) return;
    Snapshot s;
    const auto& m = sensor.lastMeasurement();
    s.sample = m.timestampMs;
    s.valid = m.status == UltrasonicSensor::ReadingStatus::Valid;
    s.distance = m.distanceCm;
    s.presence = sensor.presence() == UltrasonicSensor::Presence::Unknown ? -1 :
                 (sensor.presence() == UltrasonicSensor::Presence::Present ? 1 : 0);
    s.open = barrier.commandedState() == BarrierServo::State::Open;
    s.ready = barrier.positionEstimated();
    s.moving = barrier.moving();
    s.applied = mainApplied_; s.serial = mainSerial_;
    s.parkingValid = true;
    s.parking = parking.state();
    s.order = mainOrder_;
    xQueueOverwrite(snapshots_, &s);
    history_.observeParking(parking.state(), sensor, millis());
}
void AwsIoTManager::noteSerialCommand(bool open, bool blocked, bool available, bool moved) {
    if (moved) history_.motionStarted(Origin::Edge, "");
    ++mainSerial_;
    mainOrder_ = Order{};
    mainOrder_.origin = Origin::Edge;
    mainOrder_.open = open;
    mainOrder_.outcome = blocked ? Outcome::Blocked : (available ? Outcome::Accepted : Outcome::Unavailable);
}
void AwsIoTManager::observeBarrier(const BarrierServo& barrier) {
    if (task_) history_.observeBarrier(barrier);
}
void AwsIoTManager::printStatus() {
    Status s;
    if (!statuses_ || xQueuePeek(statuses_, &s, 0) != pdTRUE) {
        Serial.println("AWS: tarea no iniciada o inicializando; control local disponible"); return;
    }
    Serial.printf("AWS WiFi=%s MQTT/TLS=%s ultimo=%s reconexiones=%lu\n",
        s.wifi?"conectado":"desconectado", s.mqtt?"conectado":"desconectado",
        s.lastCommand, (unsigned long)s.reconnects);
    Serial.printf("Shadow UPDATE=%lu solicitudesTotales=%lu: periodicas=%lu sensor=%lu control=%lu GET=%lu; accepted=%lu rejected=%lu omitidas=%lu\n",
        (unsigned long)(s.counts[0]+s.counts[1]+s.counts[2]),
        (unsigned long)(s.counts[0]+s.counts[1]+s.counts[2]+s.counts[3]),
        (unsigned long)s.counts[0], (unsigned long)s.counts[1], (unsigned long)s.counts[2],
        (unsigned long)s.counts[3], (unsigned long)s.accepted, (unsigned long)s.rejected, (unsigned long)s.skipped);
    Serial.printf("Diagnostico NVS=%s horaUTC=%s recuperacion=%s MQTTstate=%d intentos=%lu siguiente=%lu ms\n",
        s.storage?"OK":"ERROR", s.time?"OK":"INVALIDA", s.recoveryOK?"disponible":"bloqueada",
        s.mqttState, (unsigned long)s.attempts, (unsigned long)s.retryIn);
    Serial.printf("Presupuesto ordinario=%s diaUTC=%lu; ultima publicacion hace=%lu ms (0 si ninguna); demo=%s conectadoAcumulado=%lu ms; reintentos=%s\n",
        s.budgetOK?"disponible":"agotado/sin hora/NVS", (unsigned long)s.day,
        (unsigned long)(s.lastPublish ? millis()-s.lastPublish : 0),
        s.demoEnded?"finalizada":"activa", (unsigned long)s.connectedMs, s.stopped?"suspendidos":"habilitados");
    Serial.printf("Historico: MQTT local=%lu cola_llena=%lu sin_hora=%lu fallos_envio=%lu descartes_red=%lu\n",
        (unsigned long)s.history.written, (unsigned long)s.history.overflow,
        (unsigned long)s.history.withoutClock, (unsigned long)s.history.failures,
        (unsigned long)s.history.discarded);
}
void AwsIoTManager::taskEntry(void* arg) { static_cast<AwsIoTManager*>(arg)->run(); }
bool AwsIoTManager::loadBudget() {
    storageOK_ = prefs_.begin("smartpark-aws", false);
    if (storageOK_ && prefs_.isKey("budget")) {
        ShadowBudgetRecord r;
        storageOK_ = prefs_.getBytesLength("budget") == sizeof(r) &&
            prefs_.getBytes("budget", &r, sizeof(r)) == sizeof(r) && r.magic == budget_.record.magic;
        if (storageOK_) {
            budget_.record = r;
            uint64_t total = 0;
            for (auto count : r.counts) total += count;
            storageOK_ = r.day > 0 && total <= AWS_MAX_SHADOW_UPDATES_PER_DAY;
        }
    }
    if (storageOK_ && prefs_.isKey("orders")) {
        ledgerOK_ = prefs_.getBytesLength("orders") == sizeof(ledger_) &&
                    prefs_.getBytes("orders", &ledger_, sizeof(ledger_)) == sizeof(ledger_) &&
                    ledger_.modernId[sizeof(ledger_.modernId) - 1] == '\0';
    }
    return storageOK_;
}
void AwsIoTManager::run() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    tls_.setCACert(AWS_ROOT_CA);
    tls_.setCertificate(AWS_DEVICE_CERT);
    tls_.setPrivateKey(AWS_PRIVATE_KEY);
    tls_.setHandshakeTimeout(5);
    tls_.setTimeout(2); // WiFiClientSecure expresa este valor en segundos.
    mqtt_.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
    if (!mqtt_.setBufferSize(MQTT_BUFFER_SIZE)) {
        Serial.println("AWS: sin memoria MQTT, red suspendida");
        attempts_ = AWS_MAX_CONNECTION_ATTEMPTS;
    }
    mqtt_.setKeepAlive(120); // PING de protocolo, sin heartbeat de aplicacion.
    mqtt_.setSocketTimeout(2);
    mqtt_.setCallback([this](char* t, uint8_t* p, unsigned int n) { receive(t,p,n); });
    retryDelay_ = WIFI_RETRY_INTERVAL_MS;
    clockMs_ = millis();
    for (;;) { step(); vTaskDelay(pdMS_TO_TICKS(20)); }
}
bool AwsIoTManager::refreshDay() {
    const time_t epoch = time(nullptr);
    if (!storageOK_ || !timeReady_ || epoch < AWS_MIN_VALID_EPOCH) return false;
    const uint32_t day = static_cast<uint32_t>(epoch/86400);
    if (day < budget_.record.day) return false; // Retroceso temporal: cerrar presupuesto.
    if (budget_.advance(day)) {
        storageOK_ = prefs_.putBytes("budget", &budget_.record, sizeof(budget_.record)) == sizeof(budget_.record);
    }
    return storageOK_;
}
bool AwsIoTManager::reserve(ShadowBudget::Kind kind) {
    if (!refreshDay() || !budget_.allows(kind, AWS_MAX_SHADOW_UPDATES_PER_DAY, AWS_RESERVED_CONTROL_UPDATES)) return false;
    ++budget_.record.counts[kind];
    // Cuenta intentos incluso si TCP falla: nunca devolver una reserva incierta.
    storageOK_ = prefs_.putBytes("budget", &budget_.record, sizeof(budget_.record)) == sizeof(budget_.record);
    return storageOK_;
}
void AwsIoTManager::disconnect() {
    ++generation_;
    mqtt_.disconnect(); tls_.stop();
    synced_ = false; inflight_ = false; getPending_ = false;
}
void AwsIoTManager::failedAttempt(uint32_t now) {
    const int mqttState = mqtt_.state();
    disconnect(); ++attempts_;
    const uint32_t jitter = esp_random() % 1000;
    const uint32_t wait = retryDelay_ > AWS_RETRY_MAX_MS-jitter ? AWS_RETRY_MAX_MS : retryDelay_+jitter;
    nextAttempt_ = now + wait;
    retryDelay_ = retryDelay_ >= AWS_RETRY_MAX_MS/2 ? AWS_RETRY_MAX_MS : retryDelay_*2;
    Serial.printf("AWS: intento fallido %lu/%u; siguiente en %lu ms; MQTT=%d\n",
        (unsigned long)attempts_, AWS_MAX_CONNECTION_ATTEMPTS,
        (unsigned long)wait, mqttState);
}
void AwsIoTManager::connectStep(uint32_t now) {
    if (demoEnded_ || attempts_ >= AWS_MAX_CONNECTION_ATTEMPTS || !storageOK_) return;
    if (static_cast<int32_t>(now-nextAttempt_) < 0) return;
    if (WiFi.status() != WL_CONNECTED) {
        // Una llamada por ventana; WiFi.begin no espera asociacion.
        WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        ++attempts_;
        const uint32_t jitter = esp_random()%1000;
        nextAttempt_ = now + (retryDelay_ > AWS_RETRY_MAX_MS-jitter ? AWS_RETRY_MAX_MS : retryDelay_+jitter);
        retryDelay_ = retryDelay_ >= AWS_RETRY_MAX_MS/2 ? AWS_RETRY_MAX_MS : retryDelay_*2;
        Serial.printf("AWS WiFi: intento %lu/%u\n", (unsigned long)attempts_, AWS_MAX_CONNECTION_ATTEMPTS);
        return;
    }
    if (!ntpStarted_) {
        configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
        ntpStarted_ = true; ntpAt_ = now;
        Serial.println("AWS WiFi conectado; esperando NTP");
    }
    if (!timeReady_) {
        if (time(nullptr) >= AWS_MIN_VALID_EPOCH) timeReady_ = true;
        else {
            if (now-ntpAt_ >= AWS_NTP_TIMEOUT_MS) {
                Serial.println("AWS: NTP sin hora valida; reintento con backoff");
                ntpStarted_ = false; failedAttempt(now);
            }
            return;
        }
    }
    history_.enableClock();
    if (!refreshDay() || !budget_.allows(ShadowBudget::Recovery, AWS_MAX_SHADOW_UPDATES_PER_DAY, AWS_RESERVED_CONTROL_UPDATES)) return;
    Serial.println("AWS: conectando MQTT con TLS X.509 verificado");
    if (!mqtt_.connect(THING_NAME)) { failedAttempt(millis()); return; }
    const char* topics[] = {TOPIC_SHADOW_DELTA, TOPIC_SHADOW_GET_ACCEPTED,
        TOPIC_SHADOW_GET_REJECTED, TOPIC_SHADOW_UPDATE_ACCEPTED, TOPIC_SHADOW_UPDATE_REJECTED};
    for (const char* topic : topics) {
        if (!mqtt_.subscribe(topic, 1)) { failedAttempt(millis()); return; }
    }
    if (connections_++) ++status_.reconnects;
    // Solo una sincronizacion GET valida restablece los fallos consecutivos.
    ++attempts_; retryDelay_ = MQTT_RETRY_INTERVAL_MS;
    version_ = 0; requestAttempts_ = 0; rejectedBlocked_ = false;
    Serial.println("AWS MQTT/TLS conectado; suscripciones enviadas; GET unico");
    requestShadow();
}
void AwsIoTManager::requestShadow() {
    if (!mqtt_.connected() || !reserve(ShadowBudget::Recovery)) return;
    snprintf(token_, sizeof(token_), "%08lx-%lu", (unsigned long)esp_random(), (unsigned long)++tokenSequence_);
    char body[96]; snprintf(body, sizeof(body), "{\"clientToken\":\"%s\"}", token_);
    getPending_ = true; requestAt_ = millis(); lastPublish_ = requestAt_;
    if (!mqtt_.publish(TOPIC_SHADOW_GET, body)) failedAttempt(millis());
}
bool AwsIoTManager::rememberOrder(const OrderLedger& next) {
    if (!ledgerOK_ || !storageOK_) return false;
    ledgerOK_ = prefs_.putBytes("orders", &next, sizeof(next)) == sizeof(next);
    if (ledgerOK_) ledger_ = next;
    return ledgerOK_;
}

void AwsIoTManager::applyDesired(JsonObjectConst desired, JsonObjectConst metadata,
                                bool get) {
    if (!remoteDebounceMs_) remoteDebounceMs_ = IR_DEBOUNCE_DEFAULT_MS;
    auto mode = remoteMode_;
    auto debounce = remoteDebounceMs_;
    bool validConfig = true;
    const bool hasMode = desired.containsKey("modo");
    const bool hasParams = desired.containsKey("parametros_auto");
    if (hasMode) {
        const char* value = desired["modo"] | "";
        if (!strcmp(value, "automatico")) mode = Parking::Mode::Automatic;
        else if (!strcmp(value, "manual")) mode = Parking::Mode::Manual;
        else validConfig = false;
    }
    if (hasParams) {
        const auto value = desired["parametros_auto"]["antirrebote_ir_ms"];
        if (!value.is<uint16_t>() || value.as<uint16_t>() < IR_DEBOUNCE_MIN_MS ||
            value.as<uint16_t>() > IR_DEBOUNCE_MAX_MS) validConfig = false;
        else debounce = value.as<uint16_t>();
    }

    if (validConfig && (mode != remoteMode_ || debounce != remoteDebounceMs_)) {
        const uint16_t settings[] = {static_cast<uint16_t>(mode == Parking::Mode::Automatic), debounce};
        validConfig = storageOK_ && prefs_.putBytes("parking", settings, sizeof(settings)) == sizeof(settings);
    }
    Command cmd;
    cmd.generation = generation_.load();
    cmd.mode = validConfig ? mode : remoteMode_;
    cmd.debounceMs = validConfig ? debounce : remoteDebounceMs_;
    cmd.configure = validConfig && (get || hasMode || hasParams);

    const auto modern = desired["orden_barrera"];
    const uint32_t legacyTime = metadata["barrera"]["timestamp"] | 0U;
    const uint32_t modernTime = metadata["orden_barrera"]["id"]["timestamp"] | 0U;
    const bool useModern = !modern.isNull() && (!legacyTime || modernTime >= legacyTime);
    const char* target = useModern ? (modern["objetivo"] | "") : (desired["barrera"] | "");
    const bool open = !strcmp(target, "abierta");
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    if (now >= AWS_MIN_VALID_EPOCH && (open || !strcmp(target, "cerrada"))) {
        OrderLedger next = ledger_;
        if (useModern) {
            const char* id = modern["id"] | "";
            const char* origin = modern["origen"] | "";
            const uint64_t issued = modern["solicitada_ms"] | uint64_t{0};
            const uint32_t expires = modern["vence_s"] | 0U;
            const uint64_t nowMs = uint64_t{now} * 1000;
            if (*id && strlen(id) <= 32 && strspn(id, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") == strlen(id) &&
                (!strcmp(origin, "voz") || !strcmp(origin, "nube")) &&
                issued > ledger_.modern && strcmp(id, ledger_.modernId) && issued <= nowMs + 5000 &&
                issued >= uint64_t{ledger_.legacy[0]} * 1000 && issued >= uint64_t{ledger_.legacy[1]} * 1000 &&
                expires > now && uint64_t{expires} * 1000 > issued &&
                uint64_t{expires} * 1000 - issued <= uint64_t{AWS_ORDER_TTL_S} * 1000) {
                next.modern = issued;
                strcpy(next.modernId, id);
                if (rememberOrder(next) && validConfig) {
                    strcpy(cmd.order.id, id);
                    cmd.order.origin = !strcmp(origin, "voz") ? Origin::Voice : Origin::Cloud;
                    cmd.expiresAt = expires;
                }
            }
        } else if (legacyTime > ledger_.legacy[open] && legacyTime >= ledger_.legacy[!open] &&
                   legacyTime > ledger_.modern / 1000 && legacyTime <= now && now - legacyTime < AWS_ORDER_TTL_S) {
            next.legacy[open] = legacyTime;
            if (rememberOrder(next) && validConfig) {
                cmd.order.origin = Origin::Legacy;
                cmd.expiresAt = legacyTime + AWS_ORDER_TTL_S;
            }
        }
    }
    cmd.order.open = open;
    if (get && !hasMode && !hasParams && cmd.order.origin == Origin::None) return;
    if (cmd.configure || cmd.order.origin != Origin::None) {
        if (cmd.order.origin != Origin::None) cmd.motionSequence = desiredSequence_ + 1;
        else if (pendingCommand_.generation == cmd.generation && validConfig) {
            cmd.order = pendingCommand_.order;
            cmd.motionSequence = pendingCommand_.motionSequence;
            cmd.expiresAt = pendingCommand_.expiresAt;
        }
        // Every mailbox entry carries the latest accepted configuration.
        cmd.configure = true;
        remoteMode_ = cmd.mode;
        remoteDebounceMs_ = cmd.debounceMs;
        controlPending_ = true;
        cmd.sequence = ++desiredSequence_;
        pendingCommand_ = cmd;
        xQueueOverwrite(commands_, &cmd);
    }
}
void AwsIoTManager::receive(char* topic, uint8_t* payload, unsigned int length) {
    static const auto filter = [] {
        StaticJsonDocument<2048> fields;
        fields["clientToken"] = fields["version"] = fields["code"] = true;
        auto state = fields.createNestedObject("state");
        auto desired = state.createNestedObject("desired");
        for (auto target : {state, desired}) {
            target["barrera"] = target["modo"] = true;
            target["parametros_auto"]["antirrebote_ir_ms"] = true;
            auto order = target.createNestedObject("orden_barrera");
            for (const auto* key : {"id", "objetivo", "origen", "solicitada_ms", "vence_s"}) order[key] = true;
        }
        auto reported = state.createNestedObject("reported");
        reported["barrera"] = reported["distancia_cm"] = reported["vehiculo_detectado"] = true;
        auto metadata = fields.createNestedObject("metadata");
        auto desiredMetadata = metadata.createNestedObject("desired");
        for (auto target : {metadata, desiredMetadata}) {
            target["barrera"]["timestamp"] = true;
            target["orden_barrera"]["id"]["timestamp"] = true;
        }
        return fields;
    }();
    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, payload, length, DeserializationOption::Filter(filter))) { Serial.println("AWS: JSON invalido/excede buffer; se esperara timeout si corresponde"); return; }
    const char* receivedToken = doc["clientToken"] | "";
    const bool ours = !strcmp(receivedToken, token_);
    if (!strcmp(topic, TOPIC_SHADOW_UPDATE_ACCEPTED)) {
        if (!inflight_ || !ours) return;
        inflight_ = false; ++status_.accepted;
        confirmed_ = sent_; haveConfirmed_ = true;
        requestAttempts_ = 0;
        if (current_.open == confirmed_.open && sent_.applied >= desiredSequence_ && current_.serial == sent_.serial) controlPending_ = false;
        return;
    }
    if (!strcmp(topic, TOPIC_SHADOW_UPDATE_REJECTED)) {
        if (!inflight_ || !ours) return;
        inflight_ = false; ++status_.rejected;
        rejectedBlocked_ = true; // No martillar AWS con un documento/politica erroneos.
        Serial.printf("AWS: UPDATE rechazado codigo=%d; informes suspendidos hasta reconectar\n", doc["code"].as<int>());
        return;
    }
    if (!strcmp(topic, TOPIC_SHADOW_GET_REJECTED)) {
        if (!getPending_ || !ours) return;
        getPending_ = false; ++status_.rejected;
        if (doc["code"].as<int>() == 404) { synced_ = true; haveConfirmed_ = false; attempts_ = 0; }
        else { rejectedBlocked_ = true; Serial.println("AWS GET rechazado; informes suspendidos"); }
        return;
    }
    bool get = !strcmp(topic, TOPIC_SHADOW_GET_ACCEPTED);
    if (!get && strcmp(topic, TOPIC_SHADOW_DELTA)) return;
    if (get && (!getPending_ || !ours)) return;
    if (!get && !synced_) {
        if (getPending_) {
            const uint32_t incoming = doc["version"] | 0U;
            if (incoming > version_) version_ = incoming;
        }
        return;
    }
    const uint32_t version = doc["version"] | 0U;
    if (get) {
        // A newer delta observed during GET requires resynchronization.
        if (version < version_ || !version) { failedAttempt(millis()); return; }
        getPending_ = false; synced_ = true; attempts_ = 0;
        JsonObject r = doc["state"]["reported"];
        const char* b = r["barrera"] | "";
        confirmed_ = Snapshot{};
        confirmed_.ready = !strcmp(b,"abierta") || !strcmp(b,"cerrada");
        confirmed_.open = !strcmp(b,"abierta");
        confirmed_.valid = r["distancia_cm"].is<float>();
        confirmed_.distance = r["distancia_cm"] | 0.0f;
        confirmed_.presence = r["vehiculo_detectado"].is<bool>() ? (r["vehiculo_detectado"].as<bool>()?1:0) : -1;
        haveConfirmed_ = true;
        version_ = version;
        applyDesired(doc["state"]["desired"], doc["metadata"]["desired"], true);
    } else {
        if (version <= version_) return;
        version_ = version;
        applyDesired(doc["state"], doc["metadata"], false);
    }
}
void AwsIoTManager::report(ShadowBudget::Kind kind) {
    if (!reserve(kind)) return;
    StaticJsonDocument<1536> doc;
    snprintf(token_, sizeof(token_), "%08lx-%lu", (unsigned long)esp_random(), (unsigned long)++tokenSequence_);
    doc["clientToken"] = token_;
    JsonObject r = doc.createNestedObject("state").createNestedObject("reported");
    r["barrera_posicion_estimada"] = current_.ready; // SG90 sin realimentacion fisica.
    if (current_.ready) r["barrera"] = current_.open ? "abierta" : "cerrada";
    else r["barrera"] = nullptr;
    if (current_.parkingValid) {
        const auto& parking = current_.parking;
        auto slots = r.createNestedObject("slots");
        constexpr const char* keys[] = {"1", "2", "3", "4"};
        constexpr const char* states[] = {"desconocido", "libre", "ocupado"};
        for (uint8_t i = 0; i < Parking::capacity; ++i)
            slots[keys[i]] = states[static_cast<uint8_t>(parking.slots[i])];
        // Shadow merges nested objects; null removes slots from an earlier larger configuration.
        for (uint8_t i = Parking::capacity; i < sizeof(keys) / sizeof(keys[0]); ++i)
            slots[keys[i]] = nullptr;
        r["ocupados"] = parking.occupied;
        r["disponibles"] = parking.available;
        r["capacidad"] = Parking::capacity;
        r["modo"] = parkingMode(parking.mode);
        r["lleno"] = parking.full();
        r["bloqueo_apertura"] = parking.openingBlocked();
        r.createNestedObject("parametros_auto")["antirrebote_ir_ms"] = parking.debounceMs;
    }
    if (current_.order.origin != Origin::None) {
        constexpr const char* origins[] = {"", "legado", "voz", "nube", "edge"};
        constexpr const char* outcomes[] = {"", "aceptada", "bloqueada", "vencida", "no_disponible"};
        auto last = r.createNestedObject("ultima_orden");
        if (*current_.order.id) last["id"] = current_.order.id;
        else last["id"] = nullptr;
        last["objetivo"] = current_.order.open ? "abierta" : "cerrada";
        last["origen"] = origins[static_cast<uint8_t>(current_.order.origin)];
        last["resultado"] = outcomes[static_cast<uint8_t>(current_.order.outcome)];
    }
    // null borra datos viejos: un timeout nunca se presenta como distancia/presencia valida.
    if (current_.valid) r["distancia_cm"] = roundf(current_.distance*10)/10; else r["distancia_cm"] = nullptr;
    const int reportedPresence = current_.valid ? stablePresence_ : -1;
    if (reportedPresence >= 0) r["vehiculo_detectado"] = reportedPresence == 1; else r["vehiculo_detectado"] = nullptr;
    char body[768];
    if (doc.overflowed() || measureJson(doc) >= sizeof(body)) {
        Serial.println("AWS: reporte excede capacidad");
        return;
    }
    serializeJson(doc, body, sizeof(body));
    sent_ = current_; sent_.presence = reportedPresence;
    inflightKind_ = kind;
    inflight_ = true; requestAt_ = millis(); lastPublish_ = requestAt_;
    lastReport_ = requestAt_;
    ++requestAttempts_;
    Serial.printf("AWS UPDATE clase=%u intento=%lu\n", static_cast<unsigned>(kind), (unsigned long)requestAttempts_);
    if (!mqtt_.publish(TOPIC_SHADOW_UPDATE, body)) failedAttempt(millis());
}
void AwsIoTManager::step() {
    uint32_t now = millis();
    const uint32_t elapsed = now-clockMs_; clockMs_ = now;
    if (mqtt_.connected()) connectedMs_ += elapsed;
    if (AWS_DEMO_MAX_CONNECTED_MS && connectedMs_ >= AWS_DEMO_MAX_CONNECTED_MS && !demoEnded_) {
        demoEnded_ = true; disconnect(); WiFi.disconnect(true);
        Serial.println("AWS: demo finalizada; no reconectar hasta reiniciar. Sensor y servo locales activos.");
    }
    Snapshot s;
    bool sampled = false;
    if (xQueueReceive(snapshots_, &s, 0) == pdTRUE) {
        current_ = s;
        if (s.serial != seenSerial_) {
            seenSerial_ = s.serial;
            snprintf(status_.lastCommand,sizeof(status_.lastCommand),"Serial %s",s.order.open?"OPEN":"CLOSE");
        }
        if (s.sample != lastSample_) {
            // Durante TLS pueden perderse muestras intermedias en la cola de ultima lectura.
            // No declarar estabilidad acumulando observaciones separadas por una reconexion.
            if (lastSample_ && s.sample-lastSample_ > SENSOR_INTERVAL_MS+ECHO_TIMEOUT_US/1000+100) {
                stableCount_ = 0; candidate_ = -2;
            }
            lastSample_ = s.sample; sampled = true;
            if (candidate_ == s.presence) { if (stableCount_ < AWS_PRESENCE_STABLE_SAMPLES) ++stableCount_; }
            else { candidate_ = s.presence; stableCount_ = 1; }
            if (stableCount_ >= AWS_PRESENCE_STABLE_SAMPLES) stablePresence_ = candidate_;
        }
    }
    if (!demoEnded_) {
        if (!mqtt_.connected()) {
            if (synced_ || inflight_ || getPending_) {
                Serial.println("AWS: conexion perdida; backoff"); failedAttempt(now);
            }
            connectStep(now);
        } else {
            mqtt_.loop(); now = millis();
            if (getPending_ && now-requestAt_ >= AWS_SHADOW_RESPONSE_TIMEOUT_MS) {
                Serial.println("AWS GET sin respuesta; resincronizacion con backoff"); failedAttempt(now);
            }
            // Un cambio ejecutado del servo no espera el acuse de un informe viejo.
            // La reserva anterior queda consumida; su token tardio sera ignorado.
            if (inflight_ && current_.ready && current_.applied >= desiredSequence_ && current_.open != sent_.open) {
                inflight_ = false; controlPending_ = true; requestAttempts_ = 0;
            }
            // Una presencia confirmada desplaza un reporte anterior sin esperar su timeout.
            if (inflight_ && inflightKind_ != ShadowBudget::Control && !controlPending_ && stableCount_ >= AWS_PRESENCE_STABLE_SAMPLES &&
                stablePresence_ != sent_.presence) inflight_ = false;
            if (inflight_ && now-requestAt_ >= AWS_SHADOW_RESPONSE_TIMEOUT_MS) {
                inflight_ = false;
                if (requestAttempts_ >= AWS_MAX_REQUEST_ATTEMPTS) {
                    rejectedBlocked_ = true;
                    Serial.println("AWS: sin confirmacion tras limite de intentos; informes suspendidos");
                }
            }
            const bool ready = !current_.moving && current_.applied >= desiredSequence_;
            if (mqtt_.connected() && synced_ && !inflight_ && !rejectedBlocked_ && ready) {
                bool control = controlPending_ || !haveConfirmed_ || current_.ready != confirmed_.ready ||
                    current_.open != confirmed_.open || current_.serial != confirmed_.serial ||
                    current_.applied != confirmed_.applied;
                bool presence = stableCount_ >= AWS_PRESENCE_STABLE_SAMPLES &&
                    stablePresence_ != confirmed_.presence;
                bool slotsChanged = current_.parkingValid && !confirmed_.parkingValid;
                for (uint8_t i = 0; current_.parkingValid && i < Parking::capacity; ++i)
                    slotsChanged |= current_.parking.slots[i] != confirmed_.parking.slots[i];
                bool sensor = slotsChanged || presence || current_.valid != confirmed_.valid ||
                    (current_.valid && fabsf(current_.distance-confirmed_.distance) >= AWS_DISTANCE_CHANGE_THRESHOLD_CM);
                bool periodic = now-lastReport_ >= AWS_REPORT_INTERVAL_MS;
                if (!control && !sensor && !periodic && sampled) ++status_.skipped;
                if (control || sensor || periodic) {
                    ShadowBudget::Kind kind = control ? ShadowBudget::Control : (sensor ? ShadowBudget::Sensor : ShadowBudget::Periodic);
                    bool interval = control || presence || now-lastPublish_ >= AWS_MIN_PUBLISH_INTERVAL_MS;
                    bool allowed = refreshDay() && budget_.allows(kind, AWS_MAX_SHADOW_UPDATES_PER_DAY,AWS_RESERVED_CONTROL_UPDATES);
                    if (!allowed && !limitLogged_) {
                        Serial.println("AWS: limite local/hora/NVS impide envios; lecturas y comandos siguen activos");
                        limitLogged_ = true;
                    }
                    if (allowed) limitLogged_ = false;
                    if (interval && allowed) report(kind);
                    else if (now-lastSkip_ >= 1000) {
                        // Contador de decisiones omitidas, como maximo una por segundo.
                        ++status_.skipped; lastSkip_ = now;
                    }
                }
            }
        }
    }
    if (synced_ && WiFi.status() == WL_CONNECTED) history_.publish(mqtt_, millis());
    updateStatus();
}
void AwsIoTManager::updateStatus() {
    status_.history = history_.stats();
    status_.time = timeReady_ && time(nullptr) >= AWS_MIN_VALID_EPOCH &&
        static_cast<uint32_t>(time(nullptr)/86400) >= budget_.record.day;
    status_.recoveryOK = refreshDay() && budget_.allows(ShadowBudget::Recovery,
        AWS_MAX_SHADOW_UPDATES_PER_DAY, AWS_RESERVED_CONTROL_UPDATES);
    status_.storage = storageOK_;
    status_.mqttState = mqtt_.state(); status_.attempts = attempts_;
    status_.retryIn = static_cast<int32_t>(nextAttempt_-millis()) > 0 ? nextAttempt_-millis() : 0;
    status_.wifi = WiFi.status() == WL_CONNECTED; status_.mqtt = mqtt_.connected();
    for (unsigned i=0;i<4;++i) status_.counts[i] = budget_.record.counts[i];
    status_.budgetOK = refreshDay() && budget_.allows(ShadowBudget::Periodic,AWS_MAX_SHADOW_UPDATES_PER_DAY,AWS_RESERVED_CONTROL_UPDATES);
    status_.day = budget_.record.day;
    status_.lastPublish = lastPublish_; status_.connectedMs = connectedMs_;
    status_.demoEnded = demoEnded_; status_.stopped = attempts_ >= AWS_MAX_CONNECTION_ATTEMPTS || rejectedBlocked_ || !storageOK_;
    xQueueOverwrite(statuses_, &status_);
}
