#pragma once

#include <time.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#define DS3231_I2C_ADDR_DEFAULT     0b1101000
#define DS3231_I2C_CLK_SPEED        100000UL
#define DS3231_TIMEOUT_MS           50

#ifdef __cplusplus
extern "C"{
#endif

typedef enum {
    REG
} rtc_registers;

typedef void *ds3231_handle_t;

esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle);
void ds3231_delete(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_get_time_valid(ds3231_handle_t ds3231_handle, bool *time_valid);
esp_err_t ds3231_osc_battery_get_status(ds3231_handle_t ds3231_handle, bool *enabled);
esp_err_t ds3231_osc_battery_set_status(ds3231_handle_t ds3231_handle, bool enabled);
esp_err_t ds3231_set_time_format(ds3231_handle_t ds3231_handle);
esp_err_t ds3231_set_time(ds3231_handle_t ds3231_handle, struct tm timeinfo);
esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo);

#ifdef __cplusplus
}
#endif