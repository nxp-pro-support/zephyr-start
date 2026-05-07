/*
 * Copyright 2026 Wavenumber LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#define DEFAULT_PERIOD_MS 1000U
#define MIN_PERIOD_MS 20U
#define MAX_PERIOD_MS 5000U
#define CAN_BITRATE 500000U

#define HEARTBEAT_ID 0x156U
#define STATUS_ID 0x321U
#define SENSOR_EXT_ID 0x18FF1560U
#define TX_QUEUE_DEPTH 3U
#define RECOVERY_FAILURE_LIMIT 4U
#define AUTO_RECOVERY_BACKOFF_MS 5000U

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

K_SEM_DEFINE(tx_queue_sem, TX_QUEUE_DEPTH, TX_QUEUE_DEPTH);

static volatile bool generator_enabled = true;
static volatile uint32_t period_ms = DEFAULT_PERIOD_MS;
static uint32_t tx_sequence;
static uint32_t tx_enqueued;
static uint32_t tx_ok;
static uint32_t tx_err;
static uint32_t tx_busy;
static uint32_t tx_fail_streak;
static uint32_t tx_recoveries;
static int64_t next_auto_recovery_ms;

static const char *can_state_name(enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return "error-active";
	case CAN_STATE_ERROR_WARNING:
		return "error-warning";
	case CAN_STATE_ERROR_PASSIVE:
		return "error-passive";
	case CAN_STATE_BUS_OFF:
		return "bus-off";
	case CAN_STATE_STOPPED:
		return "stopped";
	default:
		return "unknown";
	}
}

static int parse_period_ms(const char *text, uint32_t *value)
{
	char *end;
	unsigned long parsed;

	errno = 0;
	parsed = strtoul(text, &end, 10);
	if (errno != 0 || *end != '\0' || parsed < MIN_PERIOD_MS || parsed > MAX_PERIOD_MS) {
		return -EINVAL;
	}

	*value = (uint32_t)parsed;
	return 0;
}

static void tx_callback(const struct device *dev, int error, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (error == 0) {
		tx_ok++;
		tx_fail_streak = 0U;
		if (led.port != NULL) {
			gpio_pin_toggle_dt(&led);
		}
	} else {
		tx_err++;
		tx_fail_streak++;
	}

	k_sem_give(&tx_queue_sem);
}

static int send_frame(const struct can_frame *frame)
{
	int ret;

	ret = k_sem_take(&tx_queue_sem, K_NO_WAIT);
	if (ret != 0) {
		tx_busy++;
		tx_fail_streak++;
		return ret;
	}

	ret = can_send(can_dev, frame, K_NO_WAIT, tx_callback, NULL);
	if (ret != 0) {
		tx_err++;
		tx_fail_streak++;
		k_sem_give(&tx_queue_sem);
		return ret;
	}

	tx_enqueued++;
	return 0;
}

static int send_demo_frames(void)
{
	struct can_frame heartbeat = {
		.id = HEARTBEAT_ID,
		.dlc = 2,
	};
	struct can_frame status = {
		.id = STATUS_ID,
		.dlc = 8,
	};
	struct can_frame sensor = {
		.id = SENSOR_EXT_ID,
		.dlc = 8,
		.flags = CAN_FRAME_IDE,
	};
	uint32_t seq = tx_sequence++;
	uint32_t uptime_ms = k_uptime_get_32();
	int ret;

	heartbeat.data[0] = (uint8_t)(seq & 0xffU);
	heartbeat.data[1] = (seq & 1U) != 0U ? 0xa5U : 0x5aU;

	status.data[0] = 0xf0U;
	status.data[1] = 0x0dU;
	sys_put_le16((uint16_t)seq, &status.data[2]);
	sys_put_le32(uptime_ms, &status.data[4]);

	sys_put_le16((uint16_t)seq, &sensor.data[0]);
	sys_put_le16((uint16_t)(2200U + ((seq % 80U) * 5U)), &sensor.data[2]);
	sys_put_le16((uint16_t)(3300U + (seq % 25U)), &sensor.data[4]);
	sensor.data[6] = (uint8_t)(seq & 0xffU);
	sensor.data[7] = 0xc3U;

	ret = send_frame(&heartbeat);
	if (ret != 0) {
		return ret;
	}

	ret = send_frame(&status);
	if (ret != 0) {
		return ret;
	}

	return send_frame(&sensor);
}

static void print_status(const struct shell *sh)
{
	struct can_bus_err_cnt err_cnt = {0};
	enum can_state state = CAN_STATE_STOPPED;
	int ret;

	ret = can_get_state(can_dev, &state, &err_cnt);
	if (ret != 0) {
		shell_error(sh, "can_get_state failed: %d", ret);
		return;
	}

	shell_print(sh, "device: %s", can_dev->name);
	shell_print(sh, "bitrate: %u", CAN_BITRATE);
	shell_print(sh, "generator: %s, period: %u ms",
		    generator_enabled ? "running" : "stopped", period_ms);
	shell_print(sh, "tx enqueued: %u, tx ok: %u, tx err: %u, tx busy: %u, next seq: %u",
		    tx_enqueued, tx_ok, tx_err, tx_busy, tx_sequence);
	shell_print(sh, "tx recoveries: %u, fail streak: %u", tx_recoveries, tx_fail_streak);
	shell_print(sh, "state: %s, rx errors: %u, tx errors: %u",
		    can_state_name(state), err_cnt.rx_err_cnt, err_cnt.tx_err_cnt);
}

static int cmd_can_gen_start(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		uint32_t value;
		int ret = parse_period_ms(argv[1], &value);

		if (ret != 0) {
			shell_error(sh, "period must be %u..%u ms", MIN_PERIOD_MS, MAX_PERIOD_MS);
			return ret;
		}

		period_ms = value;
	}

	generator_enabled = true;
	shell_print(sh, "CAN generator running at %u ms", period_ms);
	return 0;
}

static int cmd_can_gen_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	generator_enabled = false;
	shell_print(sh, "CAN generator stopped");
	return 0;
}

static int cmd_can_gen_rate(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t value;
	int ret;

	if (argc != 2) {
		shell_error(sh, "usage: can_gen rate <period_ms>");
		return -EINVAL;
	}

	ret = parse_period_ms(argv[1], &value);
	if (ret != 0) {
		shell_error(sh, "period must be %u..%u ms", MIN_PERIOD_MS, MAX_PERIOD_MS);
		return ret;
	}

	period_ms = value;
	shell_print(sh, "period set to %u ms", period_ms);
	return 0;
}

static int cmd_can_gen_burst(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t count = 10U;
	int ret = 0;

	if (argc > 1) {
		char *end;
		unsigned long parsed;

		errno = 0;
		parsed = strtoul(argv[1], &end, 10);
		if (errno != 0 || *end != '\0' || parsed == 0UL || parsed > 1000UL) {
			shell_error(sh, "count must be 1..1000");
			return -EINVAL;
		}

		count = (uint32_t)parsed;
	}

	for (uint32_t i = 0U; i < count; i++) {
		ret = send_demo_frames();
		if (ret != 0) {
			shell_error(sh, "send failed at burst frame set %u: %d", i, ret);
			break;
		}

		k_sleep(K_MSEC(10));
	}

	shell_print(sh, "burst complete, requested %u frame sets", count);
	return ret;
}

static void reset_tx_semaphore(void)
{
	k_sem_reset(&tx_queue_sem);

	for (uint32_t i = 0U; i < TX_QUEUE_DEPTH; i++) {
		k_sem_give(&tx_queue_sem);
	}
}

static int restart_can_controller(void)
{
	int ret;

	ret = can_stop(can_dev);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}

	reset_tx_semaphore();

	ret = can_start(can_dev);
	if (ret != 0) {
		return ret;
	}

	tx_fail_streak = 0U;
	tx_recoveries++;
	return 0;
}

static int cmd_can_gen_restart(const struct shell *sh, size_t argc, char **argv)
{
	bool was_enabled = generator_enabled;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	generator_enabled = false;

	ret = restart_can_controller();
	if (ret != 0) {
		shell_error(sh, "CAN restart failed: %d", ret);
		generator_enabled = was_enabled;
		return ret;
	}

	generator_enabled = was_enabled;
	shell_print(sh, "CAN controller restarted");
	return 0;
}

static int cmd_can_gen_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	print_status(sh);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(can_gen_cmds,
	SHELL_CMD_ARG(start, NULL, "[period_ms]", cmd_can_gen_start, 1, 1),
	SHELL_CMD_ARG(stop, NULL, NULL, cmd_can_gen_stop, 1, 0),
	SHELL_CMD_ARG(rate, NULL, "<period_ms>", cmd_can_gen_rate, 2, 0),
	SHELL_CMD_ARG(burst, NULL, "[count]", cmd_can_gen_burst, 1, 1),
	SHELL_CMD_ARG(restart, NULL, NULL, cmd_can_gen_restart, 1, 0),
	SHELL_CMD_ARG(status, NULL, NULL, cmd_can_gen_status, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(can_gen, &can_gen_cmds, "FRDM CAN traffic generator", NULL);

static void maybe_auto_recover(int send_ret)
{
	int64_t now = k_uptime_get();
	int ret;

	if (tx_fail_streak < RECOVERY_FAILURE_LIMIT || now < next_auto_recovery_ms) {
		return;
	}

	printk("CAN TX stalled after %u failed attempts (last err %d). Restarting and retrying; "
	       "check that another active CAN node ACKs frames.\n",
	       tx_fail_streak, send_ret);

	ret = restart_can_controller();
	if (ret != 0) {
		printk("CAN auto-restart failed: %d; will retry.\n", ret);
	} else {
		printk("CAN auto-restart complete; retrying.\n");
	}

	next_auto_recovery_ms = k_uptime_get() + AUTO_RECOVERY_BACKOFF_MS;
}

int main(void)
{
	int ret;

	if (!device_is_ready(can_dev)) {
		printk("CAN device %s is not ready\n", can_dev->name);
		return 0;
	}

	ret = can_set_bitrate(can_dev, CAN_BITRATE);
	if (ret != 0) {
		printk("Failed to set CAN bitrate to %u: %d\n", CAN_BITRATE, ret);
		return 0;
	}

	ret = can_start(can_dev);
	if (ret != 0) {
		printk("Failed to start CAN controller: %d\n", ret);
		return 0;
	}

	if (led.port != NULL && gpio_is_ready_dt(&led)) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			printk("Failed to configure LED: %d\n", ret);
		}
	}

	printk("FRDM CAN traffic generator ready on %s at %u bit/s\n", can_dev->name, CAN_BITRATE);
	printk("Frames: std 0x%03x, std 0x%03x, ext 0x%08x every %u ms\n",
	       HEARTBEAT_ID, STATUS_ID, SENSOR_EXT_ID, period_ms);
	printk("Shell: can_gen status | can_gen stop | can_gen start [ms] | can_gen burst [n] | can_gen restart\n");

	while (true) {
		if (generator_enabled) {
			ret = send_demo_frames();
			if (ret != 0) {
				maybe_auto_recover(ret);
			}
			k_sleep(K_MSEC(period_ms));
		} else {
			k_sleep(K_MSEC(100));
		}
	}
}
