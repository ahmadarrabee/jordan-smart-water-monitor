#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>

constexpr int TRIG_PIN = 5, ECHO_PIN = 18, FLOW_PIN = 19;
constexpr float TANK_HEIGHT_CM = 150.0f;
// Calibrate the distance from the sensor face to the full water surface.
// Keep the full surface outside the sensor blind zone.
constexpr float SENSOR_FULL_DISTANCE_CM = 25.0f;
constexpr float FLOW_K_FACTOR = 7.5f;
constexpr uint32_t SAMPLE_MS = 5000;
constexpr size_t MAX_LOG_BYTES = 128 * 1024;
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// Public, unencrypted demonstration broker.
const char* mqtt_server = "broker.hivemq.com";
volatile uint32_t pulseCount = 0;
portMUX_TYPE pulseMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t lastSample = 0, lastReconnect = 0;
bool storageReady = false;
String nodeId, topic;
WiFiClient espClient;
PubSubClient client(espClient);

void IRAM_ATTR pulseCounter() {
    portENTER_CRITICAL_ISR(&pulseMux);
    ++pulseCount;
    portEXIT_CRITICAL_ISR(&pulseMux);
}

bool getFilteredDistance(float& distance) {
    float samples[10];
    int count = 0;
    for (int i = 0; i < 10; ++i) {
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        const unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
        const float cm = duration * 0.0343f / 2.0f;
        if (duration && cm >= SENSOR_FULL_DISTANCE_CM &&
            cm <= SENSOR_FULL_DISTANCE_CM + TANK_HEIGHT_CM) samples[count++] = cm;
        delay(60);
    }
    if (count < 6) return false;
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (samples[i] > samples[j]) {
                const float temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }
    distance = count % 2 ? samples[count / 2]
                        : (samples[count / 2 - 1] + samples[count / 2]) / 2.0f;
    return true;
}

void logToSPIFFS(const String& payload) {
    if (!storageReady) {
        Serial.println("Telemetry not saved: storage unavailable.");
        return;
    }
    File file = SPIFFS.open("/offline_log.jsonl", FILE_APPEND);
    if (!file || file.size() + payload.length() + 1 > MAX_LOG_BYTES) {
        Serial.println("Telemetry not saved: log unavailable or full; export and clear it.");
        return;
    }
    const size_t written = file.print(payload + "\n");
    file.close();
    Serial.println(written == payload.length() + 1 ? "Saved telemetry locally."
                                                   : "Telemetry write failed.");
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(FLOW_PIN, INPUT_PULLUP);
    // Provision the filesystem explicitly; never erase logs on a mount error.
    storageReady = SPIFFS.begin(false);
    if (!storageReady) Serial.println("SPIFFS unavailable; provision filesystem before use.");
    WiFi.mode(WIFI_STA);
    nodeId = "water-" + WiFi.macAddress();
    nodeId.replace(":", "");
    topic = "jordan_water/" + nodeId + "/telemetry";
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    configTime(0, 0, "pool.ntp.org");
    client.setServer(mqtt_server, 1883);
    client.setBufferSize(512);
    client.setSocketTimeout(2);
    lastSample = millis();
    lastReconnect = lastSample - SAMPLE_MS;
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);
}

void loop() {
    uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED && !client.connected() &&
        now - lastReconnect >= SAMPLE_MS) {
        lastReconnect = now;
        client.connect(nodeId.c_str());
    }
    client.loop();
    now = millis();
    if (now - lastSample < SAMPLE_MS) return;
    portENTER_CRITICAL(&pulseMux);
    const uint32_t pulses = pulseCount;
    pulseCount = 0;
    portEXIT_CRITICAL(&pulseMux);
    const float flow = (1000.0f * pulses / (now - lastSample)) / FLOW_K_FACTOR;
    lastSample = now;
    float distance = 0;
    const bool valid = getFilteredDistance(distance);
    const float level = constrain((TANK_HEIGHT_CM + SENSOR_FULL_DISTANCE_CM - distance)
                                  * 100.0f / TANK_HEIGHT_CM, 0.0f, 100.0f);
    StaticJsonDocument<512> doc;
    doc["node_id"] = nodeId;
    doc["uptime_ms"] = now; // Wraps after approximately 49.7 days.
    const time_t epoch = time(nullptr);
    if (epoch >= 1704067200) {
        struct tm utc;
        gmtime_r(&epoch, &utc);
        char timestamp[21];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utc);
        doc["timestamp"] = timestamp;
    } else doc["timestamp"] = nullptr;
    doc["level_valid"] = valid;
    if (valid) {
        doc["tank_level_pct"] = level;
        doc["high_level"] = level > 95.0f;
    } else {
        doc["tank_level_pct"] = nullptr;
        doc["high_level"] = nullptr;
    }
    doc["flow_rate_lpm"] = flow;
    String output;
    serializeJson(doc, output);
    if (client.connected() && client.publish(topic.c_str(), output.c_str())) {
        Serial.println("Sent to MQTT (QoS 0): " + output);
    } else logToSPIFFS(output);
}
