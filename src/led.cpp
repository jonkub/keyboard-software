#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include "led.h"

LOG_MODULE_DECLARE(vpk, CONFIG_LOG_DEFAULT_LEVEL);

namespace vpk::led {
    #define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
    #define STRIP_NODE		DT_ALIAS(led_strip)
    constexpr led_rgb colors[] = {
        {0, 159, 0, 0},
        {0, 0, 159, 0},
        {0, 0, 0, 159},
        {0, 79, 80, 0},
        {0, 53, 53, 53}
    };

    const device *dev = DEVICE_DT_GET(STRIP_NODE);
    led_rgb pixels[STRIP_NUM_PIXELS];
    bool enabled;
    k_work_delayable flash_work;

    inline int update() {
        int err;
        err = led_strip_update_rgb(dev, pixels, STRIP_NUM_PIXELS);
		if (err) {
			LOG_ERR("Error updating led strip: %d", err);
            return -1;
		}
        return 0;
    }

    void flash_work_handler(k_work *work) {
        update();
    }

    int initialize() {
        enabled = false;

        k_work_init_delayable(&flash_work, flash_work_handler);

        if (!device_is_ready(dev)) {
            LOG_ERR("LED strip device %s is not ready", dev->name);
            return -1;
        }
        return 0;
    }

    int enable() {
        if (enabled) {
            return 0;
        }
        enabled = true;
		memset(&pixels, 0x53, sizeof(pixels));
        return update();
    }

    int disable() {
        if (!enabled) {
            return 0;
        }
        enabled = false;
        memset(&pixels, 0x00, sizeof(pixels));
        return update();
    }

    int toggle() {
        return enabled ? disable() : enable();
    }

    int flash_once(int led_id, color_t color, uint8_t pulse_s) {
        int err;
        led_rgb tmp = pixels[led_id];
        pixels[led_id] = colors[(int)color];
        err = update();
        if (err) {
            return -1;
        }
        pixels[led_id] = tmp;
        err = k_work_reschedule(&flash_work, K_SECONDS(pulse_s));
        if (err < 0) {
            return -1;
        }
        return 0;
    }
};