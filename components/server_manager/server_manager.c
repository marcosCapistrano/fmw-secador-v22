#include "server_manager.h"

#include <string.h>

#include "common_state.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "list.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mdns.h"
#include "storage.h"

static const char *TAG = "SERVER_MANAGER";

#define WIFI_AP_SSID CONFIG_WIFI_AP_SSID
#define WIFI_AP_PASS CONFIG_WIFI_AP_PASS

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
}

static void wifi_ap_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_SSID,
            .max_connection = 6,
            .beacon_interval = 100}};

    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wifi AP Started! SSID:%s Password:%s",
             WIFI_AP_SSID, WIFI_AP_PASS);
}

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }

        char buffer[6];
        sprintf(buffer, "%s", (char *)buf);
        char *split;

        split = strtok(buffer, ",");
        int id = atoi(split);
        split = strtok(NULL, ",");
        int temp = atoi(split);

        server_manager_t server_manager = (server_manager_t)req->user_ctx;
        QueueHandle_t state_manager_queue = server_manager->state_manager_queue;

        uint8_t temp_to_send;
        if (temp > 0) {
            temp_to_send = (int)temp;
        } else {
            temp_to_send = 0;
        }

        int sock_fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "Received from %d", sock_fd);
        state_msg_t state_msg;
        if (id == 1) {
            if (server_manager->m1_sock_fd == -1) {  // First M1 Connect
                state_msg = state_msg_create(UPDATE, CONEXAO_1, 1);
                xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
            }

            server_manager->m1_sock_fd = sock_fd;

            state_msg_t state_msg = state_msg_create(UPDATE, SENSOR_MASSA_1, temp_to_send);
            xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
        } else if (id == 2) {
            if (server_manager->m2_sock_fd == -1) {  // First M2 Connect
                state_msg = state_msg_create(UPDATE, CONEXAO_2, 1);
                xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
            }

            server_manager->m2_sock_fd = sock_fd;

            state_msg_t state_msg = state_msg_create(UPDATE, SENSOR_MASSA_2, temp_to_send);
            xQueueSend(state_manager_queue, &state_msg, 0);
        }
    }

    free(buf);
    return ret;
}

static httpd_close_func_t on_close_session(httpd_handle_t *handle, int sock_fd) {
    server_manager_t server_manager = (server_manager_t)httpd_get_global_user_ctx(handle);
    QueueHandle_t state_manager_queue = server_manager->state_manager_queue;

    ESP_LOGE(TAG, "Session closed: %d", sock_fd);
    ESP_LOGE(TAG, "Massa 1 sockfd: %d", server_manager->m1_sock_fd);
    ESP_LOGE(TAG, "Massa 2 sockfd: %d", server_manager->m2_sock_fd);

    state_msg_t state_msg;

    if (sock_fd == server_manager->m1_sock_fd) {
        server_manager->m1_sock_fd = -1;
        state_msg = state_msg_create(UPDATE, CONEXAO_1, 0);
        xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
    } else if (sock_fd == server_manager->m2_sock_fd) {
        server_manager->m1_sock_fd = -1;
        state_msg = state_msg_create(UPDATE, CONEXAO_2, 0);
        xQueueSend(state_manager_queue, &state_msg, portMAX_DELAY);
    }

    close(sock_fd);

    return ESP_OK;
}

static esp_err_t on_lotes_url(httpd_req_t *req) {
    ListEntry *lotes_list = storage_list_files();
    int length = list_length(lotes_list);

    ESP_LOGE(TAG, "Length of files: %d", length);

    char resp[200] = {0};
    for (int i = 0; i < length; i++) {
        int lote_id = (int)list_nth_data(lotes_list, i);
        char lote_str[10] = {};

        if (i < length - 1) {
            sprintf(lote_str, "%d,", lote_id);
        } else {
            sprintf(lote_str, "%d", lote_id);
        }

        strncat(resp, lote_str, 5);
    }

    httpd_resp_sendstr(req, resp);
    list_free(lotes_list);

    return ESP_OK;
}

static esp_err_t on_lote_id_url(httpd_req_t *req) {
    char path[600] = {0};

    const char s[2] = "/";
    char *token = strtok(req->uri, s);
    token = strtok(NULL, s);
    
    sprintf(path, "/spiffs/%s.txt", token);

    FILE *file = storage_open_file_r(path);
    if (file == NULL) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    char lineRead[256];
    while (fgets(lineRead, sizeof(lineRead), file)) {
        httpd_resp_sendstr_chunk(req, lineRead);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    storage_close_file(file);

    return ESP_OK;
}

static esp_err_t on_default_url(httpd_req_t *req) {
    char path[600];
    if (strcmp(req->uri, "/") == 0) {
        strcpy(path, "/spiffs/index.html");
    } else {
        sprintf(path, "/spiffs%s", req->uri);
    }

    char *ext = strrchr(path, '.');
    if (strcmp(ext, ".css") == 0) {
        httpd_resp_set_type(req, "text/css");
    }
    if (strcmp(ext, ".js") == 0) {
        httpd_resp_set_type(req, "text/javascript");
    }
    if (strcmp(ext, ".png") == 0) {
        httpd_resp_set_type(req, "image/png");
    }

    // handle other files

    FILE *file = storage_open_file_r(path);
    if (file == NULL) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    char lineRead[256];
    while (fgets(lineRead, sizeof(lineRead), file)) {
        httpd_resp_sendstr_chunk(req, lineRead);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    storage_close_file(file);
    return ESP_OK;
}

static httpd_handle_t server_init(server_manager_t server_manager) {
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true,
        .user_ctx = server_manager};

    httpd_uri_t lotes_url = {
        .uri = "/lotes",
        .method = HTTP_GET,
        .handler = on_lotes_url};

    httpd_uri_t lote_id_url = {
        .uri = "/lote/*",
        .method = HTTP_GET,
        .handler = on_lote_id_url};

    httpd_uri_t default_url = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = on_default_url};

    httpd_config_t config = {
        .task_priority = tskIDLE_PRIORITY + 5,
        .stack_size = 4096,
        .core_id = tskNO_AFFINITY,
        .server_port = 80,
        .ctrl_port = 32768,
        .max_open_sockets = 7,
        .max_uri_handlers = 8,
        .max_resp_headers = 8,
        .backlog_conn = 5,
        .lru_purge_enable = false,
        .recv_wait_timeout = 5,
        .send_wait_timeout = 5,
        .global_user_ctx = server_manager,
        .global_user_ctx_free_fn = NULL,
        .global_transport_ctx = NULL,
        .global_transport_ctx_free_fn = NULL,
        .open_fn = NULL,
        .close_fn = on_close_session,
        .uri_match_fn = httpd_uri_match_wildcard};

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &ws);  // WEBSOCKET dos ESP32
        httpd_register_uri_handler(server, &lote_id_url);
        httpd_register_uri_handler(server, &lotes_url);
        httpd_register_uri_handler(server, &default_url);
    }

    return server;
}

void start_mdns_service() {
    mdns_init();
    mdns_hostname_set("ausyx.local");
    mdns_instance_name_set("Ausyx");
}

server_manager_t server_manager_init(QueueHandle_t state_manager_queue, QueueHandle_t server_update_queue) {
    wifi_ap_init();

    server_manager_t server_manager = malloc(sizeof(s_server_manager_t));
    server_manager->state_manager_queue = state_manager_queue;
    server_manager->server_update_queue = server_update_queue;
    server_manager->m1_sock_fd = -1;
    server_manager->m2_sock_fd = -1;
    server_manager->server = server_init(server_manager);

    return server_manager;
}

/*
    if (strstr(req->uri, "/download") != NULL) {
        char str[600];
        sprintf(str, "%s", req->uri);
        const char s[2] = "/";
        char *token;
        int lote_number = 0;

        token = strtok(str, s);
        while (token != NULL) {
            if (strcmp(token, "download") == 0) {
                token = strtok(NULL, s);
                lote_number = atoi(token);
            }
            token = strtok(NULL, s);
        }

        sprintf(path, "/spiffs/lote_%d.txt", lote_number);

        FILE *file = storage_open_file_r(path);
        if (file == NULL) {
            httpd_resp_send_404(req);
            return ESP_OK;
        }

        char lineRead[256];
        while (fgets(lineRead, sizeof(lineRead), file)) {
            httpd_resp_sendstr_chunk(req, lineRead);
        }
        httpd_resp_sendstr_chunk(req, NULL);
        storage_close_file(file);

        return ESP_OK;
        */