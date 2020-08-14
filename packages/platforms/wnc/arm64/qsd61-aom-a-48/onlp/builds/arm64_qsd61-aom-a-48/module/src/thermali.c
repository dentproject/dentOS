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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>
#include "platform_lib.h"
#include <onlplib/file.h>

#define THERMAL_PATH_FORMAT "/sys/bus/i2c/devices/%s/hwmon/hwmon%s/temp1_input"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


static char* i2c_directory[] =	/* must map with onlp_thermal_id */
{
	NULL,
	NULL,        /* CPU_CORE files */
	"6-0048",	 /* on MAIN board */
	"7-0049",	 /* on MAIN board */
	"8-004a",	 /* on MAIN board *//* fail */
};

static char* hwmon_directory[] =  /* must map with onlp_thermal_id */
{
    NULL,
	NULL,   /* CPU_CORE files */
    "1",   	/* on MAIN board */
    "2",   	/* on MAIN board */
    "3",   	/* on MAIN board */
};

enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
	THERMAL_CPU_CORE,
    THERMAL_1_ON_MAIN_BROAD,
    THERMAL_2_ON_MAIN_BROAD,
    THERMAL_3_ON_MAIN_BROAD,
};

/* Static values */
static onlp_thermal_info_t linfo[] = {
	{ }, /* Not used */
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_CPU_CORE), "CPU Core", 0},
      ONLP_THERMAL_STATUS_PRESENT,
      ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD), "TMP75_MAIN_1", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD), "TMP75_MAIN_2", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD), "TMP75_MAIN_3", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        }
};

/*
 * Get CPU temperature for system nodes and return the maximum temprature of them.
*/
int static get_cpu_core_temp_max(void)
{
    FILE *fp;
    int tmp, max_temp = 0;

    /* Get system node of cpu core temprature */
    if ((fp = popen("ls /sys/class/thermal/thermal_zone*/temp | xargs cat", "r")) == NULL){
        AIM_LOG_ERROR("Unable to get numbers of cpu core temprature system node \r\n");
    }

    while (fscanf(fp, "%d", &tmp) != EOF)
    {
        if (tmp > max_temp)
            max_temp = tmp;
    }

    pclose(fp);

    return max_temp;
}

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    // FIXME: WARNING FREEZE
    if ( 0 ) {
        printf("%d", linfo[0].status);
    }
    // END WARNING FREEZE
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
    char  path[64] = {0};

    VALIDATE(id);

    tid = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[tid];

    switch (tid) 
    {
    	case THERMAL_CPU_CORE:
            info->mcelsius = get_cpu_core_temp_max();
			if(!info->mcelsius) {
				return ONLP_STATUS_E_INTERNAL;
            }
			break;
        case THERMAL_1_ON_MAIN_BROAD:
        case THERMAL_2_ON_MAIN_BROAD:
        case THERMAL_3_ON_MAIN_BROAD:
            /* get path */
            sprintf(path, THERMAL_PATH_FORMAT, i2c_directory[tid], hwmon_directory[tid]);

            if (onlp_file_read_int(&info->mcelsius, path) < 0) {
                AIM_LOG_ERROR("Unable to read thermal from file (%s)\r\n", path);
                return ONLP_STATUS_E_INTERNAL;
            }
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    };

    return ONLP_STATUS_OK;
}

