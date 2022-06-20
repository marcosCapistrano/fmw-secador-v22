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

static void ihm_send(QueueHandle_t ihm_update_queue, ihm_event_t type, uint8_t target_id, uint8_t value);

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
            ihm_send(ihm_update_queue, PAGE, 0, 2);
        } else {
            ihm_send(ihm_update_queue, PAGE, 0, 1);
        }
    }

    return state_manager;
}

void state_manager_task(void *pvParameters) {
    state_manager_t state_manager = (state_manager_t)pvParameters;
    nvs_handle_t nvs_handle = state_manager->nvs_handle;
    QueueHandle_t ihm_update_queue = state_manager->ihm_update_queue;

    state_msg_t event;

    for (;;) {
        if (xQueueReceive(state_manager->state_manager_queue, (void *)&event, portMAX_DELAY)) {
            switch (event->msg_type) {
                case UPDATE:
                    ESP_LOGI(TAG, "Received event of type: update with value: %d", event->value);
                    switch (event->target) {
                        case MODE:
                            if (state_manager->mode == 0) {
                                state_manager->mode = 1;
                            } else {
                                state_manager->mode = 0;
                            }

                            storage_set_mode(nvs_handle, state_manager->mode);
                            ihm_send(ihm_update_queue, VALUE, 3, state_manager->mode);
                            break;

                        case ENTRADA_MIN:
                            state_manager->sensor_entrada_min = event->value;
                            storage_set_sensor_entrada_min(nvs_handle, state_manager->sensor_entrada_min);
                            break;

                        case ENTRADA_MAX:
                            state_manager->sensor_entrada_max = event->value;
                            storage_set_sensor_entrada_max(nvs_handle, state_manager->sensor_entrada_max);
                            break;

                        case MASSA_1_MIN:
                            state_manager->sensor_massa_1_min = event->value;
                            storage_set_sensor_massa_1_min(nvs_handle, state_manager->sensor_massa_1_min);
                            break;

                        case MASSA_1_MAX:
                            state_manager->sensor_massa_1_max = event->value;
                            storage_set_sensor_massa_1_max(nvs_handle, state_manager->sensor_massa_1_max);
                            break;

                        case MASSA_2_MIN:
                            state_manager->sensor_massa_2_min = event->value;
                            storage_set_sensor_massa_2_min(nvs_handle, state_manager->sensor_massa_2_min);
                            break;

                        case MASSA_2_MAX:
                            state_manager->sensor_massa_2_max = event->value;
                            storage_set_sensor_massa_2_max(nvs_handle, state_manager->sensor_massa_2_max);
                            break;

                        case FINISHED:
                            ESP_LOGI(TAG, "Updating finished to: %d", event->value);
                            state_manager->finished = event->value;
                            storage_set_finished(nvs_handle, state_manager->finished);

                        default:
                            break;
                    }
                    break;

                case REQUEST:
                    switch (event->target) {
                        case MODE:
                            storage_get_mode(nvs_handle, &state_manager->mode);
                            ihm_send(ihm_update_queue, VALUE, 3, state_manager->mode);
                            break;

                        case SENSOR_ENTRADA:
                            storage_get_sensor_entrada(nvs_handle, &state_manager->sensor_entrada);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_entrada);
                            break;

                        case SENSOR_MASSA_1:
                            storage_get_sensor_massa_1(nvs_handle, &state_manager->sensor_massa_1);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_massa_2);
                            break;

                        case SENSOR_MASSA_2:
                            storage_get_sensor_massa_2(nvs_handle, &state_manager->sensor_massa_2);
                            ihm_send(ihm_update_queue, VALUE, 9, state_manager->sensor_massa_2);
                            break;

                        case ENTRADA_MIN:
                            storage_get_sensor_entrada_min(nvs_handle, &state_manager->sensor_entrada_min);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_entrada_min);
                            break;

                        case ENTRADA_MAX:
                            storage_get_sensor_entrada_max(nvs_handle, &state_manager->sensor_entrada_max);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_entrada_max);
                            break;

                        case MASSA_1_MIN:
                            storage_get_sensor_massa_1_min(nvs_handle, &state_manager->sensor_massa_1_min);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_massa_1_min);
                            break;

                        case MASSA_1_MAX:
                            storage_get_sensor_massa_1_max(nvs_handle, &state_manager->sensor_massa_1_max);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_massa_1_max);
                            break;

                        case MASSA_2_MIN:
                            storage_get_sensor_massa_2_min(nvs_handle, &state_manager->sensor_massa_2_min);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_massa_2_min);
                            break;

                        case MASSA_2_MAX:
                            storage_get_sensor_massa_2_max(nvs_handle, &state_manager->sensor_massa_2_max);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_massa_2_max);
                            break;

                        case FINISHED:
                            storage_get_finished(nvs_handle, &state_manager->finished);
                            if (state_manager->finished == 0) {
                                ihm_send(ihm_update_queue, PAGE, 2, state_manager->finished);
                            } else {
                                ihm_send(ihm_update_queue, PAGE, 1, state_manager->finished);
                            }

                        default:
                            break;
                    }
                    break;

                case RESPONSE:
                    break;
            }
        }
    }
}

static void ihm_send(QueueHandle_t ihm_update_queue, ihm_event_t type, uint8_t target_id, uint8_t value) {
    ihm_update_t ihm_update = ihm_update_create(type, target_id, value);

    xQueueSend(ihm_update_queue, &ihm_update, portMAX_DELAY);
}