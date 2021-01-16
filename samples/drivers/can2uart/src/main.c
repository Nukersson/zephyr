/*
 * Copyright (c) 2021 Alexander Kozhinov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "can_module.h"
#include "uart_module.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

void main(void)
{
	LOG_INF("Starting...");

	can_module_init();
	uart_module_init();

	LOG_INF("Running...");
}
