#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "bluetooth.h"

LOG_MODULE_DECLARE(vpk, CONFIG_LOG_DEFAULT_LEVEL);

namespace vpk::bt {
    const bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL), BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
    };

    const bt_le_adv_param params[] = { 
        BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, 
        BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL) 
    };

    bool enabled;
    bt_conn *active_connection;
    ble_callback_t callback;

    void connected(bt_conn *conn, uint8_t err) {
        if (err) {
            LOG_ERR("Failed to connect with err(%u)", err);
            return;
        }
        active_connection = conn;
        callback(CONNECTED);

        if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
            LOG_ERR("Failed to set security");
        }
    }

    void disconnected(bt_conn *conn, uint8_t reason) {
        active_connection = NULL;
        callback(DISCONNECTED);
    }

    void security_changed(bt_conn *conn, bt_security_t level, bt_security_err err) {
        if (err) {
            LOG_ERR("Changing security failed: level %u err %d", level, err);
        }
        if (level >= 2) {
            callback(AUTHENTICATED);
        }
    }

    void auth_passkey_display(bt_conn *conn, unsigned int passkey) {
        LOG_WRN("Passkey entry required on client: %06u", passkey);
    }

    void auth_cancel(bt_conn *conn) {
        LOG_INF("Pairing cancelled");
    }

    void auth_passkey_entry (bt_conn *conn) {
        LOG_INF("Passkey entry required via keyboard");
        callback(AWAITING_PASSKEY);
    }

    bt_conn_auth_cb auth_cb_display = {
        .passkey_display = NULL,
        .passkey_entry = auth_passkey_entry,
        .passkey_confirm = NULL,
        .cancel = auth_cancel
    };

    void bt_ready(int err) {
        if (err) {
            LOG_ERR("Bluetooth init failed (err %d)", err);
            return;
        }
        LOG_INF("Bluetooth initialized\n");

        if (IS_ENABLED(CONFIG_SETTINGS)) {
            settings_load();
        }
    }

    int initialize(ble_callback_t new_callback) {
        int err;
        enabled = false;
        err = bt_enable(bt_ready);
        if (err) {
            LOG_ERR("Bluetooth init failed (err %d)", err);
            return -1;
        }

	    bt_conn_auth_cb_register(&auth_cb_display);

        callback = new_callback;
        return 0;
    }

    int enable() {
        if (enabled) {
            return 0;
        }

        int err;
        err = bt_le_adv_start(params, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Advertising failed to start (err %d)", err);
            return -1;
        }
        enabled = true;
        LOG_INF("Advertising started");
        return 0;
    }

    int disable() {
        if (!enabled) {
            return 0;
        }

        int err;
        err = bt_le_adv_stop();
        if (err) {
            LOG_ERR("Advertising failed to stop (err %d)", err);
            return -1;
        }
        enabled = false;
        LOG_INF("Advertising started");
        return 0;
    }

    int drop_connections() {
        if (!enabled) {
            return 0;
        }

        int err;
        if (active_connection) {
            err = bt_conn_disconnect(active_connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            if (err) {
                LOG_ERR("Failed to drop connection (err %d)", err);
                return -1;
            }
        }
        err = bt_unpair(BT_ID_DEFAULT, NULL);
        if (err) {
            LOG_ERR("Failed to unpair (err %d)", err);
            return -1;
        }
        return 0;
    }

    int enter_passkey(int passkey) {
        if (!active_connection) {
            LOG_WRN("Passkey entry attempted but not required");
        }
        int err;
        err = bt_conn_auth_passkey_entry(active_connection, passkey);
        if (err) {
            LOG_ERR("Error entering passkey: %d", err);
        }
        return 0;
    }
};


BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = vpk::bt::connected,
    .disconnected = vpk::bt::disconnected,
    .security_changed = vpk::bt::security_changed,
};