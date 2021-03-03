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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <onlplib/file.h>
#include <limits.h>
#include <unistd.h>
#include "arm64_delta_tn48m_log.h"
#include "platform_lib.h"

#define TN48M_FAN_DEF_CAP   ONLP_FAN_CAPS_F2B

static int _fan_present(void *e);
static int _psu_fan_present(void *e);

static plat_fan_t plat_fans[] = {
    [PLAT_FAN_ID_1] = {
        .name = "Chassis Fan 1",
        .model = "ADT7473",
        .present = _fan_present,
        .present_data = NULL,
        .rpm_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/fan1_input",
        .rpm_set_path = NULL,
        .max_rpm = 13000,
        .per_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm1",
        .per_set_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm1",
        .caps = TN48M_FAN_DEF_CAP,

        PLAT_FAN_INTERNAL_DEF,
    },
    [PLAT_FAN_ID_2] = {
        .name = "Chassis Fan 2",
        .model = "ADT7473",
        .present = _fan_present,
        .present_data = NULL,
        .rpm_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/fan2_input",
        .rpm_set_path = NULL,
        .max_rpm = 13000,
        .per_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm2",
        .per_set_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm2",
        .caps = TN48M_FAN_DEF_CAP,

        PLAT_FAN_INTERNAL_DEF,
    },
    [PLAT_FAN_ID_3] = {
        .name = "Chassis Fan 3",
        .model = "ADT7473",
        .present = _fan_present,
        .present_data = NULL,
        .rpm_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/fan3_input",
        .rpm_set_path = NULL,
        .max_rpm = 13000,
        .per_get_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm3",
        .per_set_path = "/sys/bus/i2c/devices/1-002e/hwmon/*/pwm3",
        .caps = TN48M_FAN_DEF_CAP,

        PLAT_FAN_INTERNAL_DEF,
    },
    [PLAT_FAN_ID_4] = {
        .name = "PSU1 Fan",
        .present = _psu_fan_present,
        .rpm_get_path = "/sys/bus/i2c/devices/0-005b/psu_fan1_speed_rpm",
        .rpm_set_path = NULL,
        .per_get_path = NULL,
        .per_set_path = NULL,
        .caps = TN48M_FAN_DEF_CAP,

        PLAT_FAN_INTERNAL_DEF,
    },
    [PLAT_FAN_ID_5] = {
        .name = "PSU2 Fan",
        .present = _psu_fan_present,

        .rpm_get_path = "/sys/bus/i2c/devices/0-005a/psu_fan1_speed_rpm",
        .rpm_set_path = NULL,
        .per_get_path = NULL,
        .per_set_path = NULL,
        .caps = TN48M_FAN_DEF_CAP,

        PLAT_FAN_INTERNAL_DEF,
    },
};


static int _psu_fan_present(void *e)
{
    plat_fan_t *fan = e;
    return plat_os_file_is_existed(fan->rpm_get_path) ? 1 : 0;
}

static int _fan_present(void *e)
{
    plat_fan_t *fan = e;
    int fan_rpm = 0;

    fan->state = PLAT_FAN_STATE_UNPRESENT;

    if (fan->rpm_get_path) {
        plat_os_file_read_int(&fan_rpm, fan->rpm_get_path, NULL);

        if (fan_rpm == 0) fan->state = PLAT_FAN_STATE_UNPRESENT;
        else fan->state = PLAT_FAN_STATE_PRESENT;
    }

    return fan->state;
}

static int plat_fan_is_valid(int id)
{
    if (id > PLAT_FAN_ID_INVALID && id < PLAT_FAN_ID_MAX) {
        if (plat_fans[id].name)
            return 1;
    }
    return 0;
}

int onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}


int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int error;
    plat_fan_t *fan;
    int fid;
    int present = 0;
    int rpm_lower_bound, rpm_upper_bound;

    if (!ONLP_OID_IS_FAN(id)) {
        AIM_LOG_ERROR("not a valid oid");
        return ONLP_STATUS_E_INVALID;
    }

    fid = ONLP_OID_ID_GET(id);

    if (!plat_fan_is_valid(fid)) {
        AIM_LOG_ERROR("plat_fan_is_valid failed");
        return ONLP_STATUS_E_INVALID;
    }

    fan = &plat_fans[fid];

    if (fan->present)
        present = fan->present(fan);

    memset (info, 0, sizeof(*info));

    info->hdr.id = id;
    if (fan->name)
        snprintf(info->hdr.description, sizeof(info->hdr.description),
                 "%s", fan->name);
    if (fan->model)
        snprintf(info->model, sizeof(info->model), "%s", fan->model);

    info->caps = fan->caps;
    if (fan->rpm_get_path) info->caps |= ONLP_FAN_CAPS_GET_RPM;
    if (fan->rpm_set_path) info->caps |= ONLP_FAN_CAPS_SET_RPM;
    if (fan->per_get_path) info->caps |= ONLP_FAN_CAPS_GET_PERCENTAGE;
    if (fan->per_set_path) info->caps |= ONLP_FAN_CAPS_SET_PERCENTAGE;

    error = 0;
    if (present) {
        info->status |= ONLP_FAN_STATUS_PRESENT;
        if (info->caps & ONLP_FAN_CAPS_GET_PERCENTAGE) {
            if (plat_os_file_read_int(&info->percentage,
                                      fan->per_get_path, NULL) < 0) {
                AIM_LOG_TRACE("plat_os_file_read_int failed for "
                              "ONLP_FAN_CAPS_GET_PERCENTAGE path=%s",
                              fan->per_get_path);
                error++;
            } else
                info->percentage = (info->percentage * 100) / 255;
        }
        if (info->caps & ONLP_FAN_CAPS_GET_RPM) {
            /* fan speed tolerance: 25% */
            rpm_lower_bound = (info->percentage * fan->max_rpm * 75) / 10000;
            rpm_upper_bound = (info->percentage * fan->max_rpm * 125) / 10000;

            int retry = 5;
            while (retry >= 0) {
                if (plat_os_file_read_int(&info->rpm,
                                          fan->rpm_get_path, NULL) < 0) {
                    AIM_LOG_TRACE("plat_os_file_read_int failed for "
                                  "ONLP_FAN_CAPS_GET_RPM rpm_path=%s",
                                  fan->rpm_get_path);
                    error++;
                } else if (fid <= PLAT_FAN_ID_3){
                    if (info->rpm >= rpm_lower_bound &&
                        info->rpm <= rpm_upper_bound) {
                        break;
                    } else if (retry == 0) {
                        info->status |= ONLP_FAN_STATUS_FAILED;
                        break;
                    }

                    /*  If the current fan rpm is out of lower/upper bond,
                     *  wait 1 second and retry
                     */
                    sleep(1);
                    retry--;
                } else
		    break;
            }
        }

    }

    if ((info->caps & (ONLP_FAN_CAPS_B2F | ONLP_FAN_CAPS_B2F)) ==
                       (ONLP_FAN_CAPS_B2F | ONLP_FAN_CAPS_B2F)) {
        // Should check it auto
        // TODO
    } else if (info->caps & ONLP_FAN_CAPS_B2F) {
        info->status |= ONLP_FAN_STATUS_B2F;
    } else if (info->caps & ONLP_FAN_CAPS_F2B) {
        info->status |= ONLP_FAN_STATUS_F2B;
    }

    return error ? ONLP_STATUS_E_INTERNAL : ONLP_STATUS_OK;
}


/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int error;
    int fid;
    int pwm;
    plat_fan_t *fan;

    if (!ONLP_OID_IS_FAN(id))
        return ONLP_STATUS_E_INVALID;

    fid = ONLP_OID_ID_GET(id);
    if (!plat_fan_is_valid(fid))
        return ONLP_STATUS_E_INVALID;

    fan = &plat_fans[fid];

    if (fan->per_set_path) {
        /* Reject percentage = 0 (stop fan) */
        if (p == 0) return ONLP_STATUS_E_INVALID;

        /* Fan pwm value from 0x00 to 0xFF */
        pwm = (p * 255) / 100;

        error = plat_os_file_write_int(pwm, fan->per_set_path, NULL);
    } else
        return ONLP_STATUS_E_UNSUPPORTED;

    if (error < 0) {
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}


/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

