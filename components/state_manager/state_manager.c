#include "state_manager.h"

#include <stdio.h>

#include "common_ihm.h"
#include "common_state.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage.h"

static const char *TAG = "STATE MANAGER";

static void ihm_change_page_to(QueueHandle_t ihm_update_queue, uint8_t page_number);

state_manager_t state_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue) {
    state_manager_t state_manager = 0;

    // Checa a EEPROM se estamos começando ou dando andamento em uma seca
    // Checamos o funcionamento do sensor físico e a conexão dos sensores remotos
    // Setamos os valores iniciais dos sensores e periféricos
    // - Alarme: OFF
    // - Queimador: ON

    ESP_LOGI(TAG, "Abrindo armazenamento não-volátil...");
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro (%s) ao abrir NVS", esp_err_to_name(err));
    } else {
        state_manager = (state_manager_t)malloc(sizeof(s_state_manager_t));
        state_manager->nvs_handle = nvs_handle;
        state_manager->ihm_update_queue = ihm_update_queue;
        state_manager->state_manager_queue = state_manager_queue;

        ESP_ERROR_CHECK(storage_get_mode(nvs_handle, &state_manager->mode));
        ESP_ERROR_CHECK(storage_get_lote_number(nvs_handle, &state_manager->lote_number));
        ESP_ERROR_CHECK(storage_get_sensor_entrada(nvs_handle, &state_manager->sensor_entrada));
        ESP_ERROR_CHECK(storage_get_sensor_massa_1(nvs_handle, &state_manager->sensor_massa_1));
        ESP_ERROR_CHECK(storage_get_sensor_massa_2(nvs_handle, &state_manager->sensor_massa_2));
        ESP_ERROR_CHECK(storage_get_sensor_entrada_min(nvs_handle, &state_manager->sensor_entrada_min));
        ESP_ERROR_CHECK(storage_get_sensor_entrada_max(nvs_handle, &state_manager->sensor_entrada_max));
        ESP_ERROR_CHECK(storage_get_sensor_massa_1_min(nvs_handle, &state_manager->sensor_massa_1_min));
        ESP_ERROR_CHECK(storage_get_sensor_massa_1_max(nvs_handle, &state_manager->sensor_massa_1_max));
        ESP_ERROR_CHECK(storage_get_sensor_massa_2_min(nvs_handle, &state_manager->sensor_massa_2_min));
        ESP_ERROR_CHECK(storage_get_sensor_massa_2_max(nvs_handle, &state_manager->sensor_massa_2_max));
        ESP_ERROR_CHECK(storage_get_finished(nvs_handle, &state_manager->finished));

        ESP_LOGI(TAG, "carregados valores do nvs");

        if (state_manager->finished == 0) {
            ESP_LOGI(TAG, "Finished: %d", state_manager->finished);
            ihm_change_page_to(ihm_update_queue, 2);
        } else {
            ihm_change_page_to(ihm_update_queue, 1);
        }
    }

    return state_manager;
}

void state_manager_task(void *pvParameters) {
    state_manager_t state_manager = (state_manager_t)pvParameters;
}

static void ihm_change_page_to(QueueHandle_t ihm_update_queue, uint8_t page_number) {
    ihm_update_t ihm_update = ihm_update_create(PAGE, 0, page_number);

    ESP_LOGI(TAG, "Sending page...");
    xQueueSend(ihm_update_queue, &ihm_update, portMAX_DELAY);
    ESP_LOGI(TAG, "Sent!");
}