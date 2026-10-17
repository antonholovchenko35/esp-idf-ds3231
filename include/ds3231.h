#pragma once

#include <time.h>
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#define DS3231_I2C_ADDR_DEFAULT     0x68
#define DS3231_I2C_CLK_SPEED        100000UL
#define DS3231_TIMEOUT_MS           50

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Time format of the device
 */
typedef enum {
    DS3231_TIME_FORMAT_12H, /**< 12-hour format with am/pm  */
    DS3231_TIME_FORMAT_24H /**< 24-hour format */
} ds3231_time_format_t;

typedef void *ds3231_handle_t;

/**
 * @brief initialize device handle
 * @param[in] i2c_bus i2c bus handle
 * @param[out] ds3231_handle device handle
 * @return ESP_OK on success
 */
esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle);

/**
 * @brief free device handle
 * 
 * @param[in] ds3231_handle device handle
 */
void ds3231_delete(ds3231_handle_t ds3231_handle);

/**
 * @brief check validity of the time module saved
 * 
 * @warning The result is reliable only when battery is installed
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
 * @brief Get the current time format
 *
 * @param[in] ds3231_handle device handle
 * @param[out] timeformat format of time
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time_format(ds3231_handle_t ds3231_handle, ds3231_time_format_t *timeformat);

/**
 * @brief Set current date time
 * 
 * @note Only years 2000-2099 are supported. The tm_yday and tm_isdst fields
 *       of the tm structure are ignored
 * 
 * @warning THe time format of the device is set by this function.
 *          If is_pm is NULL, the time will be stored in 24-hour format.
 *          Otherwise, it is stored in 12-hour format
 * 
 * @param[in] ds3231_handle device handle
 * @param[in] timeinfo date and time to set. Uses tm_sec, tm_min, tm_hour,
 *                     tm_mday, tm_mon, tm_year and tm_wday. tm_hour must be 0-23 in 
 *                     24-hour format and 1-12 in 12-hour format
 * 
 * @param[in] is_pm NULL for 24-hour format. For 12-hour format: true for pm, false for am
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_time(ds3231_handle_t ds3231_handle, struct tm timeinfo, bool *is_pm);

/**
 * @brief Get the current date and time. 
 * 
 * @note The tm_yday and tm_isdst fields of the tm structure are ignored
 * 
 * @note this function doesn't check whether the time is valid. Use ds3231_get_time_valid() for it
 * 
 * @warning The tm_hour field of the tm structure is in the range 1-12 in 12-hour format
 * 
 * @param[in] ds3231_handle device handle
 * @param[out] timeinfo date and time read from the device. Uses tm_sec, tm_min, tm_hour,
 *                      tm_mday, tm_mon, tm_year and tm_wday
 * @param[out] is_pm Must be not NULL in 12-hour format
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo, bool *is_pm);

#ifdef __cplusplus
}
#endif
