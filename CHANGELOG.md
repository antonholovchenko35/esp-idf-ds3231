# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.0] - 2026-11-11

### Added
- `ds3231_get_time()` fills `tm_yday` now (day of the year, 0..365).
- `ds3231_get_time()` fills `tm_isdst` now. It's always set to 0, because the driver treats
  the time stored in the RTC as UTC.

## [0.4.0] - 2026-10-26

### Added
- Alarm 1 and alarm 2 support: `ds3231_alarm1_set_time()`, `ds3231_alarm1_get_time()`,
  `ds3231_alarm2_set_time()`, `ds3231_alarm2_get_time()` with all match modes from the datasheet.
- Alarm interrupts: `ds3231_alarm1_interr_enable()`, `ds3231_alarm1_interr_disable()`,
  `ds3231_alarm2_interr_enable()`, `ds3231_alarm2_interr_disable()`, `ds3231_alarm_interr_get_status()`.
- Alarm flags: `ds3231_alarm_get_flags()`, `ds3231_alarm_clear_flags()`.
- Range checks for all `struct tm` fields in `ds3231_set_time()`.
- Missing NULL checks for output arguments.

### Changed
- **BREAKING:** `ds3231_set_time()` and `ds3231_get_time()` take `ds3231_time_format_t`
  instead of `bool *is_pm`. `tm_hour` is now always 0-23; AM/PM conversion is done by the driver.

### Fixed
- `ds3231_set_time()` and the 32 kHz output functions no longer clear alarm flags
  that are set while the status register is being updated.

## [0.3.0] - 2026-10-20

### Added
- Square wave output: `ds3231_squarewave_set_freq()`, `ds3231_squarewave_get_freq()`,
  `ds3231_squarewave_enable()`, `ds3231_squarewave_disable()`.
- Square wave on battery: `ds3231_squarewave_battery_enable()`, `ds3231_squarewave_battery_disable()`.
- 32 kHz output: `ds3231_squarewave_32khz_enable()`, `ds3231_squarewave_32khz_disable()`.

## [0.2.0] - 2026-10-19

### Added
- `ds3231_get_temperature()`.
- `ds3231_get_aging()` and `ds3231_set_aging()`.

## [0.1.1] - 2026-10-18

### Added
- README.
- GitHub Actions workflow that uploads the component to the ESP Component Registry on tag.

### Fixed
- NULL checks for arguments and range check for `tm_hour`.

## [0.1.0] - 2026-10-18

### Added
- First release: read and set time, 12/24-hour format, oscillator stop flag,
  oscillator control on battery.
- Examples `basic_12h` and `basic_24h`.

[Unreleased]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/antonholovchenko35/esp-idf-ds3231/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/antonholovchenko35/esp-idf-ds3231/releases/tag/v0.1.0
