/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "errno.h"
#include "codec.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, LOG_LEVEL_DBG);

static cJSON *json_parse_root_object(const char *input, size_t len)
{
	cJSON *obj = NULL;
	obj = cJSON_ParseWithLength(input, len);
	if (obj == NULL) {
		return NULL;
	}

	/* Verify that the incoming JSON string is an object. */
	if (!cJSON_IsObject(obj)) {
		return NULL;
	}

	return obj;
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static cJSON *json_create_reported_object(cJSON *obj, char* str) 
{
	// LOG_INF("skjer");
	cJSON *root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		return NULL;
	}

	cJSON *state_obj = cJSON_CreateObject();
	if (state_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 

	cJSON_AddItemToObject(root_obj, "state", state_obj);
	cJSON *reported_obj = cJSON_CreateObject();
	if (reported_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 

	cJSON *desired_obj = cJSON_CreateNull();
	if (desired_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 
	// LOG_INF("skjer2");
	cJSON_AddItemToObject(state_obj, "reported", reported_obj);
	cJSON_AddItemToObject(state_obj, "desired", desired_obj);

	// LOG_INF("skjer2");
	cJSON_AddItemToObject(reported_obj, str, obj);

	return root_obj;
}

static cJSON *json_get_object_in_state(cJSON *root_obj, char *str) 
{
	cJSON *state_obj = NULL;
	cJSON *desired_obj = NULL;
	state_obj = cJSON_GetObjectItem(root_obj, "state");
	if (state_obj == NULL) {
		return NULL;
	}

	desired_obj = cJSON_GetObjectItem(state_obj, str);
	if (desired_obj == NULL) {
		return NULL;
	}

	return desired_obj;
}

int codec_decode_version(const char *input, size_t len)
{
	cJSON *version_obj;
    cJSON *root_obj = json_parse_root_object(input, len);

	version_obj = cJSON_GetObjectItem(root_obj, "version");
	if (version_obj == NULL) {
		/* No version number present in message. */
		return -ENODATA;
	}

    cJSON_Delete(root_obj);

	return version_obj->valueint;
}

bool codec_decode_movement(char *id, const char *input, size_t len, struct codec_movement *movement)
{
    bool movement_config = false;

    cJSON *root_obj;
	cJSON *robots_obj;
	cJSON *robot_obj;
	cJSON *value_obj;
    
    root_obj = json_parse_root_object(input, len);
	
	// LOG_INF("delta: %s", cJSON_Print(root_obj));
	robots_obj = json_get_object_in_state(root_obj, "robots");
	if (robots_obj == NULL) {
		return movement_config;
	}

    robot_obj = json_object_decode(robots_obj, id);
    if (robot_obj == NULL) {
        return movement_config;
    }

    value_obj = json_object_decode(robot_obj, "driveTimeMs");
    if(value_obj != NULL) {
        movement->drive_time = value_obj->valueint;
        movement_config = true;
    }

    value_obj = json_object_decode(robot_obj, "angleDeg");
    if (value_obj != NULL) {
        movement->rotation = value_obj->valueint;
        movement_config = true;
    }


    cJSON_Delete(root_obj);

	return movement_config;
}

bool codec_decode_led(char *id, const char *input, size_t len, struct codec_led *led)
{
    bool led_config = false;

    cJSON *root_obj;
	cJSON *robots_obj;
	cJSON *robot_obj;
	cJSON *led_obj;
	cJSON *value_obj;
    
    root_obj = json_parse_root_object(input, len);
	robots_obj = json_get_object_in_state(root_obj, "robots");
	if (robots_obj == NULL) {
		return led_config;
	}

    robot_obj = json_object_decode(robots_obj, id);
    if (robot_obj == NULL) {
        return led_config;
    }
    
    led_obj = json_object_decode(robot_obj, "led");
    if(led_obj != NULL) {
        led_config = true;
        value_obj = cJSON_GetArrayItem(led_obj, 0);
        if(value_obj != NULL) {
            led->red = value_obj->valueint;
        }

        value_obj = cJSON_GetArrayItem(led_obj, 1);
        if(value_obj != NULL) {
            led->green = value_obj->valueint;
        }

        value_obj = cJSON_GetArrayItem(led_obj, 2);
        if(value_obj != NULL) {
            led->blue = value_obj->valueint;
        }

        value_obj = cJSON_GetArrayItem(led_obj, 3);
        if(value_obj != NULL) {
            led->time = value_obj->valueint;
        }
    }

    cJSON_Delete(root_obj);

	return led_config;
}

char* codec_encode_movement_report(char *id, struct codec_movement movement)
{
	char *msg;
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 
	
	cJSON *robot_obj = cJSON_CreateObject();
	if (robot_obj == NULL) {
		cJSON_Delete(robots_obj);
		return NULL;
	} 

	if (!cJSON_AddNumberToObject(robot_obj, "driveTimeMs", movement.drive_time)) {
		return NULL;
	}

	if (!cJSON_AddNumberToObject(robot_obj, "angleDeg", movement.rotation)) {
		return NULL;
	}

	// if (!cJSON_AddNumberToObject(robot_obj, "speed", movement.speed)) {
	// 	return NULL;
	// }

	cJSON_AddItemToObject(robots_obj, id, robot_obj);
	root_obj = json_create_reported_object(robots_obj, "robots");
	msg = cJSON_PrintUnformatted(root_obj);
	// LOG_INF("movereport: %s", cJSON_Print(root_obj));
	cJSON_Delete(root_obj);

	return msg;
}

char* codec_encode_led_report(char *id, uint8_t red, uint8_t green, uint8_t blue, uint16_t time)
{
	char *msg;
	int led[4];
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}

	cJSON *robot_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}


	cJSON_AddItemToObject(robots_obj, id, robot_obj);

	led[0] = red;
	led[1] = green;
	led[2] = blue;
	led[3] = time;

	cJSON *led_obj = cJSON_CreateIntArray(led, 4);
	if (!cJSON_AddItemToObject(robot_obj, "led", led_obj)) {
		return NULL;
	};

	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

char* codec_encode_revolution_count_report(char *id, uint8_t revolutions)
{
	char *msg;
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}

    cJSON *robot_obj = cJSON_CreateObject();
    if (robots_obj == NULL) {
        return NULL;
    } 


    cJSON_AddItemToObject(robots_obj, id, robot_obj);

    if (!cJSON_AddNumberToObject(robot_obj, "revolutionCount", revolutions)) {
        return NULL;
    }

	root_obj = json_create_reported_object(robots_obj, "robots");
	
	msg = cJSON_PrintUnformatted(root_obj);
	// LOG_INF("revoreport: %s", cJSON_Print(root_obj));
	cJSON_Delete(root_obj);
	return msg;
}

char* codec_encode_remove_robot_report(char *id) 
{
	char *msg;
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 

	cJSON *robot_obj = cJSON_CreateNull();
	if (robot_obj == NULL) {
		return NULL;
	}  

	cJSON_AddItemToObject(robots_obj, id, robot_obj);
	
	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

char* codec_encode_remove_robots_report(void) 
{
	char *msg;
	cJSON *root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		return NULL;
	}

	cJSON *state_obj = cJSON_CreateNull();
	if (state_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 

	cJSON_AddItemToObject(root_obj, "state", state_obj);

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

 