/*
 * SPDX-License-Identifier: MIT
 *
 * Split-relay helper for TPS43 runtime state sync.
 *
 * When CONFIG_ZMK_SPLIT_RELAY_EVENT is enabled the central uses
 * tps43_relay_send_state() to ship the full runtime state to the peripheral
 * after every state-changing Studio RPC. The peripheral's handler applies the
 * 12 fields via tps43_state_set_*().
 *
 * The relay carrier is a plain, packed C struct -- NOT protobuf.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

/* Bump when the wire layout of struct tps43_relay_state changes. */
#define TPS43_RELAY_STATE_VERSION 1

struct tps43_relay_state {
	uint8_t source;
	uint8_t version;

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
} __packed;

ZMK_EVENT_DECLARE(tps43_relay_state);

void tps43_relay_send_state(const struct device *dev);
