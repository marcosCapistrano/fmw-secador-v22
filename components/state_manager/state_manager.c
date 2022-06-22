#include "state_manager.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "common_ihm.h"
#include "common_perif.h"
#include "common_state.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage.h"

static const char *TAG = "STATE MANAGER";

static void perif_send(QueueHandle_t peripherals_update_queue, perif_event_t type, perif_t target, perif_resp_t resp_type, int64_t value);
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

        state_manager->last_sensor_massa_1 = 0;
        state_manager->last_sensor_massa_2 = 0;

        state_manager->event_last_entrada = 2;
        state_manager->event_last_massa_1 = 2;
        state_manager->event_last_massa_2 = 2;
        state_manager->event_last_entrada_min = 2;
        state_manager->event_last_entrada_max = 2;
        state_manager->event_last_massa_1_min = 2;
        state_manager->event_last_massa_1_max = 2;
        state_manager->event_last_massa_2_min = 2;
        state_manager->event_last_massa_2_max = 2;
        state_manager->event_last_conexao_1 = 2;
        state_manager->event_last_conexao_2 = 2;
        state_manager->event_last_buzina = 2;

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

        ESP_LOGI(TAG, "Initializing SPIFFS");

        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = "storage",  // TODO MUDAR
            .max_files = 5,
            .format_if_mount_failed = true};

        esp_err_t ret = esp_vfs_spiffs_register(&conf);

        storage_list_files();

        vTaskDelay(pdMS_TO_TICKS(2000));
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

static void state_add_event(state_manager_t state_manager, state_target_t event_type, uint8_t value) {
    char path[50];
    sprintf(path, "/spiffs/lote_%d.txt", state_manager->lote_number);

    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    time(&now);
    struct tm tm = *localtime(&now);
    char message[500];

    switch (event_type) {
        ESP_LOGI("OI", "%02d-%02d-%d %02d:%02d:%02d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
        case SENSOR_ENTRADA:
            if (state_manager->event_last_entrada != value) {
                sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, ENTRADA, %d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, value);
                storage_write_file(path, message);
            }
            break;

        case SENSOR_MASSA_1:
            sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, MASSA_1, %d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, value);
            storage_write_file(path, message);
            break;

        case SENSOR_MASSA_2:
            sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, MASSA_2, %d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, value);
            storage_write_file(path, message);
            break;

        case ENTRADA_MIN:
            if (state_manager->event_last_entrada_min != value) {
            }
            break;

        case ENTRADA_MAX:
            if (state_manager->event_last_entrada_max != value) {
            }
            break;

        case MASSA_1_MIN:
            if (state_manager->event_last_massa_1_min != value) {
            }
            break;

        case MASSA_1_MAX:
            if (state_manager->event_last_massa_1_max != value) {
            }
            break;

        case MASSA_2_MIN:
            if (state_manager->event_last_massa_2_min != value) {
            }
            break;

        case MASSA_2_MAX:
            if (state_manager->event_last_massa_2_max != value) {
            }
            break;

            // case CONEXAO_1:
            //     if (state_manager->event_last_conexao_1 != value) {
            //         if (value == 1) {
            //             sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, CONEXAO MASSA 1, OK", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //             storage_write_file(path, message);
            //         } else {
            //             sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, CONEXAO MASSA 1, PERDIDA", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //             storage_write_file(path, message);
            //         }
            //     }
            //     break;

            // case CONEXAO_2:
            //     if (state_manager->event_last_conexao_2 != value) {
            //         if (value == 1) {
            //             sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, CONEXAO MASSA 2 OK", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //             storage_write_file(path, message);
            //         } else {
            //             sprintf(message, "%02d-%02d-%d %02d:%02d:%02d, CONEXAO MASSA 2 PERDIDA", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //         }
            //     }
            //     break;

        case BUZINA:
            if (state_manager->event_last_buzina != value) {
            }
            break;

        default:
            break;
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
                            ihm_send(ihm_update_queue, VALUE, TARGET_MODE, state_manager->mode);
                            break;

                        case SENSOR_ENTRADA:
                            state_manager->sensor_entrada = event->value;
                            storage_set_sensor_entrada(nvs_handle, state_manager->sensor_entrada);
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA, state_manager->sensor_entrada);

                            state_add_event(state_manager, SENSOR_ENTRADA, event->value);
                            check_sensor_entrada(state_manager);
                            break;

                        case SENSOR_MASSA_1:
                            ESP_LOGI("TAG", "atualizando Massa 1");
                            state_manager->sensor_massa_1 = event->value;
                            state_manager->last_sensor_massa_1 = esp_timer_get_time();
                            storage_set_sensor_massa_1(nvs_handle, state_manager->sensor_massa_1);
                            state_add_event(state_manager, SENSOR_MASSA_1, event->value);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1, state_manager->sensor_massa_1);

                            check_sensor_massa_1(state_manager);
                            break;

                        case SENSOR_MASSA_2:
                            ESP_LOGI("TAG", "atualizando Massa 2");
                            state_manager->sensor_massa_2 = event->value;
                            state_manager->last_sensor_massa_2 = esp_timer_get_time();
                            storage_set_sensor_massa_2(nvs_handle, state_manager->sensor_massa_2);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_2, state_manager->sensor_massa_2);

                            state_add_event(state_manager, SENSOR_MASSA_2, event->value);
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
                            state_manager->finished = event->value;
                            storage_set_finished(nvs_handle, state_manager->finished);

                            if (state_manager->finished == 0) {
                                ESP_LOGI(TAG, "Começando nova torra...");
                            }

                            if (state_manager->finished == 1) {
                                storage_get_lote_number(nvs_handle, &state_manager->lote_number);
                                state_manager->lote_number = state_manager->lote_number + 1;
                                storage_set_lote_number(nvs_handle, state_manager->lote_number);
                            }
                        default:
                            break;
                    }
                    break;

                case REQUEST:
                    switch (event->target) {
                        case MODE:
                            storage_get_mode(nvs_handle, &state_manager->mode);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MODE, state_manager->mode);
                            break;

                        case LOTE_NUMBER:
                            storage_get_lote_number(nvs_handle, &state_manager->lote_number);
                            ihm_send(ihm_update_queue, VALUE, TARGET_LOTE_NUMBER, state_manager->lote_number);
                            break;

                        case SENSOR_ENTRADA:
                            storage_get_sensor_entrada(nvs_handle, &state_manager->sensor_entrada);
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA, state_manager->sensor_entrada);
                            break;

                        case SENSOR_MASSA_1:
                            storage_get_sensor_massa_1(nvs_handle, &state_manager->sensor_massa_1);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1, state_manager->sensor_massa_1);
                            break;

                        case SENSOR_MASSA_2:
                            storage_get_sensor_massa_2(nvs_handle, &state_manager->sensor_massa_2);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_2, state_manager->sensor_massa_2);
                            break;

                        case LAST_SENSOR_MASSA_1:
                            perif_send(peripherals_update_queue, PERIF_RESPONSE, PERIF_NONE, MASSA_1, state_manager->last_sensor_massa_1);
                            ESP_LOGI(TAG, "SENDING 1:%d", (int)state_manager->last_sensor_massa_1);
                            break;

                        case LAST_SENSOR_MASSA_2:
                            perif_send(peripherals_update_queue, PERIF_RESPONSE, PERIF_NONE, MASSA_2, state_manager->last_sensor_massa_2);
                            ESP_LOGI(TAG, "SENDING 2:%d", (int)state_manager->last_sensor_massa_1);
                            break;

                        case ENTRADA_MIN:
                            storage_get_sensor_entrada_min(nvs_handle, &state_manager->sensor_entrada_min);
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA_MIN, state_manager->sensor_entrada_min);
                            break;

                        case ENTRADA_MAX:
                            storage_get_sensor_entrada_max(nvs_handle, &state_manager->sensor_entrada_max);
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA_MAX, state_manager->sensor_entrada_max);
                            break;

                        case MASSA_1_MIN:
                            storage_get_sensor_massa_1_min(nvs_handle, &state_manager->sensor_massa_1_min);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1_MIN, state_manager->sensor_massa_1_min);
                            break;

                        case MASSA_1_MAX:
                            storage_get_sensor_massa_1_max(nvs_handle, &state_manager->sensor_massa_1_max);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1_MAX, state_manager->sensor_massa_1_max);
                            break;

                        case MASSA_2_MIN:
                            storage_get_sensor_massa_2_min(nvs_handle, &state_manager->sensor_massa_2_min);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_2_MIN, state_manager->sensor_massa_2_min);
                            break;

                        case MASSA_2_MAX:
                            storage_get_sensor_massa_2_max(nvs_handle, &state_manager->sensor_massa_2_max);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_2_MAX, state_manager->sensor_massa_2_max);
                            break;

                        case FINISHED:
                            storage_get_finished(nvs_handle, &state_manager->finished);

                            if (state_manager->finished == 0) {
                                ihm_send(ihm_update_queue, PAGE, TARGET_NONE, 2);
                            } else {
                                ihm_send(ihm_update_queue, PAGE, TARGET_NONE, 1);
                            }
                            break;

                        default:
                            break;
                    }
                    break;
            }
        }
    }
}

static void perif_send(QueueHandle_t peripherals_update_queue, perif_event_t type, perif_t target, perif_resp_t resp_type, int64_t value) {
    perif_update_t perif_update = perif_update_create(type, target, resp_type, value);

    xQueueSend(peripherals_update_queue, &perif_update, portMAX_DELAY);
}

static void ihm_send(QueueHandle_t ihm_update_queue, ihm_event_t type, uint8_t target_id, uint8_t value) {
    ihm_update_t ihm_update = ihm_update_create(type, target_id, value);

    xQueueSend(ihm_update_queue, &ihm_update, portMAX_DELAY);
}