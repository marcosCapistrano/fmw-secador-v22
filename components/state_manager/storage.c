#include "storage.h"

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "STORAGE";

static esp_err_t storage_get_i8(nvs_handle_t nvs_handle, const char *key, int8_t *out_value);
static esp_err_t storage_set_i8(nvs_handle_t nvs_handle, const char *key, int8_t value);
static esp_err_t storage_get_u8(nvs_handle_t nvs_handle, const char *key, uint8_t *out_value);
static esp_err_t storage_set_u8(nvs_handle_t nvs_handle, const char *key, uint8_t value);

esp_err_t storage_get_mode(nvs_handle_t nvs_handle, uint8_t *mode) {
    return storage_get_u8(nvs_handle, KEY_MODE, mode);
}

esp_err_t storage_set_mode(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_MODE, value);
}

esp_err_t storage_get_lote_number(nvs_handle_t nvs_handle, uint8_t *lote_number) {
    return storage_get_u8(nvs_handle, KEY_LOTE_NUM, lote_number);
}

esp_err_t storage_set_lote_number(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_LOTE_NUM, value);
}

esp_err_t storage_get_sensor_entrada(nvs_handle_t nvs_handle, int8_t *sensor_entrada) {
    return storage_get_i8(nvs_handle, KEY_SENSOR_ENTRADA, sensor_entrada);
}

esp_err_t storage_set_sensor_entrada(nvs_handle_t nvs_handle, int8_t value) {
    return storage_set_i8(nvs_handle, KEY_SENSOR_ENTRADA, value);
}

esp_err_t storage_get_sensor_massa_1(nvs_handle_t nvs_handle, int8_t *sensor_massa_1) {
    return storage_get_i8(nvs_handle, KEY_SENSOR_MASSA_1, sensor_massa_1);
}

esp_err_t storage_set_sensor_massa_1(nvs_handle_t nvs_handle, int8_t value) {
    return storage_set_i8(nvs_handle, KEY_SENSOR_MASSA_1, value);
}

esp_err_t storage_get_sensor_massa_2(nvs_handle_t nvs_handle, int8_t *sensor_massa_2) {
    return storage_get_i8(nvs_handle, KEY_SENSOR_MASSA_2, sensor_massa_2);
}

esp_err_t storage_set_sensor_massa_2(nvs_handle_t nvs_handle, int8_t value) {
    return storage_set_i8(nvs_handle, KEY_SENSOR_MASSA_2, value);
}

esp_err_t storage_get_sensor_entrada_min(nvs_handle_t nvs_handle, uint8_t *sensor_entrada_min) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_ENTRADA_MIN, sensor_entrada_min);
}

esp_err_t storage_set_sensor_entrada_min(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_ENTRADA_MIN, value);
}

esp_err_t storage_get_sensor_entrada_max(nvs_handle_t nvs_handle, uint8_t *sensor_entrada_max) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_ENTRADA_MIN, sensor_entrada_max);
}

esp_err_t storage_set_sensor_entrada_max(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_ENTRADA_MAX, value);
}

esp_err_t storage_get_sensor_massa_1_min(nvs_handle_t nvs_handle, uint8_t *sensor_massa_1_min) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_MASSA_1_MIN, sensor_massa_1_min);
}

esp_err_t storage_set_sensor_massa_1_min(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_MASSA_1_MIN, value);
}

esp_err_t storage_get_sensor_massa_1_max(nvs_handle_t nvs_handle, uint8_t *sensor_massa_1_max) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_MASSA_1_MAX, sensor_massa_1_max);
}

esp_err_t storage_set_sensor_massa_1_max(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_MASSA_1_MAX, value);
}

esp_err_t storage_get_sensor_massa_2_min(nvs_handle_t nvs_handle, uint8_t *sensor_massa_2_min) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_MASSA_2_MIN, sensor_massa_2_min);
}

esp_err_t storage_set_sensor_massa_2_min(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_MASSA_2_MIN, value);
}

esp_err_t storage_get_sensor_massa_2_max(nvs_handle_t nvs_handle, uint8_t *sensor_massa_2_max) {
    return storage_get_u8(nvs_handle, KEY_SENSOR_MASSA_2_MAX, sensor_massa_2_max);
}

esp_err_t storage_set_sensor_massa_2_max(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_SENSOR_MASSA_2_MAX, value);
}

esp_err_t storage_get_finished(nvs_handle_t nvs_handle, uint8_t *finished) {
    return storage_get_u8(nvs_handle, KEY_FINISHED, finished);
}

esp_err_t storage_set_finished(nvs_handle_t nvs_handle, uint8_t value) {
    return storage_set_u8(nvs_handle, KEY_FINISHED, value);
}

esp_err_t storage_get_i8(nvs_handle_t nvs_handle, const char *key, int8_t *out_value) {
    esp_err_t err = nvs_get_i8(nvs_handle, key, out_value);

    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            storage_set_i8(nvs_handle, key, 0);
            return storage_get_i8(nvs_handle, key, out_value);
        }
        ESP_LOGE(TAG, "Erro ao procurar valor: %s", key);
    }

    return ESP_OK;
}

esp_err_t storage_set_i8(nvs_handle_t nvs_handle, const char *key, int8_t value) {
    esp_err_t err = nvs_set_i8(nvs_handle, key, value);

    return err;
}

esp_err_t storage_get_u8(nvs_handle_t nvs_handle, const char *key, uint8_t *out_value) {
    esp_err_t err = nvs_get_u8(nvs_handle, key, out_value);

    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            storage_set_u8(nvs_handle, key, 0);
            return storage_get_u8(nvs_handle, key, out_value);
        }
        ESP_LOGE(TAG, "Erro ao procurar valor: %s", key);
    }

    return ESP_OK;
}

esp_err_t storage_set_u8(nvs_handle_t nvs_handle, const char *key, uint8_t value) {
    esp_err_t err = nvs_set_u8(nvs_handle, key, value);

    return err;
}