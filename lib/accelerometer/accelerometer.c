#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "accelerometer.h"

const int AXES_PINS[] = {26, 27, 28}; // Axes input pins (X, Y, Z)

/**
 * \brief Sets up the accelerometer
 * 
 * \return Non-zero if error occured
 */
int setupAccelerometer() {
    adc_init();

    for (int i = 0; i < 3; i++) {
        adc_gpio_init(AXES_PINS[i]);
    }

    adc_set_round_robin(0x07);
    adc_select_input(0);

    return 0;
}

/**
 * \brief Reads the G forces on each axes to an array
 * 
 * \param axes Location to record acceleration of each axis (X, Y, Z)
 * \return Non-zero if error
 */
int readAccelerometer(float axes[]) {
    const float CONV_READ_TO_V = 3.3f / (1 << 12);
    const float CONV_V_TO_G = 1.0f / 0.430; // This is about 10% of supply rail

    for (int i = 0; i < 3; i++) {
        int channel = adc_get_selected_input();
        int reading = adc_read();

        // Need to offset from center range and convert
        axes[i] = (reading - (1 << 11)) * CONV_READ_TO_V * CONV_V_TO_G;

        printf("Reading channel %d: %4d (%5.3f V, %5.3f G)\n", channel, reading, axes[i] / CONV_V_TO_G, axes[i] );
    }

    return 0;
}