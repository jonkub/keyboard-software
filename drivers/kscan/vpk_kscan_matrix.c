#define DT_DRV_COMPAT vpk_kscan_matrix

#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(vpk, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef INTELLISENSE
#define Z_FOR_LOOP_1(...) 0
#endif

#define DEBOUNCE_THRESHOLD CONFIG_VPK_KSCAN_MATRIX_DEBOUNCE_US / CONFIG_VPK_KSCAN_MATRIX_PERIOD_US
#define DEBOUNCE_CTR_MAX DEBOUNCE_THRESHOLD * 2

// Pin State
struct vpk_pin_state {
    uint8_t counter;
};

// Constant information
struct vpk_matrix_config {
    const struct gpio_dt_spec *row_pins;
    size_t row_count;
    const struct gpio_dt_spec *col_pins;
    size_t col_count;

};

// Mutable data storage
struct vpk_matrix_data {
    kscan_callback_t callback;
    bool do_callbacks;

    struct vpk_pin_state *pin_states;
    const struct device *dev;
    struct k_work_delayable work;

    k_timeout_t scan_period;
};

/**
 * @brief Helper for mapping keys to a 1-D array
 * 
 * @param config device config
 * @param row index of the row pin
 * @param col index of the col pin
 * @return size_t index into a 1-D array
 */
static inline size_t pin_idx(const struct vpk_matrix_config *config, const size_t row, const size_t col) {
    return (col * config->row_count) + row;   
}

/**
 * @brief Processes state and performs debouncing.
 * Currently uses "quick draw" debouncing. EMI-Sensitive!
 * 
 * @param config driver configuration
 * @param state state of the pin
 * @param current_value current value of the pin
 * @return true if the pin value changed,
 * @return false otherwise
 */
bool process_pin_state(const struct vpk_matrix_config *config, struct vpk_pin_state *state, bool current_value) {
    if (!current_value) {
        if (state->counter > 0) {
            if (--state->counter == DEBOUNCE_THRESHOLD) {
                state->counter = 0;
                return true;
            }
        }
    } else {
        if (state->counter < DEBOUNCE_CTR_MAX) {
            if (++state->counter == DEBOUNCE_THRESHOLD) {
                state->counter = DEBOUNCE_CTR_MAX;
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Main work function. Scans the GPIO Matrix
 * NOT CALLED DIRECTLY. Use k_work_reschedule().
 * 
 * @param work Workqueue api object
 */
static void vpk_matrix_work_handler(struct k_work *work) {
    int err;
    // Get data and config pointers
    struct k_work_delayable *delayable_work = CONTAINER_OF(work, struct k_work_delayable, work);
    struct vpk_matrix_data *data = CONTAINER_OF(delayable_work, struct vpk_matrix_data, work);
    const struct vpk_matrix_config *config = data->dev->config;

    // Scan the GPIO matrix
    for (size_t col = 0; col < config->col_count; ++col) {
        err = gpio_pin_set_dt(&config->col_pins[col], 0);
        if (err) {
            return;
        }

        k_busy_wait(2);

        for (size_t row = 0; row < config->row_count; ++row) {
            err = gpio_pin_get_dt(&config->row_pins[row]);
            if (err < 0) {
                return;
            }
            bool pressed = !err;
            size_t idx = pin_idx(config, row, col);

            // Process and debounce. Call callback if the state changed
            if (process_pin_state(config, &data->pin_states[idx], pressed)) {
                data->callback(data->dev, row, col, pressed);
            }
        }

        err = gpio_pin_set_dt(&config->col_pins[col], 1);
        if (err) {
            return;
        }
    }

    // If still running, reschedule the work according to the scan period
    if (data->do_callbacks) {
        k_work_reschedule(&data->work, data->scan_period);
    }
}


// --------------------------
// -- DRIVER API FUNCTIONS --
// --------------------------

// Called through kscan_configure(dev, callback)
static int vpk_matrix_configure(const struct device *dev, kscan_callback_t callback) {
    struct vpk_matrix_data *data = dev->data;
    data->callback = callback;
    return 0;
}

// Called through kscan_enable_callback(dev)
static int vpk_matrix_enable_callback(const struct device *dev) {
    struct vpk_matrix_data *data = dev->data;
    data->do_callbacks = true;
    k_work_reschedule(&data->work, K_NO_WAIT);
    return 0;
}

// Called through kscan_disable_callback(dev)
static int vpk_matrix_disable_callback(const struct device *dev) {
    struct vpk_matrix_data *data = dev->data;
    data->do_callbacks = false;
    k_work_cancel_delayable(&data->work);
    return 0;
}

static const struct kscan_driver_api vpk_matrix_driver_api = {
    .config = vpk_matrix_configure,
    .enable_callback = vpk_matrix_enable_callback,
    .disable_callback = vpk_matrix_disable_callback
};

// ---------------------------
// -- DRIVER INITIALIZATION --
// ---------------------------

// Helper to initialize a single matrix gpio pin
static int vpk_config_matrix_pin(const struct gpio_dt_spec *pin, gpio_flags_t flags) {
    int err;
    if (!device_is_ready(pin->port)) {
        LOG_ERR("GPIO port required for Matrix but not ready: %s", pin->port->name);
        return -1;
    }
    err = gpio_pin_configure_dt(pin, flags);
    if (err) {
        LOG_ERR("Could not configure GPIO pin %d on port %s", pin->pin, pin->port->name);
        return -1;
    }
    return 0;
}

// Called at boot according to the init priority
static int vpk_matrix_init(const struct device *dev) {
    struct vpk_matrix_data *data = dev->data;
    const struct vpk_matrix_config *config = dev->config;
    data->dev = dev;
    data->scan_period = K_USEC(CONFIG_VPK_KSCAN_MATRIX_PERIOD_US);

    int err;
    for (int i = 0; i < config->row_count; ++i) {
        err = vpk_config_matrix_pin(&config->row_pins[i], GPIO_INPUT | GPIO_PULL_UP);
        if (err) {
            return -1;
        }
    }
    for (int i = 0; i < config->col_count; ++i) {
        err = vpk_config_matrix_pin(&config->col_pins[i], GPIO_OUTPUT_ACTIVE | GPIO_OPEN_DRAIN);
        if (err) {
            return -1;
        }
    }
    k_work_init_delayable(&data->work, vpk_matrix_work_handler);
    return 0;
}


#define VPK_GET_MATRIX_GPIO(gpio_idx, drv_idx, gpio_arr) \
    GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(drv_idx), gpio_arr, gpio_idx),

// Initialize one instance
#define VPK_MATRIX_INIT(index)                                                     \
    static const struct gpio_dt_spec vpk_matrix_row_pins_##index[] = {                       \
        LISTIFY(DT_INST_PROP_LEN(index, row_gpios), VPK_GET_MATRIX_GPIO, (), index, row_gpios) \
    };                                                                             \
    static const struct gpio_dt_spec vpk_matrix_col_pins_##index[] = {                       \
        LISTIFY(DT_INST_PROP_LEN(index, col_gpios), VPK_GET_MATRIX_GPIO, (), index, col_gpios) \
    };                                                                             \
	static const struct vpk_matrix_config vpk_matrix_config_##index = {	       \
        .row_pins = vpk_matrix_row_pins_##index,                                 \
        .row_count = ARRAY_SIZE(vpk_matrix_row_pins_##index),                    \
        .col_pins = vpk_matrix_col_pins_##index,                                    \
        .col_count = ARRAY_SIZE(vpk_matrix_col_pins_##index)                     \
	};								       \
    static struct vpk_pin_state vpk_pin_states_##index[                         \
        DT_INST_PROP_LEN(index, row_gpios) * DT_INST_PROP_LEN(index, col_gpios) \
    ];                 \
	static struct vpk_matrix_data vpk_matrix_data_##index = {		       \
        .do_callbacks = false,                                               \
        .pin_states = vpk_pin_states_##index                               \
    };                                                                      \
	DEVICE_DT_INST_DEFINE(index, vpk_matrix_init, NULL,			       \
			    &vpk_matrix_data_##index, &vpk_matrix_config_##index,      \
			    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,	       \
			    &vpk_matrix_driver_api);

// Initialize all compatible instances
DT_INST_FOREACH_STATUS_OKAY(VPK_MATRIX_INIT)