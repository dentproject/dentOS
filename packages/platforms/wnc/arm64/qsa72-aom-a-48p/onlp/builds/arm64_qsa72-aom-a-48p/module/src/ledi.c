/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 MiTAC Computing Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <onlplib/mmap.h>

#include "platform_lib.h"

#define sfp_led_path "/sys/class/gpio/gpio%d/value"
#define prefix_path "/sys/bus/i2c/devices/2-0077/"
#define filename    "brightness"
#define NUM_OF_SFP_PORT 4

int led_mapping_gpio_base_table[NUM_OF_SFP_PORT] =
{
    508, 509, 510, 511
};

#define SYSFS_NODE_PATH_MAX_LEN  128

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* LED related data
 */
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYS,
};

enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_AMBER,
    LED_MODE_GREEN,
    LED_MODE_GREEN_BLINK
};

enum sfp_led_light_mode {
	SFP_LED_MODE_ORANGE = 0,
	SFP_LED_MODE_GREAN
};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    {
        { ONLP_LED_ID_CREATE(LED_SYS), "Chassis LED 1 (SYS LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    }
};


static int driver_to_onlp_led_mode(enum onlp_led_id id, int value)
{
	switch (id)
    {
        case LED_SYS:
            if (value == LED_MODE_OFF)
                return ONLP_LED_MODE_OFF;
            if (value == LED_MODE_GREEN)
                return ONLP_LED_MODE_GREEN;
			if (value == LED_MODE_AMBER)
                return ONLP_LED_MODE_ORANGE;
            if (value == LED_MODE_GREEN_BLINK)
                return ONLP_LED_MODE_GREEN_BLINKING;
            AIM_LOG_ERROR("Sys led value (%d) is illegal \r\n", value);
            return ONLP_LED_MODE_OFF;
        default:
            AIM_LOG_ERROR("led(%d) not matched any ONLP_LED_MODE \r\n", id);
            break;
    }
    return ONLP_LED_MODE_OFF;
}

static int onlp_to_driver_led_mode(enum onlp_led_id id, onlp_led_mode_t onlp_led_mode)
{
    switch (id)
    {
        case LED_SYS:
            if (onlp_led_mode == ONLP_LED_MODE_OFF)
                return LED_MODE_OFF;
            if (onlp_led_mode == ONLP_LED_MODE_GREEN)
                return LED_MODE_GREEN;
			if (onlp_led_mode == ONLP_LED_MODE_ORANGE)
                return LED_MODE_AMBER;
            if (onlp_led_mode == ONLP_LED_MODE_GREEN_BLINKING)
                return LED_MODE_GREEN_BLINK;
            AIM_LOG_ERROR("Illegal led mode (%d) for sys led \r\n", onlp_led_mode);
            return LED_MODE_OFF;
        default:
            AIM_LOG_ERROR("led(%d) not matched any ONLP_LED_MODE \r\n", id);
            break;
    }
    return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    //printf("Hello, this is %s() in %s.\n", __func__, __FILE__);
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
	int local_id;
    int value;
    char node_path[SYSFS_NODE_PATH_MAX_LEN] = {0};

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

	/* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];

	/* get sysfs node */
	if(local_id==LED_SYS) {
    	sprintf(node_path, "%s/%s", prefix_path, "sys_led");
	}

	/* Get LED mode */
    if (onlp_file_read_int_hex(&value, node_path) < 0) {
        DEBUG_PRINT("%s(%d)\r\n", __FUNCTION__, __LINE__);
        return ONLP_STATUS_E_INTERNAL;
    }

	info->mode = driver_to_onlp_led_mode(local_id, value);

	/* Set the on/off status */
    if (info->mode != ONLP_LED_MODE_OFF) {
        info->status |= ONLP_LED_STATUS_ON;
    }

    return ONLP_STATUS_OK;
}

/*
 * Turn an LED on or off.
 *
 * This function will only be called if the LED OID supports the ONOFF
 * capability.
 *
 * What 'on' means in terms of colors or modes for multimode LEDs is
 * up to the platform to decide. This is intended as baseline toggle mechanism.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
	VALIDATE(id);

    if (!on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function puts the LED into the given mode. It is a more functional
 * interface for multimode LEDs.
 *
 * Only modes reported in the LED's capabilities will be attempted.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
	int  local_id;
    char node_path[SYSFS_NODE_PATH_MAX_LEN] = {0};

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    if(local_id==LED_SYS) {
    	sprintf(node_path, "%s/%s", prefix_path, "sys_led");
	}

    if (onlp_file_write_int(onlp_to_driver_led_mode(local_id, mode), node_path) != 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/*
 * Generic LED ioctl interface.
 */
int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Additional Functions
 */
int
onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_ledi_char_set(onlp_oid_t id, char c)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

