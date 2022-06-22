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
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_SSID,
        }};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

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

        QueueHandle_t state_manager_queue = (QueueHandle_t)req->user_ctx;

        uint8_t temp_to_send;
        if (temp > 0) {
            temp_to_send = (int)temp;
        } else {
            temp_to_send = 0;
        }

        if (id == 1) {
            state_msg_t state_msg = state_msg_create(UPDATE, SENSOR_MASSA_1, temp_to_send);
            xQueueSend(state_manager_queue, &state_msg, 0);
        } else if (id == 2) {
            state_msg_t state_msg = state_msg_create(UPDATE, SENSOR_MASSA_2, temp_to_send);
            xQueueSend(state_manager_queue, &state_msg, 0);
        }
    }

    free(buf);
    return ret;
}

static esp_err_t on_default_url(httpd_req_t *req) {
    char path[500];
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

static httpd_handle_t server_init(QueueHandle_t state_manager_queue) {
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true,
        .user_ctx = state_manager_queue};

    httpd_uri_t default_url = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = on_default_url};

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &ws);  // WEBSOCKET dos ESP32
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
    server_manager->server = server_init(state_manager_queue);

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
        **