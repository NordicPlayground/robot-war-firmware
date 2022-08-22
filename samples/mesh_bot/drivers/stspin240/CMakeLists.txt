cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_STSPIN240)
    target_sources(app PRIVATE
        stspin240_motor.c
    )
    include_directories(.)
endif()