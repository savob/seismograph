#include <stdio.h>
// NTP Libraries
#include "picow_ntp_client.h"
#include <tusb.h>
// RTC Libraries
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
// Accelerometer
#include "accelerometer.h"
// Logging
#include "logging.h"

const int32_t LOGGING_PERIOD_MS = 500; // Period between accelerometer logs

bool loggingCallback(struct repeating_timer *t) {
    checkNTP();

    float readings[3];
    float sum = readAccelerometer(readings);
    printf("Magnitude of acceleration is %5.3f G\n", sum);

    appendLogFile(readings);

    return true;
}

int main() {
    stdio_init_all();
    setupAccelerometer();
    setupLogging();

    // Wait for USB connection
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }

    // Wait until we get time from NTP is setup was good
    if (setupNTP() == 0) {
        datetime_t tempTime;
        do {
            checkNTP();
            rtc_get_datetime(&tempTime);
            sleep_ms(100);
        } while (tempTime.year <= 2023);
    }

    startLogFile();

    // Setup periodic logging 
    struct repeating_timer loggingTimer;
    add_repeating_timer_ms(LOGGING_PERIOD_MS, loggingCallback, NULL, &loggingTimer);

    // Wait for a bit
    sleep_ms(5000);

    cancel_repeating_timer(&loggingTimer);
    
    closeLogFile();

    closeNTP();

    printf("\n\nALL DONE. SHUTTING DOWN.");
    sleep_ms(1000);
}
