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
    DS3231_TIME_FORMAT_12H, /**< 12-hour format with AM/PM  */
    DS3231_TIME_FORMAT_24H /**< 24-hour format */
} ds3231_time_format_t;

typedef enum {
    DS3231_FREQ_1HZ    = 0x00,
    DS3231_FREQ_1024HZ = 0x08,
    DS3231_FREQ_4096HZ = 0x10,
    DS3231_FREQ_8192HZ = 0x18
} ds3231_squarewave_freq_t;

typedef enum {
    DS3231_ALARM_NONE   = 0x00,
    DS3231_ALARM_A1     = 0x01,
    DS3231_ALARM_A2     = 0x02,
    DS3231_ALARM_A1_A2  = 0x03
} ds3231_alarms_t;

typedef enum {
    DS3231_ALARM_NO_INTERR              = 0x00,
    DS3231_ALARM1_FLAG_INTERR           = 0x01,
    DS3231_ALARM2_FLAG_INTERR           = 0x02,
    DS3231_ALARM1_ALARM2_FLAGS_INTERR   = 0x03
} ds3231_alarm_flags_t;

typedef enum {
    DS3231_ALARM1_INTERR_PER_SEC          = 0x0F,   
    DS3231_ALARM1_INTERR_MATCH_SEC        = 0x0E,   
    DS3231_ALARM1_INTERR_MATCH_MINSEC     = 0x0C,       
    DS3231_ALARM1_INTERR_MATCH_HOURMINSEC = 0x08,           
    DS3231_ALARM1_INTERR_DYHOURMINSEC     = 0x01,
    DS3231_ALARM1_INTERR_DTHOURMINSEC     = 0x00
} ds3231_alarm1_match_t;

typedef void *ds3231_handle_t;

/**
 * @brief Initialize device handle
 * @param[in] i2c_bus I2C bus handle
 * @param[out] ds3231_handle Device handle
 * @return ESP_OK on success
 */
esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle);

/**
 * @brief Free device handle
 * 
 * @param[in] ds3231_handle Device handle
 */
void ds3231_delete(ds3231_handle_t ds3231_handle);

/**
 * @brief Check validity of the time module saved
 * 
 * @warning The result is reliable only when battery is installed
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] time_valid True if time is valid, false if not
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time_valid(ds3231_handle_t ds3231_handle, bool *time_valid);

/**
 * @brief Check whether the oscillator is enabled when running on battery or not
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] enabled True if the oscillator is running on battery, false if it stops
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_osc_battery_get_status(ds3231_handle_t ds3231_handle, bool *enabled);

/**
 * @brief Enable or disable the oscillator when running on battery
 * 
 * @param[in] ds3231_handle Device handle
 * @param[in] enabled True to keep the oscillator running on battery, false to stop
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_osc_battery_set_status(ds3231_handle_t ds3231_handle, bool enabled);

/**
 * @brief Get the current time format
 *
 * @param[in] ds3231_handle Device handle
 * @param[out] timeformat Format of time
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
 * @warning The time format of the device is set by this function.
 *          If is_pm is NULL, the time will be stored in 24-hour format.
 *          Otherwise, it is stored in 12-hour format
 * 
 * @param[in] ds3231_handle Device handle
 * @param[in] timeinfo Date and time to set. Uses tm_sec, tm_min, tm_hour,
 *                     tm_mday, tm_mon, tm_year and tm_wday. tm_hour must be 0-23 in 
 *                     24-hour format and 1-12 in 12-hour format
 * 
 * @param[in] is_pm NULL for 24-hour format. For 12-hour format: true for PM, false for AM
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_time(ds3231_handle_t ds3231_handle, struct tm timeinfo, bool *is_pm);

/**
 * @brief Get the current date and time. 
 * 
 * @note The tm_yday and tm_isdst fields of the tm structure are ignored
 * 
 * @note This function doesn't check whether the time is valid. Use ds3231_get_time_valid() for it
 * 
 * @warning The tm_hour field of the tm structure is in the range 1-12 in 12-hour format
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] timeinfo Date and time read from the device. Uses tm_sec, tm_min, tm_hour,
 *                      tm_mday, tm_mon, tm_year and tm_wday
 * @param[out] is_pm Must be not NULL in 12-hour format
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo, bool *is_pm);

/**
 * @brief Read temperature from the chip
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] temp Temperature, Celsius
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_temperature(ds3231_handle_t ds3231_handle, float *temp);

/**
 * @brief Get the aging offset
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] age Aging offset
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_aging(ds3231_handle_t ds3231_handle, int8_t *age);

/**
 * @brief Set the aging offset
 * 
 * @param[in] ds3231_handle Device handle
 * @param[in] age Aging offset in range -128..127
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_aging(ds3231_handle_t ds3231_handle, int8_t age);

/**
 * @brief Set square wave frequency
 * 
 * @param[in] ds3231_handle Device handle
 * @param[in] freq Frequency to set
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_set_freq(ds3231_handle_t ds3231_handle, ds3231_squarewave_freq_t freq);

/**
 * @brief Get square wave frequency
 * 
 * @param[in] ds3231_handle Device handle
 * @param[out] freq Frequency read from the device
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_get_freq(ds3231_handle_t ds3231_handle, ds3231_squarewave_freq_t *freq);

/**
 * @brief Enable generating of the square wave
 * 
 * @note Set the square wave frequency with the ds3231_squarewave_set_freq() function first.
 *       Otherwise, the value currently stored in the device will be used.
 * 
 * @warning Enabling the square wave, you automatically disable alarm interrupts
 *  
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_enable(ds3231_handle_t ds3231_handle);

/**
 * @brief Disable generating of the square wave
 * 
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_disable(ds3231_handle_t ds3231_handle);

/**
 * @brief Allow the device to run the square wave when powered from the battery
 * 
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_battery_enable(ds3231_handle_t ds3231_handle);

/**
 * @brief Disallow the device to run the square wave when powered from the battery
 * 
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_battery_disable(ds3231_handle_t ds3231_handle);

/**
 * @brief Enable 32 kHz output
 * 
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_32khz_enable(ds3231_handle_t ds3231_handle);

/**
 * @brief Disable 32 kHz output
 * 
 * @param[in] ds3231_handle Device handle
 * 
 * @return ESP_OK on success
 */
esp_err_t ds3231_squarewave_32khz_disable(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm_interr_get_status(ds3231_handle_t ds3231_handle, ds3231_alarms_t *interr);

esp_err_t ds3231_alarm1_interr_enable(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm1_interr_disable(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm1_get_time(ds3231_handle_t ds3231_handle, ds3231_alarm1_match_t *interr, struct tm *timeinfo, bool *is_pm);

esp_err_t ds3231_alarm1_set_time(ds3231_handle_t ds3231_handle, ds3231_alarm1_match_t interr, struct tm timeinfo, bool *is_pm);

esp_err_t ds3231_alarm2_get_time(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm2_set_time(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm2_interr_enable(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm2_interr_disable(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm2_set_interr(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm2_get_interr(ds3231_handle_t ds3231_handle);

esp_err_t ds3231_alarm_get_flags(ds3231_handle_t ds3231_handle, ds3231_alarm_flags_t *interr);

esp_err_t ds3231_alarm_clear_flags(ds3231_handle_t ds3231_handle, ds3231_alarm_flags_t interr);

#ifdef __cplusplus
}
#endif
