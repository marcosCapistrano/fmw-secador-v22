#include "ihm_manager.h"

#include <stdio.h>
#include <string.h>

#include "common_ihm.h"
#include "common_state.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_nextion.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define BUF2_SIZE (1024)
#define RD_BUF2_SIZE (BUF2_SIZE)
#define EX_UART_NUM UART_NUM_2

static const char *TAG = "IHM_MANAGER";

static void ihm_send_update(ihm_manager_t ihm_manager, state_msg_type_t type, state_target_t target, uint8_t value);
static void ihm_process_data(ihm_manager_t ihm_manager, uint8_t *data, uint8_t size);
static void ihm_change_page_to(int page_number);
static void ihm_change_value_to(uint8_t target, uint8_t value, int page_num);
static int uart_write(const char *data);
void ihm_send(const char *data);

ihm_manager_t ihm_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t ihm_update_queue) {
    ihm_manager_t ihm_manager = (ihm_manager_t)malloc(sizeof(s_ihm_manager_t));
    esp_nextion_init(&ihm_manager->uart_queue);
    ihm_manager->state_manager_queue = state_manager_queue;
    ihm_manager->ihm_update_queue = ihm_update_queue;

    ihm_send_update(ihm_manager, REQUEST, FINISHED, 0);

    return ihm_manager;
}

void ihm_input_task(void *pvParameters) {
    ihm_manager_t ihm_manager = (ihm_manager_t)pvParameters;

    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF2_SIZE);

    for (;;) {
        if (xQueueReceive(ihm_manager->uart_queue, (void *)&event, portMAX_DELAY)) {
            bzero(dtmp, RD_BUF2_SIZE);

            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ihm_process_data(ihm_manager, dtmp, event.size);
                    break;

                default:
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
            switch (event->type) {
                case PAGE:
                    ihm_manager->current_page = event->value;
                    ihm_change_page_to(event->value);
                    break;

                case VALUE:
                    ihm_change_value_to(event->target_id, event->value, ihm_manager->current_page);
                    break;
                default:

                    break;
            }

            free(event);
        }
    }
}

static int index_command_end(uint8_t *data, int from) {
    int count = from;

    while (data[count] != 255) {
        count++;
    }

    return count += 2;
}

static uint8_t calculate_value(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return a + b * 256 + c * 65536 + d * 16777216;
}

static void ihm_send_update(ihm_manager_t ihm_manager, state_msg_type_t type, state_target_t target, uint8_t value) {
    state_msg_t state_msg = state_msg_create(type, target, value);
    xQueueSend(ihm_manager->state_manager_queue, &state_msg, portMAX_DELAY);
}

static void ihm_process_data(ihm_manager_t ihm_manager, uint8_t *data, uint8_t size) {
    int index = 0;
    int start = 0;
    int end = size;

    while (index < end) {
        const uint8_t *head = data[index];
        int current_end = index_command_end(data, index);

        if (head == 101) {                         // Click event
            if (ihm_manager->current_page == 3) {  // Toggle palha/lenha
                if (data[index + 2 == 3]) {
                    ihm_send_update(ihm_manager, UPDATE, MODE, 0);
                }
            } else if (ihm_manager->current_page == 7) {  // Finalizar
                ihm_send_update(ihm_manager, UPDATE, FINISHED, 1);
            } else if (ihm_manager->current_page == 1) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, FINISHED, 0);
            } else if (ihm_manager->current_page == 9) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_ENTRADA, 1);
            } else if (ihm_manager->current_page == 10) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_ENTRADA, 1);
            } else if (ihm_manager->current_page == 11) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_MASSA_1, 1);
            } else if (ihm_manager->current_page == 12) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_MASSA_1, 1);
            } else if (ihm_manager->current_page == 13) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_MASSA_2, 1);
            } else if (ihm_manager->current_page == 14) {  // Iniciar novo
                ihm_send_update(ihm_manager, UPDATE, IS_AWARE_MASSA_2, 1);
            }
        } else if (head == 102) {  // Page event
            ihm_manager->current_page = data[index + 1];

            if (data[index + 1] == 1) {
                ihm_send_update(ihm_manager, REQUEST, LOTE_NUMBER, 0);
            } else if (data[index + 1] == 3) {
                ihm_send_update(ihm_manager, REQUEST, LOTE_NUMBER, 0);
                ihm_send_update(ihm_manager, REQUEST, MODE, 0);
                ihm_send_update(ihm_manager, REQUEST, SENSOR_ENTRADA, 0);
                ihm_send_update(ihm_manager, REQUEST, SENSOR_MASSA_1, 0);
                ihm_send_update(ihm_manager, REQUEST, SENSOR_MASSA_2, 0);
            } else if (data[index + 1] == 4) {
                ihm_send_update(ihm_manager, REQUEST, ENTRADA_MIN, 0);
                ihm_send_update(ihm_manager, REQUEST, ENTRADA_MAX, 0);
            } else if (data[index + 1] == 5) {
                ihm_send_update(ihm_manager, REQUEST, MASSA_1_MIN, 0);
                ihm_send_update(ihm_manager, REQUEST, MASSA_1_MAX, 0);
            } else if (data[index + 1] == 6) {
                ihm_send_update(ihm_manager, REQUEST, MASSA_2_MIN, 0);
                ihm_send_update(ihm_manager, REQUEST, MASSA_2_MAX, 0);
            }
        } else if (head == 113) {
            if (ihm_manager->current_page == 4) {  // Entrada page
                ihm_send_update(ihm_manager, UPDATE, ENTRADA_MIN, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
                index = current_end + 1;
                current_end = index_command_end(data, index);

                ihm_send_update(ihm_manager, UPDATE, ENTRADA_MAX, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
            } else if (ihm_manager->current_page == 5) {
                ihm_send_update(ihm_manager, UPDATE, MASSA_1_MIN, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
                index = current_end + 1;
                current_end = index_command_end(data, index);
                ihm_send_update(ihm_manager, UPDATE, MASSA_1_MAX, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
            } else if (ihm_manager->current_page == 6) {
                ihm_send_update(ihm_manager, UPDATE, MASSA_2_MIN, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
                index = current_end + 1;
                current_end = index_command_end(data, index);
                ihm_send_update(ihm_manager, UPDATE, MASSA_2_MAX, calculate_value(data[index + 1], data[index + 2], data[index + 3], data[index + 4]));
            }
        }

        index = current_end + 1;
    }
}

static void ihm_change_value_to(uint8_t target, uint8_t value, int page_num) {
    char command_str[45];
    char target_str[15];

    if (page_num == 1) {
        if (target == TARGET_LOTE_NUMBER) {
            sprintf(target_str, "tNewLote");
            sprintf(command_str, "%s.txt=\"Lote %d\"", target_str, value);
            ihm_send(command_str);
        }
    } else if (page_num == 3) {
        if (target == TARGET_LOTE_NUMBER) {
            sprintf(target_str, "tLoteAndamento");
            sprintf(command_str, "%s.txt=\"Lote %d em andamento\"", target_str, value);
            ihm_send(command_str);
        }
        if (target == TARGET_MODE) {
            sprintf(target_str, "bPalhaLenha");
            if (value == 1) {
                value = 15;
            } else {
                value = 16;
            }

            sprintf(command_str, "%s.pic=%d", target_str, value);
            ihm_send(command_str);
            sprintf(command_str, "%s.pic2=%d", target_str, value);
            ihm_send(command_str);
        } else if (target == TARGET_ENTRADA) {
            sprintf(target_str, "nEntrada");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        } else if (target == TARGET_MASSA_1) {
            sprintf(target_str, "nMassa1");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);
        } else if (target == TARGET_MASSA_2) {
            sprintf(target_str, "nMassa2");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);
        } else if (target == TARGET_CONEXAO_1) {
            sprintf(target_str, "nMassa1");

            if (value == 1) {
                sprintf(command_str, "%s.pco=65535", target_str);
            } else if (value == 0) {
                sprintf(command_str, "%s.pco=61440", target_str);
            }
            ihm_send(command_str);
        } else if (target == TARGET_CONEXAO_2) {
            sprintf(target_str, "nMassa2");

            if (value == 1) {
                sprintf(command_str, "%s.pco=65535", target_str);
            } else if (value == 0) {
                sprintf(command_str, "%s.pco=61440", target_str);
            }
            ihm_send(command_str);
        }
    } else if (page_num == 4) {
        if (target == TARGET_ENTRADA_MIN) {
            sprintf(target_str, "nMinEntrada");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        } else if (target == TARGET_ENTRADA_MAX) {
            sprintf(target_str, "nMaxEntrada");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        }
    } else if (page_num == 5) {
        if (target == TARGET_MASSA_1_MIN) {
            sprintf(target_str, "nMinMassa1");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        } else if (target == TARGET_MASSA_1_MAX) {
            sprintf(target_str, "nMaxMassa1");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        }
    } else if (page_num == 6) {
        if (target == TARGET_MASSA_2_MIN) {
            sprintf(target_str, "nMinMassa2");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        } else if (target == TARGET_MASSA_2_MAX) {
            sprintf(target_str, "nMaxMassa2");
            sprintf(command_str, "%s.val=%d", target_str, value);
            ihm_send(command_str);

            sprintf(command_str, "%s.pco=65535", target_str);
            ihm_send(command_str);
        }
    }
}

static void ihm_change_page_to(int page_number) {
    char page_str[10];
    sprintf(page_str, "page %d", page_number);

    ihm_send(page_str);
}

void ihm_send(const char *data) {
    portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&myMutex);
    uart_write("\xFF\xFF\xFF");
    uart_write(data);
    uart_write("\xFF\xFF\xFF");
    taskEXIT_CRITICAL(&myMutex);
}

static int uart_write(const char *data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(EX_UART_NUM, data, len);
    return txBytes;
}
