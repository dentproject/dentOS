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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static char* devfiles_as4500[] =  /* must map with onlp_thermal_id */
{
    NULL,
    "/sys/bus/i2c/devices/10-0048*temp1_input",
    "/sys/bus/i2c/devices/10-0049*temp1_input",
    "/sys/bus/i2c/devices/10-004b*temp1_input",
    "/sys/bus/i2c/devices/10-004c*temp1_input",
    "/sys/bus/i2c/devices/6-0058*psu_temp1_input",
    "/sys/bus/i2c/devices/6-0058*psu_temp2_input",
    "/sys/bus/i2c/devices/6-0058*psu_temp3_input",
    "/sys/bus/i2c/devices/5-0059*psu_temp1_input",
    "/sys/bus/i2c/devices/5-0059*psu_temp2_input",
    "/sys/bus/i2c/devices/5-0059*psu_temp3_input"
};

static char* devfiles_as4581[] =  /* must map with onlp_thermal_id */
{
    NULL,
    "/sys/bus/i2c/devices/16-0048*temp1_input",
    "/sys/bus/i2c/devices/16-0049*temp1_input",
    "/sys/bus/i2c/devices/16-004b*temp1_input",
    "/sys/bus/i2c/devices/16-004c*temp1_input",
    "/sys/bus/i2c/devices/26-0060*temp1_input",
    NULL, /* CPU_CORE files */
    "/sys/bus/i2c/devices/12-0058*psu_temp1_input",
    "/sys/bus/i2c/devices/12-0058*psu_temp2_input",
    "/sys/bus/i2c/devices/12-0058*psu_temp3_input",
    "/sys/bus/i2c/devices/11-0059*psu_temp1_input",
    "/sys/bus/i2c/devices/11-0059*psu_temp2_input",
    "/sys/bus/i2c/devices/11-0059*psu_temp3_input"
};

static char* cpu_coretemp_files_as4581[] = {
    "/sys/class/thermal/thermal_zone0*temp1_input",
    "/sys/class/thermal/thermal_zone1*temp1_input",
    "/sys/class/thermal/thermal_zone2*temp1_input",
    "/sys/class/thermal/thermal_zone3*temp1_input",
    "/sys/class/thermal/thermal_zone4*temp1_input",
    "/sys/class/thermal/thermal_zone5*temp1_input",
    "/sys/class/thermal/thermal_zone6*temp1_input",
    NULL
};

char* onlp_thermal_get_devfiles(int tid)
{
    as45xx_52p_platform_id_t pid;
    char **devfiles[] = { devfiles_as4500, devfiles_as4581 };

    pid = get_platform_id();
    if (pid >= PID_UNKNOWN)
        return NULL;

    return devfiles[pid][tid];
}

/* Static values */
static onlp_thermal_info_t linfo = {
    { 0, {0}, 0, {0} },
      ONLP_THERMAL_STATUS_PRESENT,
      ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
};

onlp_oid_hdr_t hdr_as4500[] = {
    { }, /* Not used */
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD), "NST175-1-48", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD), "NST175-2-49", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD), "NST175-3-4B", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAIN_BROAD), "NST175-4-4C", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU1-2), "PSU-1 Thermal Sensor 1", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU1-2), "PSU-1 Thermal Sensor 2", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU1-2), "PSU-1 Thermal Sensor 3", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU2-2), "PSU-2 Thermal Sensor 1", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU2-2), "PSU-2 Thermal Sensor 2", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU2-2), "PSU-2 Thermal Sensor 3", 0, {0} }
};

onlp_oid_hdr_t hdr_as4581[] = {
    { }, /* Not used */
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD), "NST175-1-48", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD), "NST175-2-49", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD), "NST175-3-4B", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAIN_BROAD), "NST175-4-4C", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_ASC), "ASC10-1", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_CPU_CORE), "CPU Core", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU1), "PSU-1 Thermal Sensor 1", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU1), "PSU-1 Thermal Sensor 2", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU1), "PSU-1 Thermal Sensor 3", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU2), "PSU-2 Thermal Sensor 1", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU2), "PSU-2 Thermal Sensor 2", 0, {0} },
    { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU2), "PSU-2 Thermal Sensor 3", 0, {0} }
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int   tid;
    VALIDATE(id);
    as45xx_52p_platform_id_t pid;

    tid = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo;

    pid = get_platform_id();
    info->hdr = (pid == AS4581_52PL) ? hdr_as4581[tid] :
                                       hdr_as4500[tid] ;

    if ((pid == AS4581_52PL) && (tid == THERMAL_CPU_CORE)) {
        return onlp_file_read_int_max(&info->mcelsius, cpu_coretemp_files_as4581);
    }

    return onlp_file_read_int(&info->mcelsius, onlp_thermal_get_devfiles(tid));
}
