/*
 * Copyright (c) 2021 Alexander Kozhinov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <drivers/can.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(can_module, LOG_LEVEL_DBG);

#include "can_module.h"

#define RX_MSG_HND_ID			0x456  // rx message handler id
#define TX_MSG_HND_ID			0x458  // tx message handler id

#define MAX_MSG_LEN_BYTES		8U
#define MSGQ_LEN_MSGS			4U  // message queue len (in messages)

const struct device *can_dev;

struct zcan_work rx_work;
struct k_work state_change_work;

enum can_state current_state;
struct can_bus_err_cnt current_err_cnt;

static struct zcan_filter rx_msg_hnd_filter =
{
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = RX_MSG_HND_ID,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

CAN_DEFINE_MSGQ(rx_msgq, MSGQ_LEN_MSGS);  // recieve message queue

static void rx_msg_hnd_func(struct zcan_frame *msg, void *unused);
static int tx_msg_hnd_func(struct zcan_frame *msg, void *unused);

static void tx_irq_callback(uint32_t error_flags, void *arg);

static void state_change_work_handler(struct k_work *work);

static char *state_to_str(enum can_state state);
static void state_change_isr(enum can_state state,
			     struct can_bus_err_cnt err_cnt);

static void state_change_work_handler(struct k_work *work)
{
	LOG_INF("State Change ISR\n"
		"state: %s\n"
		"rx error count: %d\n"
		"tx error count: %d\n",
		state_to_str(current_state),
		current_err_cnt.rx_err_cnt, current_err_cnt.tx_err_cnt);

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	if (current_state == CAN_BUS_OFF) {
		LOG_INF("Recover from bus-off!");

		if (can_recover(can_dev, K_MSEC(100) != 0)) {
			LOG_WRN("Recovery timed out!");
		}
	}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
}

static void state_change_isr(enum can_state state,
			     struct can_bus_err_cnt err_cnt)
{
	current_state = state;
	current_err_cnt = err_cnt;
	k_work_submit(&state_change_work);
}

static void rx_msg_hnd_func(struct zcan_frame *msg, void *unused)
{
	ARG_UNUSED(unused);

	if (msg->dlc != MAX_MSG_LEN_BYTES) {
		LOG_WRN("wrong data length of %u bytes!"
			" Expecting %u bytes!\n", msg->dlc, MAX_MSG_LEN_BYTES);
		return;
	}

	LOG_INF("-----------------");
	printk("\nhex input: ");
	for (size_t i = 0; i < msg->dlc; i++) {
		LOG_INF("msg->data[%d]: 0x%x", i, msg->data[i]);
		printk("%x", msg->data[i]);
	}
	printk("\n");
	LOG_INF("-----------------");

	// ----------------------------------------------
	struct zcan_frame tx_frame = {
		.id_type = CAN_STANDARD_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.id = TX_MSG_HND_ID,
		.dlc = MAX_MSG_LEN_BYTES
	};
	tx_frame.data[0] = 11;
	tx_frame.data[1] = 22;
	tx_frame.data[2] = 33;
	tx_frame.data[3] = 44;
	tx_frame.data[4] = 55;
	tx_frame.data[5] = 66;
	tx_frame.data[6] = 77;
	tx_frame.data[7] = 88;

	const int ret = tx_msg_hnd_func(&tx_frame, NULL);
	if (ret != CAN_TX_OK) {
		LOG_ERR("Sending failed [%d]", ret);
	}
}

static void tx_irq_callback(uint32_t error_flags, void *arg)
{
	char *sender = (char *)arg;

	if (error_flags) {
		LOG_ERR("Sendig failed [%d]\nSender: %s\n",
			error_flags, sender);
	}
}

static int tx_msg_hnd_func(struct zcan_frame *msg, void *unused)
{
	ARG_UNUSED(unused);

	return can_send(can_dev, msg, K_FOREVER,
			tx_irq_callback, "tx_msg_hnd_func");
}

static char *state_to_str(enum can_state state)
{
	switch (state) {
	case CAN_ERROR_ACTIVE:
		return "error-active";
	case CAN_ERROR_PASSIVE:
		return "error-passive";
	case CAN_BUS_OFF:
		return "bus-off";
	default:
		return "unknown";
	}
}

void can_module_init(void)
{
	int ret = 0;

	can_dev = device_get_binding(DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL);
	if (!can_dev) {
		LOG_ERR("can device driver not found!");
		return;
	}

#ifdef CONFIG_LOOPBACK_MODE
	can_set_mode(can_dev, CAN_LOOPBACK_MODE);
#endif  /* CONFIG_LOOPBACK_MODE */

	k_work_init(&state_change_work, state_change_work_handler);

	ret = can_attach_workq(can_dev, &k_sys_work_q, &rx_work,
				rx_msg_hnd_func, NULL, &rx_msg_hnd_filter);
	if (ret == CAN_NO_FREE_FILTER) {
		LOG_ERR("no filter available!");
		return;
	}

	can_register_state_change_isr(can_dev, state_change_isr);
}

void can_module_destroy()
{
	can_detach(can_dev, rx_msg_hnd_filter.id);
}
