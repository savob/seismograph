# Seismograph

Basic seismograph data logger to an SD card. Designed around an [ADXL355](https://www.analog.com/en/products/adxl335.html) analog accelerometer IC connected to an RPi Pico W. Makes use of NTP to syncronize the the real time clock inside the Pico automatically for accurate time stamps with online clocks.

This project is built using the Raspberry Pi Pico SDK.

# Setting Up NTP

The NTP code requires a minor bit of configuration to work, just the WiFi credentials and offset from UTC for your location. 

The configuration for your WiFi network resides in `lib/picow_ntp_client/wifi_config.h`, simply update the values for `SSID_NAME` and `WIFI_PASSWORD`. *Be careful not to accidentally include these updated values in any commits!* 

Since NTP exchanges time in UTC, you will likely need to add an offset to this value to put the time into your local timezone. There is a constant used to do this: `TIME_OFFSET` defined in `lib/picow_ntp_client/picow_ntp_client.c`. This should be set to your offset from UTC in seconds.

### Ignoring Network Credential Changes

It is annoying to have to manually ignore changes to the `wifi_config.h` file so you don't accidentally include them in a commit - even more annoying to have to retract such a commit. To make this easier you can make git ignore the file by using the following command from the root of this repository.

```bash
git update-index --skip-worktree lib/picow_ntp_client/wifi_config.h
```

# Library Dependancies

Using an SD card library from [carlk3](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico). Should be able to automatically download the latest version using the following commands in the root of this repository.

```bash
git submodule init
git submodule update
```
