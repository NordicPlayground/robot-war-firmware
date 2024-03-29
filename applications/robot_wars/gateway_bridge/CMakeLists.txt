#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dev_options.conf")
    message(WARNING "Using dev_options.conf. Ensure any critical settings are moved to prj.conf before committing to source control.")
    list(APPEND CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dev_options.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_bot)


# NORDIC SDK APP START
target_sources(app PRIVATE 
    src/main.c
    src/model_handler.c
    src/uart_handler.c
)

include_directories(
    src
)
# NORDIC SDK APP END
