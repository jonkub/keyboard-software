#include <cstring>
#include <stdint.h>

#include "usb_hid.h"
#include "hid.h"
#include "bluetooth/hog.h"

namespace vpk::hid {
    uint8_t input_report[input_report_size];
    uint8_t consumer_report[2];
    const device *usb_hid_dev;

    atomic_t int_in_ready = ATOMIC_INIT(1);

    void int_in_ready_cb(const struct device *dev) {
        ARG_UNUSED(dev);
        atomic_set_bit(&int_in_ready, 0);
    }

    const hid_ops ops = {
        .int_in_ready = int_in_ready_cb
    };

    struct event_t {
        uint8_t key;
        bool pressed;
    };

    K_MSGQ_DEFINE(event_queue, sizeof(event_t), 8, 4);
    k_work_delayable work;
    constexpr int int_in_sleep_ms = 1;

    report_type_t generate_report(uint8_t key, bool pressed) {
        if (key >= hid::MEDIA_SKIP && key <= hid::MEDIA_MUTE) {
            if (pressed) {
                consumer_report[1] |= 0x1 << (key - hid::MEDIA_SKIP);
            } else {
                consumer_report[1] &= ~(0x1 << (key - hid::MEDIA_SKIP));
            }
            return CONSUMER_REPORT;
        }

        if (key >= hid::LCTRL) {
            if (pressed) {
                input_report[1] |= 0x1 << (key - hid::LCTRL);
            } else {
                input_report[1] &= ~(0x1 << (key - hid::LCTRL));
            }
        } else {
            if (pressed) {
                input_report[2 + (key >> 3)] |= 0x1 << (key & 0x7);
            } else {
                input_report[2 + (key >> 3)] &= ~(0x1 << (key & 0x7));
            }
        }
        return INPUT_REPORT;
    }

    void event_handler(k_work *_) {
        if (!atomic_test_and_clear_bit(&int_in_ready, 0)) {
            k_work_reschedule(&work, K_MSEC(int_in_sleep_ms));
            return;
        }

        event_t evt;
        if (k_msgq_get(&event_queue, &evt, K_NO_WAIT) != 0) {
            // No event left in queue
            return;
        }

        report_type_t type = generate_report(evt.key, evt.pressed);
        if (type == INPUT_REPORT) {
            hid_int_ep_write(usb_hid_dev, input_report, input_report_size, NULL);
        } else {
            hid_int_ep_write(usb_hid_dev, consumer_report, consumer_report_size, NULL);
        }

        // Might need to handle more events
        if (k_msgq_num_used_get(&event_queue) > 0) {
            k_work_reschedule(&work, K_MSEC(int_in_sleep_ms));
        }
    }

    int initialize(const device *dev) {
        int err;
        usb_hid_register_device(dev, report_descriptor, sizeof(report_descriptor), &ops);
        err = usb_hid_init(dev);
        if (err) {
            return -1;
        }
        usb_hid_dev = dev;
        k_work_init_delayable(&work, event_handler);

        memset(input_report, 0, sizeof(input_report));
        input_report[0] = INPUT_REPORT;
        memset(consumer_report, 0, sizeof(consumer_report));
        consumer_report[0] = CONSUMER_REPORT;

        return 0;
    }

    int create_report(uint8_t key, bool pressed) {
        event_t evt = { key, pressed };
        // Discard oldest event if queue is full
        if (k_msgq_num_free_get(&event_queue) <= 0) {
            event_t discard;
            k_msgq_get(&event_queue, &discard, K_NO_WAIT);
        }
        // Put new event in queue
        if (k_msgq_put(&event_queue, &evt, K_NO_WAIT) != 0) {
            return -1;
        }

        k_work_reschedule(&work, K_NO_WAIT);
        return 0;
    }

    int clear_report() {
        memset(input_report, 0, sizeof(input_report));
        input_report[0] = INPUT_REPORT;
        memset(consumer_report, 0, sizeof(consumer_report));
        consumer_report[0] = CONSUMER_REPORT;

        // Force both reports
        create_report(hid::A, false);
        create_report(hid::MEDIA_SKIP, false);
        return 0;
    }
};