#pragma once

namespace vpk::led {
    enum color_t {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
        YELLOW = 3,
        WHITE = 4
    };

    // Initialize led driver and load pattern from flash
    int initialize();

    int enable();
    int disable();
    int toggle();
    int flash_once(int led_id, color_t color, uint8_t pulse_s = 1);
}