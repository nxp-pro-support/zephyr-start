/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * t1s_device: 10BASE-T1S node (LAN8651) that serves the LIGHT STUDIO
 * dashboard and drives the on-board RGB LED via PWM.
 *
 * - GET /    -> gzipped single-file dashboard (web/index.html)
 * - GET /ws  -> WebSocket. On connect the device pushes its state:
 *                 {"name":"...","plca":N,"r":..,"g":..,"b":..}
 *               The browser sends {"r":..,"g":..,"b":..} to set the LED
 *               (PWM duty 0-255 per channel). State changes are rebroadcast
 *               to the other connected clients so all open tabs stay in sync.
 *
 * The dashboard discovers devices by probing each candidate IP on the T1S
 * segment, so only nodes that accept the WebSocket appear as cards.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/data/json.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/sys/reboot.h>
#endif
#include <app_version.h>

/* The LAN865x driver sends a frame only if it fits the chip's free TX
 * chunk credit in one shot (oa_tc6_send_chunks: chunks > txc -> -EIO,
 * no retry). Full-size 1514-byte frames need 24x 64-byte chunks and get
 * dropped deterministically, while small frames pass. Clamp the MTU so
 * TCP advertises an MSS whose segments always fit; this also makes the
 * peer send small segments toward us (its MSS comes from our SYN/ACK),
 * keeping the bridge's T1S egress under the same limit.
 */
#define T1S_SAFE_MTU 576

LOG_MODULE_REGISTER(t1s_device, LOG_LEVEL_INF);

#define PLCA_NODE_ID DT_PROP(DT_NODELABEL(lan8651_phy), plca_node_id)

/* ----------------------------- RGB LED (PWM) ----------------------------- */

static const struct pwm_dt_spec led_pwm[3] = {
	PWM_DT_SPEC_GET(DT_NODELABEL(red_pwm_led)),
	PWM_DT_SPEC_GET(DT_NODELABEL(green_pwm_led)),
	PWM_DT_SPEC_GET(DT_NODELABEL(blue_pwm_led)),
};

static K_MUTEX_DEFINE(state_lock);
static uint8_t rgb[3];

static void rgb_apply(void)
{
	for (int i = 0; i < 3; i++) {
		uint32_t pulse = (uint32_t)(((uint64_t)led_pwm[i].period * rgb[i]) / 255U);
		int ret = pwm_set_pulse_dt(&led_pwm[i], pulse);

		if (ret < 0) {
			LOG_ERR("pwm_set_pulse_dt(ch %d) failed: %d", i, ret);
		}
	}
}

/* ------------------------------- WebSocket ------------------------------- */

#define WS_MAX_CLIENTS    3
#define WS_THREAD_STACK   2560
#define WS_RX_BUF_SIZE    128

static int ws_clients[WS_MAX_CLIENTS] = { -1, -1, -1 };
static struct k_thread ws_threads[WS_MAX_CLIENTS];
K_THREAD_STACK_ARRAY_DEFINE(ws_stacks, WS_MAX_CLIENTS, WS_THREAD_STACK);

struct rgb_cmd {
	int r;
	int g;
	int b;
};

static const struct json_obj_descr rgb_cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, r, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, g, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, b, JSON_TOK_NUMBER),
};

static int state_json(char *buf, size_t len)
{
	return snprintk(buf, len,
			"{\"name\":\"%s\",\"ver\":\"" APP_VERSION_STRING "\","
			"\"plca\":%u,\"r\":%u,\"g\":%u,\"b\":%u}",
			CONFIG_APP_DEVICE_NAME, (unsigned int)PLCA_NODE_ID,
			rgb[0], rgb[1], rgb[2]);
}

/* Callers hold state_lock: serializes both the state snapshot and the
 * socket write (broadcasts may run from any client thread).
 */
static void ws_send_state(int sock)
{
	char buf[128];
	int len = state_json(buf, sizeof(buf));
	int ret = websocket_send_msg(sock, buf, len, WEBSOCKET_OPCODE_DATA_TEXT,
				     false, true, SYS_FOREVER_MS);

	if (ret < 0) {
		LOG_DBG("ws send to %d failed: %d", sock, ret);
	}
}

static void ws_broadcast_state(int except_sock)
{
	for (int i = 0; i < WS_MAX_CLIENTS; i++) {
		if (ws_clients[i] >= 0 && ws_clients[i] != except_sock) {
			ws_send_state(ws_clients[i]);
		}
	}
}

static void ws_client_thread(void *p1, void *p2, void *p3)
{
	int slot = POINTER_TO_INT(p1);
	int sock = ws_clients[slot];
	uint8_t buf[WS_RX_BUF_SIZE];
	struct zsock_pollfd fds = { .fd = sock, .events = ZSOCK_POLLIN };

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&state_lock, K_FOREVER);
	ws_send_state(sock);
	k_mutex_unlock(&state_lock);

	while (true) {
		struct rgb_cmd cmd;
		uint64_t remaining = 0;
		uint32_t msg_type = 0;
		int len;
		int ret;

		if (zsock_poll(&fds, 1, -1) < 0) {
			break;
		}

		if (fds.revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
			break;
		}

		len = websocket_recv_msg(sock, buf, sizeof(buf) - 1, &msg_type,
					 &remaining, 0);
		if (len < 0) {
			if (len == -EAGAIN) {
				continue;
			}
			break;
		}

		if (msg_type & WEBSOCKET_FLAG_CLOSE) {
			/* Complete the close handshake so well-behaved
			 * clients don't hang waiting for the close ack.
			 */
			(void)websocket_send_msg(sock, buf, len,
						 WEBSOCKET_OPCODE_CLOSE, false,
						 true, SYS_FOREVER_MS);
			break;
		}

		if (msg_type & WEBSOCKET_FLAG_PING) {
			(void)websocket_send_msg(sock, buf, len,
						 WEBSOCKET_OPCODE_PONG, false,
						 true, SYS_FOREVER_MS);
			continue;
		}

		if (len == 0 || !(msg_type & (WEBSOCKET_FLAG_TEXT | WEBSOCKET_FLAG_BINARY))) {
			continue;
		}
		buf[len] = '\0';

		ret = json_obj_parse((char *)buf, len, rgb_cmd_descr,
				     ARRAY_SIZE(rgb_cmd_descr), &cmd);
		if (ret != BIT_MASK(ARRAY_SIZE(rgb_cmd_descr))) {
			LOG_WRN("ws[%d]: bad message (%d): %s", slot, ret, buf);
			continue;
		}

		k_mutex_lock(&state_lock, K_FOREVER);
		rgb[0] = (uint8_t)CLAMP(cmd.r, 0, 255);
		rgb[1] = (uint8_t)CLAMP(cmd.g, 0, 255);
		rgb[2] = (uint8_t)CLAMP(cmd.b, 0, 255);
		rgb_apply();
		ws_broadcast_state(sock);
		k_mutex_unlock(&state_lock);
	}

	LOG_INF("ws[%d]: connection closed", slot);

	(void)websocket_unregister(sock);
	ws_clients[slot] = -1;
}

static int ws_setup_cb(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	int slot;

	ARG_UNUSED(request_ctx);
	ARG_UNUSED(user_data);

	for (slot = 0; slot < WS_MAX_CLIENTS; slot++) {
		if (ws_clients[slot] < 0) {
			break;
		}
	}

	if (slot == WS_MAX_CLIENTS) {
		LOG_WRN("ws: no free client slot");
		return -ENOENT;
	}

	ws_clients[slot] = ws_socket;

	k_thread_create(&ws_threads[slot], ws_stacks[slot],
			K_THREAD_STACK_SIZEOF(ws_stacks[slot]),
			ws_client_thread, INT_TO_POINTER(slot), NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&ws_threads[slot], "ws_client");

	LOG_INF("ws[%d]: client connected", slot);

	return 0;
}

/* -------------------------- Firmware upload (OTA) ------------------------ */

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)

static struct flash_img_context fw_ctx;
static bool fw_in_progress;
static size_t fw_received;

static void fw_reboot_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_INF("Rebooting to apply firmware update");
	sys_reboot(SYS_REBOOT_COLD);
}

/* Delay the reboot so the HTTP response reaches the browser first. */
static K_WORK_DELAYABLE_DEFINE(fw_reboot_work, fw_reboot_handler);

static void fw_abort(void)
{
	fw_in_progress = false;
	fw_received = 0;
}

static int firmware_handler(struct http_client_ctx *client, enum http_transaction_status status,
			    const struct http_request_ctx *request_ctx,
			    struct http_response_ctx *response_ctx, void *user_data)
{
	static char resp[64];
	bool final = (status == HTTP_SERVER_REQUEST_DATA_FINAL);
	int ret;

	ARG_UNUSED(client);
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		LOG_WRN("Firmware upload aborted after %zu bytes", fw_received);
		fw_abort();
		return 0;
	}

	if (status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		return 0;
	}

	if (!fw_in_progress) {
		ret = flash_img_init(&fw_ctx);
		if (ret < 0) {
			LOG_ERR("flash_img_init failed: %d", ret);
			goto error;
		}
		fw_in_progress = true;
		fw_received = 0;
		LOG_INF("Firmware upload started");
	}

	if (request_ctx->data_len > 0) {
		ret = flash_img_buffered_write(&fw_ctx, request_ctx->data,
					       request_ctx->data_len, final);
		if (ret < 0) {
			LOG_ERR("flash write failed at %zu bytes: %d", fw_received, ret);
			goto error;
		}
		fw_received += request_ctx->data_len;
	}

	if (final) {
		/* Allow the dashboard served from ANOTHER node to read the
		 * result (cross-origin upload to this device).
		 */
		static const struct http_header cors = {
			.name = "Access-Control-Allow-Origin", .value = "*",
		};
		size_t total = fw_received;

		response_ctx->headers = &cors;
		response_ctx->header_count = 1;

		fw_abort();

		/* Mark slot1 for a TEST boot (not permanent): MCUboot swaps it
		 * in once; if the new image never confirms itself, the next
		 * reboot restores the current firmware.
		 */
		ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (ret < 0) {
			LOG_ERR("boot_request_upgrade failed: %d", ret);
			goto error;
		}

		LOG_INF("Firmware received (%zu bytes), marked for test boot", total);

		ret = snprintk(resp, sizeof(resp), "{\"ok\":true,\"bytes\":%zu}", total);
		response_ctx->body = (uint8_t *)resp;
		response_ctx->body_len = ret;
		response_ctx->final_chunk = true;

		k_work_schedule(&fw_reboot_work, K_SECONDS(2));
	}

	return 0;

error:
	fw_abort();
	ret = snprintk(resp, sizeof(resp), "{\"ok\":false}");
	response_ctx->body = (uint8_t *)resp;
	response_ctx->body_len = ret;
	response_ctx->final_chunk = true;
	return 0;
}

static struct http_resource_detail_dynamic fw_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = firmware_handler,
};

#endif /* CONFIG_MCUBOOT_IMG_MANAGER */

/* ------------------------------ HTTP service ----------------------------- */

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/html",
	},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static uint8_t ws_upgrade_buf[256];

static struct http_resource_detail_websocket ws_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_WEBSOCKET,
		/* HTTP/1.1 GET is used for the upgrade handshake */
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = ws_setup_cb,
	.data_buffer = ws_upgrade_buf,
	.data_buffer_len = sizeof(ws_upgrade_buf),
};

static uint16_t http_port = 80;

HTTP_SERVICE_DEFINE(t1s_http_service, NULL, &http_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_resource, t1s_http_service, "/",
		     &index_html_gz_resource_detail);

HTTP_RESOURCE_DEFINE(ws_resource, t1s_http_service, "/ws", &ws_resource_detail);

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
HTTP_RESOURCE_DEFINE(fw_resource, t1s_http_service, "/firmware", &fw_resource_detail);
#endif

/* --------------------------------- main ---------------------------------- */

int main(void)
{
	for (int i = 0; i < 3; i++) {
		if (!pwm_is_ready_dt(&led_pwm[i])) {
			LOG_ERR("PWM channel %d not ready", i);
			return -ENODEV;
		}
	}

	rgb_apply(); /* LED off at boot */

	net_if_set_mtu(net_if_get_default(), T1S_SAFE_MTU);

	http_server_start();

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
	/* We reached the point where the device is serving: confirm this image
	 * so MCUboot doesn't revert it on the next reboot. After an OTA update
	 * this runs in the swapped-in image — if it never gets here (e.g. the
	 * new firmware crashes before networking is up), the old image comes
	 * back automatically.
	 */
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret < 0) {
			LOG_ERR("Failed to confirm image: %d", ret);
		} else {
			LOG_INF("Image confirmed (will not revert)");
		}
	}
#endif

	LOG_INF("T1S device '%s' (PLCA node %u) — dashboard on http://<this-ip>/",
		CONFIG_APP_DEVICE_NAME, (unsigned int)PLCA_NODE_ID);

	return 0;
}
