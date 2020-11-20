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
 ***********************************************************/
#include <onlp/platformi/sfpi.h>
#include <onlplib/file.h>
#include <limits.h>
#include "arm64_delta_tx4810_log.h"
#include "platform_lib.h"

#define MLX_PCI_DEV_PATH                "/dev/mst/mt52100_pciconf0"
#define MLX_SFP_EEPROM_DEV_NAME         "mt52100_pciconf0_cable_%d"
#define MLX_SFP_EEPROM_DEV_PATH         "/dev/mst/%s"

#define MLX_CMD_MST_START               "mst start"
#define MLX_CMD_CABLE_ADD               "mst cable add"

#define MLX_CMD_GET_SFP_MODULE_STAT     "mlxreg -d %s --reg_name MCION --get \
--indexes \"module=%d\" | grep module_status_bits | awk {'print $3'} \
| tr -d '\r\n'"

#define MLX_CMD_READ_SFP_EEPROM         "mlxcables -d %s -r -a 0x00 -l 256 \
| awk {'print $3'}"

#define MLX_REG_MCION_BIT_PRSNT         0
#define MLX_REG_MCION_BIT_RX_LOS        1
#define MLX_REG_MCION_BIT_TX_FAULT      2

#define BIT(nr)                         ((1) << (nr))

int
get_sfp_module_status(int port, plat_sfp_status_t sfp_status)
{
    /*
     * Return 1 if present, rx_los, or tx_fault.
     * Return 0 if not present, rx normal, or tx normal.
     * Return < 0 if error.
     */
    int rv = ONLP_STATUS_OK;
    int module_status;
    char mlx_cmd[PATH_MAX] = {0};
    char reg[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];
    FILE *pFd = NULL;

    if (port < plat_info->sfp_start_idx || port > plat_info->sfp_end_idx) {
        AIM_LOG_ERROR("The port %d is invalid\n", port);
        rv = ONLP_STATUS_E_UNSUPPORTED;
        goto err;
    }

    sprintf(mlx_cmd, MLX_CMD_GET_SFP_MODULE_STAT, MLX_PCI_DEV_PATH, port - 1);
    pFd = popen(mlx_cmd, "r");
    if (pFd) {
        if (fgets(reg, PATH_MAX, pFd) == NULL)
        {
            AIM_LOG_ERROR("Unable to read module status on port %d\n", port);
            rv = ONLP_STATUS_E_INTERNAL;
            goto err;
        }
        pclose(pFd);
    }
    else {
        AIM_LOG_ERROR("Failed to execute command \"%s\"\n", mlx_cmd);
        rv = ONLP_STATUS_E_INTERNAL;
        goto err;
    }

    module_status = strtoul(reg, NULL, 16);

    switch (sfp_status) {
    case PLAT_SFP_STATUS_PRESENT:
        return (module_status & BIT(MLX_REG_MCION_BIT_PRSNT)) ? 1 : 0;
    case PLAT_SFP_STATUS_RX_LOS:
        return (module_status & BIT(MLX_REG_MCION_BIT_RX_LOS)) ? 1 : 0;
    case PLAT_SFP_STATUS_TX_FAULT:
        return (module_status & BIT(MLX_REG_MCION_BIT_TX_FAULT)) ? 1 : 0;
    case PLAT_SFP_STATUS_ALL:
        return module_status;
    default:
        rv = ONLP_STATUS_E_UNSUPPORTED;
        break;
    }

err:
    if (pFd != NULL)
        pclose(pFd);

    return rv;
}


int
onlp_sfpi_init(void)
{
    if (!plat_os_file_is_existed(MLX_PCI_DEV_PATH)) {
        if (system(MLX_CMD_MST_START) != 0) {
            AIM_LOG_ERROR("Failed to execute command \"%s\"\n",
                          MLX_CMD_MST_START);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    for (p = plat_info->sfp_start_idx; p <= plat_info->sfp_end_idx; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    return get_sfp_module_status(port, PLAT_SFP_STATUS_PRESENT);
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i, prsnt;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];
    AIM_BITMAP_CLR_ALL(dst);

    /* Populate bitmap */
    for(i = plat_info->sfp_start_idx; i <= plat_info->sfp_end_idx; i++) {
        prsnt = get_sfp_module_status(i, PLAT_SFP_STATUS_PRESENT);
        if (prsnt < 0)
            return ONLP_STATUS_E_INTERNAL;

        AIM_BITMAP_MOD(dst, i, prsnt);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i, rx_los;
    plat_info_t *plat_info = &gPlat_info[gPlat_id];
    AIM_BITMAP_CLR_ALL(dst);

    /* Populate bitmap */
    for(i = plat_info->sfp_start_idx; i <= plat_info->sfp_end_idx; i++) {
        rx_los = get_sfp_module_status(i, PLAT_SFP_STATUS_RX_LOS);
        if (rx_los < 0)
            return ONLP_STATUS_E_INTERNAL;

        AIM_BITMAP_MOD(dst, i, rx_los);
    }

    return ONLP_STATUS_OK;
}

/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int rv = ONLP_STATUS_OK;
    int idx = 0;
    char mlx_cmd[PATH_MAX] = {0};
    char eeprom_dev_name[PATH_MAX] = {0};
    char eeprom_dev_path[PATH_MAX] = {0};
    char eeprom_data[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];
    FILE *pFd = NULL;

    if (port < plat_info->sfp_start_idx || port > plat_info->sfp_end_idx) {
        AIM_LOG_ERROR("The port %d is invalid\n", port);
        rv = ONLP_STATUS_E_UNSUPPORTED;
        goto err;
    }

    sprintf(eeprom_dev_name, MLX_SFP_EEPROM_DEV_NAME, port - 1);
    sprintf(eeprom_dev_path, MLX_SFP_EEPROM_DEV_PATH, eeprom_dev_name);
    if (!plat_os_file_is_existed(eeprom_dev_path)) {
        if (system(MLX_CMD_CABLE_ADD) != 0) {
            AIM_LOG_ERROR("Failed to execute command \"%s\"\n",
                          MLX_CMD_CABLE_ADD);
            rv = ONLP_STATUS_E_INTERNAL;
            goto err;
        }

        if (!plat_os_file_is_existed(eeprom_dev_path)) {
            AIM_LOG_ERROR("Failed to add connected cable on port %d\n", port);
            rv = ONLP_STATUS_E_INTERNAL;
            goto err;
        }
    }

    memset(data, 0, 256);
    sprintf(mlx_cmd, MLX_CMD_READ_SFP_EEPROM, eeprom_dev_name);
    pFd = popen(mlx_cmd, "r");
    if (pFd) {
        while (fgets(eeprom_data, PATH_MAX, pFd) != NULL && idx < 256) {
            data[idx++] = (uint8_t)strtol(eeprom_data, NULL, 16);
        }
    }
    else {
        AIM_LOG_ERROR("Failed to execute command \"%s\"\n", mlx_cmd);
        rv = ONLP_STATUS_E_INTERNAL;
        goto err;
    }

err:
    if (pFd != NULL)
        pclose(pFd);

    return rv;
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    return onlp_sfpi_eeprom_read(port, data);
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int module_status;
    int present;

    module_status = get_sfp_module_status(port, PLAT_SFP_STATUS_ALL);
    if (module_status < 0) {
        AIM_LOG_INFO("Get modules status on Port %d failed!\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    present = (module_status & BIT(MLX_REG_MCION_BIT_PRSNT)) ? 1 : 0;
    if (!present) {
        AIM_LOG_INFO("The Port %d is not present\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch (control)
    {
    case ONLP_SFP_CONTROL_RX_LOS:
        (*value) = (module_status & BIT(MLX_REG_MCION_BIT_RX_LOS)) ? 1 : 0;
        break;
    case ONLP_SFP_CONTROL_TX_FAULT:
        (*value) = (module_status & BIT(MLX_REG_MCION_BIT_TX_FAULT)) ? 1 : 0;
        break;
    default:
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

/*
 * De-initialize the SFPI subsystem.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
