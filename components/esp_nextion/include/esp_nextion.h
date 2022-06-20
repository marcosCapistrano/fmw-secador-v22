#ifndef ESP_NEXTION_H
#define ESP_NEXTION_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void esp_nextion_init(QueueHandle_t *uart_queue);

#endif
