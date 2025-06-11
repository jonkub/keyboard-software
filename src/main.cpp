// Common includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>

#include <zephyr/sys/reboot.h>

#include "keyboard.h"
#include "usb_hid.h"
#include "bluetooth/hog.h"
#include "bluetooth/bluetooth.h"
#include "led.h"
#include "bat.h"
LOG_MODULE_REGISTER(vpk);

#ifdef INTELLISENSE
#define Z_FOR_LOOP_1(...) 0
#endif

constexpr int bat_led = 0;
constexpr int ble_led = 2;

bool require_passkey;
bool usb_connected;
bool do_ble_reports;
int passkey;
uint8_t passkey_pos;
k_work_delayable usb_timeout_work;

void usb_timeout_check(k_work *work) {
	if (!usb_connected) {
		vpk::bt::enable();
		vpk::led::flash_once(ble_led, vpk::led::BLUE);
	}
}

void handle_special_key(uint8_t key) {
	switch (key) {
	case vpk::hid::FNC_RESET:
		sys_reboot(SYS_REBOOT_WARM);
		break;
	case vpk::hid::FNC_BAT_STATUS:
	{
		vpk::bat::status_t status = vpk::bat::get_status();
		vpk::led::color_t color = (status == vpk::bat::NO_BATTERY) ? vpk::led::RED :
									((status == vpk::bat::NO_BATTERY) ? vpk::led::GREEN : vpk::led::YELLOW); 
		vpk::led::flash_once(bat_led, color);
		break;
	}
	case vpk::hid::FNC_BLE_ON:
		vpk::bt::enable();
		vpk::led::flash_once(ble_led, vpk::led::BLUE);
		break;
	case vpk::hid::FNC_BLE_OFF:
		vpk::bt::disable();
		vpk::led::flash_once(ble_led, vpk::led::RED);
		break;
	case vpk::hid::FNC_BLE_DIS:
		vpk::bt::drop_connections();
		vpk::led::flash_once(ble_led, vpk::led::RED, 3);
		break;
	case vpk::hid::FNC_LED_ON:
		vpk::led::enable();
		break;
	case vpk::hid::FNC_LED_OFF:
		vpk::led::disable();
		break;
	case vpk::hid::FNC_LED_TOG:
		vpk::led::toggle();
		break;
	default:
		LOG_ERR("Invalid special key");
		break;
	}
}

void kbd_callback(uint8_t key, bool pressed) {
	if (key < vpk::hid::FN) {
		if (usb_connected) {
			vpk::hid::create_report(key, pressed);
		}
		if (do_ble_reports) {
			vpk::bt::hog::create_report(key, pressed);
		} else if (require_passkey && pressed) {
			if (key >= vpk::hid::N1 && key <= vpk::hid::N0) {
				int value = (key >= vpk::hid::N0) ? 0 : (key - vpk::hid::N1 + 1);
				passkey = passkey * 10 + value;
				if (++passkey_pos >= 6) {
					vpk::bt::enter_passkey(passkey);
					require_passkey = false;
				}
			}
		}
	} else if (key == vpk::hid::FN) {
		if (usb_connected) {
			vpk::hid::clear_report();
		}
		if (do_ble_reports) {
			vpk::bt::hog::clear_report();
		}
	} else if (pressed) {
		handle_special_key(key);
	}
}

void usb_callback(usb_dc_status_code cb_status, const uint8_t *param) {
	if (cb_status == USB_DC_CONFIGURED) {
		do_ble_reports = false;
		vpk::bt::disable();
		vpk::led::flash_once(ble_led, vpk::led::RED);
		usb_connected = true;
	} else {
		usb_connected = false;
	}
}

void ble_callback(vpk::bt::status_t evt) {
	if (evt == vpk::bt::status_t::AWAITING_PASSKEY) {
		vpk::led::flash_once(ble_led, vpk::led::YELLOW, 3);
		require_passkey = true;
		passkey = 0;
		passkey_pos = 0;
	} else {
		require_passkey = false;
	}
	if (evt == vpk::bt::status_t::AUTHENTICATED) {
		do_ble_reports = true;
		vpk::led::flash_once(ble_led, vpk::led::GREEN);
	} else {
		do_ble_reports = false;
	}
}

int main() {
	int err;
	require_passkey = false;
	usb_connected = false;
	do_ble_reports = false;

	k_work_init_delayable(&usb_timeout_work, usb_timeout_check);

    const device *hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Failed to get HID device");
		return -1;
	}

    const device *kscan_dev = device_get_binding("KSCAN");
	if (kscan_dev == NULL) {
		LOG_ERR("Failed to get KSCAN device");
		return -1;
	}
	
	err = vpk::bat::initialize();
	if (err) {
		LOG_ERR("Falied to init battery control");
		return -1;
	}

	err = vpk::led::initialize();
	if (err) {
		LOG_ERR("Falied to init LEDs");
		return -1;
	}

	err = vpk::bt::initialize(ble_callback);
	if (err) {
		LOG_ERR("Failed to initialize BLE");
	}

	err = vpk::hid::initialize(hid_dev);
	if (err) {
		LOG_ERR("Failed to initialize HID");
		return -1;
	}
	
	err = usb_enable(usb_callback);
	if (err != 0) {
		LOG_ERR("Failed to enable USB");
		return -1;
	}

	err = vpk::kbd::initialize(kscan_dev, kbd_callback);
	if (err != 0) {
		LOG_ERR("Failed to initialize keyboard matrix");
		return -1;
	}

	k_work_reschedule(&usb_timeout_work, K_SECONDS(5));
}