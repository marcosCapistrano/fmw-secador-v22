#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "ds3231.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "inttypes.h"
#include "nvs.h"

typedef struct s_state_manager_t *state_manager_t;
typedef struct s_state_manager_t {
    uint8_t mode;
    uint8_t lote_number;

    uint8_t sensor_entrada;

    uint8_t sensor_massa_1;
    int64_t last_sensor_massa_1;
    uint8_t sensor_massa_2;
    int64_t last_sensor_massa_2;

    uint8_t sensor_entrada_min;
    uint8_t sensor_entrada_max;
    uint8_t sensor_massa_1_min;
    uint8_t sensor_massa_1_max;
    uint8_t sensor_massa_2_min;
    uint8_t sensor_massa_2_max;

    _Bool is_aware_entrada;
    _Bool is_aware_massa_1;
    _Bool is_aware_massa_2;

    uint8_t finished;  // Indica se terminamos a seca ou não

    uint8_t event_last_entrada;
    uint8_t event_last_massa_1;
    uint8_t event_last_massa_2;

    uint8_t event_last_entrada_min;
    uint8_t event_last_entrada_max;
    uint8_t event_last_massa_1_min;
    uint8_t event_last_massa_1_max;
    uint8_t event_last_massa_2_min;
    uint8_t event_last_massa_2_max;
    uint8_t event_last_conexao_1;
    uint8_t event_last_conexao_2;
    uint8_t event_last_buzina;
    uint8_t event_last_finished;
    uint8_t event_last_central;

    i2c_dev_t rtc_dev;
    nvs_handle_t nvs_handle;
    QueueHandle_t ihm_update_queue;
    QueueHandle_t state_manager_queue;
    QueueHandle_t peripherals_update_queue;
} s_state_manager_t;

state_manager_t state_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue, QueueHandle_t peripherals_update_queue);

void state_manager_task(void *pvParameters);

#endif