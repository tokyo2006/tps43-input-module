/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zmk/studio/custom.h>
#include <tokyo2006/tps43/tps43.pb.h>

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
#include <cormoran/zmk/custom_settings.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
#include <tokyo2006/tps43/tps43_relay.h>
#endif

#include <zmk/tps43_settings.h>
#include <zmk/tps43_state.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_rpc_custom_subsystem_meta tps43_subsystem_meta = {
	ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("https://tokyo2006.github.io/tps43-input-module/"),
	.security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(tokyo2006__tps43, &tps43_subsystem_meta, tps43_rpc_handle_request);

ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(tokyo2006__tps43, tokyo2006_tps43_Response);

static int handle_get_state(const tokyo2006_tps43_GetStateRequest *req,
			    tokyo2006_tps43_Response *resp);
static int handle_set_state(const tokyo2006_tps43_SetStateRequest *req,
			    tokyo2006_tps43_Response *resp);
static int handle_reset(const tokyo2006_tps43_ResetRequest *req,
			tokyo2006_tps43_Response *resp);

static bool tps43_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
				     pb_callback_t *encode_response)
{
	tokyo2006_tps43_Response *resp =
		ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(tokyo2006__tps43, encode_response);

	tokyo2006_tps43_Request req = tokyo2006_tps43_Request_init_zero;

	pb_istream_t req_stream =
		pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);
	if (!pb_decode(&req_stream, tokyo2006_tps43_Request_fields, &req)) {
		LOG_WRN("Failed to decode tps43 request: %s", PB_GET_ERROR(&req_stream));
		tokyo2006_tps43_ErrorResponse err = tokyo2006_tps43_ErrorResponse_init_zero;
		snprintf(err.message, sizeof(err.message), "Failed to decode request");
		resp->which_response_type = tokyo2006_tps43_Response_error_tag;
		resp->response_type.error = err;
		return true;
	}

	int rc = 0;
	switch (req.which_request_type) {
	case tokyo2006_tps43_Request_get_state_tag:
		rc = handle_get_state(&req.request_type.get_state, resp);
		break;
	case tokyo2006_tps43_Request_set_state_tag:
		rc = handle_set_state(&req.request_type.set_state, resp);
		break;
	case tokyo2006_tps43_Request_reset_tag:
		rc = handle_reset(&req.request_type.reset, resp);
		break;
	default:
		LOG_WRN("Unsupported tps43 request type: %d", req.which_request_type);
		rc = -1;
	}

	if (rc != 0 && resp->which_response_type != tokyo2006_tps43_Response_error_tag) {
		tokyo2006_tps43_ErrorResponse err = tokyo2006_tps43_ErrorResponse_init_zero;
		snprintf(err.message, sizeof(err.message), "Failed to process request");
		resp->which_response_type = tokyo2006_tps43_Response_error_tag;
		resp->response_type.error = err;
	}
	return true;
}

static int handle_get_state(const tokyo2006_tps43_GetStateRequest *req,
			    tokyo2006_tps43_Response *resp)
{
	ARG_UNUSED(req);

	const struct device *dev = DEVICE_DT_GET_ANY(azoteq_iqs5xx);

	tokyo2006_tps43_Tps43State state = tokyo2006_tps43_Tps43State_init_zero;
	state.one_finger_tap = tps43_state_get_one_finger_tap(dev);
	state.press_and_hold = tps43_state_get_press_and_hold(dev);
	state.two_finger_tap = tps43_state_get_two_finger_tap(dev);
	state.press_and_hold_time = tps43_state_get_press_and_hold_time(dev);
	state.scroll = tps43_state_get_scroll(dev);
	state.natural_scroll_x = tps43_state_get_natural_scroll_x(dev);
	state.natural_scroll_y = tps43_state_get_natural_scroll_y(dev);
	state.switch_xy = tps43_state_get_switch_xy(dev);
	state.flip_x = tps43_state_get_flip_x(dev);
	state.flip_y = tps43_state_get_flip_y(dev);
	state.bottom_beta = tps43_state_get_bottom_beta(dev);
	state.stationary_threshold = tps43_state_get_stationary_threshold(dev);

	/* proto3 `optional` requires explicit presence. */
	state.has_one_finger_tap = true;
	state.has_press_and_hold = true;
	state.has_two_finger_tap = true;
	state.has_press_and_hold_time = true;
	state.has_scroll = true;
	state.has_natural_scroll_x = true;
	state.has_natural_scroll_y = true;
	state.has_switch_xy = true;
	state.has_flip_x = true;
	state.has_flip_y = true;
	state.has_bottom_beta = true;
	state.has_stationary_threshold = true;

	tokyo2006_tps43_GetStateResponse get_resp = tokyo2006_tps43_GetStateResponse_init_zero;
	get_resp.state = state;
	get_resp.has_state = true;
	resp->which_response_type = tokyo2006_tps43_Response_get_state_tag;
	resp->response_type.get_state = get_resp;
	return 0;
}

static int handle_set_state(const tokyo2006_tps43_SetStateRequest *req,
			    tokyo2006_tps43_Response *resp)
{
	const struct device *dev = DEVICE_DT_GET_ANY(azoteq_iqs5xx);
	bool persist = req->persist;

	int first_rc = 0;
	char first_err[128] = {0};

#define APPLY(field, set_call)                                                                    \
	do {                                                                                      \
		int rc = set_call;                                                                \
		if (rc != 0 && first_rc == 0) {                                                   \
			first_rc = rc;                                                            \
			snprintf(first_err, sizeof(first_err), "%s failed (%d)", #field, rc);      \
		}                                                                                 \
	} while (0)

	if (req->state.has_one_finger_tap) {
		APPLY(one_finger_tap, tps43_state_set_one_finger_tap(dev, req->state.one_finger_tap));
	}
	if (req->state.has_press_and_hold) {
		APPLY(press_and_hold, tps43_state_set_press_and_hold(dev, req->state.press_and_hold));
	}
	if (req->state.has_two_finger_tap) {
		APPLY(two_finger_tap, tps43_state_set_two_finger_tap(dev, req->state.two_finger_tap));
	}
	if (req->state.has_press_and_hold_time) {
		APPLY(press_and_hold_time,
		      tps43_state_set_press_and_hold_time(dev, (uint16_t)req->state.press_and_hold_time));
	}
	if (req->state.has_scroll) {
		APPLY(scroll, tps43_state_set_scroll(dev, req->state.scroll));
	}
	if (req->state.has_natural_scroll_x) {
		APPLY(natural_scroll_x,
		      tps43_state_set_natural_scroll_x(dev, req->state.natural_scroll_x));
	}
	if (req->state.has_natural_scroll_y) {
		APPLY(natural_scroll_y,
		      tps43_state_set_natural_scroll_y(dev, req->state.natural_scroll_y));
	}
	if (req->state.has_switch_xy) {
		APPLY(switch_xy, tps43_state_set_switch_xy(dev, req->state.switch_xy));
	}
	if (req->state.has_flip_x) {
		APPLY(flip_x, tps43_state_set_flip_x(dev, req->state.flip_x));
	}
	if (req->state.has_flip_y) {
		APPLY(flip_y, tps43_state_set_flip_y(dev, req->state.flip_y));
	}
	if (req->state.has_bottom_beta) {
		APPLY(bottom_beta, tps43_state_set_bottom_beta(dev, (uint8_t)req->state.bottom_beta));
	}
	if (req->state.has_stationary_threshold) {
		APPLY(stationary_threshold,
		      tps43_state_set_stationary_threshold(dev, (uint8_t)req->state.stationary_threshold));
	}

#undef APPLY

	if (first_rc != 0) {
		tokyo2006_tps43_ErrorResponse err = tokyo2006_tps43_ErrorResponse_init_zero;
		snprintf(err.message, sizeof(err.message), "%s", first_err);
		resp->which_response_type = tokyo2006_tps43_Response_error_tag;
		resp->response_type.error = err;
		return first_rc;
	}

	if (persist) {
		(void)tps43_settings_save_all(dev);
	}

	(void)tps43_state_apply_all(dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_INPUT_AZOTEQ_IQS5XX)
	tps43_relay_send_state(dev);
#endif

	tokyo2006_tps43_SetStateResponse set_resp = tokyo2006_tps43_SetStateResponse_init_zero;
	set_resp.persisted = persist;
	tokyo2006_tps43_GetStateRequest greq = tokyo2006_tps43_GetStateRequest_init_zero;
	handle_get_state(&greq, resp);
	set_resp.state = resp->response_type.get_state.state;
	set_resp.has_state = true;
	resp->which_response_type = tokyo2006_tps43_Response_set_state_tag;
	resp->response_type.set_state = set_resp;
	return 0;
}

static int handle_reset(const tokyo2006_tps43_ResetRequest *req, tokyo2006_tps43_Response *resp)
{
	const struct device *dev = DEVICE_DT_GET_ANY(azoteq_iqs5xx);

	tps43_state_load_defaults(dev);
	(void)tps43_state_apply_all(dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_INPUT_AZOTEQ_IQS5XX)
	tps43_relay_send_state(dev);
#endif

	if (req->factory_defaults) {
		(void)tps43_settings_reset_all(dev);
	}

	tokyo2006_tps43_GetStateRequest greq = tokyo2006_tps43_GetStateRequest_init_zero;
	handle_get_state(&greq, resp);

	tokyo2006_tps43_ResetResponse reset_resp = tokyo2006_tps43_ResetResponse_init_zero;
	reset_resp.state = resp->response_type.get_state.state;
	reset_resp.has_state = true;
	resp->which_response_type = tokyo2006_tps43_Response_reset_tag;
	resp->response_type.reset = reset_resp;
	return 0;
}
