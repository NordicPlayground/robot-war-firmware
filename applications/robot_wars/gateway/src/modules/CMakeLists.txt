#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
	ui_module.c
	modem_module.c
	cloud_module.c
	robot_module.c
	mesh_uart_module.c
)
