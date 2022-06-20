#include "peripherals_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_perif.h"
#include "common_state.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define PIN_QUEIMADOR 12
#define PIN_BUZINA 13
#define PIN_LED_MASSA_1_QUENTE 5
#define PIN_LED_MASSA_1_FRIO 4
#define PIN_LED_MASSA_2_QUENTE 5
#define PIN_LED_MASSA_2_FRIO 4
#define PIN_LED_ENTRADA_QUENTE 0
#define PIN_LED_ENTRADA_FRIO 2
#define PIN_LED_CONEXAO 18
#define PIN_SENSORT 15

static const char *TAG = "PERIF MANAGER";

peripherals_manager_t peripherals_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t peripherals_update_queue) {
    peripherals_manager_t peripherals_manager = malloc(sizeof(s_peripherals_manager_t));
    peripherals_manager->state_manager_queue = state_manager_queue;
    peripherals_manager->peripherals_update_queue = peripherals_update_queue;

    gpio_pad_select_gpio(PIN_QUEIMADOR);
    gpio_set_direction(PIN_QUEIMADOR, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_BUZINA);
    gpio_set_direction(PIN_BUZINA, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_MASSA_1_QUENTE);
    gpio_set_direction(PIN_LED_MASSA_1_QUENTE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_MASSA_1_FRIO);
    gpio_set_direction(PIN_LED_MASSA_1_FRIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_MASSA_2_QUENTE);
    gpio_set_direction(PIN_LED_MASSA_2_QUENTE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_MASSA_2_FRIO);
    gpio_set_direction(PIN_LED_MASSA_2_FRIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_ENTRADA_QUENTE);
    gpio_set_direction(PIN_LED_ENTRADA_QUENTE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_ENTRADA_FRIO);
    gpio_set_direction(PIN_LED_ENTRADA_FRIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_CONEXAO);
    gpio_set_direction(PIN_LED_CONEXAO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_SENSORT);
    gpio_set_direction(PIN_SENSORT, GPIO_MODE_INPUT);

    return peripherals_manager;
}

void peripherals_update_task(void *pvParameters) {
    peripherals_manager_t peripherals_manager = (peripherals_manager_t)pvParameters;
    QueueHandle_t state_manager_queue = peripherals_manager->state_manager_queue;

    perif_update_t event;

    state_msg_t event_entrada = state_msg_create(UPDATE, SENSOR_ENTRADA, 0);

    for (;;) {
        if (xQueueReceive(peripherals_manager->peripherals_update_queue, (void *)&event, pdMS_TO_TICKS(10000))) {
            // Recebe respostas e ações a serem feitas
            switch (event->type) {
                case ACT: {
                    switch (event->target) {
                        case QUEIMADOR: {
                            gpio_pad_select_gpio(PIN_QUEIMADOR);

                            if (event->value == 1) {
                                gpio_set_level(PIN_QUEIMADOR, 0);
                            } else {
                                gpio_set_level(PIN_QUEIMADOR, 1);
                            }
                        } break;

                        case BUZINA: {
                            gpio_pad_select_gpio(PIN_BUZINA);

                            if (event->value == 1) {
                                gpio_set_level(PIN_BUZINA, 0);
                            } else {
                                gpio_set_level(PIN_BUZINA, 1);
                            }
                        } break;

                        case LED_ENTRADA_QUENTE: {
                            gpio_pad_select_gpio(PIN_LED_ENTRADA_QUENTE);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_ENTRADA_QUENTE, 0);
                            } else {
                                gpio_set_level(PIN_LED_ENTRADA_QUENTE, 1);
                            }
                        }

                        break;

                        case LED_ENTRADA_FRIO: {
                            gpio_pad_select_gpio(PIN_LED_ENTRADA_FRIO);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_ENTRADA_FRIO, 0);
                            } else {
                                gpio_set_level(PIN_LED_ENTRADA_FRIO, 1);
                            }
                        }

                        break;

                        case LED_MASSA_1_QUENTE: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_1_QUENTE);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_1_QUENTE, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_1_QUENTE, 1);
                            }
                        } break;

                        case LED_MASSA_1_FRIO: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_1_FRIO);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_1_FRIO, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_1_FRIO, 1);
                            }
                        } break;

                        case LED_MASSA_2_QUENTE: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_2_QUENTE);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_2_QUENTE, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_2_QUENTE, 1);
                            }
                        } break;

                        case LED_MASSA_2_FRIO: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_2_FRIO);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_2_FRIO, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_2_FRIO, 1);
                            }
                        } break;

                        default:

                            break;
                    }
                } break;

                case PERIF_RESPONSE: {
                    int64_t curr_time = esp_timer_get_time();
                    int64_t last_time_sensor = event->value;

                    switch (event->resp_type) {
                        case MASSA_1: {
                            if (curr_time - last_time_sensor > 30000) {
                                ESP_LOGI(TAG, "Desconectar a porra do led conexao");
                            }
                        } break;

                        case MASSA_2: {
                            if (curr_time - last_time_sensor > 30000) {
                                ESP_LOGI(TAG, "Desconectar a porra do led conexao");
                            }
                        } break;

                        default:

                            break;
                    }
                } break;
            }
        }

        // Requisita ultimas vezs que sensores comunicaram
    }
}

void peripherals_monitor_task(void *pvParameters) {
    peripherals_manager_t peripherals_manager = (peripherals_manager_t)pvParameters;
    QueueHandle_t state_manager_queue = peripherals_manager->state_manager_queue;

    state_msg_t event_request_last_m1 = state_msg_create(REQUEST, LAST_SENSOR_MASSA_1, 0);
    state_msg_t event_request_last_m2 = state_msg_create(REQUEST, LAST_SENSOR_MASSA_2, 0);

    for (;;) {
        xQueueSend(state_manager_queue, &event_request_last_m1, portMAX_DELAY);
        xQueueSend(state_manager_queue, &event_request_last_m2, portMAX_DELAY);

        // TODO pegar temperatura da ENTRADA e enviar tmb

        vTaskDelay(1000);
    }
}