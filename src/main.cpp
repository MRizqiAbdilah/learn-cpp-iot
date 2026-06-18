// Deklarasikan QueueHandle_t bernama queueJarakAir dan SemaphoreHandle_t bernama mutexSerial.
#include <Arduino.h>

QueueHandle_t queueJarakAir;
SemaphoreHandle_t mutexSerial;

// Buat TaskSensor (dipaku di Core 1):
void TaskSensor(void *pvParameters){
    // Buat variabel simulasi int jarakAir = 20.
    int jarak_air = 20;
    
    
    for(;;){
        // Di dalam loop tak terbatas, kurangi jarakAir sebanyak 1 (simulasi air bertambah naik/jarak sensor ke air memendek). Jika jarakAir < 4, kembalikan nilainya ke 20.
        jarak_air--;

        if (jarak_air < 4) jarak_air = 20;
        
        // Gunakan Mutex untuk mencetak log secara aman: [Core 1] Membaca Jarak: [X] cm.
        if(xSemaphoreTake(mutexSerial, portMAX_DELAY) == pdTRUE){
            // Kirim jarakAir tersebut ke dalam queueJarakAir.
            Serial.printf("[Core 1] Membaca Jarak Air: %d cm\n", jarak_air);
            xSemaphoreGive(mutexSerial);
        }

        xQueueSend(queueJarakAir, &jarak_air, portMAX_DELAY);
        
        // Berikan delay RTOS 1000 milidetik.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Buat TaskAktuator (dipaku di Core 0):
void TaksAktuator(void *pvParameters){
    int dataAirMasuk;
    // Buat variabel status bool pompaMenyala = false.
    bool pompaMenyala = false;
    
    for(;;){
        
        // Di dalam loop tak terbatas, tertidurlah sampai menerima data jarak dari queueJarakAir.
        if(xQueueReceive(queueJarakAir, &dataAirMasuk, portMAX_DELAY) == pdPASS) {
            // Lakukan evaluasi logika pompa berdasarkan Aturan Aktuator di atas.

            // Jika jarak air >= 15 cm (air surut/hampir habis), pompa menyala (ON) untuk mengisi.
            // Jika jarak air <= 5 cm (air sudah penuh/batas bahaya luber), pompa mati (OFF).
            // Di antara jarak tersebut (6 cm - 14 cm), pompa mempertahankan status terakhirnya.
            if (dataAirMasuk >= 15) {
                pompaMenyala = true;
            } else if (dataAirMasuk <= 5){
                pompaMenyala = false;
            }

            // Gunakan Mutex untuk mencetak log hasil evaluasi secara aman: [Core 0] Evaluasi Jarak: [X] cm -> Pompa [ON / OFF].
            if(xSemaphoreTake(mutexSerial, portMAX_DELAY) == pdTRUE){
                Serial.printf("[Core 0] Evaluasi Jarak: %d cm -> Pompa %s\n", 
                              dataAirMasuk, pompaMenyala ? "[ON]" : "[OFF]");
                xSemaphoreGive(mutexSerial);
            }
        }
    }
}

void setup(){
    Serial.begin(115200);

    // Di setup(), inisialisasi antrean untuk kapasitas 5 data int, dan ciptakan Mutex-nya. Pastikan keduanya tidak bernilai NULL.
    queueJarakAir = xQueueCreate(5, sizeof(int));
    mutexSerial = xSemaphoreCreateMutex();

    if(queueJarakAir != NULL && mutexSerial != NULL){
        // Daftarkan kedua Task di setup() dengan memori 2048 dan prioritas 1.
        xTaskCreatePinnedToCore(TaskSensor, "SensorTask", 2048, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(TaksAktuator, "ActuatorTask", 2048, NULL, 1, NULL, 0);
    }
}

// Kosongkan fungsi loop() bawaan dengan delay maksimum RTOS.
void loop(){
    vTaskDelay(portMAX_DELAY);
}




