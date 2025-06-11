#pragma once

#include <array>
#include <zephyr/usb/class/usb_hid.h>

namespace vpk::hid {
    enum report_type_t {
        INPUT_REPORT = 1,
        CONSUMER_REPORT = 2
    };

    constexpr size_t input_report_size = 16;
    constexpr size_t consumer_report_size = 2;
    
	constexpr uint8_t report_descriptor[] = {				
		HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),			
		HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),		
		HID_COLLECTION(HID_COLLECTION_APPLICATION),
            HID_REPORT_ID((uint8_t)INPUT_REPORT),
			/* 1 Byte for Control Keys */	
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_KEYPAD),	
			HID_USAGE_MIN8(0xE0),	/* HID_USAGE_MINIMUM(Keyboard LeftControl) */			
			HID_USAGE_MAX8(0xE7),	/* HID_USAGE_MAXIMUM(Keyboard Right GUI) */			
			HID_LOGICAL_MIN8(0),				
			HID_LOGICAL_MAX8(1),				
			HID_REPORT_SIZE(1),				
			HID_REPORT_COUNT(8),				
			HID_INPUT(0x02),	/* HID_INPUT(Data,Var,Abs) */			
            /* 16 Bytes for Normal Keys */
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_KEYPAD),	
			HID_USAGE_MIN8(0),	
			HID_USAGE_MAX8((input_report_size - 2) * 8 - 1),	
			HID_LOGICAL_MIN8(0),				
			HID_LOGICAL_MAX8(1),				
			HID_REPORT_SIZE(1),				
			HID_REPORT_COUNT((input_report_size - 2) * 8),				
			HID_INPUT(0x02),	/* HID_INPUT (Data,Var,Abs) */			
		HID_END_COLLECTION,
        /* Media key report */
        HID_USAGE_PAGE(0x0c),   /* usage consumer devices */
        HID_USAGE(0x01),        /* usage consumer control */
        HID_COLLECTION(HID_COLLECTION_APPLICATION),
            HID_REPORT_ID((uint8_t)CONSUMER_REPORT),
            HID_USAGE(0xb5),    /* scan next track */
            HID_USAGE(0xb6),    /* scan prev track*/
            HID_USAGE(0xb7),    /* stop */
            HID_USAGE(0xcd),    /* play/pause */
            HID_USAGE(0xe9),    /* volume up */
            HID_USAGE(0xea),    /* volume down */
            HID_USAGE(0xe2),    /* mute */
            HID_LOGICAL_MIN8(0),
            HID_LOGICAL_MAX8(1),
            HID_REPORT_SIZE(1),
            HID_REPORT_COUNT(7),
            HID_INPUT(0x02),    /* HID_INPUT(Data,Var,Abs) */
            HID_REPORT_COUNT(1),
            HID_INPUT(0x01),    /* HID_INPUT(Cnst,Arr,Abs) */
        HID_END_COLLECTION
	};

    enum keycode {
        // Normal Keys
        NONE        = 0,
        ERR_OVF     = 1, // overflow (too many keys pressed)
        ERR_POST    = 2, // keyboard POST fail
        ERR_UNKNOWN = 3, // unknown keyboard error
        A		    = 4,
        B		    = 5,
        C		    = 6,
        D		    = 7,
        E		    = 8,
        F		    = 9,
        G		    = 10,
        H		    = 11,
        I		    = 12,
        J		    = 13,
        K		    = 14,
        L		    = 15,
        M		    = 16,
        N		    = 17,
        O		    = 18,
        P		    = 19,
        Q		    = 20,
        R		    = 21,
        S		    = 22,
        T		    = 23,
        U		    = 24,
        V		    = 25,
        W		    = 26,
        X		    = 27,
        Y		    = 28,
        Z		    = 29,
        N1		    = 30,
        N2		    = 31,
        N3		    = 32,
        N4		    = 33,
        N5		    = 34,
        N6		    = 35,
        N7		    = 36,
        N8		    = 37,
        N9		    = 38,
        N0		    = 39,
        ENTER		= 40,
        ESC		    = 41,
        BACKSPACE	= 42,
        TAB		    = 43,
        SPACE		= 44,
        MINUS		= 45,
        EQUAL		= 46,
        LEFTBRACE	= 47,
        RIGHTBRACE	= 48,
        BACKSLASH	= 49,
        HASH		= 50, /* Non-US # and ~ */
        SEMICOLON	= 51,
        APOSTROPHE	= 52,
        GRAVE		= 53,
        COMMA		= 54,
        DOT		    = 55,
        SLASH		= 56,
        CAPSLOCK	= 57,
        F1		    = 58,
        F2		    = 59,
        F3		    = 60,
        F4		    = 61,
        F5		    = 62,
        F6		    = 63,
        F7		    = 64,
        F8		    = 65,
        F9		    = 66,
        F10		    = 67,
        F11		    = 68,
        F12		    = 69,
        SYSRQ		= 70, /* PRINTSCREEN */
        SCROLLLOCK	= 71,
        PAUSE		= 72,
        INSERT		= 73,
        HOME		= 74,
        PAGEUP		= 75,
        DELETE		= 76,
        END		    = 77,
        PAGEDOWN	= 78,
        RIGHT		= 79,
        LEFT		= 80,
        DOWN		= 81,
        UP		    = 82,
        NUMLOCK		= 83,
        KPSLASH		= 84, /* NUMPAD DIVIDE */
        KPASTERISK	= 85, /* NUMPAD MULTIPLY */
        KPMINUS		= 86,
        KPPLUS		= 87,
        KPENTER		= 88,
        KP_1		= 89,
        KP_2		= 90,
        KP_3		= 91,
        KP_4		= 92,
        KP_5		= 93,
        KP_6		= 94,
        KP_7		= 95,
        KP_8		= 96,
        KP_9		= 97,
        KP_0		= 98,
        KP_DOT      = 99,
        VBAR        = 100, // Keyboard Non-US \ and |
        COMPOSE     = 101,
        POWER       = 102,
        KP_EQUAL    = 103,
        F13         = 104,
        F14         = 105,
        F15         = 106,
        F16         = 107,
        F17         = 108,
        F18         = 109,
        F19         = 110,
        F20         = 111,

        // Media Keys
        MEDIA_SKIP,
        MEDIA_PREV,
        MEDIA_STOP,
        MEDIA_PLAY,
        MEDIA_VUP,
        MEDIA_VDOWN,
        MEDIA_MUTE,

        // Modifiers
        LCTRL      = 0xe0,
        LSHIFT      = 0xe1,
        LALT        = 0xe2,
        LMETA       = 0xe3,
        RCTRL       = 0xe4,
        RSHIFT      = 0xe5,
        RALT        = 0xe6,
        RMETA       = 0xe7,

        // FN KEY
        FN,

        // Special Keys
        FNC_RESET = 247,    // Reset MCU
        FNC_BAT_STATUS,     // Indicate battery status
        FNC_LED_ON,         // Turn on leds
        FNC_LED_OFF,        // Turn off leds
        FNC_LED_TOG,        // Toggle leds on/off
        FNC_BLE_ON,         // Turn on bluetooth
        FNC_BLE_OFF,        // Turn off bluetooth
        FNC_BLE_DIS,        // Drop current connection
    };

    constexpr std::array<uint8_t, 84> keymap_qwerty = { 
            ESC, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, SYSRQ, SCROLLLOCK, PAUSE,
            GRAVE, N1, N2, N3, N4, N5, N6, N7, N8, N9, N0, MINUS, EQUAL, BACKSPACE, INSERT,
            TAB, Q, W, E, R, T, Y, U, I, O, P, LEFTBRACE, RIGHTBRACE, BACKSLASH, DELETE,
            CAPSLOCK, A, S, D, F, G, H, J, K, L, SEMICOLON, APOSTROPHE, ENTER, PAGEUP,
            LSHIFT, Z, X, C, V, B, N, M, COMMA, DOT, SLASH, RSHIFT, UP, PAGEDOWN,
            LCTRL, LMETA, LALT, SPACE, RALT, RMETA, FN, LEFT, DOWN, RIGHT};

    constexpr std::array<uint8_t, 84> keymap_fn = { 
        FNC_BAT_STATUS, NONE, FNC_BLE_ON, FNC_BLE_OFF, FNC_BLE_DIS, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, FNC_LED_TOG,
        NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, MEDIA_STOP, NONE,
        NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
        NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, MEDIA_PLAY, HOME,
        NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, MEDIA_MUTE, MEDIA_VUP, END,
        NONE, NONE, NONE, NONE, NONE, NONE, FN, MEDIA_PREV, MEDIA_VDOWN, MEDIA_SKIP};
};
