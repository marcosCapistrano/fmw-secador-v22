#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct s_server_manager_t *server_manager_t;
typedef struct s_server_manager_t {
	QueueHandle_t state_manager_queue;
	QueueHandle_t server_update_queue;

	httpd_handle_t server;
} s_server_manager_t;

server_manager_t server_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t server_update_queue);

#endif
