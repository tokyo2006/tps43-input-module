/*
 * SPDX-License-Identifier: MIT
 *
 * Split-relay helper for TPS43 runtime state sync -- see
 * include/tokyo2006/tps43/tps43_relay.h for the overview.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zmk/event_manager.h>
#include <zmk/tps43_state.h>
#include <tokyo2006/tps43/tps43_relay.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Event implementation for the relay carrier struct. */
ZMK_EVENT_IMPL(tps43_relay_state);

ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(tps43_relay_state, T43, source)
ZMK_RELAY_EVENT_HANDLE(tps43_relay_state, T43, source)

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static int tps43_relay_on_state(const zmk_event_t *eh)
{
	const struct tps43_relay_state *ev = as_tps43_relay_state(eh);
	if (ev == NULL) {
		return ZMK_EV_EVENT_BUBBLE;
	}

	const struct device *dev = DEVICE_DT_GET_ANY(azoteq_iqs5xx);
	if (dev == NULL) {
		LOG_WRN("No azoteq,iqs5xx device on peripheral; ignoring relayed state");
		return ZMK_EV_EVENT_HANDLED;
	}

	(void)tps43_state_set_one_finger_tap(dev, ev->one_finger_tap);
	(void)tps43_state_set_press_and_hold(dev, ev->press_and_hold);
	(void)tps43_state_set_two_finger_tap(dev, ev->two_finger_tap);
	(void)tps43_state_set_press_and_hold_time(dev, ev->press_and_hold_time);
	(void)tps43_state_set_scroll(dev, ev->scroll);
	(void)tps43_state_set_natural_scroll_x(dev, ev->natural_scroll_x);
	(void)tps43_state_set_natural_scroll_y(dev, ev->natural_scroll_y);
	(void)tps43_state_set_switch_xy(dev, ev->switch_xy);
	(void)tps43_state_set_flip_x(dev, ev->flip_x);
	(void)tps43_state_set_flip_y(dev, ev->flip_y);
	(void)tps43_state_set_bottom_beta(dev, ev->bottom_beta);
	(void)tps43_state_set_stationary_threshold(dev, ev->stationary_threshold);

	LOG_DBG("Peripheral applied relayed state (v%u)", ev->version);
	return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(tps43_relay_peripheral, tps43_relay_on_state);
ZMK_SUBSCRIPTION(tps43_relay_peripheral, tps43_relay_state);

#endif /* !CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

void tps43_relay_send_state(const struct device *dev)
{
	struct tps43_relay_state ev = {
		.source = ZMK_RELAY_EVENT_SOURCE_SELF,
		.version = TPS43_RELAY_STATE_VERSION,
	};

	ev.one_finger_tap = tps43_state_get_one_finger_tap(dev);
	ev.press_and_hold = tps43_state_get_press_and_hold(dev);
	ev.two_finger_tap = tps43_state_get_two_finger_tap(dev);
	ev.press_and_hold_time = tps43_state_get_press_and_hold_time(dev);
	ev.scroll = tps43_state_get_scroll(dev);
	ev.natural_scroll_x = tps43_state_get_natural_scroll_x(dev);
	ev.natural_scroll_y = tps43_state_get_natural_scroll_y(dev);
	ev.switch_xy = tps43_state_get_switch_xy(dev);
	ev.flip_x = tps43_state_get_flip_x(dev);
	ev.flip_y = tps43_state_get_flip_y(dev);
	ev.bottom_beta = tps43_state_get_bottom_beta(dev);
	ev.stationary_threshold = tps43_state_get_stationary_threshold(dev);

	raise_tps43_relay_state(ev);
}
