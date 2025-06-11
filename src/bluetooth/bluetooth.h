#pragma once

namespace vpk::bt {
    enum status_t {
        CONNECTED,
        AWAITING_PASSKEY,
        AUTHENTICATED,
        DISCONNECTED
    };

    using ble_callback_t = void(*)(status_t status);

    int initialize(ble_callback_t callback);
    int enable();
    int disable();
    int drop_connections();
    int enter_passkey(int passkey);
};