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
#include <limits.h>

#include <onlplib/file.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include "platform_lib.h"

#include "arm64_accton_as45xx_52p_int.h"
#include "arm64_accton_as45xx_52p_log.h"

int onlp_sysi_get_thermal_temp(int tid, int *temp);
int onlp_sysi_set_fan_duty_all(int duty);
int onlp_sysi_get_fan_status(aim_bitmap32_t* fan_bitmap);

int temp_sensors[] = {
    THERMAL_1_ON_MAIN_BROAD,
    THERMAL_2_ON_MAIN_BROAD,
    THERMAL_3_ON_MAIN_BROAD,
    THERMAL_4_ON_MAIN_BROAD,
    THERMAL_ASC
};

typedef int (*temp_getter_t)(int tid, int *temp);
typedef int (*fan_pwm_setter_t)(int pwm);
typedef int (*fan_status_getter_t)(aim_bitmap32_t* fan_bitmap);

typedef struct {
   int fan_duty;       /* In percetage */
   int step_up_temp;   /* In mini-Celsius */
   int step_down_temp; /* In mini-Celsius */
   int step_up_duty;   /* In mini-Celsius */
   int step_down_duty; /* In mini-Celsius */
} fan_normal_policy_t;

typedef struct {
    int one_fail_duty;
    int two_fail_duty;
} fan_fail_policy_t;

typedef struct temp_handler {
    int thermal_count[PLATFORM_COUNT];
    temp_getter_t temp_readers[AIM_ARRAYSIZE(temp_sensors)];
} temp_handler_t;

typedef struct fan_handler {
    int default_duty[PLATFORM_COUNT];
    fan_pwm_setter_t    pwm_writer;
    fan_status_getter_t status_reader;
    fan_normal_policy_t policy_normal[PLATFORM_COUNT][4];
    fan_fail_policy_t   policy_fail[PLATFORM_COUNT];
} fan_handler_t;

struct thermal_policy_manager {
    temp_handler_t  temp_hdlr;
    fan_handler_t   fan_hdlr; /* fan state reader/writer */
};

/*********************************************************************************
 * COMe + LTE SKU
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) < 197 = 10% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) > 200 = 15% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) < 232 = 15% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) > 235 = 25% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) < 262 = 25% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) + ASC10(ASC_TMON1) > 265 = 35% FAN speed
 *    COMe ASC10 location is U38 (ASC_TMON1)
 *    One FAN failure = 50% FAN speed
 *    Two FAN failure = 80% FAN speed
 *
 * Base SKU (Without COMe)
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) < 196.5 = 10% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) > 199 = 20% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) < 214 = 20% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) > 217 = 30% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) < 227.5 = 30% FAN speed
 *    U48(0x48)+U19(0x49)+U106(0x4B)+U135(0x4C) > 230 = 35% FAN speed
 *    One FAN failure = 35% FAN speed
 *    Two FAN failure = 50% FAN speed
 *
 *    Note1: 0x48, 0x49, 0x4B, 0x4C are i2c address
 *    Note2: Critical components with shutdown rules when it is over temperature.
 *    1. COMe CPU: NXP LX2080SE71826B > 105C Tj, it will be shutdown
 *    2. PSU G1482-0920WNA > 55C , it will be shutdwon
 ********************************************************************************/
struct thermal_policy_manager tp_mgr = {
    .temp_hdlr = {
        .thermal_count = { [AS4500_52P] = 4, [AS4581_52PL] = 5 },
        .temp_readers = {
            [0 ... (AIM_ARRAYSIZE(temp_sensors)-1)] = onlp_sysi_get_thermal_temp
        }
    },
    .fan_hdlr = {
        .pwm_writer = onlp_sysi_set_fan_duty_all,
        .status_reader = onlp_sysi_get_fan_status,
        .default_duty = { [AS4500_52P] = 35, [AS4581_52PL] = 35 },
        .policy_normal = { [AS4500_52P][0] = { 10,  199000, INT_MIN, 20, 10 },
                           [AS4500_52P][1] = { 20,  217000,  196500, 30, 10 },
                           [AS4500_52P][2] = { 30,  230000,  214000, 35, 20 },
                           [AS4500_52P][3] = { 35, INT_MAX,  227500, 35, 30 },

                           [AS4581_52PL][0] = { 10,  200000, INT_MIN, 15, 10 },
                           [AS4581_52PL][1] = { 15,  235000,  197000, 25, 10 },
                           [AS4581_52PL][2] = { 25,  265000,  232000, 35, 15 },
                           [AS4581_52PL][3] = { 35, INT_MAX,  262000, 35, 25 } },
        .policy_fail = { [AS4500_52P]  = { 35, 50 },
                         [AS4581_52PL] = { 50, 80 } }
    }
};

const char*
onlp_sysi_platform_get(void)
{
    as45xx_52p_platform_id_t pid = get_platform_id();

    switch (pid)
    {
    case AS4500_52P: return "arm64-accton-as4500-52p-r0";
    case AS4581_52PL: return "arm64-accton-as4581-52pl-r0";
    default: break;
    }

    return "Unknown Platform";
}

const char*
onlp_sysi_get_eeprom_path(void)
{
    as45xx_52p_platform_id_t pid;
    char *path[] = { AS4500_IDPROM_PATH, AS4581_IDPROM_PATH };

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return NULL;

    return path[pid];
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);
    if(onlp_file_read(rdata, 256, size, onlp_sysi_get_eeprom_path()) == ONLP_STATUS_OK) {
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

    /* Thermal sensors on the chassis */
    for (i = 1; i <= chassis_thermal_count(); i++) {
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

    /* 4 Fans on the chassis */
    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    return 0;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int i = 0;
    int ret = 0;
    int version[CHASSIS_CPLD_COUNT] = { 0 };
    int sub_version[CHASSIS_CPLD_COUNT] = { 0 };
    char *cpld_as4500[] = { AS4500_CPLD_M_PATH, AS4500_CPLD_S_PATH };
    char *cpld_as4581[] = { AS4581_CPLD_M_PATH, AS4581_CPLD_S_PATH };
    char **cpld_path[] = { cpld_as4500, cpld_as4581 };
    as45xx_52p_platform_id_t pid;

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return ONLP_STATUS_E_INTERNAL;

    for (i = 0; i < CHASSIS_CPLD_COUNT; i++) {
        ret = onlp_file_read_int(&version[i], "%s%s", cpld_path[pid][i], "version_major");
        if (ret < 0) {
            return ret;
        }

        ret = onlp_file_read_int(&sub_version[i], "%s%s", cpld_path[pid][i], "version_minor");
        if (ret < 0) {
            return ret;
        }
    }

    pi->cpld_versions = aim_fstrdup("\r\nCPLD-M:%x.%x\r\nCPLD-S:%x.%x",
                            version[0], sub_version[0],
                            version[1], sub_version[1]);
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

int onlp_sysi_get_thermal_temp(int tid, int *temp)
{
    int ret;
    onlp_thermal_info_t ti;

    ret = onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(tid), &ti);
    if (ret != ONLP_STATUS_OK)
        return ret;

    *temp = ti.mcelsius;
    return ONLP_STATUS_OK;
}

int onlp_sysi_set_fan_duty_all(int duty)
{
    int fid, ret = ONLP_STATUS_OK;

    for (fid = 1; fid <= CHASSIS_FAN_COUNT; fid++) {
        if (ONLP_STATUS_OK != onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(fid), duty)) {
            ret = ONLP_STATUS_E_INTERNAL;
        }
    }

    return ret;
}

int onlp_sysi_get_fan_status(aim_bitmap32_t* fan_bitmap)
{
    int i, ret;
    onlp_fan_info_t fi[CHASSIS_FAN_COUNT];
    memset(fi, 0, sizeof(fi));

    for (i = 0; i < CHASSIS_FAN_COUNT; i++) {
        ret = onlp_fani_info_get(ONLP_FAN_ID_CREATE(i+1), &fi[i]);
        if (ret != ONLP_STATUS_OK) {
			AIM_LOG_ERROR("Unable to get fan(%d) status\r\n", i+1);
            AIM_BITMAP_SET(fan_bitmap, (i+1));
            continue;
        }

        if (!(fi[i].status & ONLP_FAN_STATUS_PRESENT)) {
            AIM_LOG_ERROR("Fan(%d) is NOT present\r\n", i+1);
            AIM_BITMAP_SET(fan_bitmap, (i+1));
            continue;
        }

        if (fi[i].status & ONLP_FAN_STATUS_FAILED) {
            AIM_LOG_ERROR("Fan(%d) is NOT operational\r\n", i+1);
            AIM_BITMAP_SET(fan_bitmap, (i+1));
            continue;
        }
    }

    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_fans(void)
{
    as45xx_52p_platform_id_t pid;
    int i, ret, found = 0;
    int sum_temp = 0;
    int fail_count = 0;
    int temp[AIM_ARRAYSIZE(temp_sensors)] = {0};
    aim_bitmap32_t fan_fail_bitmap;
    static int fan_duty = 0;

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return ONLP_STATUS_E_INTERNAL;

    if (fan_duty == 0)
        fan_duty = tp_mgr.fan_hdlr.default_duty[pid];

    AIM_BITMAP_INIT(&fan_fail_bitmap, 31);
    AIM_BITMAP_CLR_ALL(&fan_fail_bitmap);

    /* Get fan status
     * Bring fan speed to MAX if failed to get fan status
     */
    ret = tp_mgr.fan_hdlr.status_reader(&fan_fail_bitmap);
    if (ret != ONLP_STATUS_OK) {
        fan_duty = tp_mgr.fan_hdlr.policy_fail[pid].two_fail_duty;
        tp_mgr.fan_hdlr.pwm_writer(fan_duty);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Check fan fail count */
    fail_count = AIM_BITMAP_COUNT(&fan_fail_bitmap);
    if (fail_count) {
        fan_duty = (fail_count == 1) ?
                    tp_mgr.fan_hdlr.policy_fail[pid].one_fail_duty :
                    tp_mgr.fan_hdlr.policy_fail[pid].two_fail_duty;
        tp_mgr.fan_hdlr.pwm_writer(fan_duty);
        return ONLP_STATUS_OK;
    }

    /* Get thermal status */
    for (i = 0; i < tp_mgr.temp_hdlr.thermal_count[pid]; i++) {
        ret = tp_mgr.temp_hdlr.temp_readers[i](temp_sensors[i], &temp[i]);
        if (ret != ONLP_STATUS_OK) {
            fan_duty = tp_mgr.fan_hdlr.policy_fail[pid].two_fail_duty;
            tp_mgr.fan_hdlr.pwm_writer(fan_duty);
            return ret;
        }

        sum_temp += temp[i];
    }

    /* Apply thermal policy */
    for (i = 0; i < AIM_ARRAYSIZE(tp_mgr.fan_hdlr.policy_normal[pid]); i++) {
        if (fan_duty != tp_mgr.fan_hdlr.policy_normal[pid][i].fan_duty)
            continue;

        if (sum_temp > tp_mgr.fan_hdlr.policy_normal[pid][i].step_up_temp)
            fan_duty = tp_mgr.fan_hdlr.policy_normal[pid][i].step_up_duty;
        else if (sum_temp < tp_mgr.fan_hdlr.policy_normal[pid][i].step_down_temp)
            fan_duty = tp_mgr.fan_hdlr.policy_normal[pid][i].step_down_duty;

        found = 1;
        break;
    }

    if (!found)
        fan_duty = tp_mgr.fan_hdlr.default_duty[pid];

    tp_mgr.fan_hdlr.pwm_writer(fan_duty);
    return ONLP_STATUS_OK;
}
