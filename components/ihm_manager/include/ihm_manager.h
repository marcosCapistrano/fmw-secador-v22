#ifndef IHM_MANAGER_H
#define IHM_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct s_ihm_manager_t *ihm_manager_t;
typedef struct s_ihm_manager_t {
	QueueHandle_t uart_queue;
	QueueHandle_t state_manager_queue;
	QueueHandle_t ihm_update_queue;

	uint8_t current_page;
} s_ihm_manager_t;

ihm_manager_t ihm_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue);
void ihm_input_task(void *pvParameters);
void ihm_update_task(void *pvParameters);

#endif