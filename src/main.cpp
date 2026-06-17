#include <Arduino.h>

// --- KONFIGURASI PIN ---
// RELAY 32, POTENTIOMETER 34, TRIG 21, ECHO 19, LED_MERAH 27, LED_BIRU 26, BTN 16
constexpr uint8_t PIN_RELAY = 2;
constexpr uint8_t PIN_POTENTIOMETER = 34;
constexpr uint8_t PIN_TRIG = 21;
constexpr uint8_t PIN_ECHO = 19;
constexpr uint8_t PIN_LED_MERAH = 27;
constexpr uint8_t PIN_LED_BIRU = 26;
constexpr uint8_t PIN_BTN = 16;

// -- INDIKATOR --
int INDIKATOR_ON = 1;

// --- STATE SISTEM ---
volatile bool sistemAktif = true; // Diubah oleh ISR
bool statusPompa = false;         // Diubah oleh logika sensor
int levelAir = 0;                 // Diubah oleh fungsi sensor
constexpr uint8_t MAX_TANGKI = 100;
int buttonState = 1;
// Variabel penyimpan state sebelumnya
bool statusSistemSebelumnya = true;

// --- TRACKER WAKTU ---
unsigned long waktuTerakhirLED = 0;
const unsigned long jedaLED = 300;

unsigned long waktuTerakhirSensor = 0;
const unsigned long jedaSensor = 2000;

unsigned long waktuTerakhirDebounce = 0;
const unsigned long jedaDebounce = 250; // Abaikan pantulan selama 250ms

// // ISR (Interrupt Service Routine)
// void IRAM_ATTR isrTombol()
// {
//     sistemAktif = !sistemAktif; // Toggle sistem
// }

void IRAM_ATTR isrTombol()
{
    unsigned long waktuSekarang = millis();
    // Hanya eksekusi jika jarak antar klik lebih dari 250 milidetik
    if (waktuSekarang - waktuTerakhirDebounce > jedaDebounce) {
        sistemAktif = !sistemAktif;
        waktuTerakhirDebounce = waktuSekarang;
    }
}

void setup()
{
    Serial.begin(115200);
    // LED INTERUPT, LED INDIKATOR NYALA
    pinMode(PIN_LED_MERAH, OUTPUT);
    pinMode(PIN_LED_BIRU, OUTPUT);

    // SENSOR HC-SR04
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    // POTENTIOMETER
    pinMode(PIN_POTENTIOMETER, INPUT);

    // RELAY
    pinMode(PIN_RELAY, OUTPUT);

    // BUTTON
    pinMode(PIN_BTN, INPUT_PULLUP);

    // INTTERUPT
    attachInterrupt(digitalPinToInterrupt(PIN_BTN), isrTombol, FALLING);
}

void loop()
{

    if (!sistemAktif)
    {
        if (statusSistemSebelumnya == true) {
            digitalWrite(PIN_RELAY, LOW); // Amankan mesin utama
            digitalWrite(PIN_LED_BIRU, LOW); 
            INDIKATOR_ON = 0;
            Serial.println("[SISTEM] STANDBY - Mesin Dimatikan");

            statusSistemSebelumnya = false; // Tandai bahwa sistem sudah mati
        }
        return;
    }

    // Kembalikan status jika sistem kembali dinyalakan
    if (statusSistemSebelumnya == false) {
        Serial.println("[SISTEM] AKTIF - Memulai ulang...");
        INDIKATOR_ON = 1;
        digitalWrite(PIN_LED_MERAH, LOW);
        statusSistemSebelumnya = true;
    }

    // --- TASK 1: Heartbeat LED (Non-blocking) ---
    unsigned long waktuSekarang = millis();
    if (waktuSekarang - waktuTerakhirLED >= jedaLED)
    {
        waktuTerakhirLED = waktuSekarang;
        if (INDIKATOR_ON == 1) {
            // Berkedip normal
            digitalWrite(PIN_LED_BIRU, !digitalRead(PIN_LED_BIRU)); 
        } else {
            // Mati total saat standby/error
            digitalWrite(PIN_LED_BIRU, LOW); 
        }
    }

    // --- TASK 2: (Lakukan pembacaan sensor dan kontrol relay di sini) ---
    if (waktuSekarang - waktuTerakhirSensor >= jedaSensor)
    {
        waktuTerakhirSensor = waktuSekarang;
        // Set Trigger Sensor-HCSR04
        // LOW - HIGH -> 2ms
        digitalWrite(PIN_TRIG, LOW);
        delayMicroseconds(2);
        // HIGH - LOW -> 10ms
        digitalWrite(PIN_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_TRIG, LOW);

        // Mengukur sinyal ECHO
        long durasi = pulseIn(PIN_ECHO, HIGH);

        // konversi ke cm
        float jarak_cm = (durasi * 0.034) / 2;

        // Mencetak hasil ke serial monitor
        Serial.print("Jarak (cm): ");
        Serial.print(jarak_cm);
        Serial.println(" cm");

        // konversi ke dalam persentase
        float jarak_persentase = (MAX_TANGKI - jarak_cm) / MAX_TANGKI * 100;

        // Mencetak hasil ke serial monitor
        Serial.print("Jarak (%): ");
        Serial.print(jarak_persentase);
        Serial.println("%");

        if (jarak_persentase < 20 && jarak_persentase > 0){
            levelAir = 1;
        } else if (jarak_persentase > 80 && jarak_persentase < 100) {
            levelAir = 0;
        } else if (jarak_persentase >= 20 && jarak_persentase <= 80){
            levelAir = 2;
        } else {
            levelAir = 3;
        }

        switch (levelAir)
        {
        case 0:
            Serial.println("[AKSI] Air Penuh, Pompa Dimatikan");
            digitalWrite(PIN_RELAY, LOW);
            statusPompa = false;
            break;
        case 1: 
            Serial.println("[AKSI] | Air Surut, Pompa Dinyalakan");
            digitalWrite(PIN_RELAY, HIGH);
            statusPompa = true;
            break;
        case 2: 
            Serial.println("[INFO] Air Aman");
            break;
        default:
            // Serial.println("Air Tidak Diketahui, Data Anomali Nyalakan LED MERAH");
            // digitalWrite(PIN_LED_MERAH, HIGH);
            // sistemAktif = false;
            // statusSistemSebelumnya = false;
            // break;
            Serial.println("[ERROR] Data Anomali! Pompa Dimatikan Darurat.");
            digitalWrite(PIN_LED_MERAH, HIGH);
            digitalWrite(PIN_LED_BIRU, LOW);
            digitalWrite(PIN_RELAY, LOW); // Matikan pompa saja, bukan mematikan sistem
            INDIKATOR_ON = 0;
            statusPompa = false;
            break;
        }
    }
}