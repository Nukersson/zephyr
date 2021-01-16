/*
 * Copyright (c) 2021 Alexander Kozhinov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_module, LOG_LEVEL_DBG);

#include "uart_module.h"

const struct device *uart_dev;

static void uart_callback_handler(const struct device *dev,
				  struct uart_event *evt,
				  void *user_data);


static void uart_callback_handler(const struct device *dev,
				  struct uart_event *evt,
				  void *user_data)
{
	LOG_INF("uart_callback");
}

void uart_module_init()
{
	int ret = 0;

	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if (!uart_dev) {
		LOG_ERR("uart device driver not found!");
		return;
	}

	ret = uart_callback_set(uart_dev, uart_callback_handler, NULL);
	if (ret != 0) {
		LOG_ERR("can not set uart event handler function: "
			"not supported!");
		return;
	}
}

void uart_module_destroy()
{
}
