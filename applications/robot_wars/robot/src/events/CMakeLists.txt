#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

include_directories(.)

target_sources(app PRIVATE
	mesh_module_event.c
	motor_module_event.c
	ui_module_event.c
)
