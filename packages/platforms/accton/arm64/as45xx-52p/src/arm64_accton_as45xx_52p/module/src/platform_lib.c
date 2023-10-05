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

#define PSU_MODEL_NAME_LEN 13

char* psu_get_eeprom_dir(int id)
{
    as45xx_52p_platform_id_t pid;
    char *path_as4500[] = { AS4500_PSU1_EEPROM_PATH, AS4500_PSU2_EEPROM_PATH };
    char *path_as4581[] = { AS4581_PSU1_EEPROM_PATH, AS4581_PSU2_EEPROM_PATH };
    char **path[] = { path_as4500, path_as4581 };

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return NULL;

    return path[pid][id-1];
}

char* psu_get_pmbus_dir(int id)
{
    as45xx_52p_platform_id_t pid;
    char *path_as4500[] = { AS4500_PSU1_PMBUS_PATH, AS4500_PSU2_PMBUS_PATH };
    char *path_as4581[] = { AS4581_PSU1_PMBUS_PATH, AS4581_PSU2_PMBUS_PATH };
    char **path[] = { path_as4500, path_as4581 };

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return NULL;

    return path[pid][id-1];
}

psu_type_t get_psu_type(int id, char *data_buf, int data_len)
{
    int ret = 0;
    psu_type_t ptype = PSU_TYPE_UNKNOWN;

    ret = psu_eeprom_str_get(id, data_buf, data_len, "psu_model_name");
    if (ONLP_STATUS_OK != ret)
        return PSU_TYPE_UNKNOWN;

    /* Check AC model name */
    if (strncmp(data_buf, "G1482-1600WNA", strlen("G1482-1600WNA")) == 0)
        ptype = PSU_TYPE_G1482_1600WNA_F2B;

    return ptype;
}

as45xx_52p_platform_id_t get_platform_id(void)
{
    char* cpld_path = NULL;
    int   ret = ONLP_STATUS_OK;

    ret = onlp_file_find("/sys/bus/i2c/devices/0-0042/", "platform_id", &cpld_path);
    if (cpld_path)
        free(cpld_path);

    if (ONLP_STATUS_OK == ret)
        return AS4500_52P;

    ret = onlp_file_find("/sys/bus/i2c/devices/1-0061/", "platform_id", &cpld_path);
    if (cpld_path)
        free(cpld_path);

    return (ONLP_STATUS_OK == ret) ? AS4581_52PL : PID_UNKNOWN;
}

int psu_eeprom_str_get(int id, char *data_buf, int data_len, char *data_name)
{
    char *path;
    int   len    = 0;
    char *str = NULL;

    path = psu_get_eeprom_dir(id);
    if (path == NULL)
        return ONLP_STATUS_E_INTERNAL;

    /* Read attribute */
    len = onlp_file_read_str(&str, "%s%s", path, data_name);
    if (!str || len <= 0) {
        AIM_FREE_IF_PTR(str);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (len > data_len) {
        AIM_FREE_IF_PTR(str);
        return ONLP_STATUS_E_INVALID;
    }

    aim_strlcpy(data_buf, str, len+1);
    AIM_FREE_IF_PTR(str);
    return ONLP_STATUS_OK;
}

int psu_pmbus_info_get(int id, char *node, int *value)
{
    char *path;
    *value = 0;

    path = psu_get_pmbus_dir(id);
    if (path == NULL)
        return ONLP_STATUS_E_INTERNAL;

    return onlp_file_read_int(value, "%s%s", path, node);
}
