#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BT_MESH mesh)

if(CONFIG_BT)
    zephyr_link_libraries(INTERFACE SOFTDEVICE_CONTROLLER_LIB_HEADERS)
endif()