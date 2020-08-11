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
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <onlplib/file.h>
#include <onlp/onlp.h>
#include "platform_lib.h"

int get_cpld_attr_int(char *attr, int *value)
{
    return onlp_file_read_int(value, CPLD_SYSFS_ATTR_FMT, attr);
}

int set_cpld_attr_int(char *attr, int value)
{
    return onlp_file_write_int(value, CPLD_SYSFS_ATTR_FMT, attr);
}

as4224_platform_id_t get_platform_id(void)
{
    int ret = 0;
    int pid = PID_UNKNOWN;

    ret = get_cpld_attr_int("platform_id", &pid);
    return (ret < 0) ? PID_UNKNOWN : pid;
}

int platform_fan_count(void)
{
    int ret   = 0;
    int value = 0;
    ret = onlp_file_read_int(&value, "%s%s", FAN_SYSFS_PATH, "fan_count");
    return (ret < 0) ? 0 : value;
}

int platform_psu_count(void)
{
    int ret   = 0;
    int value = 0;
    ret = onlp_file_read_int(&value, "%s%s", PSU_SYSFS_PATH, "psu_count");
    return (ret < 0) ? 0 : value;
}