#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "inttypes.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

typedef struct s_server_manager_t *server_manager_t;
typedef struct s_server_manager_t {
	QueueHandle_t state_manager_queue;
	QueueHandle_t server_update_queue;

	httpd_handle_t server;
	int m1_sock_fd;
	int m2_sock_fd;
	TimerHandle_t m1_timer;
	TimerHandle_t m2_timer;
} s_server_manager_t;

server_manager_t server_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t server_update_queue);

#endif
