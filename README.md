# esp-idf-ds3231

A minimalistic ESP-IDF driver for the RTC module DS3231, built on the `i2c_master` API. Support for other features of the DS3231 may be added in the future versions. 

## Features
- Read and set time with `struct tm` from the standard library `time.h`
- 12-hours and 24-hours format
- Run and stop of the oscillator when running on battery
- Only you own I2C bus, so it can be shared with other devices
- Time and data registers are read and written with a single I2C transaction, so there are no rollover errors between seconds, and hours

## Requirements
- ESP-IDF v5.3 or newer

## Installation
```sh
idf.py add-dependency "antonholovchenko35/ds3231"
```

## Notes
- Support 2000-2099 years only
- Fields `tm_yday` and `tm_isdst` of the `struct tm` are not filled
- 12-hour mode is in the range 1-12 only
- set time also sets the format of saved time and turn off the OSF bit
- OSF flag is not reliable without battery
- The I2C address 0x68 and bus speed 100 kHz are fixed

## Usage
```c
#include "ds3231.h"

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
    ESP_ERROR_CHECK(ds3231_set_time(rtc, timeinfo, NULL));
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
```

See [examples](examples/) for complete projects.

## Examples
- [basic_24h](examples/basic_24h) - sets and reads time in 24-hour format
- [basic_12h](examples/basic_12h) - sets and reads time in 12-hour format

## License
MIT, see LICENSE
