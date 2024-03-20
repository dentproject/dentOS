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
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include "arm64_accton_as45xx_52p_log.h"

#define CHASSIS_THERMAL_COUNT 4
#define CHASSIS_LED_COUNT     6
#define CHASSIS_PSU_COUNT     2
#define CHASSIS_FAN_COUNT     4
#define CHASSIS_CPLD_COUNT    2

#define PSU1_ID 1
#define PSU2_ID 2

#define AS4500_CPLD_M_PATH "/sys/bus/i2c/devices/0-0042/"
#define AS4500_CPLD_S_PATH "/sys/bus/i2c/devices/2-0062/"
#define AS4581_CPLD_M_PATH "/sys/bus/i2c/devices/1-0061/"
#define AS4581_CPLD_S_PATH "/sys/bus/i2c/devices/8-0062/"

#define AS4500_IDPROM_PATH "/sys/bus/i2c/devices/9-0054/eeprom"
#define AS4581_IDPROM_PATH "/sys/bus/i2c/devices/15-0054/eeprom"

#define AS4500_PSU1_PMBUS_PATH "/sys/bus/i2c/devices/6-0058/"
#define AS4500_PSU2_PMBUS_PATH "/sys/bus/i2c/devices/5-0059/"
#define AS4581_PSU1_PMBUS_PATH "/sys/bus/i2c/devices/12-0058/"
#define AS4581_PSU2_PMBUS_PATH "/sys/bus/i2c/devices/11-0059/"

#define AS4500_PSU1_EEPROM_PATH "/sys/bus/i2c/devices/6-0050/"
#define AS4500_PSU2_EEPROM_PATH "/sys/bus/i2c/devices/5-0051/"
#define AS4581_PSU1_EEPROM_PATH "/sys/bus/i2c/devices/12-0050/"
#define AS4581_PSU2_EEPROM_PATH "/sys/bus/i2c/devices/11-0051/"

#define FAN_SYSFS_PATH "/sys/devices/platform/as45xx_fan/"
#define SFP_SYSFS_PATH "/sys/devices/platform/as45xx_sfp/"

typedef enum as45xx_52p_platform_id {
    AS4500_52P,
    AS4581_52PL,
    PID_UNKNOWN
} as45xx_52p_platform_id_t;

enum onlp_thermal_id {
    THERMAL_RESERVED = 0,
    THERMAL_1_ON_MAIN_BROAD,
    THERMAL_2_ON_MAIN_BROAD,
    THERMAL_3_ON_MAIN_BROAD,
    THERMAL_4_ON_MAIN_BROAD,
    THERMAL_1_ON_PSU1,
    THERMAL_1_ON_PSU2
};

typedef enum psu_type {
    PSU_TYPE_UNKNOWN,
    PSU_TYPE_G1482_1600WNA_F2B
} psu_type_t;

as45xx_52p_platform_id_t get_platform_id(void);
psu_type_t get_psu_type(int id, char* modelname, int modelname_len);
char* psu_get_eeprom_dir(int id);
int psu_eeprom_str_get(int id, char *data_buf, int data_len, char *data_name);
int psu_pmbus_info_get(int id, char *node, int *value);
int get_cpld_attr_int(char* cpld_fmt, char *attr, int *value);
int set_cpld_attr_int(char* cpld_fmt, char *attr, int value);

#define AIM_FREE_IF_PTR(p) \
    do \
    { \
        if (p) { \
            aim_free(p); \
            p = NULL; \
        } \
    } while (0)

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)                                        \
		printf("%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#endif  /* __PLATFORM_LIB_H__ */
