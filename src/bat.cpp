#include "bat.h"
#include <zephyr/drivers/gpio.h>

namespace vpk::bat {
    constexpr const device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    constexpr gpio_dt_spec stdby_pin = {gpio0, 20};
    constexpr gpio_dt_spec chrg_pin = {gpio0, 22};
    constexpr gpio_dt_spec ilim_pin = {gpio0, 23};

    int initialize() {
        if (!device_is_ready(gpio0)) {
            return -1;
        }

        int err;
        err = gpio_pin_configure_dt(&stdby_pin, GPIO_INPUT | GPIO_PULL_UP);
        if (err) {
            return -1;
        }

        err = gpio_pin_configure_dt(&chrg_pin, GPIO_INPUT | GPIO_PULL_UP);
        if (err) {
            return -1;
        }

        err = gpio_pin_configure_dt(&ilim_pin, GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL);
        if (err) {
            return -1;
        }
        return 0;
    }

    status_t get_status() {
        int stdby = !gpio_pin_get_dt(&stdby_pin);
        int chrg = !gpio_pin_get_dt(&chrg_pin);

        if (stdby && !chrg) {
            return FULL;
        } else if (!stdby && chrg) {
            return CHARGING;
        }
        return NO_BATTERY;
    }

    int set_fastcharge(bool enable) {
        int err;
        err = gpio_pin_set_dt(&ilim_pin, enable);
        if (err) {
            return -1;
        }
        return 0;
    }
}