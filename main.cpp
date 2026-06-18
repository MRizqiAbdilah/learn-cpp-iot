// #include <Arduino.h>

// void setup()
// {
//     Serial.begin(115200);
//     Serial.print("[Log] Fungsi setup() berjalan pada Core: ");
//     Serial.println(xPortGetCoreID());
// }

// void loop()
// {
//     Serial.print("[Log] Fungsi loop() berjalan pada Core: ");
//     Serial.println(xPortGetCoreID());
//     // Delays the task for 2000 milliseconds / 2 seconds
//     vTaskDelay(pdMS_TO_TICKS(2000));
// }

#include <Arduino.h>

// 1. Deklarasi Fungsi Task
// Wajib memiliki signature: menerima parameter void pointer dan tidak me-return apapun
// void TaskCekSensor(void *pvParameters) {
//     // Area inisialisasi Task (Berjalan 1 kali saat Task dibuat)
//     Serial.println("Task Sensor dimulai...");

//     // Area Loop Task (Berjalan selamanya)
//     for (;;) {
//         Serial.print("Membaca Sensor di Core: ");
//         Serial.println(xPortGetCoreID());
        
//         // Wajib memberikan waktu jeda agar Scheduler bisa bernapas (yield)
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// void TaskLevelAir(void *pvParameters){
//     for(;;) {
//         Serial.print("[Log] Membaca sensor ultrasonik di Core: ");
//         Serial.println(xPortGetCoreID());

//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }

// void TaskKomunikasi(void *pvParameters){
//     for(;;){
//         Serial.print("[Log] Mengirim data ke MQTT Cloud di Core: ");
//         Serial.println(xPortGetCoreID());

//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

// void setup() {
//     Serial.begin(115200);

//     // 2. Registrasi Task ke Scheduler
//     // xTaskCreatePinnedToCore(
//     //     TaskCekSensor,      // Fungsi yang dieksekusi
//     //     "Task_Sensor",      // Nama Task (bebas)
//     //     2048,               // Alokasi ukuran Stack memori (dalam Byte untuk ESP32)
//     //     NULL,               // Parameter yang dikirim (tidak ada)
//     //     1,                  // Prioritas Task (1 = normal)
//     //     NULL,               // Task Handle (tidak butuh mengontrol Task ini dari luar)
//     //     1                   // Paku di Core 1 (APP_CPU)
//     // );

//     xTaskCreatePinnedToCore(
//         TaskLevelAir,       // Fungsi yang dieksekusi
//         "Level Air",        // Nama Task (bebas)
//         2048,               // Alokasi ukuran Stack memori (dalam Byte untuk ESP32)
//         NULL,               // Parameter yang dikirim (tidak ada)
//         1,                  // Prioritas Task (1 = normal)
//         NULL,               // Task Handle (tidak butuh mengontrol Task ini dari luar)
//         1                   // Paku di Core 1 (APP_CPU)
//     );

//     xTaskCreatePinnedToCore(
//         TaskKomunikasi,
//         "Komunikasi MQTT",
//         2048,
//         NULL, 
//         1,
//         NULL,
//         0
//     );

//     Serial.println("Setup Selesai.");
// }

// void loop() {
//     // Loop utama sekarang bisa dibiarkan kosong, atau digunakan untuk hal lain
//     vTaskDelay(portMAX_DELAY); // Memblokir loop selamanya agar tidak memakan siklus CPU
// }


#include <Arduino.h>

// Deklarasi global Queue Handle (sebagai "Papan Tiket")
QueueHandle_t queueJarakAir;

void TaskSensor(void *pvParameters) {
    int jarakSimulasi = 10;
    
    for (;;) {
        jarakSimulasi++; // Simulasi baca sensor
        Serial.printf("[Sensor] Mengirim jarak: %d cm\n", jarakSimulasi);
        
        // Memasukkan data ke belakang antrean
        // PortMAX_DELAY: Jika antrean penuh (isi 5), tunggu selamanya sampai ada slot kosong
        xQueueSend(queueJarakAir, &jarakSimulasi, portMAX_DELAY);
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Baca setiap 2 detik
    }
}

void TaskMQTT(void *pvParameters) {
    int dataDiterima;
    
    for (;;) {
        // Mengambil data dari depan antrean (menghapusnya dari Queue)
        // portMAX_DELAY: Jika antrean kosong, Task akan "Tidur" secara otomatis!
        if (xQueueReceive(queueJarakAir, &dataDiterima, portMAX_DELAY) == pdPASS) {
            Serial.printf("[MQTT] Mem-publish jarak: %d cm ke Cloud\n", dataDiterima);
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Membuat Queue: Kapasitas 5 item, setiap item seukuran integer (4 byte)
    queueJarakAir = xQueueCreate(5, sizeof(int));

    if (queueJarakAir != NULL) {
        xTaskCreatePinnedToCore(TaskSensor, "Sensor", 2048, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(TaskMQTT, "MQTT", 2048, NULL, 1, NULL, 0);
    } else {
        Serial.println("Gagal membuat antrean RAM!");
    }
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}