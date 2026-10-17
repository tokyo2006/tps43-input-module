/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compile-time defaults (used when neither DT nor settings provide a value). */
#define TPS43_ONE_FINGER_TAP_DEFAULT        false
#define TPS43_PRESS_AND_HOLD_DEFAULT        false
#define TPS43_TWO_FINGER_TAP_DEFAULT        false
#define TPS43_PRESS_AND_HOLD_TIME_DEFAULT   250
#define TPS43_SCROLL_DEFAULT                false
#define TPS43_NATURAL_SCROLL_X_DEFAULT      false
#define TPS43_NATURAL_SCROLL_Y_DEFAULT      false
#define TPS43_SWITCH_XY_DEFAULT             false
#define TPS43_FLIP_X_DEFAULT                false
#define TPS43_FLIP_Y_DEFAULT                false
#define TPS43_BOTTOM_BETA_DEFAULT           5
#define TPS43_STATIONARY_THRESHOLD_DEFAULT  5

struct tps43_runtime_state {
    /* gesture configuration */
    bool one_finger_tap;
    bool press_and_hold;
    bool two_finger_tap;
    uint16_t press_and_hold_time;

    /* scrolling configuration */
    bool scroll;
    bool natural_scroll_x;
    bool natural_scroll_y;

    /* axes configuration */
    bool switch_xy;
    bool flip_x;
    bool flip_y;

    /* sensitivity / filtering */
    uint8_t bottom_beta;
    uint8_t stationary_threshold;
};

/* Getters (read-only, cheap) */
bool     tps43_state_get_one_finger_tap(const struct device *dev);
bool     tps43_state_get_press_and_hold(const struct device *dev);
bool     tps43_state_get_two_finger_tap(const struct device *dev);
uint16_t tps43_state_get_press_and_hold_time(const struct device *dev);
bool     tps43_state_get_scroll(const struct device *dev);
bool     tps43_state_get_natural_scroll_x(const struct device *dev);
bool     tps43_state_get_natural_scroll_y(const struct device *dev);
bool     tps43_state_get_switch_xy(const struct device *dev);
bool     tps43_state_get_flip_x(const struct device *dev);
bool     tps43_state_get_flip_y(const struct device *dev);
uint8_t  tps43_state_get_bottom_beta(const struct device *dev);
uint8_t  tps43_state_get_stationary_threshold(const struct device *dev);

/* Setters (validate, write state, raise trackpad_status_changed) */
int tps43_state_set_one_finger_tap(const struct device *dev, bool v);
int tps43_state_set_press_and_hold(const struct device *dev, bool v);
int tps43_state_set_two_finger_tap(const struct device *dev, bool v);
int tps43_state_set_press_and_hold_time(const struct device *dev, uint16_t v);
int tps43_state_set_scroll(const struct device *dev, bool v);
int tps43_state_set_natural_scroll_x(const struct device *dev, bool v);
int tps43_state_set_natural_scroll_y(const struct device *dev, bool v);
int tps43_state_set_switch_xy(const struct device *dev, bool v);
int tps43_state_set_flip_x(const struct device *dev, bool v);
int tps43_state_set_flip_y(const struct device *dev, bool v);
int tps43_state_set_bottom_beta(const struct device *dev, uint8_t v);
int tps43_state_set_stationary_threshold(const struct device *dev, uint8_t v);

/* Bulk ops */
int tps43_state_load_defaults(const struct device *dev);
int tps43_state_load_from_dt(const struct device *dev);
int tps43_state_load_from_settings(const struct device *dev);
int tps43_state_apply_all(const struct device *dev);

/* Direct access for drivers that need a write-through cache */
struct tps43_runtime_state *tps43_state_get_struct(const struct device *dev);

#ifdef __cplusplus
}
#endif
