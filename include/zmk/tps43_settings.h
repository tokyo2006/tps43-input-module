/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Persist every field of tps43_runtime_state to its custom-settings record
 * under the "tokyo2006__tps43" namespace. Called by handle_set_state when the
 * web UI sets `persist=true`.
 */
int tps43_settings_save_all(const struct device *dev);

/* Erase every persisted record under the "tokyo2006__tps43" namespace. */
int tps43_settings_reset_all(const struct device *dev);

/* Read every "tokyo2006__tps43" custom setting and apply it to the runtime
 * state via tps43_state_set_*. Wired to zmk_custom_settings_initialized. */
int tps43_settings_load_cb(const struct device *dev);

#ifdef __cplusplus
}
#endif
