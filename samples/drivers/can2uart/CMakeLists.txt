#
# Copyright (c) 2018 Alexander Kozhinov
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.17)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(CAN2UART)

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/can_module.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/uart_module.c
)
