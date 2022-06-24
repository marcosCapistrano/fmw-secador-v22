#include "state_manager.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "common_ihm.h"
#include "common_perif.h"
#include "common_state.h"
#include "ds3231.h"
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

    // Inicializa o RTC
    i2c_dev_t rtc_dev;
    if (ds3231_init_desc(&rtc_dev, I2C_NUM_0, CONFIG_RTC_SDA_GPIO, CONFIG_RTC_SCL_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "Could not init device descriptor.");
    } else {
        ESP_LOGI(TAG, "Found RTC!");

        struct tm rtcinfo;

        if (ds3231_get_time(&rtc_dev, &rtcinfo) != ESP_OK) {
            ESP_LOGE(TAG, "Could not get time.");
        }

        ESP_LOGI(TAG, "Current date:");
        ESP_LOGI(TAG, "%04d-%02d-%02d%02d:%02d:%02d:000Z", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);
    }

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
        state_manager->rtc_dev = rtc_dev;
        state_manager->ihm_update_queue = ihm_update_queue;
        state_manager->state_manager_queue = state_manager_queue;
        state_manager->peripherals_update_queue = peripherals_update_queue;

        state_manager->sensor_entrada = 0;
        state_manager->sensor_massa_1 = 0;
        state_manager->sensor_massa_2 = 0;

        state_manager->event_last_entrada = 0;
        state_manager->event_last_massa_1 = 0;
        state_manager->event_last_massa_2 = 0;
        state_manager->event_last_entrada_min = 0;
        state_manager->event_last_entrada_max = 0;
        state_manager->event_last_massa_1_min = 0;
        state_manager->event_last_massa_1_max = 0;
        state_manager->event_last_massa_2_min = 0;
        state_manager->event_last_massa_2_max = 0;
        state_manager->event_last_conexao_1 = 0;
        state_manager->event_last_conexao_2 = 0;
        state_manager->event_last_buzina = 0;

        state_manager->is_aware_entrada = false;
        state_manager->is_aware_massa_1 = false;
        state_manager->is_aware_massa_2 = false;

        ESP_ERROR_CHECK(storage_get_mode(nvs_handle, &state_manager->mode));
        ESP_ERROR_CHECK(storage_get_lote_number(nvs_handle, &state_manager->lote_number));
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

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    return state_manager;
}

static _Bool check_sensor_entrada(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_entrada_min;
    uint8_t max = state_manager->sensor_entrada_max;
    uint8_t current = state_manager->sensor_entrada;

    _Bool buzina = false;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, PERIF_QUEIMADOR, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 1);

        if (current > max + 7) {
            buzina = true;
        }
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 0);

        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, PERIF_QUEIMADOR, PERIF_RESP_NONE, 1);

        if (current < min - 7) {
            buzina = true;
        }
    } else {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_QUENTE, PERIF_RESP_NONE, 0);

        perif_send(peripherals_update_queue, ACT, PERIF_LED_ENTRADA_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_QUEIMADOR, PERIF_RESP_NONE, 1);

        state_manager->is_aware_entrada = false;
    }

    return buzina;
}

static _Bool check_sensor_massa_1(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_massa_1_min;
    uint8_t max = state_manager->sensor_massa_1_max;
    uint8_t current = state_manager->sensor_massa_1;

    _Bool buzina = false;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 1);

        if (current > max + 7 && !state_manager->is_aware_massa_1) {
            ihm_send(ihm_update_queue, PAGE, TARGET_NONE, 10);
            buzina = true;
        }
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 0);

        if (current < min - 7) {
            buzina = true;
        }
        buzina = true;
        // Avisar temperatura baixa
    } else {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_1_QUENTE, PERIF_RESP_NONE, 0);

        state_manager->is_aware_massa_1 = false;
    }

    return buzina;
}

static _Bool check_sensor_massa_2(state_manager_t state_manager) {
    QueueHandle_t peripherals_update_queue = state_manager->peripherals_update_queue;

    uint8_t min = state_manager->sensor_massa_2_min;
    uint8_t max = state_manager->sensor_massa_2_max;
    uint8_t current = state_manager->sensor_massa_2;

    _Bool buzina = false;

    if (current > max) {
        // Avisar temperatura alta
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 1);

        if (current > max + 7) {
            buzina = true;
        }
    } else if (current < min) {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_FRIO, PERIF_RESP_NONE, 1);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 0);
        // Avisar temperatura baixa


        if (current < min - 7) {
            buzina = true;
        }
    } else {
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_FRIO, PERIF_RESP_NONE, 0);
        perif_send(peripherals_update_queue, ACT, PERIF_LED_MASSA_2_QUENTE, PERIF_RESP_NONE, 0);

        state_manager->is_aware_massa_2 = false;
    }

    return buzina;
}

static void state_add_event_no_check(state_manager_t state_manager, state_target_t event_type) {
    char path[50];
    sprintf(path, "/spiffs/%d.txt", state_manager->lote_number);

    char message[500];

    struct tm rtcinfo;
    if (ds3231_get_time(&state_manager->rtc_dev, &rtcinfo) != ESP_OK) {
        ESP_LOGE(TAG, "Could not get time.");
    }

    ESP_LOGI(TAG, "Current date:");
    ESP_LOGI(TAG, "%04d-%02d-%02d%02d:%02d:%02d:000Z", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);

    switch (event_type) {
        case SENSOR_ENTRADA: {
            sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_ENTRADA, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, state_manager->event_last_entrada);
            storage_write_file(path, message);
        } break;

        case SENSOR_MASSA_1: {
            sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_MASSA_1, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, state_manager->event_last_massa_1);
            storage_write_file(path, message);
        } break;

        case SENSOR_MASSA_2: {
            sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_MASSA_2, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, state_manager->event_last_massa_2);
            storage_write_file(path, message);
        } break;

        case CONEXAO_1: {
            sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, CONEXAO_1, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, state_manager->event_last_conexao_1);
            storage_write_file(path, message);
        } break;

        case CONEXAO_2: {
            sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, CONEXAO_2, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, state_manager->event_last_conexao_2);
            storage_write_file(path, message);
        } break;

        default:
            break;
    }
}

static void state_add_event(state_manager_t state_manager, state_target_t event_type, uint8_t value) {
    char path[50];
    sprintf(path, "/spiffs/%d.txt", state_manager->lote_number);

    char message[500];

    struct tm rtcinfo;
    if (ds3231_get_time(&state_manager->rtc_dev, &rtcinfo) != ESP_OK) {
        ESP_LOGE(TAG, "Could not get time.");
    }

    ESP_LOGI(TAG, "Current date:");
    ESP_LOGI(TAG, "%04d-%02d-%02d%02d:%02d:%02d:000Z", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec);

    switch (event_type) {
        case SENSOR_ENTRADA:
            if (state_manager->event_last_entrada != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_ENTRADA, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);
                state_manager->event_last_entrada = value;
            }
            break;

        case SENSOR_MASSA_1:
            if (state_manager->event_last_massa_1 != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_MASSA_1, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_massa_1 = value;
            }
            break;

        case SENSOR_MASSA_2:

            if (state_manager->event_last_massa_1 != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, SENSOR_MASSA_2, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_massa_2 = value;
            }
            break;

        case CONEXAO_1:
            if (state_manager->event_last_conexao_1 != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, CONEXAO_1, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_conexao_1 = value;
            }
            break;

        case CONEXAO_2:
            if (state_manager->event_last_conexao_2 != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, CONEXAO_2, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_conexao_2 = value;
            }
            break;

        case FINISHED:
            if (state_manager->event_last_finished != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, FINISHED, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_finished = value;
            }
            break;

        case CENTRAL:
            if (state_manager->event_last_central != value) {
                sprintf(message, "%04d-%02d-%d %02d:%02d:%02d, CENTRAL, %d;", rtcinfo.tm_year, rtcinfo.tm_mon + 1, rtcinfo.tm_mday, rtcinfo.tm_hour, rtcinfo.tm_min, rtcinfo.tm_sec, value);
                storage_write_file(path, message);

                state_manager->event_last_central = value;
            }
            break;

        default:
            break;
    }
}

static void state_manager_reset(state_manager_t state_manager) {
    state_manager->event_last_entrada = 0;
    state_manager->event_last_massa_1 = 0;
    state_manager->event_last_massa_2 = 0;
    state_manager->event_last_entrada_min = 0;
    state_manager->event_last_entrada_max = 0;
    state_manager->event_last_massa_1_min = 0;
    state_manager->event_last_massa_1_max = 0;
    state_manager->event_last_massa_2_min = 0;
    state_manager->event_last_massa_2_max = 0;
    state_manager->event_last_conexao_1 = 0;
    state_manager->event_last_conexao_2 = 0;
    state_manager->event_last_buzina = 0;

    state_manager->is_aware_entrada = false;
    state_manager->is_aware_massa_1 = false;
    state_manager->is_aware_massa_2 = false;
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
                    storage_get_finished(nvs_handle, &state_manager->finished);
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
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA, state_manager->sensor_entrada);

                            if (state_manager->finished == 0) {
                                state_add_event(state_manager, SENSOR_ENTRADA, event->value);
                            }

                            if (check_sensor_entrada(state_manager) || check_sensor_massa_1(state_manager) || check_sensor_massa_2(state_manager)) {
                                perif_send(peripherals_update_queue, ACT, PERIF_BUZINA, PERIF_RESP_NONE, 1);
                            } else if (!check_sensor_entrada(state_manager) && !check_sensor_massa_1(state_manager) && !check_sensor_massa_2(state_manager)) {
                                perif_send(peripherals_update_queue, ACT, PERIF_BUZINA, PERIF_RESP_NONE, 0);
                            }
                            break;

                        case SENSOR_MASSA_1:
                            ESP_LOGI("TAG", "atualizando Massa 1");
                            state_manager->sensor_massa_1 = event->value;
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1, state_manager->sensor_massa_1);

                            if (state_manager->finished == 0) {
                                ESP_LOGE(TAG, "Finished ta 0");
                                state_add_event(state_manager, SENSOR_MASSA_1, event->value);
                            }

                            check_sensor_massa_1(state_manager);
                            break;

                        case SENSOR_MASSA_2:
                            ESP_LOGI("TAG", "atualizando Massa 2");
                            state_manager->sensor_massa_2 = event->value;
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_2, state_manager->sensor_massa_2);

                            if (state_manager->finished == 0) {
                                ESP_LOGE(TAG, "Finished ta 0");
                                state_add_event(state_manager, SENSOR_MASSA_2, event->value);
                            }

                            check_sensor_massa_2(state_manager);
                            break;

                        case CONEXAO_1:
                            ESP_LOGI("TAG", "atualizando Conexao Massa 1");
                            state_manager->conexao_1 = event->value;

                            perif_send(peripherals_update_queue, ACT, PERIF_LED_CONEXAO_1, PERIF_RESP_NONE, event->value);
                            ihm_send(ihm_update_queue, VALUE, TARGET_CONEXAO_1, event->value);

                            if (state_manager->finished == 0) {
                                ESP_LOGE(TAG, "Finished ta 0");
                                state_add_event(state_manager, CONEXAO_1, event->value);
                            }
                            break;

                        case CONEXAO_2:
                            ESP_LOGI("TAG", "atualizando Conexao Massa 2");
                            state_manager->conexao_2 = event->value;

                            perif_send(peripherals_update_queue, ACT, PERIF_LED_CONEXAO_2, PERIF_RESP_NONE, event->value);
                            ihm_send(ihm_update_queue, VALUE, TARGET_CONEXAO_2, event->value);

                            if (state_manager->finished == 0) {
                                ESP_LOGE(TAG, "Finished ta 0");
                                state_add_event(state_manager, CONEXAO_2, event->value);
                            }
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

                            state_add_event_no_check(state_manager, SENSOR_ENTRADA);
                            state_add_event_no_check(state_manager, SENSOR_MASSA_1);
                            state_add_event_no_check(state_manager, SENSOR_MASSA_2);
                            state_add_event_no_check(state_manager, CONEXAO_1);
                            state_add_event_no_check(state_manager, CONEXAO_2);
                            state_manager_reset(state_manager);

                            state_add_event(state_manager, FINISHED, event->value);
                            storage_set_finished(nvs_handle, event->value);

                            if (event->value == 1) {
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
                            ihm_send(ihm_update_queue, VALUE, TARGET_ENTRADA, state_manager->sensor_entrada);
                            break;

                        case SENSOR_MASSA_1:
                            ihm_send(ihm_update_queue, VALUE, TARGET_CONEXAO_1, state_manager->conexao_1);
                            ihm_send(ihm_update_queue, VALUE, TARGET_MASSA_1, state_manager->sensor_massa_1);
                            break;

                        case SENSOR_MASSA_2:
                            ihm_send(ihm_update_queue, VALUE, TARGET_CONEXAO_2, state_manager->conexao_2);
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

                default:

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