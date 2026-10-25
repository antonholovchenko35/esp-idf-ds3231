#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "ds3231.h"
#include "driver/i2c_master.h"
#include "esp_check.h"

#define DS3231_ADDR_SEC                 0x00
#define DS3231_ADDR_MIN                 0x01
#define DS3231_ADDR_HOUR                0x02
#define DS3231_ADDR_DAY                 0x03
#define DS3231_ADDR_DATE                0x04
#define DS3231_ADDR_MONTH               0x05
#define DS3231_ADDR_YEAR                0x06

#define DS3231_ADDR_ALARM1_SEC          0x07
#define DS3231_ADDR_ALARM1_MIN          0x08
#define DS3231_ADDR_ALARM1_HOUR         0x09
#define DS3231_ADDR_ALARM1_DY_DT        0x0A

#define DS3231_ADDR_ALARM2_MIN          0x0B
#define DS3231_ADDR_ALARM2_HOUR         0x0C
#define DS3231_ADDR_ALARM2_DY_DT        0x0D

#define DS3231_ADDR_CNTRL               0x0E
#define DS3231_ADDR_STAT                0x0F
#define DS3231_ADDR_AGING               0x10
#define DS3231_ADDR_TEMP                0x11

#define DS3231_OSF_BITMASK              0x80
#define DS3231_FREQ_BITMASK             0x18
#define DS3231_EOSC_BITMASK             0x80
#define DS3231_ALARM1_DY_DT_BITMASK     0x40
#define DS3231_ALARM1_MASKBITS_BITMASK  0x80
#define DS3231_ALARM1_INTERR_BITMASK    0x01
#define DS3231_ALARM2_DY_DT_BITMASK     0x40
#define DS3231_ALARM2_INTERR_BITMASK    0x02
#define DS3231_ALARM_FLAG_BITMASK       0x03

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

/**
 * @brief Modify register
 * 
 * @param[in] rtc rtc dev
 * @param[in] reg_addr register address
 * @param[in] mask bitmask of the necessary bits to clear
 * @param[in] val the new value to be written to the bits
 */
static esp_err_t s_modify_reg(ds3231_dev_t *rtc, uint8_t reg_addr, uint8_t mask, uint8_t val){
    uint8_t reg_buf[2];
    reg_buf[0] = reg_addr;

    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &(reg_buf[0]), 1, &(reg_buf[1]), 1, DS3231_TIMEOUT_MS),
                        TAG, "Failed to read register 0x%02X", reg_addr);

    reg_buf[1] = (reg_buf[1] & ~mask) | val;

    ESP_RETURN_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, reg_buf, 2, DS3231_TIMEOUT_MS),
                        TAG, "Failed to write register 0x%02X", reg_addr);

    return ESP_OK;
}

esp_err_t ds3231_create(i2c_master_bus_handle_t i2c_bus, ds3231_handle_t *ds3231_handle){
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(i2c_bus != NULL, ESP_ERR_INVALID_ARG, TAG, "\"i2c_bus\" must not be NULL");
    ESP_RETURN_ON_FALSE(ds3231_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "\"ds3231_handle\" must not be NULL");

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
    ESP_GOTO_ON_FALSE(time_valid != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"time_valid\" must not be NULL");

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
    ESP_GOTO_ON_FALSE(enabled != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"enabled\" must not be NULL");

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
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    if (enabled)
        return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_EOSC_BITMASK, 0);
    else
        return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_EOSC_BITMASK, DS3231_EOSC_BITMASK);
}

esp_err_t ds3231_get_time_format(ds3231_handle_t ds3231_handle, ds3231_time_format_t *timeformat){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(timeformat != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"timeformat\" must not be NULL");

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
    ESP_GOTO_ON_FALSE((is_pm == NULL && timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23)
                      || (is_pm != NULL && timeinfo.tm_hour >= 1 && timeinfo.tm_hour <= 12),
                      ESP_ERR_INVALID_ARG, err, TAG, "tm_hour is out of range for the selected time format");
    ESP_GOTO_ON_FALSE(timeinfo.tm_sec >= 0 && timeinfo.tm_sec <= 59, ESP_ERR_INVALID_ARG, err,
                      TAG, "tm_sec is out of range (0-59)");
    ESP_GOTO_ON_FALSE(timeinfo.tm_min >= 0 && timeinfo.tm_min <= 59, ESP_ERR_INVALID_ARG, err,
                      TAG, "tm_min is out of range (0-59)");
    ESP_GOTO_ON_FALSE(timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6, ESP_ERR_INVALID_ARG, err,
                      TAG, "tm_wday is out of range (0-6)");
    ESP_GOTO_ON_FALSE(timeinfo.tm_mday >= 1 && timeinfo.tm_mday <= 31, ESP_ERR_INVALID_ARG, err,
                      TAG, "tm_mday is out of range (1-31)");
    ESP_GOTO_ON_FALSE(timeinfo.tm_mon >= 0 && timeinfo.tm_mon <= 11, ESP_ERR_INVALID_ARG, err,
                      TAG, "tm_mon is out of range (0-11)");

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

    ret = s_modify_reg(rtc, DS3231_ADDR_STAT, DS3231_OSF_BITMASK | DS3231_ALARM_FLAG_BITMASK, DS3231_ALARM_FLAG_BITMASK);

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_get_time(ds3231_handle_t ds3231_handle, struct tm *timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(timeinfo != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"timeinfo\" must not be NULL");

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

esp_err_t ds3231_get_temperature(ds3231_handle_t ds3231_handle, float *temp){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(temp != NULL, ESP_ERR_INVALID_ARG, err, TAG, "\"temp\" must not be NULL");

    uint8_t temp_addr = DS3231_ADDR_TEMP;;
    uint8_t temp_buf[2];

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &temp_addr, 1, temp_buf, 2, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read temp registers");

    *temp = (int8_t)temp_buf[0] + (temp_buf[1] >> 6) * 0.25f;

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_get_aging(ds3231_handle_t ds3231_handle, int8_t *age){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(age != NULL, ESP_ERR_INVALID_ARG, err, TAG, "\"age\" must not be NULL");

    uint8_t aging_addr = DS3231_ADDR_AGING;
    uint8_t aging_read;

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &aging_addr, 1, &aging_read, 1, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read aging register");

    *age = (int8_t)aging_read;

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_set_aging(ds3231_handle_t ds3231_handle, int8_t age){
    esp_err_t ret = ESP_OK;

    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");

    uint8_t aging_buf[2]; 
    aging_buf[0] = DS3231_ADDR_AGING;
    aging_buf[1] = (uint8_t)age;

    ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, aging_buf, 2, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to write aging register");

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_squarewave_set_freq(ds3231_handle_t ds3231_handle, ds3231_squarewave_freq_t freq){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");
    ESP_RETURN_ON_FALSE(!(freq & ~DS3231_FREQ_BITMASK), ESP_ERR_INVALID_ARG,
                        TAG, "The freq out of range. See ds3231_squarewave_freq_t for the allowed values");
                        
    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_FREQ_BITMASK, freq);
}

esp_err_t ds3231_squarewave_get_freq(ds3231_handle_t ds3231_handle, ds3231_squarewave_freq_t *freq){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");
    ESP_RETURN_ON_FALSE(freq != NULL, ESP_ERR_INVALID_ARG, TAG, "\"freq\" must not be NULL");

    uint8_t cntrl_addr = DS3231_ADDR_CNTRL;
    uint8_t cntrl_read;

    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &cntrl_addr, 1, &cntrl_read, 1, DS3231_TIMEOUT_MS),
                        TAG, "Failed to read control register");

    cntrl_read &= DS3231_FREQ_BITMASK;

    switch (cntrl_read) {
        case DS3231_FREQ_1HZ:
        case DS3231_FREQ_1024HZ:
        case DS3231_FREQ_4096HZ:
        case DS3231_FREQ_8192HZ:
            *freq = (ds3231_squarewave_freq_t)cntrl_read;
            return ESP_OK;

        default:
            return ESP_ERR_INVALID_RESPONSE;
    }
}

esp_err_t ds3231_squarewave_enable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, (1 << 2), 0);
}

esp_err_t ds3231_squarewave_disable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, (1 << 2), (1 << 2));
}

esp_err_t ds3231_squarewave_battery_enable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, (1 << 6), (1 << 6));
}

esp_err_t ds3231_squarewave_battery_disable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, (1 << 6), 0);
}

esp_err_t ds3231_squarewave_32khz_enable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");
    
    return s_modify_reg(rtc, DS3231_ADDR_STAT, (1 << 3) | DS3231_ALARM_FLAG_BITMASK, (1 << 3) | DS3231_ALARM_FLAG_BITMASK); 
}

esp_err_t ds3231_squarewave_32khz_disable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_STAT, (1 << 3) | DS3231_ALARM_FLAG_BITMASK, DS3231_ALARM_FLAG_BITMASK);
}

esp_err_t ds3231_alarm_interr_get_status(ds3231_handle_t ds3231_handle, ds3231_alarms_t *interr){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");
    ESP_RETURN_ON_FALSE(interr != NULL, ESP_ERR_INVALID_ARG, TAG, "\"interr\" must not be NULL");
    
    uint8_t cntrl_addr = DS3231_ADDR_CNTRL;
    uint8_t cntrl_reg;

    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &cntrl_addr, 1, &cntrl_reg, 1, DS3231_TIMEOUT_MS),
                      TAG, "Failed to read control register for alarm interrupts");

    cntrl_reg &= (DS3231_ALARM1_INTERR_BITMASK | DS3231_ALARM2_INTERR_BITMASK);

    switch (cntrl_reg){
        case DS3231_ALARM_NONE:
        case DS3231_ALARM_A1:
        case DS3231_ALARM_A2:
        case DS3231_ALARM_A1_A2:
            *interr = (ds3231_alarms_t)cntrl_reg;
            break;
        default:
            return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t ds3231_alarm1_interr_enable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_ALARM1_INTERR_BITMASK, DS3231_ALARM1_INTERR_BITMASK);
}

esp_err_t ds3231_alarm1_interr_disable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_ALARM1_INTERR_BITMASK, 0);
}

esp_err_t ds3231_alarm1_get_time(ds3231_handle_t ds3231_handle, ds3231_alarm1_match_t *interr, struct tm *timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(timeinfo != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"timeinfo\" must not be NULL");
    ESP_GOTO_ON_FALSE(interr != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"interr\" must not be NULL");

    uint8_t data_buf_write = DS3231_ADDR_ALARM1_SEC;
    uint8_t data_buf_read[4];

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &data_buf_write, 1, data_buf_read, 4, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read alarm1 registers for time");


    uint8_t alarm_mask = 0;

    for (size_t i = 0; i < 4; ++i){
        alarm_mask |= ((data_buf_read[i] & DS3231_ALARM1_MASKBITS_BITMASK) >> (7 - i));
        data_buf_read[i] &= ~DS3231_ALARM1_MASKBITS_BITMASK;
    }

    if (!alarm_mask)
        alarm_mask |= ((data_buf_read[3] & (1 << 6)) >> 2);

    switch (alarm_mask){
        case DS3231_ALARM1_INTERR_PER_SEC:
        case DS3231_ALARM1_INTERR_MATCH_SEC:
        case DS3231_ALARM1_INTERR_MATCH_MINSEC:
        case DS3231_ALARM1_INTERR_MATCH_HOURMINSEC:
        case DS3231_ALARM1_INTERR_HOURMINSECDAY:
        case DS3231_ALARM1_INTERR_HOURMINSECDATE:
            *interr = (ds3231_alarm1_match_t)alarm_mask;
            break;
        default:
            return ESP_ERR_INVALID_RESPONSE;
    }
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

    s_convert_from_bcd(data_buf_read, sizeof(data_buf_read) - 1);

    timeinfo->tm_sec = data_buf_read[0];
    timeinfo->tm_min = data_buf_read[1];
    timeinfo->tm_hour = data_buf_read[2];

    if (data_buf_read[3] & DS3231_ALARM1_DY_DT_BITMASK){
        data_buf_read[3] &= ~DS3231_ALARM1_DY_DT_BITMASK;
        s_convert_from_bcd(&(data_buf_read[3]), 1);
        timeinfo->tm_wday = data_buf_read[3] - 1;
    }else{
        s_convert_from_bcd(&(data_buf_read[3]), 1);
        timeinfo->tm_mday = data_buf_read[3];
    }

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_alarm1_set_time(ds3231_handle_t ds3231_handle, ds3231_alarm1_match_t interr, struct tm timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE((is_pm == NULL && timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23)
                      || (is_pm != NULL && timeinfo.tm_hour >= 1 && timeinfo.tm_hour <= 12),
                      ESP_ERR_INVALID_ARG, err, TAG, "tm_hour is out of range for the selected time format");

    uint8_t data[5] = {0};

    data[0] = DS3231_ADDR_ALARM1_SEC;
    data[1] = s_convert_to_bcd(timeinfo.tm_sec);
    data[2] = s_convert_to_bcd(timeinfo.tm_min);

    if (is_pm == NULL)
        data[3] = s_convert_to_bcd(timeinfo.tm_hour);
    else if (*is_pm == true)
        data[3] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 5) | (1 << 6);
    else
        data[3] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 6);

    switch (interr) {
        case DS3231_ALARM1_INTERR_PER_SEC:
            for (size_t i = 4; i >= 1; --i)
                data[i] |= (1 << 7);
            break;
        case DS3231_ALARM1_INTERR_MATCH_SEC:
            for (size_t i = 4; i >= 2; --i)
                data[i] |= (1 << 7);
            break;
        case DS3231_ALARM1_INTERR_MATCH_MINSEC:
            for (size_t i = 4; i >= 3; --i)
                data[i] |= (1 << 7);
            break;
        case DS3231_ALARM1_INTERR_MATCH_HOURMINSEC:
            data[4] |= (1 << 7);
            break;
        case DS3231_ALARM1_INTERR_HOURMINSECDAY:
            data[4] = s_convert_to_bcd(timeinfo.tm_wday + 1);
            data[4] |= (1 << 6);
            break;
        case DS3231_ALARM1_INTERR_HOURMINSECDATE:
            data[4] = s_convert_to_bcd(timeinfo.tm_mday);
            data[4] &= ~(1 << 6);
            break;
        default:
            ESP_LOGE(TAG, "Invalid interr arg");
            return ESP_ERR_INVALID_ARG;
    }

    ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, data, 5, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to write alarm1 registers");

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_alarm2_get_time(ds3231_handle_t ds3231_handle, ds3231_alarm2_match_t *interr, struct tm *timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE(timeinfo != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"timeinfo\" must not be NULL");
    ESP_GOTO_ON_FALSE(interr != NULL, ESP_ERR_INVALID_ARG, err,
                      TAG, "\"interr\" must not be NULL");

    uint8_t data_buf_write = DS3231_ADDR_ALARM2_MIN;
    uint8_t data_buf_read[3];

    ESP_GOTO_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &data_buf_write, 1, data_buf_read, 3, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to read alarm2 registers for time");


    uint8_t alarm_mask = 0;

    for (size_t i = 0; i < 3; ++i){
        alarm_mask |= ((data_buf_read[i] & DS3231_ALARM1_MASKBITS_BITMASK) >> (7 - i));
        data_buf_read[i] &= ~DS3231_ALARM1_MASKBITS_BITMASK;
    }

    if (!alarm_mask)
        alarm_mask |= ((data_buf_read[2] & (1 << 6)) >> 2);

    switch (alarm_mask){
        case DS3231_ALARM2_INTERR_PER_MIN:
        case DS3231_ALARM2_INTERR_MATCH_MIN:
        case DS3231_ALARM2_INTERR_MATCH_HOURMIN:
        case DS3231_ALARM2_INTERR_MATCH_HOURMINDATE:
        case DS3231_ALARM2_INTERR_HOURMINDAY:
            *interr = (ds3231_alarm2_match_t)alarm_mask;
            break;
        default:
            return ESP_ERR_INVALID_RESPONSE;
    }
    // check the format of time
    if (data_buf_read[1] & (1 << 6)){
        ESP_GOTO_ON_FALSE(is_pm != NULL, ESP_ERR_INVALID_ARG, err, TAG, "is_pm must be not NULL in 12-hour mode");

        data_buf_read[1] &= ~(1 << 6);
        
        if (data_buf_read[1] & (1 << 5))
            *is_pm = true;
        else
            *is_pm = false;

        data_buf_read[1] &= ~(1 << 5);
    }

    s_convert_from_bcd(data_buf_read, sizeof(data_buf_read) - 1);

    timeinfo->tm_min = data_buf_read[0];
    timeinfo->tm_hour = data_buf_read[1];

    if (data_buf_read[2] & DS3231_ALARM2_DY_DT_BITMASK){
        data_buf_read[2] &= ~DS3231_ALARM2_DY_DT_BITMASK;
        s_convert_from_bcd(&(data_buf_read[2]), 1);
        timeinfo->tm_wday = data_buf_read[2] - 1;
    }else{
        s_convert_from_bcd(&(data_buf_read[2]), 1);
        timeinfo->tm_mday = data_buf_read[2];
    }

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_alarm2_set_time(ds3231_handle_t ds3231_handle, ds3231_alarm2_match_t interr, struct tm timeinfo, bool *is_pm){
    esp_err_t ret = ESP_OK;
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_GOTO_ON_ERROR(s_precond_checks(rtc), err, TAG, "Invalid or uninitialized device");
    ESP_GOTO_ON_FALSE((is_pm == NULL && timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23)
                      || (is_pm != NULL && timeinfo.tm_hour >= 1 && timeinfo.tm_hour <= 12),
                      ESP_ERR_INVALID_ARG, err, TAG, "tm_hour is out of range for the selected time format");

    uint8_t data[4] = {0};

    data[0] = DS3231_ADDR_ALARM2_MIN;
    data[1] = s_convert_to_bcd(timeinfo.tm_min);

    if (is_pm == NULL)
        data[2] = s_convert_to_bcd(timeinfo.tm_hour);
    else if (*is_pm == true)
        data[2] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 5) | (1 << 6);
    else
        data[2] = s_convert_to_bcd(timeinfo.tm_hour) | (1 << 6);

    switch (interr) {
        case DS3231_ALARM2_INTERR_PER_MIN:
            for (size_t i = 3; i >= 1; --i)
                data[i] |= (1 << 7);
            break;
        case DS3231_ALARM2_INTERR_MATCH_MIN:
            for (size_t i = 3; i >= 2; --i)
                data[i] |= (1 << 7);
            break;
        case DS3231_ALARM2_INTERR_MATCH_HOURMIN:
            data[3] |= (1 << 7);
            break;
        case DS3231_ALARM2_INTERR_HOURMINDAY:
            data[3] = s_convert_to_bcd(timeinfo.tm_wday + 1);
            data[3] |= (1 << 6);
            break;
        case DS3231_ALARM2_INTERR_MATCH_HOURMINDATE:
            data[3] = s_convert_to_bcd(timeinfo.tm_mday);
            data[3] &= ~(1 << 6);
            break;
        default:
            ESP_LOGE(TAG, "Invalid interr arg");
            return ESP_ERR_INVALID_ARG;
    }

    ESP_GOTO_ON_ERROR(i2c_master_transmit(rtc->i2c_handle, data, 4, DS3231_TIMEOUT_MS), err,
                      TAG, "Failed to write alarm2 registers");

    return ret;

    err:
        return ret;
}

esp_err_t ds3231_alarm2_interr_enable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_ALARM2_INTERR_BITMASK, DS3231_ALARM2_INTERR_BITMASK);
}

esp_err_t ds3231_alarm2_interr_disable(ds3231_handle_t ds3231_handle){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;

    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    return s_modify_reg(rtc, DS3231_ADDR_CNTRL, DS3231_ALARM2_INTERR_BITMASK, 0);
}

esp_err_t ds3231_alarm_get_flags(ds3231_handle_t ds3231_handle, ds3231_alarm_flags_t *interr){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_FALSE(interr != NULL, ESP_ERR_INVALID_ARG, TAG, "\"interr\" must not be NULL");
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");

    uint8_t status_addr = DS3231_ADDR_STAT;
    uint8_t status_read;

    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc->i2c_handle, &status_addr, 1, &status_read, 1, DS3231_TIMEOUT_MS),
                        TAG, "Failed to read status register");

    status_read &= DS3231_ALARM_FLAG_BITMASK;

    switch (status_read) {
        case DS3231_ALARM_NO_INTERR:
        case DS3231_ALARM1_FLAG_INTERR:
        case DS3231_ALARM2_FLAG_INTERR:
        case DS3231_ALARM1_ALARM2_FLAGS_INTERR:
            *interr = (ds3231_alarm_flags_t)status_read;
            return ESP_OK;

        default:
            return ESP_ERR_INVALID_RESPONSE;
    }
}

esp_err_t ds3231_alarm_clear_flags(ds3231_handle_t ds3231_handle, ds3231_alarm_flags_t interr){
    ds3231_dev_t *rtc = (ds3231_dev_t *)ds3231_handle;
    
    ESP_RETURN_ON_ERROR(s_precond_checks(rtc), TAG, "Invalid or uninitialized device");
    ESP_RETURN_ON_FALSE(!(interr & ~DS3231_ALARM_FLAG_BITMASK), ESP_ERR_INVALID_ARG,
                        TAG, "The interr out of range. See ds3231_alarm_flags_t for the allowed values");
                        
    return s_modify_reg(rtc, DS3231_ADDR_STAT, DS3231_ALARM_FLAG_BITMASK, ~interr & DS3231_ALARM_FLAG_BITMASK);
}
