#include "common_ihm.h"
#include "common_perif.h"
#include "common_server.h"
#include "common_state.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ihm_manager.h"
#include "server_manager.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "peripherals_manager.h"
#include "state_manager.h"

void app_main(void) {
    // Inicializa armazenamento não-volátil
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    QueueHandle_t ihm_update_queue = xQueueCreate(10, sizeof(ihm_update_t));
    QueueHandle_t state_manager_queue = xQueueCreate(10, sizeof(state_msg_t));
    QueueHandle_t peripherals_update_queue = xQueueCreate(10, sizeof(perif_update_t));
    QueueHandle_t server_update_queue = xQueueCreate(10, sizeof(server_update_t));

    ihm_manager_t ihm_manager = ihm_manager_init(state_manager_queue, ihm_update_queue);
    state_manager_t state_manager = state_manager_init(state_manager_queue, ihm_update_queue, peripherals_update_queue);
    peripherals_manager_t peripherals_manager = peripherals_manager_init(state_manager_queue, peripherals_update_queue);

    server_manager_t server_manager = server_manager_init(state_manager_queue, server_update_queue);

    xTaskCreatePinnedToCore(ihm_input_task, "UART INPUT TASK", 2048, ihm_manager, 4, NULL, 1);
    xTaskCreatePinnedToCore(ihm_update_task, "IHM UPDATE TASK", 2048, ihm_manager, 3, NULL, 1);
    xTaskCreatePinnedToCore(state_manager_task, "STATE MANAGER TASK", 10000, state_manager, 5, NULL, 1);
    xTaskCreatePinnedToCore(peripherals_update_task, "PERIF MANAGER TASK", 9000, peripherals_manager, 1, NULL, 0);
    xTaskCreatePinnedToCore(peripherals_monitor_task, "PERIF MANAGER TASK", 5000, peripherals_manager, 2, NULL, 0);
}
