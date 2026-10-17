#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "ds3231.h"
#include "driver/i2c_master.h"
#include "esp_check.h"

#define DS3231_ADDR_SEC     0x00
#define DS3231_ADDR_MIN     0x01
#define DS3231_ADDR_HOUR    0x02
#define DS3231_ADDR_DAY     0x03
#define DS3231_ADDR_DATE    0x04
#define DS3231_ADDR_MONTH   0x05
#define DS3231_ADDR_YEAR    0x06

#define DS3231_ADDR_CNTRL   0x0E
#define DS3231_ADDR_STAT    0x0F

#define DS3231_OSF_BITMASK  0x80
#define DS3231_EOSC_BITMASK 0x80

static const char *TAG = "DS3231";

typedef struct {
    i2c_master_dev_handle_t i2c_handle;
} ds3231_dev_t;

static uint8_t s_convert_to_bcd(uint8_t val){
    return ((val / 10) << 4) + (val % 10);
}

static void s_convert_from_bcd(uint8_t *data, size_t len){
    uint8_t val;

    for (size_t i = 0; i < len; ++i){
        val = data[i];
        data[i] = (val >> 4) * 10 + (val & 0x0F);
    }
}

static esp_err_t s_precond_checks(ds3231_dev_t *rtc){
    if (rtc == NULL)
        return ESP_ERR_INVALID_ARG;

    if (!rtc->i2c_handle)
        return ESP_ERR_INVALID_STATE;

    return ESP_OK;
}

esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *) calloc(1, sizeof(ds3231_dev_t));
    ESP_RETURN_ON_FALSE(rtc != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate device descriptor");

    const i2c_device_config_t i2c_dev_cfg = {
        .device_address = DS3231_I2C_ADDR_DEFAULT,
        .scl_speed_hz = DS3231_I2C_CLK_SPEED
    };

    ESP_GOTO_ON_ERROR(i2c_master_probe(i2c_bus, DS3231_I2C_ADDR_DEFAULT, DS3231_TIMEOUT_MS), err,
                      TAG, "No ACK from device at 0x%02X", DS3231_I2C_ADDR_DEFAULT);

    ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &i2c_dev_cfg, &rtc->i2c_handle), err,
                      TAG, "Failed to add new I2C device");

    *ds3231_handle = rtc;
    return ret;

    err:
        ds3231_delete(rtc);
        return ret;
}

void ds3231_delete(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    if (rtc == NULL)
        return;

    if (rtc->i2c_handle)
        i2c_master_bus_rm_device(rtc->i2c_handle);

    free(rtc);
}

esp_err_t ds3231_get_time_valid(ds3231_handle_t ds3231_handle, bool *time_valid){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t status_buf_write = DS3231_ADDR_STAT;
    uint8_t status_buf_read;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &status_buf_write, 1, &status_buf_read, 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read status register");

    if (!(status_buf_read & DS3231_OSF_BITMASK))
        *time_valid = true;
    else
        *time_valid = false;

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_osc_battery_get_status(ds3231_handle_t ds3231_handle, bool *enabled){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t control_reg_addr = DS3231_ADDR_CNTRL;
    uint8_t control_reg_buf;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &control_reg_addr, 1, &control_reg_buf, 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read control register");

    if (!(control_reg_buf & DS3231_EOSC_BITMASK))
        *enabled = true;
    else
        *enabled = false;

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_osc_battery_set_status(ds3231_handle_t ds3231_handle, bool enabled){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    
    uint8_t control_reg_buf[2];
    control_reg_buf[0] = DS3231_ADDR_CNTRL;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &(control_reg_buf[0]), 1, &(control_reg_buf[1]), 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read control register");
                      
    if (enabled)
        control_reg_buf[1] &= ~DS3231_EOSC_BITMASK;
    else
        control_reg_buf[1] |= DS3231_EOSC_BITMASK;
        
    ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, control_reg_buf, 2, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to write control register");

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_get_time_format(ds3231_handle_t ds3231_handle, ds3231_time_format_t *timeformat){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t data_buf_write = DS3231_ADDR_HOUR;
    uint8_t data_buf_read;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &data_buf_write, 1, &data_buf_read, 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read hour register");

    if (data_buf_read & (1 << 6))
        *timeformat = DS3231_TIME_FORMAT_12H;
    else
        *timeformat = DS3231_TIME_FORMAT_24H;

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_set_time(ds3231_handle_t ds3231_handle, struct tm timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t data[8];

    data[0] = DS3231_ADDR_SEC;
    data[1] = s_convert_to_bcd(timeinfo.tm_sec);
    data[2] = s_convert_to_bcd(timeinfo.tm_min);

    if (is_pm == NULL)
        data[3] = s_convert_to_bcd(timeinfo.tm_hour);
    else if (*is_pm == true)
        data[3] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 5) | (1 << 6);
    else
        data[3] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 6);

    data[4] = s_convert_to_bcd(timeinfo.tm_wday + 1);
    data[5] = s_convert_to_bcd(timeinfo.tm_mday);
    data[6] = s_convert_to_bcd(timeinfo.tm_mon + 1);
    data[7] = s_convert_to_bcd(timeinfo.tm_year - 100);

    ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, data, 8, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to write time registers");

    uint8_t change_osf[2];
    change_osf[0] = DS3231_ADDR_STAT;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &(change_osf[0]), 1, &(change_osf[1]), 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read status register");

    if ((change_osf[1] & DS3231_OSF_BITMASK)){
        change_osf[1] &= ~DS3231_OSF_BITMASK;
        ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, change_osf, 2, DS3231_TIMEOUT_MS), err,
                                              TAG, "Failed to write status register");
    }

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t data_buf_write = DS3231_ADDR_SEC;
    uint8_t data_buf_read[7];

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &data_buf_write, 1, data_buf_read, 7, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read time registers");


    // check the format of time
    if (data_buf_read[2] & (1 << 6)){
        ESP_GOTO_ON_FALSE(is_pm != NULL, ESP_ERR_INVALID_ARG, err, TAG, "is_pm must be not NULL in 12-hour mode");

        data_buf_read[2] &= ~(1 << 6);
        
        if (data_buf_read[2] & (1 << 5))
            *is_pm = true;
        else
            *is_pm = false;

        data_buf_read[2] &= ~(1 << 5);
    }

    data_buf_read[5] &= ~(1 << 7); // turn off century bit
    s_convert_from_bcd(data_buf_read, sizeof(data_buf_read));

    timeinfo->tm_sec = data_buf_read[0];
    timeinfo->tm_min = data_buf_read[1];
    timeinfo->tm_hour = data_buf_read[2];
    timeinfo->tm_wday = data_buf_read[3] - 1;
    timeinfo->tm_mday = data_buf_read[4];
    timeinfo->tm_mon = data_buf_read[5] - 1;
    timeinfo->tm_year = data_buf_read[6] + 100;

    return ret;

    err:
        return ret;
}

