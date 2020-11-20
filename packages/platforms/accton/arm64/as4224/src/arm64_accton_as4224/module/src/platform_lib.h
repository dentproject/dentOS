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

#include "arm64_accton_as4224_log.h"

#define CHASSIS_THERMAL_COUNT	5
#define CHASSIS_LED_COUNT		1

#define PSU1_ID 1
#define PSU2_ID 2

#define CPLD_SYSFS_ATTR_FMT "/sys/bus/i2c/devices/0-0040/%s"
#define PSU_SYSFS_PATH "/sys/devices/platform/as4224_psu/"
#define FAN_SYSFS_PATH "/sys/devices/platform/as4224_fan/"
#define IDPROM_PATH "/sys/class/i2c-adapter/i2c-1/1-0056/eeprom"

typedef enum as4224_platform_id {
    AS4224_48X,
    AS4224_52P,
    AS4224_52T,
    AS4224_52T_DAC,
    PID_UNKNOWN
} as4224_platform_id_t;

enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
    THERMAL_1_ON_MAIN_BROAD,
    THERMAL_2_ON_MAIN_BROAD,
    THERMAL_3_ON_MAIN_BROAD,
    THERMAL_4_ON_MAIN_BROAD,
    THERMAL_CPU_CORE
};

int get_cpld_attr_int(char *attr, int *value);
int set_cpld_attr_int(char *attr, int value);
as4224_platform_id_t get_platform_id(void);
int platform_thermal_count(void);
int platform_fan_count(void);
int platform_psu_count(void);

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)                                        \
		printf("%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#endif  /* __PLATFORM_LIB_H__ */