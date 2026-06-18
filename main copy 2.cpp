// Deklarasikan variabel global SemaphoreHandle_t dengan nama mutexI2C.

#include <Arduino.h>

SemaphoreHandle_t mutexI2C;

// Buat fungsi TaskSuhuAir. Di dalam loop tak terbatasnya, perintahkan Task untuk mencoba mengambil (Take) mutexI2C dengan waktu tunggu maksimal (portMAX_DELAY). Jika kunci berhasil didapat:
void TaskSuhuAir(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(mutexI2C, portMAX_DELAY) == pdTRUE)
        {
            // Cetak log: [Core X] Kunci I2C didapat -> Membaca Suhu Air: 24 C (Gunakan xPortGetCoreID() untuk X).
            Serial.print("[Core ");
            Serial.print(xPortGetCoreID());
            Serial.println("] Kunci I2C didapat -> Membaca Suhu Air: 24 C");

            // Simulasikan waktu transfer data I2C yang lambat dengan fungsi blocking delay(50).
            delay(50);

            // Kembalikan (Give) kunci mutexI2C agar Task lain bisa menggunakannya.
            xSemaphoreGive(mutexI2C);
        }
        // Setelah mengembalikan kunci, berikan waktu istirahat OS menggunakan vTaskDelay selama 1000 milidetik.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Buat fungsi TaskpHAir. Terapkan logika yang persis sama dengan TaskSuhuAir, tetapi ubah teks lognya menjadi: [Core X] Kunci I2C didapat -> Membaca pH Air: 6.5. Berikan juga jeda istirahat vTaskDelay selama 1000 milidetik setelah mengembalikan kunci.
void TaskpHAir(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(mutexI2C, portMAX_DELAY) == pdTRUE)
        {
            Serial.print("[Core ");
            Serial.print(xPortGetCoreID());
            Serial.println("] Kunci I2C didapat -> Membaca pH Air: 6.5");

            // Simulasikan waktu transfer data I2C yang lambat dengan fungsi blocking delay(50).
            delay(50);

            // Kembalikan (Give) kunci mutexI2C agar Task lain bisa menggunakannya.
            xSemaphoreGive(mutexI2C);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void setup()
{
    // Di dalam fungsi setup(), ciptakan Mutex tersebut di dalam memori menggunakan fungsi bawaan FreeRTOS. Pastikan Mutex berhasil dibuat (tidak bernilai NULL) sebelum melanjutkan ke pembuatan Task.
    Serial.begin(115200);

    
    mutexI2C = xSemaphoreCreateMutex();

    if(mutexI2C != NULL){
        // Daftarkan kedua Task di setup(). Paku TaskSuhuAir di Core 0, dan TaskpHAir di Core 1. Alokasikan stack 2048 dan prioritas 1 untuk masing-masing Task.
        xTaskCreatePinnedToCore(TaskSuhuAir, "Suhu Air", 2048, NULL, 1, NULL, 0);
        xTaskCreatePinnedToCore(TaskpHAir, "pH Air", 2048, NULL, 1, NULL, 1);
    }
}


// Kosongkan fungsi loop() bawaan dengan delay maksimum RTOS.
void loop(){
    vTaskDelay(portMAX_DELAY);
}