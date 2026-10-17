#pragma once

#include <time.h>
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#define DS3231_I2C_ADDR_DEFAULT     0b1101000
#define DS3231_I2C_CLK_SPEED        100000UL
#define DS3231_TIMEOUT_MS           50

#ifdef __cplusplus
extern "C"{
#endif

typedef enum {
    DS3231_TIME_FORMAT_12H,
    DS3231_TIME_FORMAT_24H
} ds3231_time_format_t;

typedef void *ds3231_handle_t;

/**
 * @brief initialize device handle
 * @param[in] i2c_bus i2c handle
 * @param[out] ds3231_handle device handle
 * @return ESP_OK on success
 */
esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle);

/**
 * @brief free device handle
 * 
 * @param ds3231_handle device handle
 */
void ds3231_delete(ds3231_handle_t ds3231_handle);

/**
 * @brief check validity of the time module saved
 * 
 * @warning Еру result is reliable only when battery is installed
 * 
 * @param[in] ds3231_handle device handle
 * @param[out] time_valid true if time is valid, false if not
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time_valid(ds3231_handle_t ds3231_handle, bool *time_valid);

/**
 * @brief check whether the oscillator is enabled when running on battery or not
 * 
 * @param[in] ds3231_handle device handle
 * @param[out] enabled true if the oscillator is running on battery, false if it stops
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_osc_battery_get_status(ds3231_handle_t ds3231_handle, bool *enabled);

/**
 * @brief Enable or disable the oscillator when running on battery
 * 
 * @param[in] ds3231_handle device handle
 * @param[in] enabled true to keep the oscillator running on battery, false to stop
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_osc_battery_set_status(ds3231_handle_t ds3231_handle, bool enabled);

/**
 * @brief Set 12-hour or 24-hour time format
 * 
 * @param[in] ds3231_handle device handle
 * @param[in] timeformat format of time
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_time_format(ds3231_handle_t ds3231_handle, ds3231_time_format_t timeformat);

/**
 * @brief Set current date time
 * 
 * @note Only years 2000-2099 are supported. The tm_yday and tm_isdst fields
 *       of the tm structure are ignored
 * 
 * @param[in] ds3231_handle device handle
 * @param[in] timeinfo date and time to set. Uses tm_sec, tm_min, tm_hour,
 *                     tm_mday, tm_mon, tm_year and tm_wday
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_time(ds3231_handle_t ds3231_handle, struct tm timeinfo);

/**
 * @brief Get the current date and time. 
 * 
 * @note The tm_yday and tm_isdst fields of the tm structure are ignored
 * 
 * @note this function doesn't check whether the time is valid. Use ds3231_get_time_valid() for it
 * 
 * @param[in] ds3231_handle device handle
 * @param[out] timeinfo date and time read from the device. Uses tm_sec, tm_min, tm_hour,
 *                      tm_mday, tm_mon, tm_year and tm_wday
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo);

#ifdef __cplusplus
}
#endif
