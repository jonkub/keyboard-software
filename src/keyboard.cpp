#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>

#include "keyboard.h"

LOG_MODULE_DECLARE(vpk, CONFIG_LOG_DEFAULT_LEVEL);

namespace vpk::kbd {
    struct event_t {
        uint8_t key;
        bool pressed;
    };

    K_MSGQ_DEFINE(event_queue, sizeof(event_t), 8, 4);
    k_work work;

	key_changed_callback_t callback;
	keymap_t keymap;
    keymap_t keymap_fn;
    keymap_t *active_keymap;
    
    void kscan_callback(const device *dev, uint32_t row, uint32_t col, bool pressed) {
        event_t evt;
        evt.key = (*active_keymap)[matrix_idx(row, col)];
        evt.pressed = pressed;
        k_msgq_put(&event_queue, &evt, K_NO_WAIT);
        k_work_submit(&work);
    }

    void process_keys(k_work *work) {
        event_t evt;
        while (k_msgq_get(&event_queue, &evt, K_NO_WAIT) == 0) {
            if (evt.key == vpk::hid::FN) {
                active_keymap = evt.pressed ? &keymap_fn : &keymap;
            }
            callback(evt.key, evt.pressed);
        }
    }

    int matrix_idx(uint32_t row, uint32_t col) {
        return row * matrix_width + col;
    }

    int initialize(const device *dev, key_changed_callback_t new_callback) {
        int err;
        err = kscan_config(dev, kscan_callback);
        if (err) {
            LOG_ERR("Could not configure KSCAN matrix");
            return -1;
        }
        k_work_init(&work, process_keys);

        load_keymap(hid::keymap_qwerty);
        load_keymap(hid::keymap_fn, true);
        active_keymap = &keymap;
        callback = new_callback;
        kscan_enable_callback(dev);
        return 0;
    }
    
    void load_keymap(const keymap_raw_t &new_keymap, bool fn_map) {
        auto it = fn_map ? keymap_fn.begin() : keymap.begin();
        auto inIt = new_keymap.begin();

        it = std::copy(inIt, inIt + 15, it);
        it = std::copy(inIt + 16, inIt + 30, it);
        *(it++) = new_keymap[15];
        it = std::copy(inIt + 31, inIt + 45, it);
        *(it++) = new_keymap[30];
        it = std::copy(inIt + 46, inIt + 60, it);
        *(it++) = new_keymap[45];
        it = std::copy(inIt + 60, inIt + 74, it);
        *(it++) = 0;
        it = std::copy(inIt + 74, inIt + 77, it);
        *(it++) = 0;
        *(it++) = 0;
        *(it++) = *(inIt + 77);
        *(it++) = 0;
        *(it++) = 0;
        it = std::copy(inIt + 78, inIt + 84, it);
        *it = 0;
    }
};