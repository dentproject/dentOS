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
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <onlplib/file.h>
#include <limits.h>

#include "platform_lib.h"

static plat_led_t plat_leds[] = {
    [PLAT_LED_ID_1] = {
        .name = "Front LED - PSU",
        .led_get_path = "/sys/class/leds/tn48m_dn::psu/brightness",
        .led_set_path = "/sys/class/leds/tn48m_dn::psu/brightness",
        .mode = {
            PLAT_LED_MODE(ONLP_LED_MODE_OFF,    LED_MODE_OFF),
            PLAT_LED_MODE(ONLP_LED_MODE_GREEN,  LED_MODE_GREEN),
            PLAT_LED_MODE(ONLP_LED_MODE_ORANGE, LED_MODE_AMBER),
            PLAT_LED_MODE_END,
        },
        PLAT_LED_INTERNAL_DEF,
    },
    [PLAT_LED_ID_2] = {
        .name = "Front LED - FAN",
        .led_get_path = "/sys/class/leds/tn48m_dn::fan/brightness",
        .led_set_path = "/sys/class/leds/tn48m_dn::fan/brightness",
        .mode = {
            PLAT_LED_MODE(ONLP_LED_MODE_OFF,    LED_MODE_OFF),
            PLAT_LED_MODE(ONLP_LED_MODE_GREEN,  LED_MODE_GREEN),
            PLAT_LED_MODE(ONLP_LED_MODE_ORANGE, LED_MODE_AMBER),
            PLAT_LED_MODE_END,
        },
        PLAT_LED_INTERNAL_DEF,
    },
    [PLAT_LED_ID_3] = {
        .name = "Front LED - SYS",
        .led_get_path = "/sys/class/leds/tn48m_dn::sys/brightness",
        .led_set_path = "/sys/class/leds/tn48m_dn::sys/brightness",
        .mode = {
            PLAT_LED_MODE(ONLP_LED_MODE_OFF,    LED_MODE_OFF),
            PLAT_LED_MODE(ONLP_LED_MODE_GREEN,  LED_MODE_GREEN),
            PLAT_LED_MODE(ONLP_LED_MODE_ORANGE, LED_MODE_AMBER),
            PLAT_LED_MODE_END,
        },
        PLAT_LED_INTERNAL_DEF,
    },
};

static int plat_led_is_valid(int id)
{
    if (id > PLAT_LED_ID_INVALID && id < PLAT_LED_ID_MAX) {
        if (plat_leds[id].name)
            return 1;
    }
    return 0;
}

static int sys_led_to_onlp_led(int id, int tgt_led_mode)
{
    plat_led_t *led = &plat_leds[id];
    led_mode_t *mod = &led->mode[0];

    while (mod->sys_led_mode >= 0) {
        if (mod->sys_led_mode == tgt_led_mode)
            return mod->onlp_led_mode;
        mod ++;
    }
    return -1;
}

static int onlp_led_to_sys_led(int id, int tgt_led_mode)
{
    plat_led_t *led = &plat_leds[id];
    led_mode_t *mod = &led->mode[0];

    while (mod->onlp_led_mode >= 0) {
        if (mod->onlp_led_mode == tgt_led_mode)
            return mod->sys_led_mode;
        mod ++;
    }
    return -1;
}

static uint32_t _onlp_cap_create(led_mode_t *mod)
{
    uint32_t cap = 0;

    while (mod->onlp_led_mode >= 0) {
        switch (mod->onlp_led_mode) {
        case ONLP_LED_MODE_OFF:     cap |= ONLP_LED_CAPS_ON_OFF; break;
        case ONLP_LED_MODE_ORANGE:  cap |= ONLP_LED_CAPS_ORANGE; break;
        case ONLP_LED_MODE_GREEN:   cap |= ONLP_LED_CAPS_GREEN; break;
        }
        mod ++;
    }
    return cap;
}


int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int rc;
    int lid, mode;
    plat_led_t *led;
    led_mode_t *mod;

    if (!ONLP_OID_IS_LED(id))
        return ONLP_STATUS_E_INVALID;

    lid = ONLP_OID_ID_GET(id);

    if (!plat_led_is_valid (lid))
        return ONLP_STATUS_E_INVALID;

    /* Set the onlp_oid_hdr_t and capabilities */
    led = &plat_leds[lid];
    mod = &led->mode[0];

    memset (info, 0, sizeof(*info));
    info->hdr.id = id;
    if (led->name)
        snprintf(info->hdr.description, sizeof(info->hdr.description),
                 "%s", led->name);

    info->caps = _onlp_cap_create(mod);

    if (led->cur_led_mode < 0) {
        rc = plat_os_file_read_int(&led->cur_led_mode, led->led_get_path, NULL);
        if (rc < 0) return rc;
    }

    mode = sys_led_to_onlp_led(lid, led->cur_led_mode);

    info->status |= ONLP_LED_STATUS_PRESENT;

    if (mode < 0) {
        info->mode = ONLP_LED_MODE_OFF;
        info->status |= ONLP_LED_STATUS_FAILED;
    } else {
        info->mode = mode;
        info->status |= ONLP_LED_STATUS_ON;
    }

    switch (info->mode) {
    case ONLP_LED_MODE_OFF:
        info->status &= ~ONLP_LED_STATUS_ON;
        break;
    default:
        break;
    }

    return ONLP_STATUS_OK;
}

/*
 * Turn an LED on or off.
 *
 * This function will only be called if the LED OID supports the ONOFF
 * capability.
 *
 * What 'on' means in terms of colors or modes for multimode LEDs is
 * up to the platform to decide. This is intended as baseline toggle mechanism.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    if (!on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function puts the LED into the given mode. It is a more functional
 * interface for multimode LEDs.
 *
 * Only modes reported in the LED's capabilities will be attempted.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int lid;
    int rc;
    plat_led_t *led = &plat_leds[id];
    int sys_led_mode;

    if (!ONLP_OID_IS_LED(id))
        return ONLP_STATUS_E_INVALID;

    lid = ONLP_OID_ID_GET(id);

    if (!plat_led_is_valid (lid))
        return ONLP_STATUS_E_INVALID;

    led = &plat_leds[lid];

    sys_led_mode = onlp_led_to_sys_led(lid, mode);
    if (sys_led_mode < 0)
        return ONLP_STATUS_E_UNSUPPORTED;

    rc = plat_os_file_write_int(sys_led_mode, led->led_set_path, NULL);
    if (rc < 0){
        return ONLP_STATUS_E_INTERNAL;
    }

    led->cur_led_mode = sys_led_mode;

    return ONLP_STATUS_OK;
}


