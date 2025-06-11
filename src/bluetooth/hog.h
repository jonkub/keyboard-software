#pragma once

#include <stdint.h>
#include "../hid.h"

namespace vpk::bt::hog {
    int create_report(uint8_t key, bool pressed);
    int clear_report();
};