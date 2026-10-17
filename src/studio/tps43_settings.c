/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Persistent custom-settings backing for tps43_runtime_state. One
 * ZMK_CUSTOM_SETTING_DEFINE per runtime field under the "tokyo2006__tps43"
 * namespace, plus save/reset/load helpers.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
#include <cormoran/zmk/custom_settings.h>
#endif

#include <zmk/tps43_settings.h>
#include <zmk/tps43_state.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define TPS43_NS "tokyo2006__tps43"

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)

#define TPS43_SETTING_BOOL(_name, _key, _default)                                                  \
	ZMK_CUSTOM_SETTING_DEFINE(                                                                 \
		_name, TPS43_NS, _key, ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,                           \
		ZMK_CUSTOM_SETTING_VALUE_BOOL(_default), ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC, \
		ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE, ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,       \
		ZMK_CUSTOM_SETTING_NO_CONSTRAINT)

#define TPS43_SETTING_INT(_name, _key, _default)                                                  \
	ZMK_CUSTOM_SETTING_DEFINE(                                                                 \
		_name, TPS43_NS, _key, ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,                           \
		ZMK_CUSTOM_SETTING_VALUE_INT32(_default), ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC, \
		ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE, ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,       \
		ZMK_CUSTOM_SETTING_NO_CONSTRAINT)

TPS43_SETTING_BOOL(tps43_setting_one_finger_tap, "one_finger_tap", TPS43_ONE_FINGER_TAP_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_press_and_hold, "press_and_hold", TPS43_PRESS_AND_HOLD_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_two_finger_tap, "two_finger_tap", TPS43_TWO_FINGER_TAP_DEFAULT);
TPS43_SETTING_INT(tps43_setting_press_and_hold_time, "press_and_hold_time",
		  TPS43_PRESS_AND_HOLD_TIME_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_scroll, "scroll", TPS43_SCROLL_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_natural_scroll_x, "natural_scroll_x",
		   TPS43_NATURAL_SCROLL_X_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_natural_scroll_y, "natural_scroll_y",
		   TPS43_NATURAL_SCROLL_Y_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_switch_xy, "switch_xy", TPS43_SWITCH_XY_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_flip_x, "flip_x", TPS43_FLIP_X_DEFAULT);
TPS43_SETTING_BOOL(tps43_setting_flip_y, "flip_y", TPS43_FLIP_Y_DEFAULT);
TPS43_SETTING_INT(tps43_setting_bottom_beta, "bottom_beta", TPS43_BOTTOM_BETA_DEFAULT);
TPS43_SETTING_INT(tps43_setting_stationary_threshold, "stationary_threshold",
		  TPS43_STATIONARY_THRESHOLD_DEFAULT);

int tps43_settings_save_all(const struct device *dev)
{
	int rc;
	int first_rc = 0;

#define SAVE_BOOL(field, get_fn)                                                                   \
	do {                                                                                       \
		rc = zmk_custom_setting_set_bool(&tps43_setting_##field, (get_fn(dev)),             \
						 ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);       \
		if (rc != 0 && first_rc == 0) {                                                    \
			first_rc = rc;                                                             \
			LOG_WRN("save_all: %s -> %d", #field, rc);                                 \
		}                                                                                  \
	} while (0)

#define SAVE_INT(field, get_fn)                                                                    \
	do {                                                                                       \
		rc = zmk_custom_setting_set_int32(&tps43_setting_##field, (int32_t)(get_fn(dev)),   \
						  ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);      \
		if (rc != 0 && first_rc == 0) {                                                    \
			first_rc = rc;                                                             \
			LOG_WRN("save_all: %s -> %d", #field, rc);                                 \
		}                                                                                  \
	} while (0)

	SAVE_BOOL(one_finger_tap, tps43_state_get_one_finger_tap);
	SAVE_BOOL(press_and_hold, tps43_state_get_press_and_hold);
	SAVE_BOOL(two_finger_tap, tps43_state_get_two_finger_tap);
	SAVE_INT(press_and_hold_time, tps43_state_get_press_and_hold_time);
	SAVE_BOOL(scroll, tps43_state_get_scroll);
	SAVE_BOOL(natural_scroll_x, tps43_state_get_natural_scroll_x);
	SAVE_BOOL(natural_scroll_y, tps43_state_get_natural_scroll_y);
	SAVE_BOOL(switch_xy, tps43_state_get_switch_xy);
	SAVE_BOOL(flip_x, tps43_state_get_flip_x);
	SAVE_BOOL(flip_y, tps43_state_get_flip_y);
	SAVE_INT(bottom_beta, tps43_state_get_bottom_beta);
	SAVE_INT(stationary_threshold, tps43_state_get_stationary_threshold);

#undef SAVE_BOOL
#undef SAVE_INT

	return first_rc;
}

int tps43_settings_reset_all(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t affected = 0;
	int rc = zmk_custom_settings_reset_scope(TPS43_NS, NULL, NULL, &affected);
	if (rc != 0) {
		LOG_WRN("reset_all: scope reset failed (%d), affected=%u", rc, affected);
	} else {
		LOG_DBG("reset_all: erased %u tps43 settings", affected);
	}
	return rc;
}

int tps43_settings_load_cb(const struct device *dev)
{
	int first_rc = 0;

#define LOAD_BOOL(field, set_fn)                                                                   \
	do {                                                                                       \
		bool v;                                                                            \
		int rc = zmk_custom_setting_get_bool(&tps43_setting_##field, &v);                   \
		if (rc == 0) {                                                                     \
			rc = set_fn(dev, v);                                                       \
			if (rc != 0 && first_rc == 0) {                                            \
				first_rc = rc;                                                     \
			}                                                                          \
		} else if (rc != -ENOENT && first_rc == 0) {                                       \
			first_rc = rc;                                                             \
		}                                                                                  \
	} while (0)

#define LOAD_INT(field, set_fn)                                                                    \
	do {                                                                                       \
		int32_t v;                                                                         \
		int rc = zmk_custom_setting_get_int32(&tps43_setting_##field, &v);                  \
		if (rc == 0) {                                                                     \
			rc = set_fn(dev, (uint32_t)v);                                             \
			if (rc != 0 && first_rc == 0) {                                            \
				first_rc = rc;                                                     \
			}                                                                          \
		} else if (rc != -ENOENT && first_rc == 0) {                                       \
			first_rc = rc;                                                             \
		}                                                                                  \
	} while (0)

	LOAD_BOOL(one_finger_tap, tps43_state_set_one_finger_tap);
	LOAD_BOOL(press_and_hold, tps43_state_set_press_and_hold);
	LOAD_BOOL(two_finger_tap, tps43_state_set_two_finger_tap);
	LOAD_INT(press_and_hold_time, tps43_state_set_press_and_hold_time);
	LOAD_BOOL(scroll, tps43_state_set_scroll);
	LOAD_BOOL(natural_scroll_x, tps43_state_set_natural_scroll_x);
	LOAD_BOOL(natural_scroll_y, tps43_state_set_natural_scroll_y);
	LOAD_BOOL(switch_xy, tps43_state_set_switch_xy);
	LOAD_BOOL(flip_x, tps43_state_set_flip_x);
	LOAD_BOOL(flip_y, tps43_state_set_flip_y);
	LOAD_INT(bottom_beta, tps43_state_set_bottom_beta);
	LOAD_INT(stationary_threshold, tps43_state_set_stationary_threshold);

#undef LOAD_BOOL
#undef LOAD_INT

	return first_rc;
}

static int tps43_settings_on_initialized(const zmk_event_t *eh)
{
	ARG_UNUSED(eh);
	const struct device *dev = DEVICE_DT_GET_ANY(azoteq_iqs5xx);
	if (dev == NULL) {
		LOG_WRN("initialized event: no azoteq,iqs5xx device; skipping load");
		return ZMK_EV_EVENT_HANDLED;
	}
	(void)tps43_settings_load_cb(dev);
	(void)tps43_state_apply_all(dev);
	return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(tps43_settings_initialized_listener, tps43_settings_on_initialized);
ZMK_SUBSCRIPTION(tps43_settings_initialized_listener, zmk_custom_settings_initialized);

#else /* !IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS) */

int tps43_settings_save_all(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

int tps43_settings_reset_all(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

int tps43_settings_load_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#endif /* IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS) */
