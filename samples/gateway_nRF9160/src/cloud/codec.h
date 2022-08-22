/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CODEC_H_
#define CODEC_H_

/**
 * @brief CODEC
 * @defgroup Cloud Codec
 * @{
 */

#include <stdio.h>
#include <stdbool.h>
#include <cJSON.h>

struct codec_led {
    uint16_t time;
    uint8_t red, green, blue;
};

struct codec_movement {
	uint32_t drive_time;
	int32_t rotation;
	uint8_t speed;
};

int codec_decode_version(const char *input, size_t len);

bool codec_decode_movement(char *id, const char *input, size_t len, struct codec_movement *movement);

bool codec_decode_led(char *id, const char *input, size_t len, struct codec_led *led);

char* codec_encode_movement_report(char *id, struct codec_movement movement);

char* codec_encode_led_report(char *id, uint8_t red, uint8_t green, uint8_t blue, uint16_t blink_time);

char* codec_encode_revolution_count_report(char *id, uint8_t revolutions);

char* codec_encode_remove_robot_report(char *id);

char* codec_encode_remove_robots_report(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* CODEC_H_ */