#include "esp_nextion.h"

#include <stdio.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "string.h"

#define TXD_PIN 17
#define RXD_PIN 16
#define nUART (UART_NUM_2)

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static const char *TAG = "ESP_NEXTION";

void esp_nextion_init(QueueHandle_t *uart_queue) {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    ESP_ERROR_CHECK(uart_driver_install(nUART, BUF_SIZE * 2, BUF_SIZE * 2, 20, uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(nUART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(nUART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}