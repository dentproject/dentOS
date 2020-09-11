/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *       Copyright 2014, 2015 Big Switch Networks, Inc.
 *       Copyright 2017, 2020 Delta Networks, Inc.
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
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <onlplib/file.h>
#include "platform_lib.h"

plat_id_t gPlat_id = PID_UNKNOWN;

plat_info_t gPlat_info[] = {
    [PID_TN48M] = {
        .name = "arm64-delta-tn48m",

        .onie_eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom",

        .cpld_bus = 0,
        .cpld_path = "/sys/bus/i2c/devices/0-0041",

        .thermal_count  = 11,
        .fan_count = 3,
        .psu_count = 1,
        .led_count = 3,

        .sfp_start_idx = 49,
        .sfp_end_idx = 52,
    },
    [PID_TN48M_POE] = {
        .name = "arm64-delta-tn48m-poe",

        .onie_eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom",

        .cpld_bus = 0,
        .cpld_path = "/sys/bus/i2c/devices/0-0041",

        .thermal_count  = 13,
        .fan_count = 5,
        .psu_count = 2,
        .led_count = 3,

        .sfp_start_idx = 49,
        .sfp_end_idx = 52,
    },
    [PID_TN4810M_PVT] = {
        .name = "arm64-delta-tn4810m",

        .onie_eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom",

        .cpld_bus = 5,
        .cpld_path = "/sys/bus/i2c/devices/5-0041",

        .thermal_count  = 11,
        .fan_count = 3,
        .psu_count = 2,
        .led_count = 3,

        .sfp_start_idx = 1,
        .sfp_end_idx = 48,
    },
    [PID_TN4810M_NONPVT] = {
        .name = "arm64-delta-tn4810m",

        .onie_eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom",

        .cpld_bus = 0,
        .cpld_path = "/sys/bus/i2c/devices/0-0041",

        .thermal_count  = 11,
        .fan_count = 3,
        .psu_count = 2,
        .led_count = 3,

        .sfp_start_idx = 1,
        .sfp_end_idx = 48,
    },
    [PID_TN48M2] = {
        .name = "arm64-delta-tn48m2",

        .onie_eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom",

        .cpld_bus = 0,
        .cpld_path = "/sys/bus/i2c/devices/0-0041",

        .thermal_count  = 11,
        .fan_count = 3,
        .psu_count = 2,
        .led_count = 3,

        .sfp_start_idx = 49,
        .sfp_end_idx = 52,
    },
};

plat_id_t get_platform_id(void)
{
    int len;
    int pid = PID_UNKNOWN;
    int cpld_bus = 0;
    char buf[4] = {0};
    char pid_fullpath[PATH_MAX] = {0};
    plat_info_t *tn48m_info = &gPlat_info[PID_TN48M];
    plat_info_t *tn4810m_pvt_info = &gPlat_info[PID_TN4810M_PVT];

    if (plat_os_file_is_existed(tn48m_info->cpld_path)) {
        sprintf(pid_fullpath, "%s/platform_id", tn48m_info->cpld_path);
        cpld_bus = tn48m_info->cpld_bus;
    } else if (plat_os_file_is_existed(tn4810m_pvt_info->cpld_path)) {
        sprintf(pid_fullpath, "%s/platform_id", tn4810m_pvt_info->cpld_path);
        cpld_bus = tn4810m_pvt_info->cpld_bus;
    } else {
        return PID_UNKNOWN;
    }

    if (plat_os_file_read_str(buf, sizeof(buf), &len, pid_fullpath) < 0)
        return PID_UNKNOWN;

    pid = (int) strtol(buf, NULL, 16);
    if (pid >= PID_UNKNOWN || pid < PID_TN48M) {
        return PID_UNKNOWN;
    }

    /* Special case to check the TN4810M non-PVT platform */
    if (pid == PID_TN4810M_PVT && cpld_bus == 0)
        pid = PID_TN4810M_NONPVT;

    return pid;
}

////////////////////////////////////////////////////////////////
// OS HELP ROUTINE
static char *plat_os_path_complete(char *path_pattern, char *buff, int len)
{
    glob_t globbuf;
    int rv = glob(path_pattern, 0, NULL, &globbuf);
    if (rv != 0) {
        globfree(&globbuf);
        snprintf(buff, len, "%s", path_pattern);
        return buff;
    }

    snprintf(buff, len, "%s", globbuf.gl_pathv[0]);
    globfree(&globbuf);

    return buff;
}

int plat_os_file_is_existed(char *path)
{
    char buff[PATH_MAX+1] = {0};

    if (path)
        return access(plat_os_path_complete(path, buff, sizeof(buff)),
                      F_OK) == 0 ? 1 : 0;
    return 0;
}

int plat_os_file_read(uint8_t *data, int max, int *len, char *path, ...)
{
    char buff[PATH_MAX+1] = {0};
    return onlp_file_read(data, max, len,
                          plat_os_path_complete(path, buff, sizeof(buff)),
                          NULL);
}

int plat_os_file_read_str(char *data, int max, int *len, char *path, ...)
{
    int rc;
    char buff[PATH_MAX+1] = {0};

    rc = onlp_file_read((uint8_t *)data, max - 1, len,
                        plat_os_path_complete(path, buff, sizeof(buff)), NULL);

    if (rc == 0) {
        data[max - 1] = '\0';
    }

    return rc;
}

int plat_os_file_read_int(int *val, char *path, ...)
{
    char buff[PATH_MAX+1] = {0};
    return onlp_file_read_int(val,
                              plat_os_path_complete(path, buff, sizeof(buff)),
                              NULL);
}

int plat_os_file_write_int(int val, char *path, ...)
{
    char buff[PATH_MAX+1] = {0};
    return onlp_file_write_int(val,
                               plat_os_path_complete(path, buff, sizeof(buff)),
                               NULL);
}

int eeprom_read(int bus, uint8_t addr, uint8_t offset, uint8_t *buff, int len)
{
    int i, r;

    for (i = 0; i < len; i ++) {
        r = onlp_i2c_readb(bus, addr, offset + i, 0);
        if (r < 0) {
            return -1;
        }
        buff[i] = (uint8_t)r;
    }

    return 0;
}
