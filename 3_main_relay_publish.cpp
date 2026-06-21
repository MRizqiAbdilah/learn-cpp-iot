#include <Arduino.h>
#include <WiFi.h>
#include <PsychicMqttClient.h>

// Inisialisasi Sensor
constexpr uint8_t PIN_RELAY = 25;

// Konfigurasi WiFi
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// Setup MQTT
constexpr const char *MQTT_SERVER = "mqtt://broker.emqx.io:1883";

constexpr const char *TOPIC_SUB_POMPA = "skripsi/hidroponik/kontrol/pompa";

uint32_t waktuSensor = 0;
uint32_t waktuJedaSensor = 1000;

uint32_t waktuPublish = 0;
uint32_t waktuJedaPublish = 5000;

uint32_t waktuMQTT = 0;
uint32_t waktuJedaMQTT = 5000; // Coba reconnect MQTT setiap 5 detik

PsychicMqttClient mqttClient;

bool statusWifi = false; // Flag untuk memastikan WiFi siap
bool statusMqtt = false; // Penanda status MQTT sebelumnya
bool statusPompa = false;

// Deklarasi Fungsi
void WiFiStationEvent(WiFiEvent_t event);
float readDistanceCm();
void publishPompa(bool statusPompa);

void setup()
{
    Serial.begin(115200);

    Serial.println("\n[SYSTEM] Memulai Sistem Hidroponik...");

    // 1. SETUP MQTT (Hanya konfigurasi, JANGAN connect dulu)
    mqttClient.setServer(MQTT_SERVER);
    mqttClient.onTopic(TOPIC_SUB_POMPA, 1, [&](const char *topic, const char *payload, int retain, int qos, bool dup)
                       {
      if (strcmp(payload, "ON") == 0) {
          digitalWrite(PIN_RELAY, HIGH);
      } else if (strcmp(payload, "OFF") == 0) {
          digitalWrite(PIN_RELAY, LOW);
      } });

    // 2. MULAI WIFI
    WiFi.onEvent(WiFiStationEvent);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    pinMode(PIN_RELAY, OUTPUT);
}

void loop()
{
    long waktuSekarang = millis();
    bool mqttConnected = mqttClient.connected();

    // --- DETEKSI PERUBAHAN STATUS MQTT (Edge Detection) ---
    if (mqttConnected && !statusMqtt)
    {
        Serial.println("[MQTT] Berhasil Terhubung ke Broker!");
        statusMqtt = true; // Update status
    }
    else if (!mqttConnected && statusMqtt)
    {
        Serial.println("[MQTT] Koneksi terputus dari broker!");
        statusMqtt = false; // Update status
    }

    // --- LOGIKA KONEKSI & RECONNECT MQTT ---
    if (statusWifi && !mqttConnected)
    {
        if (waktuSekarang - waktuMQTT > waktuJedaMQTT)
        {
            waktuMQTT = waktuSekarang;
            Serial.println("[MQTT] Mencoba terhubung ke broker..."); // 1. Ambil MAC Address dari modul WiFi
            String macAddress = WiFi.macAddress(); 

            // 2. Gabungkan dengan teks awalan
            String clientId = "Skripsi-Hidroponik-" + macAddress;

            // 3. Daftarkan Client ID ke MQTT
            mqttClient.setClientId(clientId.c_str());
            Serial.println(mqttClient.getClientId());
            mqttClient.connect();
        }
    }

    // --- LOGIKA SENSOR & PUBLISH ---
    if (waktuSekarang - waktuSensor > waktuJedaSensor)
    {
        waktuSensor = waktuSekarang;
        statusPompa = !statusPompa;

        if (mqttConnected && waktuSekarang - waktuPublish > waktuJedaPublish)
        {
            waktuPublish = waktuSekarang;
            publishPompa(statusPompa);
        }
    }
}

void WiFiStationEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("[WIFI] Station Mode dimulai.");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("[WIFI] Terhubung ke Access Point. Menunggu IP...");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("[WIFI] Sistem Hidroponik Online! IP Address: ");
        Serial.println(WiFi.localIP());
        statusWifi = true; // Izinkan MQTT untuk mulai mencoba koneksi di loop()
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("[WIFI] Koneksi terputus! Mencoba menyambung kembali...");
        statusWifi = false; // Hentikan proses MQTT
        WiFi.begin(ssid, password);
        break;
    default:
        break;
    }
}

void publishPompa(boolean statusPompa)
{
    char payloadPompa[10];

    if (statusPompa)
    {
        snprintf(payloadPompa, sizeof(payloadPompa), "%s", "ON");
    }
    else
    {
        snprintf(payloadPompa, sizeof(payloadPompa), "%s", "OFF");
    }

    mqttClient.publish(TOPIC_SUB_POMPA, 0, 0, payloadPompa);
    Serial.printf("[MQTT] Berhasil publish status pompa: %s\n", payloadPompa);
}