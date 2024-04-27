#include <stdio.h>
// FatFS libraries
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
// NTP Libraries
#include "hw_config.h"
#include "picow_ntp_client.h"
#include <tusb.h>
// RTC Libraries
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

int main() {
    stdio_init_all();
    time_init(); // Needed for FatFS library

    // Wait for USB connection
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }

    setupNTP();
    
    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;

    const char* const filename = "filename.txt";

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);

    if (f_printf(&fil, "Hello, world!%d\n", to_us_since_boot(get_absolute_time())) < 0) {
        printf("f_printf failed\n");
    }

    fr = f_close(&fil);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    f_unmount(pSD->pcName);


    datetime_t t;
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];

    while (1) {
        checkNTP(); 
        if (rtc_get_datetime(&t) == true) {
            datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
            printf("%s\n", datetime_str);
            sleep_ms(2000);
        }
    }

    closeNTP();
}
