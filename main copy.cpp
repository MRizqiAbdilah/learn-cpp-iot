#include <Arduino.h>

// Deklarasikan variabel global QueueHandle_t bernama queueLevelAir.
QueueHandle_t queueLevelAir;

void TaskSensorAir(void *pvParameters)
{
    // Buat fungsi TaskSensorAir. Di dalam loop tak terbatasnya, buat simulasi pembacaan jarak air (misal: buat variabel lokal integer yang awalnya 20, lalu berkurang 1 setiap putaran). Cetak log "[Core 1] Membaca jarak air: [X] cm".
    int jarak_air = 20;
    for (;;)
    {
        Serial.printf("[Core %d] Membaca jarak air: [%d] cm\n", xPortGetCoreID(), jarak_air);
        // Masih di TaskSensorAir, kirim data jarak tersebut ke dalam Queue menggunakan xQueueSend. Berikan delay RTOS tepat 1000 milidetik (1 detik).
        xQueueSend(queueLevelAir, &jarak_air, portMAX_DELAY);
        
        jarak_air--;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void TaskPublikasiMQTT(void *pvParameters)
{
    // Buat fungsi TaskPublikasiMQTT. Di dalam loop tak terbatasnya, perintahkan ESP32 untuk menunggu dan mengambil data dari antrean menggunakan xQueueReceive (gunakan waktu tunggu portMAX_DELAY agar Task tertidur saat antrean kosong).
    int data_diterima;
    for (;;)
    {
        // Masih di TaskPublikasiMQTT, jika data berhasil diambil, cetak log "[Core 0] Mem-publish ke hidroponik/level_air: [X] cm". Berikan delay RTOS lambat selama 3000 milidetik (3 detik) untuk menyimulasikan latensi jaringan.
        if (xQueueReceive(queueLevelAir, &data_diterima, portMAX_DELAY) == pdPASS)
        {
            Serial.printf("[Core %d] Mem-publish ke hidroponik/level_air: [%d] cm\n", xPortGetCoreID(), data_diterima);
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}

void setup()
{
    Serial.begin(115200);

    // Di dalam setup(), inisialisasi Queue tersebut agar mampu menampung 10 buah data bertipe integer.
    queueLevelAir = xQueueCreate(10, sizeof(int));

    // Di dalam setup(), daftarkan kedua Task (prioritas 1, alokasi memori 2048). Paku secara ketat TaskSensorAir di Core 1, dan TaskPublikasiMQTT di Core 0.
    if (queueLevelAir != NULL)
    {
        xTaskCreatePinnedToCore(TaskSensorAir, "Sensor Air", 2048, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(TaskPublikasiMQTT, "MQTT", 2048, NULL, 1, NULL, 0);
    }
    else
    {
        Serial.println("Gagal membuat antrean RAM!");
    }
}

// Kosongkan fungsi loop() bawaan dengan aman menggunakan delay maksimum RTOS.
void loop()
{
    vTaskDelay(portMAX_DELAY);
}
