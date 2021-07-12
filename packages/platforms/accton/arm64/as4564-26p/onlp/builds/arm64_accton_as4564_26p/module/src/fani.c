/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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
#include <onlplib/file.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define FAN_RPM_FORMAT           "fan%d_input"
#define FAN_FAULT_FORMAT         "fan%d_fault"
#define FAN_PWM_FORMAT           "fan%d_pwm"

enum fan_id {
    FAN_1_ON_FAN_BOARD = 1,
    FAN_2_ON_FAN_BOARD,
    FAN_3_ON_FAN_BOARD,
    FAN_4_ON_FAN_BOARD,
    FAN_5_ON_FAN_BOARD,
    FAN_6_ON_FAN_BOARD,
};

#define CHASSIS_FAN_INFO(fid)        \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan - "#fid, 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(1),
    CHASSIS_FAN_INFO(2),
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

int
onlp_fani_get_fan_attr_int(int fid, char *fmt, int *value)
{
    char attr_name[32] = {0};
    sprintf(attr_name, fmt, fid);
    return onlp_file_read_int(value, "%s%s", FAN_SYSFS_PATH, attr_name);
}

int
onlp_fani_set_fan_attr_int(int fid, char *fmt, int value)
{
    char attr_name[32] = {0};
    sprintf(attr_name, fmt, fid);
    return onlp_file_write_int(value, "%s%s", FAN_SYSFS_PATH, attr_name);
}

static int
_onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info)
{
    int value, ret;

    /* get fan present status
     */
    info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get fan fault status (turn on when any one fails)
     */
    ret = onlp_fani_get_fan_attr_int(fid, FAN_FAULT_FORMAT, &value);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }


    /* get fan direction (both : the same)
     */
    info->status |= ONLP_FAN_STATUS_F2B;


    /* get fan speed
     */
    ret = onlp_fani_get_fan_attr_int(fid, FAN_RPM_FORMAT, &value);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    info->rpm = value;


    /* get speed percentage from rpm
     */
    ret = onlp_file_read_int(&value, "%s%s", FAN_SYSFS_PATH, "fan_max_rpm");
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    info->percentage = (info->rpm * 100)/value;

    return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
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
        case FAN_1_ON_FAN_BOARD:
        case FAN_2_ON_FAN_BOARD:
            rc =_onlp_fani_info_get_fan(fid, info);
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
    }

    return rc;
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
    char *path = NULL;

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        return ONLP_STATUS_E_INVALID;
    }

    if (onlp_fani_set_fan_attr_int(fid, FAN_PWM_FORMAT, p) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
