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
 *
 *
 ***********************************************************/
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include "platform_lib.h"

#include "arm64_accton_as4564_26p_int.h"
#include "arm64_accton_as4564_26p_log.h"

const char*
onlp_sysi_platform_get(void)
{
    return "arm64-accton-as4564-26p-r0";
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);
    if(onlp_file_read(rdata, 256, size, IDPROM_PATH) == ONLP_STATUS_OK) {
        if(*size == 256) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    /* 8 Thermal sensors on the chassis */
    for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++) {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* 5 LEDs on the chassis */
    for (i = 1; i <= CHASSIS_LED_COUNT; i++) {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* 2 PSUs on the chassis */
    for (i = 1; i <= CHASSIS_PSU_COUNT; i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 6 Fans on the chassis */
    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    return 0;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int ret = 0;
    int version = 0;
    int sub_version = 0;

    ret = get_cpld_attr_int("version", &version);
    if (ret < 0) {
        return ret;
    }

    ret = get_cpld_attr_int("sub_version", &sub_version);
    if (ret < 0) {
        return ret;
    }

    pi->cpld_versions = aim_fstrdup("%x.%x", version, sub_version);
    return ONLP_STATUS_OK;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
}

void
onlp_sysi_onie_data_free(uint8_t* data)
{
    aim_free(data);
}

/*******************************************************************
 * 1. U1(0x49)+U12(0x48)+U29(0x4C)+U88(0x4B) > 133.5 = 30% FAN speed
 * 2. U1(0x49)+U12(0x48)+U29(0x4C)+U88(0x4B) < 131 = 15% FAN speed
 * 3. U1(0x49)+U12(0x48)+U29(0x4C)+U88(0x4B) > 215 = 50% FAN speed
 * 4.One FAN failure = 60% FAN speed.
********************************************************************/
#define FAN_DUTY_ABSENT (60)
#define FAN_DUTY_MAX (50)
#define FAN_DUTY_MID (30)
#define FAN_DUTY_LOW (15)
#define FAN_DUTY_DEFAULT FAN_DUTY_MID

static int
sysi_fanctrl_fan_set_duty(int p, int num_of_fan)
{
    int i;
    int status = 0;

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        int ret = 0;

        ret = onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), p);
        if (ret < 0) {
            status = ret;
        }
    }

    return status;
}

static int
sysi_fanctrl_fan_status_policy(onlp_fan_info_t fi[CHASSIS_FAN_COUNT],
                              onlp_thermal_info_t ti[CHASSIS_THERMAL_COUNT],
                              int *adjusted,
                              int num_of_fan)
{
    int i;
    *adjusted = 0;

    /* Bring fan speed to FAN_DUTY_MAX if any fan is not operational */
    for (i = 0; i < num_of_fan; i++) {
        if (!(fi[i].status & ONLP_FAN_STATUS_FAILED)) {
            continue;
        }

        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(60, num_of_fan);
    }

    /* Bring fan speed to FAN_DUTY_MAX if fan is not present */
    for (i = 0; i < num_of_fan; i++) {
        if (fi[i].status & ONLP_FAN_STATUS_PRESENT) {
            continue;
        }

        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(60, num_of_fan);
    }

    return ONLP_STATUS_OK;
}

static int
sysi_fanctrl_thermal_sensor_policy(onlp_fan_info_t fi[CHASSIS_FAN_COUNT],
                                onlp_thermal_info_t ti[CHASSIS_THERMAL_COUNT],
                                int *adjusted,
                                int num_of_fan)
{
    int i;
    int fanduty;
    int sum_mcelsius = 0;

    for (i = (THERMAL_1_ON_MAIN_BROAD); i <= (THERMAL_4_ON_MAIN_BROAD); i++) {
        sum_mcelsius += ti[i-1].mcelsius;
    }

    *adjusted = 0;

    if (onlp_file_read_int(&fanduty, "%s%s", FAN_SYSFS_PATH, "fan1_pwm") < 0) {
        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
    }

    switch (fanduty) {
    case FAN_DUTY_LOW:
    {
        if (sum_mcelsius >= 133500) {
            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_MID, num_of_fan);
        }

        break;
    }
    case FAN_DUTY_MID:
    {
        if (sum_mcelsius >= 215000) {
            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
        }
        else if (sum_mcelsius <= 131000) {
            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_LOW, num_of_fan);
        }

        break;
    }
    case FAN_DUTY_MAX:
    {
        if (sum_mcelsius <= 131000) {
            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_LOW, num_of_fan);
        }

        break;
    }
    default:
        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_DEFAULT, num_of_fan);
    }

    /* Set as current speed to kick watchdog */
    return sysi_fanctrl_fan_set_duty(fanduty, num_of_fan);
}

typedef int (*fan_control_policy)(onlp_fan_info_t fi[CHASSIS_FAN_COUNT],
                                onlp_thermal_info_t ti[CHASSIS_THERMAL_COUNT],
                                int *adjusted,
                                int num_of_fan);

fan_control_policy fan_control_policies[] = {
    sysi_fanctrl_fan_status_policy,
    sysi_fanctrl_thermal_sensor_policy,
};

int
onlp_sysi_platform_manage_fans(void)
{
    int i, rc;
    int num_of_fan = CHASSIS_FAN_COUNT;
    onlp_fan_info_t fi[CHASSIS_FAN_COUNT];
    onlp_thermal_info_t ti[CHASSIS_THERMAL_COUNT];

    memset(fi, 0, sizeof(fi));
    memset(ti, 0, sizeof(ti));

    /* Get fan status
     */
    for (i = 0; i < num_of_fan; i++) {
        rc = onlp_fani_info_get(ONLP_FAN_ID_CREATE(i+1), &fi[i]);

        if (rc != ONLP_STATUS_OK) {
            sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    /* Get thermal sensor status
     */
    for (i = 0; i < CHASSIS_THERMAL_COUNT; i++) {
        rc = onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(i+1), &ti[i]);

        if (rc != ONLP_STATUS_OK) {
            sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    /* Apply thermal policy according the policy list,
     * If fan duty is adjusted by one of the policies, skip the others
     */
    for (i = 0; i < AIM_ARRAYSIZE(fan_control_policies); i++) {
        int adjusted = 0;

        rc = fan_control_policies[i](fi, ti, &adjusted, num_of_fan);
        if (!adjusted) {
            continue;
        }

        return rc;
    }

    return ONLP_STATUS_OK;
}
