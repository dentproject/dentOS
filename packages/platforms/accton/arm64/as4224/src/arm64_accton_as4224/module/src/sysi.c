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

#include "arm64_accton_as4224_int.h"
#include "arm64_accton_as4224_log.h"

const char*
onlp_sysi_platform_get(void)
{
    as4224_platform_id_t pid = get_platform_id();

    switch (pid)
    {
    case AS4224_48X: return "arm64-accton-as5114-48x-r0";
    case AS4224_52P: return "arm64-accton-as4224-52p-r0";
    case AS4224_52T: return "arm64-accton-as4224-52t-r0";
    case AS4224_52T_DAC: return "arm64-accton-as4224-52t-dac-r0";
    default: break;
    }

    return "Unknown Platform";
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
    for (i = 1; i <= platform_psu_count(); i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 6 Fans on the chassis */
    for (i = 1; i <= platform_fan_count(); i++) {
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

#define FAN_DUTY_MAX (100)
#define FAN_DUTY_MID  (75)
#define FAN_DUTY_LOW  (50)
#define MAX_CHASSIS_FAN_COUNT 6

static int
sysi_fanctrl_fan_set_duty(int p, int num_of_fan)
{
    int i;
    int status = 0;

    /* Bring fan speed to FAN_DUTY_MAX if fan is not present */
    for (i = 1; i <= num_of_fan; i++) {
        int ret = 0;

        ret = onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), p);
        if (ret < 0) {
            status = ret;
        }
    }

    return status;
}

static int
sysi_fanctrl_fan_status_policy(onlp_fan_info_t fi[MAX_CHASSIS_FAN_COUNT],
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
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
    }

    /* Bring fan speed to FAN_DUTY_MAX if fan is not present */
    for (i = 0; i < num_of_fan; i++) {
        if (fi[i].status & ONLP_FAN_STATUS_PRESENT) {
            continue;
        }

        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
    }

    return ONLP_STATUS_OK;
}

static int
sysi_fanctrl_thermal_sensor_policy(onlp_fan_info_t fi[MAX_CHASSIS_FAN_COUNT],
                                           onlp_thermal_info_t ti[CHASSIS_THERMAL_COUNT],
                                           int *adjusted,
                                           int num_of_fan)
{
    int i;
    int fanduty;
    as4224_platform_id_t pid = get_platform_id();

    if (pid >= AS4224_52T) {
        pid = AS4224_52T; /* AS4224_52T/AS4224_52T_DAC use the same policy */
    }

	*adjusted = 0;

    if (onlp_file_read_int(&fanduty, "%s%s", FAN_SYSFS_PATH, "fan2_duty_cycle_percentage") < 0) {
        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
    }

    switch (fanduty) {
	case FAN_DUTY_LOW:
    {
        int threshold_48x[] = {51000, 48500, 66000, 55000, 62440};
        int threshold_52p[] = {51500, 49900, 58000, 56360, 61590};
        int threshold_52t[] = {51500, 51200, 58000, 57520, 55250};
        int *pthreshold[] = {threshold_48x, threshold_52p, threshold_52t};

        for (i = (THERMAL_1_ON_MAIN_BROAD); i <= (THERMAL_CPU_CORE); i++) {
            if (ti[i-1].mcelsius < pthreshold[pid][i-1]) {
                continue;
            }

            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_MID, num_of_fan);
        }

		break;
    }
	case FAN_DUTY_MID:
    {
        int threshold_48x[] = {58500, 54500, 80500, 62500, 75550};
        int threshold_52p[] = {62000, 60440, 76000, 73700, 85700};
        int threshold_52t[] = {64000, 63590, 76000, 75180, 81050};
        int *pthreshold[] = {threshold_48x, threshold_52p, threshold_52t};

        for (i = (THERMAL_1_ON_MAIN_BROAD); i <= (THERMAL_CPU_CORE); i++) {
            if (ti[i-1].mcelsius < pthreshold[pid][i-1]) {
                continue;
            }

            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
        }

		break;
    }
	case FAN_DUTY_MAX:
    {
        int threshold_48x[] = {49000, 46500, 64000, 53000, 60440};
        int threshold_52p[] = {49500, 47900, 56000, 54360, 59590};
        int threshold_52t[] = {49500, 49200, 56000, 55520, 53250};
        int *pthreshold[] = {threshold_48x, threshold_52p, threshold_52t};
        int below = 1;

        for (i = (THERMAL_1_ON_MAIN_BROAD); i <= (THERMAL_CPU_CORE); i++) {
            if (ti[i-1].mcelsius <= pthreshold[pid][i-1]) {
                continue;
            }

            below = 0;
            break;
        }

        if (below) {
            *adjusted = 1;
            return sysi_fanctrl_fan_set_duty(FAN_DUTY_LOW, num_of_fan);
        }

		break;
    }
	default:
        *adjusted = 1;
        return sysi_fanctrl_fan_set_duty(FAN_DUTY_MAX, num_of_fan);
    }

    /* Set as current speed to kick watchdog */
    return sysi_fanctrl_fan_set_duty(fanduty, num_of_fan);
}

typedef int (*fan_control_policy)(onlp_fan_info_t fi[MAX_CHASSIS_FAN_COUNT],
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
    int num_of_fan = platform_fan_count();
    onlp_fan_info_t fi[MAX_CHASSIS_FAN_COUNT];
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