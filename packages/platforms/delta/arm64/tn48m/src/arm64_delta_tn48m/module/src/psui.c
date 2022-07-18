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
#include <onlp/platformi/psui.h>
#include "eeprom_info.h"
#include "platform_lib.h"

static int _psu_present(void *e);
static int _psu_event(void *e, int ev);

static plat_psu_t plat_tn48m_psus[] = {
    [PLAT_PSU_ID_1] = {
        .name = "PSU",
	.type = PLAT_PSU_TYPE_AC,
        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu2_powergood",
        .state = PLAT_PSU_STATE_PRESENT,
    },
};

static plat_psu_t plat_tn48m_poe_psus[] = {
    [PLAT_PSU_ID_1] = {
        .name = "PSU1",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/0-0041/psu1_present",

        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu1_powergood",

        .vin_path = "/sys/bus/i2c/devices/0-005b/psu_v_in",
        .iin_path = "/sys/bus/i2c/devices/0-005b/psu_i_in",
        .pin_path = "/sys/bus/i2c/devices/0-005b/psu_p_in",

        .vout_path = "/sys/bus/i2c/devices/0-005b/psu_v_out",
        .iout_path = "/sys/bus/i2c/devices/0-005b/psu_i_out",
        .pout_path = "/sys/bus/i2c/devices/0-005b/psu_p_out",

        .eeprom_bus = 0,
        .eeprom_addr= 0x53,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,
        .pmbus_insert_cmd = "echo tn48m_poe_psu 0x5b > /sys/bus/i2c/devices/i2c-0/new_device",
        .pmbus_remove_cmd = "echo 0x5b > /sys/bus/i2c/devices/i2c-0/delete_device",
        .pmbus_ready_path = "/sys/bus/i2c/devices/0-005b",
        .pmbus_bus = 0,
        .pmbus_addr = 0x5b,
    },
    [PLAT_PSU_ID_2] = {
        .name = "PSU2",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/0-0041/psu2_present",

        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu2_powergood",

        .vin_path = "/sys/bus/i2c/devices/0-005a/psu_v_in",
        .iin_path = "/sys/bus/i2c/devices/0-005a/psu_i_in",
        .pin_path = "/sys/bus/i2c/devices/0-005a/psu_p_in",

        .vout_path = "/sys/bus/i2c/devices/0-005a/psu_v_out",
        .iout_path = "/sys/bus/i2c/devices/0-005a/psu_i_out",
        .pout_path = "/sys/bus/i2c/devices/0-005a/psu_p_out",

        .eeprom_bus = 0,
        .eeprom_addr= 0x52,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,

        .pmbus_insert_cmd = "echo tn48m_poe_psu 0x5a > /sys/bus/i2c/devices/i2c-0/new_device",
        .pmbus_remove_cmd = "echo 0x5a > /sys/bus/i2c/devices/i2c-0/delete_device",
        .pmbus_ready_path = "/sys/bus/i2c/devices/0-005a",
        .pmbus_bus = 0,
        .pmbus_addr = 0x5a,

    },
};

static plat_psu_t plat_tn4810m_pvt_psus[] = {
    [PLAT_PSU_ID_1] = {
        .name = "PSU1",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/5-0041/psu1_present",

        .power_status_path = "/sys/bus/i2c/devices/5-0041/psu1_powergood",

        .eeprom_bus = 4,
        .eeprom_addr= 0x51,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,
    },
    [PLAT_PSU_ID_2] = {
        .name = "PSU2",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/5-0041/psu2_present",

        .power_status_path = "/sys/bus/i2c/devices/5-0041/psu2_powergood",

        .eeprom_bus = 3,
        .eeprom_addr= 0x50,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,
    },
};

static plat_psu_t plat_tn4810m_nonpvt_psus[] = {
    [PLAT_PSU_ID_1] = {
        .name = "PSU1",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/0-0041/psu1_present",

        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu1_powergood",

        .eeprom_bus = 0,
        .eeprom_addr= 0x51,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,
    },
    [PLAT_PSU_ID_2] = {
        .name = "PSU2",
        .type = PLAT_PSU_TYPE_AC,
        .present = _psu_present,
        .present_path = "/sys/bus/i2c/devices/0-0041/psu2_present",

        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu2_powergood",

        .eeprom_bus = 0,
        .eeprom_addr= 0x50,
        .event_callback = _psu_event,

        .state = PLAT_PSU_STATE_UNPRESENT,
    },
};

static plat_psu_t plat_tn48m2_psus[] = {
    [PLAT_PSU_ID_1] = {
        .name = "PSU1",
        .type = PLAT_PSU_TYPE_AC,
        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu1_powergood",
        .state = PLAT_PSU_STATE_PRESENT,
    },
    [PLAT_PSU_ID_2] = {
        .name = "PSU2",
        .type = PLAT_PSU_TYPE_AC,
        .power_status_path = "/sys/bus/i2c/devices/0-0041/psu2_powergood",
        .state = PLAT_PSU_STATE_PRESENT,
    },
};

static plat_psu_t* get_plat_psu()
{
    if (gPlat_id == PID_TN48M)
        return plat_tn48m_psus;
    else if (gPlat_id == PID_TN48M_POE)
        return plat_tn48m_poe_psus;
    else if (gPlat_id == PID_TN4810M_PVT)
        return plat_tn4810m_pvt_psus;
    else if (gPlat_id == PID_TN4810M_NONPVT)
        return plat_tn4810m_nonpvt_psus;
    else
        /* tn48m2 and tn48m2-swdev shares the same psu configuration */
        return plat_tn48m2_psus;
}

static int plat_psu_is_valid(plat_psu_t *plat_psus, int id)
{
    if (id > PLAT_PSU_ID_INVALID && id < PLAT_PSU_ID_MAX) {
        if (plat_psus[id].name)
            return 1;
    }
    return 0;
}

static int plat_psu_state_update (plat_psu_t *psu)
{
    int present;
    plat_psu_state_t old_state;

    do {
        old_state = psu->state;
        present = PLAT_PSU_STATE_PRESENT;
        if (psu->present) {
            present = psu->present(psu);
        }

        switch (psu->state) {
        case PLAT_PSU_STATE_UNPRESENT:
            if (present == PLAT_PSU_STATE_PRESENT) {
                psu->state = PLAT_PSU_STATE_PRESENT;
                if (psu->event_callback)
                    psu->event_callback(psu, PLAT_PSU_EVENT_PLUGIN);
            }
            break;
        case PLAT_PSU_STATE_PRESENT:
            if (present == PLAT_PSU_STATE_UNPRESENT) {
                psu->state = PLAT_PSU_STATE_UNPRESENT;
                if (psu->event_callback)
                    psu->event_callback(psu, PLAT_PSU_EVENT_UNPLUG);

                break;
            }

            if (psu->pmbus_addr) {
                if (onlp_i2c_readb(psu->pmbus_bus, psu->pmbus_addr, 0x00,
                    ONLP_I2C_F_FORCE | ONLP_I2C_F_DISABLE_READ_RETRIES) >= 0) {
                    psu->state = PLAT_PSU_STATE_PMBUS_READY;
                    if (!plat_os_file_is_existed(psu->pmbus_ready_path) &&
                        psu->pmbus_insert_cmd) {
                        system(psu->pmbus_insert_cmd);
                    }
                    if (psu->event_callback)
                        psu->event_callback(psu, PLAT_PSU_PMBUS_CONNECT);
                }
            }
            break;
        case PLAT_PSU_STATE_PMBUS_READY:
            // If unplug, remove the pmbus device
            if (present == PLAT_PSU_STATE_UNPRESENT) {
                psu->state = PLAT_PSU_STATE_UNPRESENT;
                if (plat_os_file_is_existed(psu->pmbus_ready_path) &&
                    psu->pmbus_remove_cmd) {
                    system(psu->pmbus_remove_cmd);
                }
                if (psu->event_callback)
                    psu->event_callback(psu, PLAT_PSU_EVENT_UNPLUG);
                break;
            }

            if (psu->pmbus_addr) {
                // If pmbus interface is not ok, remove the pmbus device
                if (onlp_i2c_readb(psu->pmbus_bus, psu->pmbus_addr, 0x00,
                    ONLP_I2C_F_FORCE | ONLP_I2C_F_DISABLE_READ_RETRIES) < 0) {
                    psu->state = PLAT_PSU_STATE_PRESENT;
                    if (plat_os_file_is_existed(psu->pmbus_ready_path) &&
                        psu->pmbus_remove_cmd) {
                        system(psu->pmbus_remove_cmd);
                    }
                    if (psu->event_callback)
                        psu->event_callback(psu, PLAT_PSU_PMBUS_DISCONNECT);

                    break;
                }
            }
            break;
        default:
            break;
        }
    } while (old_state != psu->state);

    return 0;
}

static int _psu_present(void *e)
{
    plat_psu_t *psu = e;
    int present = PLAT_PSU_STATE_UNPRESENT;

    if (psu->present_path) {
        if (plat_os_file_read_int(&present, psu->present_path, NULL) < 0)
            return PLAT_PSU_STATE_UNPRESENT;
    }

    return present;
}

static int _psu_event (void *e, int ev)
{
    plat_psu_t *psu = e;

    switch (ev) {
    case PLAT_PSU_EVENT_PLUGIN:
        if (eeprom_read(psu->eeprom_bus,
                        psu->eeprom_addr,
                        0x00,
                        psu->eeprom,
                        sizeof(psu->eeprom)))
            memset(psu->eeprom, 0xff, sizeof(psu->eeprom));
        break;
    case PLAT_PSU_EVENT_UNPLUG:
        memset(psu->eeprom, 0xff, sizeof(psu->eeprom));
        break;
    case PLAT_PSU_PMBUS_CONNECT:
    case PLAT_PSU_PMBUS_DISCONNECT:
    default:
        break;
    }

    return 0;
}

static uint32_t _psu_vin_type_guess(int vin)
{
    uint32_t ret;

    ret = 0;
    if (vin >= 110000)
        ret |= ONLP_PSU_CAPS_AC;
    else if (vin >= 48000)
        ret |= ONLP_PSU_CAPS_DC48;
    else if (vin >= 12000)
        ret |= ONLP_PSU_CAPS_DC12;

    if (!ret)
        ret |= ONLP_PSU_CAPS_AC;

    return ret;
}

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    plat_psu_t *psu;
    int error;
    int powergood = 0;
    int pid = ONLP_OID_ID_GET(id);
    plat_psu_t *plat_psus = get_plat_psu();

    if (!ONLP_OID_IS_PSU(id))
        return ONLP_STATUS_E_INVALID;

    if (!plat_psu_is_valid(plat_psus, pid))
        return ONLP_STATUS_E_INVALID;

    psu = &plat_psus[pid];

    memset(info, 0, sizeof(onlp_psu_info_t));

    info->hdr.id = id;
    if (psu->name)
        snprintf(info->hdr.description, sizeof(info->hdr.description),
                 "%s", psu->name);

    plat_psu_state_update(psu);

    if (psu->state == PLAT_PSU_STATE_PRESENT ||
        psu->state == PLAT_PSU_STATE_PMBUS_READY) {
        info->status |= ONLP_PSU_STATUS_PRESENT;
        info->status &= ~ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        info->status |= ONLP_PSU_STATUS_UNPLUGGED;
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
    }

    // If unpresent then return directly
    if (info->status & ONLP_PSU_STATUS_UNPLUGGED) {
        return ONLP_STATUS_OK;
    }

    // get psu info
    eeprom_info_get(psu->eeprom, sizeof(psu->eeprom), "psu_model", info->model);
    eeprom_info_get(psu->eeprom, sizeof(psu->eeprom), "psu_series", info->serial);

    // Check Power Good Status
    if (psu->power_status_path)
        plat_os_file_read_int(&powergood, psu->power_status_path, NULL);

    // If power not good then return directly
    if (!powergood) {
        info->status |=  ONLP_PSU_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }

    ///////////////////////////////////////////////////////////////
    // Get caps
    if (psu->vin_path && plat_os_file_is_existed(psu->vin_path))
        info->caps |= ONLP_PSU_CAPS_VIN;
    if (psu->iin_path && plat_os_file_is_existed(psu->iin_path))
        info->caps |= ONLP_PSU_CAPS_IIN;
    if (psu->pin_path && plat_os_file_is_existed(psu->pin_path))
        info->caps |= ONLP_PSU_CAPS_PIN;
    if (psu->vout_path && plat_os_file_is_existed(psu->vout_path))
        info->caps |= ONLP_PSU_CAPS_VOUT;
    if (psu->iout_path && plat_os_file_is_existed(psu->iout_path))
        info->caps |= ONLP_PSU_CAPS_IOUT;
    if (psu->pout_path && plat_os_file_is_existed(psu->pout_path))
        info->caps |= ONLP_PSU_CAPS_POUT;

    ///////////////////////////////////////////////////////////////
    // Get and check value
    error = 0;
    if (info->caps & ONLP_PSU_CAPS_VIN) {
        if (plat_os_file_read_int(&info->mvin, psu->vin_path, NULL) < 0) {
            error ++;
        } else {
            if (info->mvin == 0)
                info->status |= ONLP_PSU_STATUS_FAILED;
        }

        // Auto detect AC / DC type
        // The PSU capability is decided by Vin value
        info->caps |= _psu_vin_type_guess(info->mvin);
    } else {
        // The PSU capability is decided by configuration
        if (psu->type == PLAT_PSU_TYPE_AC) {
            info->caps |= ONLP_PSU_CAPS_AC;
        } else if (psu->type == PLAT_PSU_TYPE_DC12) {
            info->caps |= ONLP_PSU_CAPS_DC12;
        } else if (psu->type == PLAT_PSU_TYPE_DC48) {
            info->caps |= ONLP_PSU_CAPS_DC48;
        } else {
            info->status |= ONLP_PSU_STATUS_FAILED;
        }
    }

    //// If VIN is not ok, skip other
    if ((info->status & ONLP_PSU_STATUS_FAILED) == 0) {
        if (info->caps & ONLP_PSU_CAPS_IIN) {
            if (plat_os_file_read_int(&info->miin, psu->iin_path, NULL) < 0)
                error ++;
        }
        if (info->caps & ONLP_PSU_CAPS_PIN) {
            if (plat_os_file_read_int(&info->mpin, psu->pin_path, NULL) < 0)
                error ++;
        }
        if (info->caps & ONLP_PSU_CAPS_VOUT) {
            if (plat_os_file_read_int(&info->mvout, psu->vout_path, NULL) < 0)
                error ++;
        }
        if (info->caps & ONLP_PSU_CAPS_IOUT) {
            if (plat_os_file_read_int(&info->miout, psu->iout_path, NULL) < 0)
                error ++;
        }
        if (info->caps & ONLP_PSU_CAPS_POUT) {
            if (plat_os_file_read_int(&info->mpout, psu->pout_path, NULL) < 0)
                error ++;
        }
    }

    return error ? ONLP_STATUS_E_INTERNAL : ONLP_STATUS_OK;
}

int
onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

