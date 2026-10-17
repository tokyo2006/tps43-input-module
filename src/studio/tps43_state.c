/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/tps43_state.h>
#include <zmk/tps43_settings.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Driver-side hooks, provided by drivers/input/iqs5xx.c whenever an
 * azoteq,iqs5xx node exists. Declared weak so this file links even in builds
 * without a trackpad device -- e.g. Studio RPC over USB in tests, or a split
 * half whose trackpad lives on the other hand. */
extern struct tps43_runtime_state *tps43_driver_get_state(const struct device *dev)
	__attribute__((weak));
extern void tps43_driver_apply_all(const struct device *dev) __attribute__((weak));

static void tps43_state_apply_defaults(struct tps43_runtime_state *s);

static struct tps43_runtime_state tps43_fallback_state;
static bool tps43_fallback_initialized;

struct tps43_runtime_state *tps43_state_get_struct(const struct device *dev)
{
	if (dev == NULL || tps43_driver_get_state == NULL) {
		if (!tps43_fallback_initialized) {
			tps43_fallback_initialized = true;
			tps43_state_apply_defaults(&tps43_fallback_state);
		}
		return &tps43_fallback_state;
	}
	return tps43_driver_get_state(dev);
}

static inline struct tps43_runtime_state *rt(const struct device *dev)
{
	return tps43_state_get_struct(dev);
}

#define DEFINE_GETTER(T, name, field)                                                             \
	T tps43_state_get_##name(const struct device *dev) { return rt(dev)->field; }

#define DEFINE_SETTER(T, name, field, validate_fn)                                                \
	int tps43_state_set_##name(const struct device *dev, T v)                                  \
	{                                                                                          \
		int rc = validate_fn(v);                                                           \
		if (rc != 0) {                                                                     \
			return rc;                                                                 \
		}                                                                                  \
		rt(dev)->field = v;                                                                \
		raise_trackpad_status_changed();                                                   \
		return 0;                                                                          \
	}

static int validate_bool_any(bool v) { (void)v; return 0; }
static int validate_u8_any(uint8_t v) { (void)v; return 0; }
static int validate_u16_any(uint16_t v) { (void)v; return 0; }

DEFINE_GETTER(bool, one_finger_tap, one_finger_tap)
DEFINE_GETTER(bool, press_and_hold, press_and_hold)
DEFINE_GETTER(bool, two_finger_tap, two_finger_tap)
DEFINE_GETTER(uint16_t, press_and_hold_time, press_and_hold_time)
DEFINE_GETTER(bool, scroll, scroll)
DEFINE_GETTER(bool, natural_scroll_x, natural_scroll_x)
DEFINE_GETTER(bool, natural_scroll_y, natural_scroll_y)
DEFINE_GETTER(bool, switch_xy, switch_xy)
DEFINE_GETTER(bool, flip_x, flip_x)
DEFINE_GETTER(bool, flip_y, flip_y)
DEFINE_GETTER(uint8_t, bottom_beta, bottom_beta)
DEFINE_GETTER(uint8_t, stationary_threshold, stationary_threshold)

DEFINE_SETTER(bool, one_finger_tap, one_finger_tap, validate_bool_any)
DEFINE_SETTER(bool, press_and_hold, press_and_hold, validate_bool_any)
DEFINE_SETTER(bool, two_finger_tap, two_finger_tap, validate_bool_any)
DEFINE_SETTER(uint16_t, press_and_hold_time, press_and_hold_time, validate_u16_any)
DEFINE_SETTER(bool, scroll, scroll, validate_bool_any)
DEFINE_SETTER(bool, natural_scroll_x, natural_scroll_x, validate_bool_any)
DEFINE_SETTER(bool, natural_scroll_y, natural_scroll_y, validate_bool_any)
DEFINE_SETTER(bool, switch_xy, switch_xy, validate_bool_any)
DEFINE_SETTER(bool, flip_x, flip_x, validate_bool_any)
DEFINE_SETTER(bool, flip_y, flip_y, validate_bool_any)
DEFINE_SETTER(uint8_t, bottom_beta, bottom_beta, validate_u8_any)
DEFINE_SETTER(uint8_t, stationary_threshold, stationary_threshold, validate_u8_any)

int tps43_state_load_defaults(const struct device *dev)
{
	tps43_state_apply_defaults(tps43_state_get_struct(dev));
	return 0;
}

static void tps43_state_apply_defaults(struct tps43_runtime_state *s)
{
	s->one_finger_tap = TPS43_ONE_FINGER_TAP_DEFAULT;
	s->press_and_hold = TPS43_PRESS_AND_HOLD_DEFAULT;
	s->two_finger_tap = TPS43_TWO_FINGER_TAP_DEFAULT;
	s->press_and_hold_time = TPS43_PRESS_AND_HOLD_TIME_DEFAULT;
	s->scroll = TPS43_SCROLL_DEFAULT;
	s->natural_scroll_x = TPS43_NATURAL_SCROLL_X_DEFAULT;
	s->natural_scroll_y = TPS43_NATURAL_SCROLL_Y_DEFAULT;
	s->switch_xy = TPS43_SWITCH_XY_DEFAULT;
	s->flip_x = TPS43_FLIP_X_DEFAULT;
	s->flip_y = TPS43_FLIP_Y_DEFAULT;
	s->bottom_beta = TPS43_BOTTOM_BETA_DEFAULT;
	s->stationary_threshold = TPS43_STATIONARY_THRESHOLD_DEFAULT;
}

int tps43_state_load_from_dt(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

int tps43_state_load_from_settings(const struct device *dev)
{
#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
	return tps43_settings_load_cb(dev);
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

int tps43_state_apply_all(const struct device *dev)
{
	if (dev == NULL || tps43_driver_apply_all == NULL) {
		return 0;
	}
	tps43_driver_apply_all(dev);
	return 0;
}
