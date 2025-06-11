#pragma once

namespace vpk::bat {
    enum status_t {
        NO_BATTERY,
        CHARGING,
        FULL
    };

    // Initialize bat driver
    int initialize();
    status_t get_status();
    int set_fastcharge(bool enable);
}