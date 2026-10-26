# esp-idf-ds3231

A minimalistic ESP-IDF driver for the RTC module DS3231, built on the i2c_master API. Future versions may add support for other DS3231 features. All features of this driver are tested on an ESP32-S3 board, including new ones that will be added in future releases.

## Features

- Read and set time with struct tm from the standard library time.h
- 12-hour and 24-hour formats.
- Two alarms with all match modes of the chip, alarm interupts
- Square wave output with selectable frequency
- 32 kHz output
- Read temperature and get/set aging
- Run and stop the oscillator when running on battery.
- Square wave output support.
- Uses an I2C bus handle provided by the application, so that it can be easily shared with other devices.
- Time and date registers are read and written with a single I2C transaction, so there are no rollover errors between seconds, minutes, and hours.

## Requirements
- ESP-IDF v5.3 or newer

## Installation
```sh
idf.py add-dependency "antonholovchenko35/ds3231"
```

## Notes
- Support 2000-2099 years only
- Fields `tm_yday` and `tm_isdst` of the `struct tm` are not filled
- `tm_hour` is always 0-23, in both formats. In 12-hour mode the driver does the AM/PM conversion
- The alarm hour format must match the current time format
- Alarm interrupts and the square wave share INT/SQW pin, so you should call `ds3231_squarewave_disable()` before using alarm interrupts
- Aftern an alarm fires, the INT pin stays low until the flag is cleared by the `ds3231_alarm_clear_flags()`
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
```

See [examples](examples/) for complete projects.

## Examples
- [basic_24h](examples/basic_24h) - sets and reads time in 24-hour format
- [basic_12h](examples/basic_12h) - sets and reads time in 12-hour format

## License
MIT, see LICENSE
