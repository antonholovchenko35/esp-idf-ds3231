#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "ds3231.h"

#define I2C_SDA_GPIO    GPIO_NUM_6
#define I2C_SCL_GPIO    GPIO_NUM_7

static const char *TAG = "ds3231_24h";

void app_main(void){
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true // set to false if you have external pull-up resistors
    };

    i2c_master_bus_handle_t i2c_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));

    ds3231_handle_t rtc;
    ESP_ERROR_CHECK(ds3231_create(i2c_bus, &rtc));

    bool time_valid;
    ds3231_time_format_t timeformat;

    ESP_ERROR_CHECK(ds3231_get_time_valid(rtc, &time_valid));
    ESP_ERROR_CHECK(ds3231_get_time_format(rtc, &timeformat));

    if (!time_valid || timeformat != DS3231_TIME_FORMAT_24H){
        // 2026-01-01 23:58:00, Thursday
        struct tm timeinfo = {
            .tm_year = 2026 - 1900,
            .tm_mon = 0,
            .tm_mday = 1,
            .tm_hour = 23,
            .tm_min = 58,
            .tm_sec = 0,
            .tm_wday = 4
        };

        ESP_ERROR_CHECK(ds3231_set_time(rtc, timeinfo, DS3231_TIME_FORMAT_24H));
        ESP_LOGI(TAG, "Time was invalid, set to default");
    }

    struct tm timeinfo = {0};

    while (1){
        ESP_ERROR_CHECK(ds3231_get_time(rtc, &timeinfo, NULL));

        ESP_LOGI(TAG, "%04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
