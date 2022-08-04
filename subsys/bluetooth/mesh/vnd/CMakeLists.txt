#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_include_directories(${NRF_ROBOT_DIR}/subsys/bluetooth/mesh)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ID_SRV id_srv.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_ID_CLI id_cli.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_MOVEMENT_SRV movement_srv.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_MOVEMENT_CLI movement_cli.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_ROBOT_SRV robot_srv.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_ROBOT_CLI robot_cli.c)
