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

#include "arm64_marvell_ac5x_db_log.h"

#define CHASSIS_THERMAL_COUNT	5
#define CHASSIS_LED_COUNT       3
#define CHASSIS_FAN_COUNT       2
#define CHASSIS_PSU_COUNT       1

#define CPLD_SYSFS_ATTR_FMT "/sys/bus/i2c/devices/0-0040/%s"
#define PSU_SYSFS_PATH "/sys/devices/platform/_psu/"
#define FAN_SYSFS_PATH "/sys/devices/platform/_fan/"
#define IDPROM_PATH "/sys/class/i2c-adapter/i2c-1/1-0056/eeprom"

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

#endif  /* __PLATFORM_LIB_H__ */
