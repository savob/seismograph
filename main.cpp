#include <stdio.h>
//
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
//
#include "hw_config.h"
#include <tusb.h>

int main() {
    stdio_init_all();
    time_init();

    // Wait for USB connection
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;

    const char* const filename = "filename.txt";

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);

    if (f_printf(&fil, "Hello, world!%d\n", get_absolute_time()._private_us_since_boot) < 0) {
        printf("f_printf failed\n");
    }

    fr = f_close(&fil);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    f_unmount(pSD->pcName);

    for (;;);
}
