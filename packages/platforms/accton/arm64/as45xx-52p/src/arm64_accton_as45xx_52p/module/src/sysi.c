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

#include "arm64_accton_as45xx_52p_int.h"
#include "arm64_accton_as45xx_52p_log.h"

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
