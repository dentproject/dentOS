/************************************************************
 * <bsn.cl fy=2016 v=onl>
 *
 *       Copyright 2014, 2015 Big Switch Networks, Inc.
 *       Copyright 2017, 2020, 2021 Delta Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *       http://www.eclipse.org/legal/epl-v10.html
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
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/thermali.h>
#include <linux/limits.h>
#include <onlplib/file.h>
#include "arm64_delta_tn48m_dn_log.h"
#include "platform_lib.h"

#define FAN_DUTY_CYCLE_LVL_0          (50)
#define FAN_DUTY_CYCLE_LVL_1          (60)
#define FAN_DUTY_CYCLE_LVL_2          (80)
#define FAN_DUTY_CYCLE_LVL_MAX        (100)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

static int cur_fan_pct = -1;

const char*
onlp_sysi_platform_get(void)
{
    gPlat_id = get_platform_id();

    if (gPlat_id == PID_TN48M_DN)
        return "arm64-delta-tn48m-dn";
    else if (gPlat_id == PID_TN48M_POE_DN)
        return "arm64-delta-tn48m-poe-dn";
    else if (gPlat_id == PID_TN4810M_DN)
        return "arm64-delta-tn4810m-dn";
    else
        return "Unknown Platform";
}

int
onlp_sysi_platform_set(const char* platform)
{
    gPlat_id = get_platform_id();

    if (strstr(platform, "arm64-delta-tn48m-dn-r0")) {
        return ONLP_STATUS_OK;
    }
    if (strstr(platform, "arm64-delta-tn48m-poe-dn-r0")) {
        return ONLP_STATUS_OK;
    }
    if (strstr(platform, "arm64-delta-tn4810m-dn-r0")) {
        return ONLP_STATUS_OK;
    }

    AIM_LOG_ERROR("Not supported platform '%s'\n", platform);

    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int rc = ONLP_STATUS_OK;
    int len;
    char cpld_ver[4], hw_ver[4];
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    /* CPLD Version */
    sprintf(fullpath, "%s/cpld_version", plat_info->cpld_path);
    if ((rc = plat_os_file_read_str(cpld_ver, sizeof(cpld_ver),
                                     &len, fullpath)) < 0)
        return rc;

    pi->cpld_versions = aim_fstrdup("CPLD Version = %s", cpld_ver);

    /* Hardware Version */
    sprintf(fullpath, "%s/hw_version", plat_info->cpld_path);
    if ((rc = plat_os_file_read_str(hw_ver, sizeof(hw_ver),
                                     &len, fullpath)) < 0)
        return rc;

    pi->other_versions = aim_fstrdup("Hardware Version = %s", hw_ver);

    return rc;
}

int
onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    if(!rdata){
        return ONLP_STATUS_E_INTERNAL;
    }

    *data = rdata;
    if (onlp_file_read(rdata, 256, size, plat_info->onie_eeprom_path)
        == ONLP_STATUS_OK) {
        if (*size == 256) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

void
onlp_sysi_onie_data_free(uint8_t* data)
{
    aim_free(data);
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    memset(table, 0, max*sizeof(onlp_oid_t));

    /* Thermal sensors on the platform */
    for (i = 1; i <= plat_info->thermal_count; i++) {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* LEDs on the platform */
    for (i = 1; i <= plat_info->led_count; i++) {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* Fans on the platform */
    for (i = 1; i <= plat_info->fan_count; i++) {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    /* PSUs on the platform */
    for (i = 1; i <= plat_info->psu_count; i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    return 0;
}

int
onlp_sysi_platform_manage_fans(void)
{
    onlp_thermal_info_t thermal;
    onlp_fan_info_t fan_info;
    int i;
    int max_temp_cpu = 0, max_temp_other = 0;
    int fan_pct = FAN_DUTY_CYCLE_LVL_0;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    /* If one fan failed, set all fans' speed to 100% */
    for (i = PLAT_FAN_ID_1; i <= PLAT_FAN_ID_3; i ++) {
        if (onlp_fan_info_get(ONLP_FAN_ID_CREATE(i), &fan_info)
            == ONLP_STATUS_OK) {
            if ((fan_info.rpm == 0) ||
                (fan_info.status & ONLP_FAN_STATUS_FAILED) ||
                !(fan_info.status & ONLP_FAN_STATUS_PRESENT)) {
                fan_pct = FAN_DUTY_CYCLE_LVL_MAX;
                goto set_fan;
            }
        } else {
            AIM_LOG_ERROR("Unable to get FAN %d status\r\n", i);
        }
    }

    /* Get max temperature from CPU 88F7040 */
    for (i = PLAT_THERMAL_ID_1; i <= PLAT_THERMAL_ID_6; i ++) {
        if (onlp_thermal_info_get(ONLP_THERMAL_ID_CREATE(i), &thermal)
             == ONLP_STATUS_OK) {
            max_temp_cpu = MAX(max_temp_cpu, thermal.mcelsius);
        } else {
            AIM_LOG_ERROR("Unable to get Thermal Sensor %d status\r\n", i);
        }
    }

    if((max_temp_cpu >= 54000) && (max_temp_cpu < 74000)) {
        fan_pct = FAN_DUTY_CYCLE_LVL_1;
    } else if((max_temp_cpu >= 74000) && (max_temp_cpu < 93000)) {
        fan_pct = FAN_DUTY_CYCLE_LVL_2;
    } else if( max_temp_cpu >= 93000) {
        fan_pct = FAN_DUTY_CYCLE_LVL_MAX;
        goto set_fan;
    }

    /* Get max temperature from other thermal sensors */
    for (i = PLAT_THERMAL_ID_7; i <= plat_info->thermal_count; i ++) {
        if (onlp_thermal_info_get(ONLP_THERMAL_ID_CREATE(i), &thermal)
             == ONLP_STATUS_OK) {
            max_temp_other = MAX(max_temp_other, thermal.mcelsius);
        } else {
            AIM_LOG_ERROR("Unable to get Thermal Sensor %d status\r\n", i);
        }
    }

    if((max_temp_other >= 52000) && (max_temp_other < 70000)) {
        fan_pct = MAX(fan_pct, FAN_DUTY_CYCLE_LVL_1);
    } else if((max_temp_other >= 70000) && (max_temp_other < 90000)) {
        fan_pct = FAN_DUTY_CYCLE_LVL_2;
    } else if( max_temp_other >= 90000) {
        fan_pct = FAN_DUTY_CYCLE_LVL_MAX;
    }

set_fan:
    if (fan_pct != cur_fan_pct) {
        for (i = PLAT_FAN_ID_1; i <= PLAT_FAN_ID_3 ; i ++) {
            onlp_fan_percentage_set(ONLP_FAN_ID_CREATE(i), fan_pct);
            cur_fan_pct = fan_pct;
        }
    }

    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_leds(void)
{
    int i;
    uint32_t status;
    onlp_fan_info_t fan_info;
    onlp_psu_info_t psu_info;
    onlp_led_mode_t fan_new_mode;
    onlp_led_mode_t psu_new_mode;
    onlp_led_mode_t sys_new_mode;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    /* FAN LED */
    for (i = PLAT_FAN_ID_1; i <= plat_info->fan_count; i ++) {
        status = 0;
        if (onlp_fan_info_get(ONLP_FAN_ID_CREATE(i), &fan_info) ==
             ONLP_STATUS_OK) {
            status |= fan_info.status;
        }
        fan_new_mode = ONLP_LED_MODE_GREEN;
        if (status & ONLP_FAN_STATUS_FAILED) {
            fan_new_mode = ONLP_LED_MODE_ORANGE;
            break;
        } else if ((status & ONLP_FAN_STATUS_PRESENT) == 0) {
            fan_new_mode = ONLP_LED_MODE_OFF;
            break;
        }
    }
    /* Set Fan LED (Front Fan LED) */
    onlp_led_mode_set(ONLP_LED_ID_CREATE(PLAT_LED_ID_2), fan_new_mode);

    /* PSU LED */
    for (i = PLAT_PSU_ID_1; i <= plat_info->psu_count; i ++) {
        status = 0;
        if (onlp_psu_info_get(ONLP_PSU_ID_CREATE(i), &psu_info) ==
             ONLP_STATUS_OK) {
            status |= psu_info.status;
        }
        psu_new_mode = ONLP_LED_MODE_GREEN;
        if (status & ONLP_PSU_STATUS_FAILED) {
            psu_new_mode = ONLP_LED_MODE_ORANGE;
            break;
        } else if ((status & ONLP_PSU_STATUS_PRESENT) == 0) {
            psu_new_mode = ONLP_LED_MODE_OFF;
            break;
        }
    }
    /* Set PSU LED (Front PSU LED) */
    onlp_led_mode_set(ONLP_LED_ID_CREATE(PLAT_LED_ID_1), psu_new_mode);

    /* SYS LED */
    if (fan_new_mode == ONLP_LED_MODE_GREEN &&
        psu_new_mode == ONLP_LED_MODE_GREEN)
        sys_new_mode = ONLP_LED_MODE_GREEN;
    else
        sys_new_mode = ONLP_LED_MODE_ORANGE;

    /* Set SYS LED (Front SYS LED) */
    onlp_led_mode_set(ONLP_LED_ID_CREATE(PLAT_LED_ID_3), sys_new_mode);

    return ONLP_STATUS_OK;
}


