#include "state_manager.h"

#include <stdio.h>

#include "common_ihm.h"
#include "common_perif.h"
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

static void perif_send(QueueHandle_t peripherals_update_queue, perif_event_t type, perif_t target, perif_resp_t resp_type, uint8_t value);
static void ihm_send(QueueHandle_t ihm_update_queue, ihm_event_t type, uint8_t target_id, uint8_t value);

state_manager_t state_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue, QueueHandle_t peripherals_update_queue) {
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
        state_manager->peripherals_update_queue = peripherals_update_queue;

        state_manager->is_aware_entrada = false;
        state_manager->is_aware_massa_1 = false;
        state_manager->is_aware_massa_2 = false;

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

        vTaskDelay(pdMS_TO_TICKS(2000));
        // ESP_LOGI(TAG, "finished: %d", state_manager->finished);
        // if (state_manager->finished == 0) {
        //     ihm_send(ihm_update_queue, PAGE, 0, 2);
        // } else {
        //     ihm_send(ihm_update_queue, PAGE, 0, 1);
        // }
    }

    return state_manager;
}

static void check_sensor_entrada(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_entrada_min;
    uint8_t max = state_manager->sensor_entrada_max;
    uint8_t current = state_manager->sensor_entrada;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, QUEIMADOR, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_FRIO, PERIF_RESP_NONE, 0);

        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 1);
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 0);

        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, QUEIMADOR, PERIF_RESP_NONE, 1);
        // Avisar temperatura baixa
    } else {
        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 0);

        perif_send(peripherals_update_queue, ACT, LED_ENTRADA_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, QUEIMADOR, PERIF_RESP_NONE, 1);
    }
}

static void check_sensor_massa_1(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_massa_1_min;
    uint8_t max = state_manager->sensor_massa_1_max;
    uint8_t current = state_manager->sensor_massa_1;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 1);
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 0);
        // Avisar temperatura baixa
    } else {
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 0);
    }
}

static void check_sensor_massa_2(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_massa_2_min;
    uint8_t max = state_manager->sensor_massa_2_max;
    uint8_t current = state_manager->sensor_massa_2;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 1);
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 0);
        // Avisar temperatura baixa
    } else {
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 0);
    }
}

void state_manager_task(void *pvParameters) {
    state_manager_t state_manager = (state_manager_t)pvParameters;
    nvs_handle_t nvs_handle = state_manager->nvs_handle;
    QueueHandle_t ihm_update_queue = state_manager->ihm_update_queue;
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    state_msg_t event;

    for (;;) {
        if (xQueueReceive(state_manager->state_manager_queue, (void *)&event, portMAX_DELAY)) {
            switch (event->msg_type) {
                case UPDATE:
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

                        case SENSOR_ENTRADA:
                            state_manager->sensor_entrada = event->value;
                            storage_set_sensor_entrada(nvs_handle, state_manager->sensor_entrada);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_entrada);

                            check_sensor_entrada(state_manager);
                            break;

                        case SENSOR_MASSA_1:
                            state_manager->sensor_massa_1 = event->value;
                            state_manager->last_sensor_massa_1 = esp_timer_get_time();
                            storage_set_sensor_massa_1(nvs_handle, state_manager->sensor_massa_1);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_massa_1);

                            check_sensor_massa_1(state_manager);
                            break;

                        case SENSOR_MASSA_2:
                            state_manager->sensor_massa_2 = event->value;
                            state_manager->last_sensor_massa_1 = esp_timer_get_time();
                            storage_set_sensor_massa_2(nvs_handle, state_manager->sensor_massa_2);
                            ihm_send(ihm_update_queue, VALUE, 9, state_manager->sensor_massa_2);

                            check_sensor_massa_2(state_manager);
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
                            storage_get_lote_number(nvs_handle, &state_manager->lote_number);
                            state_manager->finished = event->value;
                            state_manager->lote_number = state_manager->lote_number + 1;

                            storage_set_finished(nvs_handle, state_manager->finished);
                            storage_set_lote_number(nvs_handle, state_manager->lote_number);
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

                        case LOTE_NUMBER:
                            storage_get_lote_number(nvs_handle, &state_manager->lote_number);
                            ihm_send(ihm_update_queue, VALUE, 1, state_manager->lote_number);
                            break;

                        case SENSOR_ENTRADA:
                            storage_get_sensor_entrada(nvs_handle, &state_manager->sensor_entrada);
                            ihm_send(ihm_update_queue, VALUE, 7, state_manager->sensor_entrada);
                            break;

                        case SENSOR_MASSA_1:
                            storage_get_sensor_massa_1(nvs_handle, &state_manager->sensor_massa_1);
                            ihm_send(ihm_update_queue, VALUE, 8, state_manager->sensor_massa_1);
                            break;

                        case SENSOR_MASSA_2:
                            storage_get_sensor_massa_2(nvs_handle, &state_manager->sensor_massa_2);
                            ihm_send(ihm_update_queue, VALUE, 9, state_manager->sensor_massa_2);
                            break;

                        case LAST_SENSOR_MASSA_1:
                            perif_send(peripherals_update_queue, PERIF_RESPONSE, PERIF_NONE, MASSA_1, state_manager->last_sensor_massa_1);
                            break;

                        case LAST_SENSOR_MASSA_2:
                            perif_send(peripherals_update_queue, PERIF_RESPONSE, PERIF_NONE, MASSA_2, state_manager->last_sensor_massa_2);
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
                                ihm_send(ihm_update_queue, PAGE, 0, 2);
                            } else {
                                ihm_send(ihm_update_queue, PAGE, 0, 1);
                            }
                            break;

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

static void perif_send(QueueHandle_t peripherals_update_queue, perif_event_t type, perif_t target, perif_resp_t resp_type, uint8_t value) {
    perif_update_t perif_update = perif_update_create(type, target, resp_type, value);

    xQueueSend(peripherals_update_queue, &perif_update, portMAX_DELAY);
}

static void ihm_send(QueueHandle_t ihm_update_queue, ihm_event_t type, uint8_t target_id, uint8_t value) {
    ihm_update_t ihm_update = ihm_update_create(type, target_id, value);

    xQueueSend(ihm_update_queue, &ihm_update, portMAX_DELAY);
}