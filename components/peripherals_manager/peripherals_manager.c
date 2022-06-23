#include "peripherals_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_perif.h"
#include "common_state.h"
#include "driver/gpio.h"
#include "ds18b20.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "owb.h"
#include "owb_rmt.h"

#define GPIO_DS18B20_0 (CONFIG_ONE_WIRE_GPIO)
#define MAX_DEVICES (8)
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD (1000)  // milliseconds

#define PIN_QUEIMADOR 12
#define PIN_BUZINA 13
#define PIN_LED_MASSA_1_QUENTE 5
#define PIN_LED_MASSA_1_FRIO 4
// #define PIN_LED_MASSA_2_QUENTE 34
// #define PIN_LED_MASSA_2_FRIO 35
#define PIN_LED_ENTRADA_QUENTE 2
#define PIN_LED_ENTRADA_FRIO 0
#define PIN_LED_CONEXAO_1 18
#define PIN_LED_CONEXAO_2 32
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
    gpio_set_level(PIN_BUZINA, 1);

    gpio_pad_select_gpio(PIN_LED_MASSA_1_QUENTE);
    gpio_set_direction(PIN_LED_MASSA_1_QUENTE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_MASSA_1_FRIO);
    gpio_set_direction(PIN_LED_MASSA_1_FRIO, GPIO_MODE_OUTPUT);

    // gpio_pad_select_gpio(PIN_LED_MASSA_2_QUENTE);
    // gpio_set_direction(PIN_LED_MASSA_2_QUENTE, GPIO_MODE_OUTPUT);

    // gpio_pad_select_gpio(PIN_LED_MASSA_2_FRIO);
    // gpio_set_direction(PIN_LED_MASSA_2_FRIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_ENTRADA_QUENTE);
    gpio_set_direction(PIN_LED_ENTRADA_QUENTE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_ENTRADA_FRIO);
    gpio_set_direction(PIN_LED_ENTRADA_FRIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_CONEXAO_1);
    gpio_set_direction(PIN_LED_CONEXAO_1, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_CONEXAO_2);
    gpio_set_direction(PIN_LED_CONEXAO_2, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_SENSORT);
    gpio_set_direction(PIN_SENSORT, GPIO_MODE_INPUT);

    return peripherals_manager;
}

static void state_msg_send(QueueHandle_t state_manager_queue, state_msg_t state_msg, state_msg_type_t type, state_target_t target, uint8_t value) {
    state_msg->msg_type = type;
    state_msg->target = target;
    state_msg->value = value;

    xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
}

void peripherals_update_task(void *pvParameters) {
    peripherals_manager_t peripherals_manager = (peripherals_manager_t)pvParameters;
    QueueHandle_t state_manager_queue = peripherals_manager->state_manager_queue;

    perif_update_t event;
    state_msg_t state_msg = state_msg_create(STATE_MSG_NONE, STATE_TARGET_NONE, 0);

    for (;;) {
        if (xQueueReceive(peripherals_manager->peripherals_update_queue, (void *)&event, pdMS_TO_TICKS(10000))) {
            // Recebe respostas e ações a serem feitas
            switch (event->type) {
                case ACT: {
                    switch (event->target) {
                        case PERIF_QUEIMADOR: {
                            gpio_pad_select_gpio(PIN_QUEIMADOR);

                            if (event->value == 1) {
                                gpio_set_level(PIN_QUEIMADOR, 0);
                            } else {
                                gpio_set_level(PIN_QUEIMADOR, 1);
                            }
                        } break;

                        case PERIF_BUZINA: {
                            gpio_pad_select_gpio(PIN_BUZINA);

                            if (event->value == 1) {
                                gpio_set_level(PIN_BUZINA, 0);
                            } else {
                                gpio_set_level(PIN_BUZINA, 1);
                            }
                        } break;

                        case PERIF_LED_ENTRADA_QUENTE: {
                            gpio_pad_select_gpio(PIN_LED_ENTRADA_QUENTE);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_ENTRADA_QUENTE, 0);
                            } else {
                                gpio_set_level(PIN_LED_ENTRADA_QUENTE, 1);
                            }
                        }

                        break;

                        case PERIF_LED_ENTRADA_FRIO: {
                            gpio_pad_select_gpio(PIN_LED_ENTRADA_FRIO);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_ENTRADA_FRIO, 0);
                            } else {
                                gpio_set_level(PIN_LED_ENTRADA_FRIO, 1);
                            }
                        }

                        break;

                        case PERIF_LED_MASSA_1_QUENTE: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_1_QUENTE);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_1_QUENTE, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_1_QUENTE, 1);
                            }
                        } break;

                        case PERIF_LED_MASSA_1_FRIO: {
                            gpio_pad_select_gpio(PIN_LED_MASSA_1_FRIO);
                            if (event->value == 1) {
                                gpio_set_level(PIN_LED_MASSA_1_FRIO, 0);
                            } else {
                                gpio_set_level(PIN_LED_MASSA_1_FRIO, 1);
                            }
                        } break;

                            // case PERIF_LED_MASSA_2_QUENTE: {
                            //     gpio_pad_select_gpio(PIN_LED_MASSA_2_QUENTE);
                            //     if (event->value == 1) {
                            //         gpio_set_level(PIN_LED_MASSA_2_QUENTE, 0);
                            //     } else {
                            //         gpio_set_level(PIN_LED_MASSA_2_QUENTE, 1);
                            //     }
                            // } break;

                            // case PERIF_LED_MASSA_2_FRIO: {
                            //     gpio_pad_select_gpio(PIN_LED_MASSA_2_FRIO);
                            //     if (event->value == 1) {
                            //         gpio_set_level(PIN_LED_MASSA_2_FRIO, 0);
                            //     } else {
                            //         gpio_set_level(PIN_LED_MASSA_2_FRIO, 1);
                            //     }
                            // } break;

                        case PERIF_LED_CONEXAO_1: {
                            gpio_pad_select_gpio(PIN_LED_CONEXAO_1);
                            if (event->value == 1) {
                                ESP_LOGI(TAG, "ligando conexao 1");
                                gpio_set_level(PIN_LED_CONEXAO_1, 0);
                            } else {
                                ESP_LOGI(TAG, "desligando conexao 1");
                                gpio_set_level(PIN_LED_CONEXAO_1, 1);
                            }
                        } break;

                        case PERIF_LED_CONEXAO_2: {
                            gpio_pad_select_gpio(PIN_LED_CONEXAO_2);
                            if (event->value == 1) {
                                ESP_LOGI(TAG, "ligando conexao 2");
                                gpio_set_level(PIN_LED_CONEXAO_2, 0);
                            } else {
                                ESP_LOGI(TAG, "desligando conexao 2");
                                gpio_set_level(PIN_LED_CONEXAO_2, 1);
                            }
                        } break;

                        default:

                            break;
                    }
                } break;

                default:

                    break;
            }
        }
    }
}

void peripherals_monitor_task(void *pvParameters) {
    peripherals_manager_t peripherals_manager = (peripherals_manager_t)pvParameters;
    QueueHandle_t state_manager_queue = peripherals_manager->state_manager_queue;

    state_msg_t event_entrada = state_msg_create(UPDATE, SENSOR_ENTRADA, 0);

    // Create a 1-Wire bus, using the RMT timeslot driver
    OneWireBus *owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    DS18B20_Info *device = 0;
    DS18B20_Info *ds18b20_info = ds18b20_malloc();  // heap allocation
    device = ds18b20_info;
    ds18b20_init_solo(ds18b20_info, owb);

    ds18b20_use_crc(ds18b20_info, true);  // enable CRC check on all reads
    ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);

    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power) {
        printf("Parasitic-powered devices detected");
    }

    // In parasitic-power mode, devices cannot indicate when conversions are complete,
    // so waiting for a temperature conversion must be done by waiting a prescribed duration
    owb_use_parasitic_power(owb, parasitic_power);

    for (;;) {
        // TODO pegar temperatura da ENTRADA e enviar tmb
        ds18b20_convert_all(owb);
        ds18b20_wait_for_conversion(device);

        float reading = 0;
        DS18B20_ERROR error = ds18b20_read_temp(device, &reading);

        event_entrada->value = (uint8_t)reading;
        xQueueSend(state_manager_queue, &event_entrada, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}