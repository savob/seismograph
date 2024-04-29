#include <stdio.h>
#include "hardware/rtc.h"

// FatFS libraries
#include "hw_config.h"
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"

#include "logging.h"

static sd_card_t *pSD; // Current SD card
static FIL fil; // Current logging file
static uint64_t startLogTimeUS = 0;

int setupLogging() {
    time_init(); // Needed for FatFS library

    return 0;
}

int startLogFile() {
    pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);

    datetime_t startTime;
    rtc_get_datetime(&startTime);
    char filename[40];
    sprintf(filename, "%02d%02d%02d-%02d%02d%02d.csv", startTime.year % 100, startTime.month, startTime.day, 
            startTime.hour, startTime.min, startTime.sec);

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }

    // Add header for data
    if (f_printf(&fil, "Time (s),X (G),Y (G),Z (G)\n") < 0) {
        printf("f_printf failed for header\n");
        return 1;
    }

    // Only bother to mark the start of the log if file was successfully created
    startLogTimeUS = time_us_64();

    return 0;
}

int closeLogFile() {
    FRESULT fr = f_close(&fil);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    f_unmount(pSD->pcName);

    return fr;
}

int appendLogFile(float reads[]) {
    // Log data, takes approximately 350 us
    uint64_t elapsedTimeUS = time_us_64() - startLogTimeUS;
    int written = f_printf(&fil, "%.3f,%.3f,%.3f,%.3f\n", elapsedTimeUS / 1000000.0, reads[0], reads[1], reads[2]);

    // Return error codes, otherwise 0
    if (written < 0) return written;
    return 0;
}