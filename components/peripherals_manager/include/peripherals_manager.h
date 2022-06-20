#ifndef PERIPHERALS_MANAGER_H
#define PERIPHERALS_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct s_peripherals_manager_t *peripherals_manager_t;
typedef struct s_peripherals_manager_t {
	QueueHandle_t state_manager_queue;
	QueueHandle_t peripherals_update_queue;
} s_peripherals_manager_t;

peripherals_manager_t peripherals_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t peripherals_update_queue);
void peripherals_update_task(void *pvParameters);
void peripherals_monitor_task(void *pvParameters);

#endif