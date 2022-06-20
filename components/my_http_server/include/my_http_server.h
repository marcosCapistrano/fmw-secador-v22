#ifndef HTTP_SERVER2_H
#define HTTP_SERVER2_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

typedef struct s_my_http_server_t *my_http_server_t;
typedef struct s_my_http_server_t {
	QueueHandle_t state_manager_queue;
	QueueHandle_t server_update_queue;
} s_my_http_server_t;

void http_server_init(QueueHandle_t state_manager_queue, QueueHandle_t server_update_queue);

#endif
