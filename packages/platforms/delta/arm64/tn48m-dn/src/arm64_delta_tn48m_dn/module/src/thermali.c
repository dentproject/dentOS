/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *       Copyright 2014, 2015 Big Switch Networks, Inc.
 *       Copyright 2017, 2020, 2021 Delta Networks, Inc.
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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/thermali.h>
#include "arm64_delta_tn48m_dn_log.h"
#include "platform_lib.h"

static int _psu_thermal_present(void *e);

////////////////////////////////////////////////////////////////
// THERMALS PLAT CONFIG
static plat_thermal_t plat_thermals[] = {
    [PLAT_THERMAL_ID_1] = {
        .name = "Thermal Sensor %d - CPU - AP_IC",
        .temp_get_path     = "/sys/class/thermal/thermal_zone0/temp",

        .warning_get_path  = NULL,
        .critical_get_path = NULL,
        .shutdown_get_path = "/sys/class/thermal/thermal_zone0/trip_point_0_temp",

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_2] = {
        .name = "Thermal Sensor %d - CPU - AP_CPU0",
        .temp_get_path     = "/sys/class/thermal/thermal_zone1/temp",

        .warning_get_path  = "/sys/class/thermal/thermal_zone1/trip_point_0_temp",
        .critical_get_path = "/sys/class/thermal/thermal_zone1/trip_point_1_temp",
        .shutdown_get_path = NULL,

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_3] = {
        .name = "Thermal Sensor %d - CPU - AP_CPU1",
        .temp_get_path     = "/sys/class/thermal/thermal_zone2/temp",

        .warning_get_path  = "/sys/class/thermal/thermal_zone2/trip_point_0_temp",
        .critical_get_path = "/sys/class/thermal/thermal_zone2/trip_point_1_temp",
        .shutdown_get_path = NULL,

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_4] = {
        .name = "Thermal Sensor %d - CPU - AP_CPU2",
        .temp_get_path     = "/sys/class/thermal/thermal_zone3/temp",

        .warning_get_path  = "/sys/class/thermal/thermal_zone3/trip_point_0_temp",
        .critical_get_path = "/sys/class/thermal/thermal_zone3/trip_point_1_temp",
        .shutdown_get_path = NULL,

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_5] = {
        .name = "Thermal Sensor %d - CPU - AP_CPU3",
        .temp_get_path     = "/sys/class/thermal/thermal_zone4/temp",

        .warning_get_path  = "/sys/class/thermal/thermal_zone4/trip_point_0_temp",
        .critical_get_path = "/sys/class/thermal/thermal_zone4/trip_point_1_temp",
        .shutdown_get_path = NULL,

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_6] = {
        .name = "Thermal Sensor %d - CPU - CP_IC",
        .temp_get_path     = "/sys/class/thermal/thermal_zone5/temp",

        .warning_get_path  = NULL,
        .critical_get_path = NULL,
        .shutdown_get_path = "/sys/class/thermal/thermal_zone5/trip_point_0_temp",

        .def_warning  = 85000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = false,
    },
    [PLAT_THERMAL_ID_7] = {
        .name = "Thermal Sensor %d - Near to PHY",
        .temp_get_path     = "/sys/bus/i2c/devices/1-004a/hwmon/*/temp1_input",

        .warning_get_path  = "/sys/bus/i2c/devices/1-004a/hwmon/*/temp1_max_hyst",
        .critical_get_path = NULL,
        .shutdown_get_path = "/sys/bus/i2c/devices/1-004a/hwmon/*/temp1_max",

        .def_warning = 90000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = true,
    },
    [PLAT_THERMAL_ID_8] = {
        .name = "Thermal Sensor %d - Near to MAC",
        .temp_get_path     = "/sys/bus/i2c/devices/1-004b/hwmon/*/temp1_input",

        .warning_get_path  = "/sys/bus/i2c/devices/1-004b/hwmon/*/temp1_max_hyst",
        .critical_get_path = NULL,
        .shutdown_get_path = "/sys/bus/i2c/devices/1-004b/hwmon/*/temp1_max",

        .def_warning  = 90000,
        .def_critical = 95000,
        .def_shutdown = 100000,

        .enable_set_thresholds = true,
    },
    [PLAT_THERMAL_ID_9] = {
        .name = "Thermal Sensor %d - FAN Controller TEMP 1",
        .temp_get_path     = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp1_input",

        .warning_get_path  = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp1_crit_hyst",
        .critical_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp1_crit",
        .shutdown_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp1_max",

        .def_critical = 90000,
        .def_shutdown = 100000,

        .enable_set_thresholds = true,
    },
    [PLAT_THERMAL_ID_10] = {
        .name = "Thermal Sensor %d - FAN Controller TEMP 2",
        .temp_get_path     = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp2_input",

        .warning_get_path  = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp2_crit_hyst",
        .critical_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp2_crit",
        .shutdown_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp2_max",

        .def_critical = 90000,
        .def_shutdown = 100000,

        .enable_set_thresholds = true,
    },
    [PLAT_THERMAL_ID_11] = {
        .name = "Thermal Sensor %d - FAN Controller TEMP 3",
        .temp_get_path     = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp3_input",

        .warning_get_path  = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp3_crit_hyst",
        .critical_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp3_crit",
        .shutdown_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/temp3_max",

        .def_critical = 90000,
        .def_shutdown = 100000,

        .enable_set_thresholds = true,
    },
    [PLAT_THERMAL_ID_12] = {
        .name = "Thermal Sensor %d - PSU1",
        .present = _psu_thermal_present,
        .temp_get_path     = "/sys/bus/i2c/devices/0-005b/psu_temp1_input",

        .def_warning  = ONLP_THERMAL_THRESHOLD_WARNING_DEFAULT,
        .def_critical = ONLP_THERMAL_THRESHOLD_ERROR_DEFAULT,
        .def_shutdown = ONLP_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT,
    },
    [PLAT_THERMAL_ID_13] = {
        .name = "Thermal Sensor %d - PSU2",
        .present = _psu_thermal_present,
        .temp_get_path     = "/sys/bus/i2c/devices/0-005a/psu_temp1_input",

        .def_warning  = ONLP_THERMAL_THRESHOLD_WARNING_DEFAULT,
        .def_critical = ONLP_THERMAL_THRESHOLD_ERROR_DEFAULT,
        .def_shutdown = ONLP_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT,
    },
};

static int _psu_thermal_present(void *e)
{
    plat_thermal_t *thermal = e;
    return plat_os_file_is_existed(thermal->temp_get_path) ? 1 : 0;
}

static int plat_thermal_is_valid(int id)
{
    if (id > PLAT_THERMAL_ID_INVALID && id < PLAT_THERMAL_ID_MAX) {
        if (plat_thermals[id].name)
            return 1;
    }
    return 0;
}

int onlp_thermali_init(void)
{
    int i;
    plat_thermal_t *thermal;

    for (i = 0 ; i < PLAT_THERMAL_ID_MAX - 1; i ++) {
        if (!plat_thermal_is_valid (i))
            continue;
        thermal = &plat_thermals[i];

        if (thermal->enable_set_thresholds) {
            if (thermal->warning_get_path && thermal->def_warning)
                plat_os_file_write_int(thermal->def_warning,
                                       thermal->warning_get_path,
                                       NULL);
            if (thermal->critical_get_path && thermal->def_critical)
                plat_os_file_write_int(thermal->def_critical,
                                       thermal->critical_get_path,
                                       NULL);
            if (thermal->shutdown_get_path && thermal->def_shutdown)
                plat_os_file_write_int(thermal->def_shutdown,
                                       thermal->shutdown_get_path,
                                       NULL);
        }
    }
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
    int tid;
    int present = 1;
    plat_thermal_t *thermal;
    int value;
    int error;

    if (!ONLP_OID_IS_THERMAL(id)) {
        AIM_LOG_ERROR("Not a valid oid");
        return ONLP_STATUS_E_INVALID;
    }

    tid = ONLP_OID_ID_GET(id);

    if (!plat_thermal_is_valid(tid)) {
        AIM_LOG_ERROR("Not a valid platform thermal id");
        return ONLP_STATUS_E_INVALID;
    }

    thermal = &plat_thermals[tid];

    if (thermal->present) {
        present = thermal->present(thermal) ? 1 : 0;
    }

    memset(info, 0, sizeof(*info));

    // Fix onlp_thermal_info_t
    info->hdr.id = id;
    if (thermal->name)
        snprintf(info->hdr.description,
                 sizeof(info->hdr.description),
                 thermal->name, tid);

    if (thermal->temp_get_path)
        info->caps |= ONLP_THERMAL_CAPS_GET_TEMPERATURE;
    if (thermal->warning_get_path || thermal->def_warning)
        info->caps |= ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
    if (thermal->critical_get_path || thermal->def_critical)
        info->caps |= ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
    if (thermal->shutdown_get_path || thermal->def_shutdown)
        info->caps |= ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;

    // Get value
    error = 0;
    if (present) {
        if (info->caps & ONLP_THERMAL_CAPS_GET_TEMPERATURE) {
            if (plat_os_file_read_int(&value,
                                      thermal->temp_get_path,
                                      NULL) < 0) {
                AIM_LOG_TRACE("plat_os_file_read_int failed for "
                              "ONLP_THERMAL_CAPS_GET_TEMPERATURE, "
                              "path=%s", thermal->temp_get_path);
                error ++;
            } else
                info->mcelsius = value;
        }
        if (info->caps & ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD) {
            if (thermal->warning_get_path) {
                if (plat_os_file_read_int(&value,
                                          thermal->warning_get_path,
                                          NULL) < 0) {
                    AIM_LOG_TRACE("plat_os_file_read_int failed for "
                                  "ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD, "
                                  "path=%s", thermal->warning_get_path);
                    error ++;
                } else
                    info->thresholds.warning = value;
            } else {
                info->thresholds.warning = thermal->def_warning;
            }
        }
        if (info->caps & ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD) {
            if (thermal->critical_get_path) {
                if (plat_os_file_read_int(&value,
                                          thermal->critical_get_path,
                                          NULL) < 0) {
                    AIM_LOG_TRACE("plat_os_file_read_int failed for "
                                  "ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD,"
                                  "path=%s", thermal->critical_get_path);
                    error ++;
                } else
                    info->thresholds.error = value;
            } else {
                    info->thresholds.error = thermal->def_critical;
            }
        }
        if (info->caps & ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD) {
            if (thermal->shutdown_get_path) {
                if (plat_os_file_read_int(&value,
                                          thermal->shutdown_get_path,
                                          NULL) < 0) {
                    AIM_LOG_TRACE("plat_os_file_read_int failed for "
                                  "ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD, "
                                  "path=%s", thermal->shutdown_get_path);
                    error ++;
                } else
                    info->thresholds.shutdown = value;
            } else {
                info->thresholds.shutdown = thermal->def_shutdown;
            }
        }
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    }

    // Check threshold
    if (info->caps & ONLP_THERMAL_CAPS_GET_TEMPERATURE) {
        if (info->caps & ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD) {
            if (info->mcelsius >= info->thresholds.error) {
                info->status |= ONLP_THERMAL_STATUS_FAILED;
            }
        }
        if (info->caps & ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD) {
            if (info->mcelsius >= info->thresholds.shutdown) {
                info->status |= ONLP_THERMAL_STATUS_FAILED;
            }
        }
    }

    return error ? ONLP_STATUS_E_INTERNAL : ONLP_STATUS_OK;
}

