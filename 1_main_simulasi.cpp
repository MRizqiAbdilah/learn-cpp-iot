#include <WiFi.h>

// Konfigurasi Kredensial Jaringan
const char* ssid = "HIDROPONIK_FARM";
const char* password = "pertanianmodern";

// Setup MQTT
const char* MQTT_SERVER = "broker.emqx.io";
const uint16_t MQTT_PORT = 1883;

// TOPIC MQTT
const char* TOPIC_PUB_LEVEL_AIR = "skripsi/hidroponik/tandon/jarak";
const char* TOPIC_SUB_POMPA = "skripsi/hidroponik/kontrol/pompa";

// const char *ssid = "Wokwi-GUEST";
// const char *password = "";

constexpr uint8_t PIN_TRIG = 32;
constexpr uint8_t PIN_ECHO = 35;

uint32_t waktuSensorUltrasonik = 0;
uint32_t waktuJedaSensorUltrasonik = 1000;



// Callback function untuk menangani berbagai event jaringan secara asinkron
void WiFiStationEvent(WiFiEvent_t event);

// Fungsi untuk membaca jarak dari Sensor Ultrasonik (Header)
float readDistanceCm();

void setup()
{
    Serial.begin(115200);

    // Mendaftarkan Event Handler sebelum memulai proses WiFi
    WiFi.onEvent(WiFiStationEvent);

    // Konfigurasi mode WiFi sebagai Station (Client)
    WiFi.mode(WIFI_STA);

    Serial.println("\n[SYSTEM] Memulai Sistem Hidroponik...");

    // Inisialisasi MQTT
    Serial.println("[SYSTEM] MQTT Berhasil Terhubung...");

    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    // Memulai koneksi secara asinkron (non-blocking)
    WiFi.begin(ssid, password);
}

void loop()
{
    // Karena WiFi ditangani oleh event di background,
    // Loop utama bebas melakukan tugas kritis secara real-time.

    // Contoh: bacaSensorUltrasonik();
    // Contoh: kontrolPompaAir();
    long waktuSekarang = millis();
    if (waktuSekarang - waktuSensorUltrasonik > waktuJedaSensorUltrasonik)
    {
        waktuSensorUltrasonik = waktuSekarang;

        float jarakCm = readDistanceCm();
        // Serial.print("Jarak: ");
        // Serial.println(jarakCm);
        // Serial.print("cm");

        Serial.printf("Jarak: %.2fcm\n", jarakCm);
    }

    Serial.printf("Topic: %s\n", TOPIC_PUB_LEVEL_AIR);
    Serial.printf("Topic: %s\n", TOPIC_SUB_POMPA);

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
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("[WIFI] Koneksi terputus! Mencoba menyambung kembali...");
        // Logika Auto-Reconnect
        WiFi.begin(ssid, password);
        break;
    default:
        break;
    }
}

// Fungsi untuk membaca jarak dari Sensor Ultrasonik (Logic)
float readDistanceCm()
{

    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long durasi = pulseIn(PIN_ECHO, HIGH);

    return (durasi * 0.034) / 2;
}