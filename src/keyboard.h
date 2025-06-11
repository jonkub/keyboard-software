#pragma once
#include <zephyr/usb/class/usb_hid.h>
#include <array>

#include "hid.h"

namespace vpk::kbd {
	constexpr static size_t matrix_width = 15;
	constexpr static size_t matrix_height = 6;
	constexpr static size_t key_count = 84;

	using key_changed_callback_t = void(*)(uint8_t key, bool pressed);
	using keymap_t = std::array<uint8_t, matrix_width * matrix_height>;
	using keymap_raw_t = std::array<uint8_t, key_count>;

	int matrix_idx(uint32_t row, uint32_t col);
	int initialize(const device *dev, key_changed_callback_t new_callback);
	void load_keymap(const keymap_raw_t &new_keymap, bool fn_map = false);
};