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
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>

#include "arm64_wnc_qsa72_aom_a_48p_int.h"
#include "arm64_wnc_qsa72_aom_a_48p_log.h"

#include "platform_lib.h"

#define PREFIX_PATH_ON_CPLD_DEV		"/sys/bus/i2c/devices/"
#define NUM_OF_CPLD             	1
#define CPLD_VER_MAX_STR_LEN		20
#define ONL_VER_NUM_MAX_STR_LEN		64

#define FAN_DUTY_CYCLE_MAX               (100)
#define FAN_DUTY_CYCLE_MID               (75)
#define FAN_DUTY_CYCLE_MIN               (50)
#define MANAGE_FANS_SENSOR_CRITICAL      (50*1000)  /* milli-celsius, 50 degrees */
#define MANAGE_FANS_SENSOR_WARN          (45*1000)  /* milli-celsius, 45 degrees */

enum manage_fans_status {
    MANAGE_FANS_STA_INIT = 0,
    MANAGE_FANS_STA_NORMAL,
    MANAGE_FANS_STA_WARN,
    MANAGE_FANS_STA_CRITICAL
};
static int manage_fans_status = MANAGE_FANS_STA_INIT;

static char arr_cpldcap_name[NUM_OF_CPLD][64] =
{
   "2-0077/cpld1_rev_cap",
};
static char arr_cpldsub_name[NUM_OF_CPLD][64] =
{
   "2-0077/cpld1_rev_sub",
};

const char*
onlp_sysi_platform_get(void)
{
    return "arm64-wnc-qsa72-aom-a-48p-r0";
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

    /* Thermal sensors */
    for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* Fans */
    for (i = 1; i <= CHASSIS_FAN_COUNT; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    /* PSUs */
    for (i = 1; i <= CHASSIS_PSU_COUNT; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* LEDs */
    for (i = 1; i <= CHASSIS_LED_COUNT; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int   i, len;
    char *string = NULL;
    char  cap[NUM_OF_CPLD][CPLD_VER_MAX_STR_LEN]={0};
	char  sub[NUM_OF_CPLD][CPLD_VER_MAX_STR_LEN]={0};

    for (i = 0; i < NUM_OF_CPLD; i++) {
        len = onlp_file_read_str(&string, "%s%s", PREFIX_PATH_ON_CPLD_DEV, arr_cpldcap_name[i]);
        if (string && len) {
            strncpy(cap[i], string, len);
            aim_free(string);
        } else {
        	printf("onlp_sysi_platform_info_get: return ONLP_STATUS_E_INTERNAL1\n");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

	for (i = 0; i < NUM_OF_CPLD; i++) {
        len = onlp_file_read_str(&string, "%s%s", PREFIX_PATH_ON_CPLD_DEV, arr_cpldsub_name[i]);
        if (string && len) {
            strncpy(sub[i], string, len);
            aim_free(string);
        } else {
        	printf("onlp_sysi_platform_info_get: return ONLP_STATUS_E_INTERNAL2\n");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    pi->cpld_versions = aim_fstrdup("SYS:%s_%s", cap[0],sub[0]);

    return 0;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
}

static int sysi_set_all_fan_speed(int percentage)
{
    int i;
    int ret = ONLP_STATUS_OK;

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++)
    {
        if (onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), percentage) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("Unable to set fan speed on fan(%d) \r\n", i);
            ret = ONLP_STATUS_E_INTERNAL;
        }
    }
    return ret;
}

static int
sysi_check_fan_failed(void){
    int ret = ONLP_STATUS_OK;
    int i;

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++)
    {
        onlp_fan_info_t fan_info;

        if (onlp_fani_info_get(ONLP_FAN_ID_CREATE(i), &fan_info) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("Unable to get fan(%d) status\r\n", i);
        }

        if ((fan_info.status & ONLP_FAN_STATUS_FAILED) || 
            !(fan_info.status & ONLP_FAN_STATUS_PRESENT)) {
            AIM_LOG_WARN("Fan(%d) is not working, set the other fans as full speed\r\n", i);
            if (manage_fans_status != MANAGE_FANS_STA_CRITICAL) {
                sysi_set_all_fan_speed(FAN_DUTY_CYCLE_MAX);
            }
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    return ret;
}

static int 
sysi_check_sensor(void){
    int ret = ONLP_STATUS_OK;
    int i, max_temp = 0;

    for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++)
    {
        onlp_thermal_info_t thermal_info;

        if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(i), &thermal_info) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("Unable to get thermal(%d) status\r\n", i);
        }

        /* If any sensor is high than 50 degrees, set fan speed to duty 100% */
        if (thermal_info.mcelsius > MANAGE_FANS_SENSOR_WARN) {
            if (thermal_info.mcelsius > MANAGE_FANS_SENSOR_CRITICAL) {
                AIM_LOG_ERROR("thermal(%d) is more than %d degree \r\n", i, MANAGE_FANS_SENSOR_CRITICAL);
            } else {
                AIM_LOG_ERROR("thermal(%d) is more than %d degree \r\n", i, MANAGE_FANS_SENSOR_WARN);
            }
        }

        if (thermal_info.mcelsius > max_temp) {
            max_temp = thermal_info.mcelsius;
        }
    }

    /* If any sensor is high than 50 degrees, set fan speed to duty 100% 
       If any sensor is high than 45 degrees, set fan speed to duty 75% */
    if (max_temp > MANAGE_FANS_SENSOR_WARN) {
        if (max_temp > MANAGE_FANS_SENSOR_CRITICAL) {
            if (manage_fans_status != MANAGE_FANS_STA_CRITICAL) {
                sysi_set_all_fan_speed(FAN_DUTY_CYCLE_MAX);
                manage_fans_status = MANAGE_FANS_STA_CRITICAL;
            }
        } else {
            if (manage_fans_status != MANAGE_FANS_STA_WARN) {
                sysi_set_all_fan_speed(FAN_DUTY_CYCLE_MID);
                manage_fans_status = MANAGE_FANS_STA_WARN;
            }
        }
        ret = ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}

/* Thermal plan:
 * Sensors include CPU_core and LM75_1 ~ LM75_5
 * 1. If any FAN failed, set all the other fans as full speed, 100%.
 * 2. If any sensor is high than 50 degrees, set fan speed to duty 100%.
 * 3. If any sensor is high than 45 degrees, set fan speed to duty 75%.
 * 4. Default, set fan speed to duty 50%.
 */
int
onlp_sysi_platform_manage_fans(void)
{
    int ret = ONLP_STATUS_OK;

    /**********************************************************
     * Decision 1: Set fan as full speed if any fan is failed.
     **********************************************************/
    ret = sysi_check_fan_failed();
    if (ONLP_STATUS_OK != ret) {
        DEBUG_PRINT("SYSi check fan failed. \r\n");
        manage_fans_status = MANAGE_FANS_STA_CRITICAL;
        return ret;
    }

    /**********************************************************
     * Decision 2:
     * If any sensor is high than 50 degrees, set fan speed to duty 100%.
     * If any sensor is high than 45 degrees, set fan speed to duty 75%.
     **********************************************************/
    ret = sysi_check_sensor();
    if (ONLP_STATUS_OK != ret) {
        DEBUG_PRINT("SYSi check sensor over threshold. \r\n");
        return ret;
    }

    /**********************************************************
     * Decision 3: Default, set fan speed to duty 50%.
     **********************************************************/
    if (manage_fans_status != MANAGE_FANS_STA_NORMAL) {
        manage_fans_status = MANAGE_FANS_STA_NORMAL;
        sysi_set_all_fan_speed(FAN_DUTY_CYCLE_MIN);
    }
    ret = ONLP_STATUS_OK;

    return ret;
}

/* Status LED plan:
 * If SDK is not ready, Status LED shows green blinking.
 * If SDK is ready, Status LED shows green.
 */
int
onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

