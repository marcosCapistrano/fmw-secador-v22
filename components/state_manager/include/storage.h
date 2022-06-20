#ifndef STORAGE_H
#define STORAGE_H

#include "inttypes.h"
#include "nvs.h"

#define KEY_MODE "mode"
#define KEY_LOTE_NUM "lote_number"
#define KEY_SENSOR_ENTRADA "s_entrada"
#define KEY_SENSOR_MASSA_1 "s_m1"
#define KEY_SENSOR_MASSA_2 "s_m1"
#define KEY_SENSOR_ENTRADA_MIN "s_entrada_min"
#define KEY_SENSOR_ENTRADA_MAX "s_entrada_max"
#define KEY_SENSOR_MASSA_1_MIN "s_m1_min"
#define KEY_SENSOR_MASSA_1_MAX "s_m1_max"
#define KEY_SENSOR_MASSA_2_MIN "s_m2_min"
#define KEY_SENSOR_MASSA_2_MAX "s_m2_max"
#define KEY_FINISHED "finished"

esp_err_t storage_get_mode(nvs_handle_t nvs_handle, uint8_t *mode);
esp_err_t storage_set_mode(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_lote_number(nvs_handle_t nvs_handle, uint8_t *lote_number);
esp_err_t storage_set_lote_number(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_entrada(nvs_handle_t nvs_handle, uint8_t *sensor_entrada);
esp_err_t storage_set_sensor_entrada(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_1(nvs_handle_t nvs_handle, uint8_t *sensor_massa_1);
esp_err_t storage_set_sensor_massa_1(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_2(nvs_handle_t nvs_handle, uint8_t *sensor_massa_2);
esp_err_t storage_set_sensor_massa_2(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_entrada_min(nvs_handle_t nvs_handle, uint8_t *sensor_entrada_min);
esp_err_t storage_set_sensor_entrada_min(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_entrada_max(nvs_handle_t nvs_handle, uint8_t *sensor_entrada_max);
esp_err_t storage_set_sensor_entrada_max(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_1_min(nvs_handle_t nvs_handle, uint8_t *sensor_massa_1_min);
esp_err_t storage_set_sensor_massa_1_min(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_1_max(nvs_handle_t nvs_handle, uint8_t *sensor_massa_1_max);
esp_err_t storage_set_sensor_massa_1_max(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_2_min(nvs_handle_t nvs_handle, uint8_t *sensor_massa_2_min);
esp_err_t storage_set_sensor_massa_2_min(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_sensor_massa_2_max(nvs_handle_t nvs_handle, uint8_t *sensor_massa_2_max);
esp_err_t storage_set_sensor_massa_2_max(nvs_handle_t nvs_handle, uint8_t value);
esp_err_t storage_get_finished(nvs_handle_t nvs_handle, uint8_t *finished);
esp_err_t storage_set_finished(nvs_handle_t nvs_handle, uint8_t value);


#endif