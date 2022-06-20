#include "ihm_manager.h"

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_nextion.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "common_state.h"
#include "common_ihm.h"

#define BUF2_SIZE (1024)
#define RD_BUF2_SIZE (BUF2_SIZE)
#define EX_UART_NUM UART_NUM_2

static const char *TAG = "IHM_MANAGER";

static void ihm_interpret_data(uint8_t *data, uint8_t size);
static void ihm_change_page_to(uint8_t page_number);
static int uart_write(const char *data);
void ihm_send(const char *data);

ihm_manager_t ihm_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue) {
    ihm_manager_t ihm_manager = (ihm_manager_t)malloc(sizeof(s_ihm_manager_t));
    esp_nextion_init(&ihm_manager->uart_queue);
    ihm_manager->state_manager_queue = state_manager_queue;
    ihm_manager->ihm_update_queue = ihm_update_queue;

    ihm_change_page_to(0);

    return ihm_manager;
}

void ihm_input_task(void *pvParameters) {
    ihm_manager_t ihm_manager = (ihm_manager_t)pvParameters;

    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF2_SIZE);

    for (;;) {
        if (xQueueReceive(ihm_manager->uart_queue, (void *)&event, portMAX_DELAY)) {
            bzero(dtmp, RD_BUF2_SIZE);
            ESP_LOGI(TAG, "Uart Event!");

            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d bytes", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ihm_interpret_data(dtmp, event.size);
                    ESP_LOG_BUFFER_HEXDUMP(TAG, dtmp, event.size, ESP_LOG_INFO);
                    break;

                default:
                    ESP_LOGI(TAG, "Unknown Event: %d", event.type);
                    break;
            }
        }
    }
}

void ihm_update_task(void *pvParameters) {
    ihm_manager_t ihm_manager = (ihm_manager_t)pvParameters;
    ihm_update_t event;

    for (;;) {
        if (xQueueReceive(ihm_manager->ihm_update_queue, (void *)&event, portMAX_DELAY)) {
            ESP_LOGI(TAG, "UPDATE Event!");
            switch(event->type) {
                case PAGE:
                    ihm_change_page_to(event->value);
                    break;

                    default:

                    break;
            }
        }
    }
}

static void ihm_interpret_data(uint8_t *data, uint8_t size) {
    if(size == 5) {
        if(data[0] == 102) {
            ESP_LOGI(TAG, "Page Changed to: %d!!", data[1]);
        }
    } else if(size == 7) {
        if(data[0] == 101) {
            //Clicked something
            if(data[1] == 3) {
                //On page 3

                if(data[2] == 3) {
                    //Palaha Lenha
                    ESP_LOGI(TAG, "Clicked palha/lenha");
                }
            }
        }
    }

    ESP_LOGI(TAG, "0: %d", data[0]);
    ESP_LOGI(TAG, "1: %d", data[1]);
    ESP_LOGI(TAG, "2: %d", data[2]);
    ESP_LOGI(TAG, "3: %d", data[3]);
    ESP_LOGI(TAG, "4: %d", data[4]);
    ESP_LOGI(TAG, "5: %d", data[5]);
    ESP_LOGI(TAG, "6: %d", data[6]);
    ESP_LOGI(TAG, "7: %d", data[7]);
    ESP_LOGI(TAG, "8: %d", data[8]);
    ESP_LOGI(TAG, "9: %d", data[9]);
}

static void ihm_change_page_to(uint8_t page_number) {
    char page_str[10];
    sprintf(page_str, "page %d", page_number);
    ihm_send(page_str);
}

void ihm_send(const char *data) {
    uart_write("\xFF\xFF\xFF");
    uart_write(data);
    uart_write("\xFF\xFF\xFF");
}

static int uart_write(const char *data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(EX_UART_NUM, data, len);
    return txBytes;
}
