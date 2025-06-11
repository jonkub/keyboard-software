#pragma once

#include <stdint.h>
#include <zephyr/device.h>

#include "hid.h"

namespace vpk::hid {
    using hid_callback_t = void(*)(int report_id, uint8_t *data, int len);

    int initialize(const device *dev);
    int create_report(uint8_t key, bool pressed);
    int clear_report();
};