
#include <zephyr/bluetooth/gatt.h>
#include "hog.h"

namespace vpk::bt::hog {
    struct hids_info {
        uint16_t version; /* version number of base USB HID Specification */
        uint8_t code; /* country HID Device hardware is localized for. */
        uint8_t flags; /* normally_connectable=0b10, remote_wake=0b01 */
    } __packed;

    struct hids_report {
        uint8_t id; /* report id */
        uint8_t type; /* report type, 1 is input, 2 is output, 3 is feature */
    } __packed;

    hids_info info = { 0x0000, 0x00, 0b10 };

    hids_report input_ref = { vpk::hid::INPUT_REPORT, 0x01 };
    uint8_t input_report[vpk::hid::input_report_size - 1];

    hids_report consumer_ref = { vpk::hid::CONSUMER_REPORT, 0x01 };
    uint8_t consumer_report[vpk::hid::consumer_report_size - 1];

    uint8_t input_ccc;
    uint8_t ctrl_point;

    ssize_t read_info(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(hids_info));
    }

    ssize_t read_report_map(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, vpk::hid::report_descriptor, sizeof(vpk::hid::report_descriptor));
    }

    ssize_t read_report_ref(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(hids_report));
    }

    void input_ccc_changed(const bt_gatt_attr *attr, uint16_t value) {
        input_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
    }

    ssize_t read_input_report(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, input_report, sizeof(input_report));
    }

    ssize_t read_consumer_report(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, consumer_report, sizeof(consumer_report));
    }

    ssize_t write_ctrl_point(bt_conn *conn, const bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
        uint8_t *value = (uint8_t*)attr->user_data;

        if (offset + len > sizeof(ctrl_point)) {
            return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }
        memcpy(value + offset, buf, len);
        return len;
    }

    /* HID Service Declaration */
    BT_GATT_SERVICE_DEFINE(hog_svc,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
        BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_info, NULL, &info),
        BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_report_map, NULL, NULL),

        /* Input Reports */
        BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_AUTHEN,
                        read_input_report, NULL, NULL),
        BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN),
        BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report_ref, NULL, &input_ref),

        /* Consumer Reports*/
        BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_AUTHEN, 
                        read_consumer_report, NULL, NULL),
        BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN),
        BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report_ref, NULL, &consumer_ref),

        BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, 
                        write_ctrl_point, &ctrl_point),
    );


    hid::report_type_t generate_report(uint8_t key, bool pressed) {
        if (key >= hid::MEDIA_SKIP && key <= hid::MEDIA_MUTE) {
            if (pressed) {
                consumer_report[0] |= 0x1 << (key - hid::MEDIA_SKIP);
            } else {
                consumer_report[0] &= ~(0x1 << (key - hid::MEDIA_SKIP));
            }
            return hid::CONSUMER_REPORT;
        }

        if (key >= hid::LCTRL) {
            if (pressed) {
                input_report[0] |= 0x1 << (key - hid::LCTRL);
            } else {
                input_report[0] &= ~(0x1 << (key - hid::LCTRL));
            }
        } else {
            if (pressed) {
                input_report[1 + (key >> 3)] |= 0x1 << (key & 0x7);
            } else {
                input_report[1 + (key >> 3)] &= ~(0x1 << (key & 0x7));
            }
        }
        return hid::INPUT_REPORT;
    }

    int create_report(uint8_t key, bool pressed) {
        if (!input_ccc) {
            return 0;
        }
        hid::report_type_t type = generate_report(key, pressed);
        int err;
        if (type == vpk::hid::INPUT_REPORT) {
	        err = bt_gatt_notify(NULL, &hog_svc.attrs[5], input_report, sizeof(input_report));
            if (err) {
                return -1;
            }
        } else {
	        err = bt_gatt_notify(NULL, &hog_svc.attrs[10], consumer_report, sizeof(consumer_report));
            if (err) {
                return -1;
            }
        }
        return 0;
    }

    int clear_report() {
        memset(input_report, 0, sizeof(input_report));
        memset(consumer_report, 0, sizeof(consumer_report));

        // Force both reports
        int err;
        err = bt_gatt_notify(NULL, &hog_svc.attrs[5], input_report, sizeof(input_report));
        if (err) {
            return -1;
        }
        err = bt_gatt_notify(NULL, &hog_svc.attrs[10], consumer_report, sizeof(consumer_report));
        if (err) {
            return -1;
        }
        return 0;
    }
}