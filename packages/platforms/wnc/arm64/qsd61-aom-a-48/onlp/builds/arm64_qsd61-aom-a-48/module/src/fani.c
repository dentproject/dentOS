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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <onlplib/mmap.h>
#include <fcntl.h>
#include "platform_lib.h"

#define PREFIX_PATH_ON_MAIN_BOARD   "/sys/bus/i2c/devices/2-0077/"

#define MAX_FAN_SPEED     17600
#define FAN_MIN_PERCENTAGE 50

#define PROJECT_NAME
#define LEN_FILE_NAME 80
#define FAN_NODE_MAX_NAME_LEN  128

#define FAN_RESERVED        0
#define FAN_1_ON_MAIN_BOARD	1
#define FAN_2_ON_MAIN_BOARD	2
#define FAN_3_ON_MAIN_BOARD	3
#define FAN_4_ON_MAIN_BOARD	4
#define FAN_5_ON_MAIN_BOARD	5
#define FAN_6_ON_MAIN_BOARD	6

typedef struct fan_path_S
{
    char present[LEN_FILE_NAME];
    char status[LEN_FILE_NAME];
    char speed[LEN_FILE_NAME];
	char direction[LEN_FILE_NAME];
    char ctrl_speed[LEN_FILE_NAME];
    char r_speed[LEN_FILE_NAME];
}fan_path_T;

#define _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) \
    { #prj"fan"#id"_present", #prj"fan"#id"_fault", #prj"fan"#id"_front_speed_rpm", \
      #prj"fan"#id"_direction", #prj"fan"#id"_duty_cycle_percentage", #prj"fan"#id"_rear_speed_rpm" }

#define MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id)

static fan_path_T fan_path[] =  /* must map with onlp_fan_id */
{
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_RESERVED),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_1_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_2_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_3_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_4_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_5_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_6_ON_MAIN_BOARD)
};

#define MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_MAIN_BOARD), "Chassis Fan "#id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(1),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(2),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(3),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(4),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(5),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(6)
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define OPEN_READ_FILE(fd,fullpath,data,nbytes,len) \
    DEBUG_PRINT("[Debug][%s][%d][openfile: %s]\n", __FUNCTION__, __LINE__, fullpath); \
    if ((fd = open(fullpath, O_RDONLY)) == -1)  \
       return ONLP_STATUS_E_INTERNAL;           \
    if ((len = read(fd, r_data, nbytes)) <= 0){ \
        close(fd);                              \
        return ONLP_STATUS_E_INTERNAL;}         \
    DEBUG_PRINT("[Debug][%s][%d][read data: %s]\n", __FUNCTION__, __LINE__, r_data); \
    if (close(fd) == -1)                        \
        return ONLP_STATUS_E_INTERNAL

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    // FIXME: WARNING FREEZE
    if ( 0 ) {
        printf("%d\t%s", finfo[0].status, fan_path[0].speed);
    }
    // END WARNING FREEZE
    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info)
{
    int   present = 0, direction = 0;
    int   value;
	char  fanId_hex[10] = {0};
    char  path[FAN_NODE_MAX_NAME_LEN] = {0};

	/* set fan fid */
	sprintf(path, "%s%s", PREFIX_PATH_ON_MAIN_BOARD, "fan_tach_sel");
	sprintf(fanId_hex, "%x", fid-1);
	if (onlp_file_write((uint8_t*)fanId_hex, strlen(fanId_hex), path) < 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }


    /* If fan is present, value is 0. */
    if (present == 0)
        info->status |= ONLP_FAN_STATUS_PRESENT;

    /* Fan direciton - 0: AFO (airflow out); F2B (Front-to-Back)
                       1: AFI (airflow in); B2F (Back-to-Front) */
    if (direction == 0)
        info->status |= ONLP_FAN_STATUS_F2B;
    if (direction == 1)
        info->status |= ONLP_FAN_STATUS_B2F;

    /* get front fan speed */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s""fan_tach_a", PREFIX_PATH_ON_MAIN_BOARD);
    DEBUG_PRINT("Fan (%d), front speed path = (%s)", fid, path);

    if (onlp_file_read_int_hex(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }
    info->rpm = value;

    /* get rear fan speed */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s""fan_tach_b", PREFIX_PATH_ON_MAIN_BOARD);
    DEBUG_PRINT("Fan (%d), rear speed path = (%s)", fid, path);

    if (onlp_file_read_int_hex(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* take the min value from front/rear fan speed */
    if (info->rpm > value) {
        info->rpm = value;
    }

	/* change rpm format 0~255 to 0~17600 */
	if(info->rpm==0xff)
		info->rpm = MAX_FAN_SPEED;
	else
		info->rpm = info->rpm*69; /* (17600/255) */

    /* get speed percentage from rpm  */
    info->percentage = ((info->rpm) * 100) / MAX_FAN_SPEED;

    /* get fan fault status */
    if ((info->status & ONLP_FAN_STATUS_PRESENT) &&
        (info->rpm == 0)) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }

    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int rc = 0;
    int fid;
    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);
    *info = finfo[fid];

    switch (fid)
    {
        case FAN_1_ON_MAIN_BOARD:
        case FAN_2_ON_MAIN_BOARD:
        case FAN_3_ON_MAIN_BOARD:
        case FAN_4_ON_MAIN_BOARD:
        case FAN_5_ON_MAIN_BOARD:
        case FAN_6_ON_MAIN_BOARD:
            rc =_onlp_fani_info_get_fan(fid, info);
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
    }

    return rc;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int  fid;
	char fanId_hex[10] = {0};
    int  percent_v;
    char path[FAN_NODE_MAX_NAME_LEN] = {0};
    char percent_hex[10] = {0};

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p<50 (protect system) */
    if (p < FAN_MIN_PERCENTAGE){
        return ONLP_STATUS_E_INVALID;
    }

    switch (fid)
    {
        case FAN_1_ON_MAIN_BOARD:
        case FAN_2_ON_MAIN_BOARD:
        case FAN_3_ON_MAIN_BOARD:
        case FAN_4_ON_MAIN_BOARD:
        case FAN_5_ON_MAIN_BOARD:
        case FAN_6_ON_MAIN_BOARD:
			percent_v = 0x80 + (int)((p-50)*2.6);
			if (percent_v > 0xFF)
            	percent_v = 0xFF;
            sprintf(percent_hex, "%x", percent_v);
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

	/* set fan fid */
	sprintf(path, "%s%s", PREFIX_PATH_ON_MAIN_BOARD, "fan_tach_sel");
	sprintf(fanId_hex, "%x", fid-1);
	if (onlp_file_write((uint8_t*)fanId_hex, strlen(fanId_hex), path) < 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

	/* set fan rwm percent */
	memset(path, 0, sizeof(path));
	sprintf(path, "%s%s", PREFIX_PATH_ON_MAIN_BOARD, "pwm_ctrl");

	DEBUG_PRINT("Fan path = (%s), Percent hex = (%s)", path, percent_hex);

	/* set fan fid */
    if (onlp_file_write((uint8_t*)percent_hex, strlen(percent_hex), path) < 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

