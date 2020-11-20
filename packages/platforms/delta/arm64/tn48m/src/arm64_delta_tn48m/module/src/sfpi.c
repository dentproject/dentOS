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
#include "arm64_delta_tn48m_log.h"
#include "platform_lib.h"

#define PORT_EEPROM_FORMAT      "/sys/bus/i2c/devices/%d-0050/eeprom"
#define MODULE_PRESENT_FORMAT   "/sys/bus/i2c/devices/%d-0041/module_present_%d"
#define MODULE_RXLOS_FORMAT     "/sys/bus/i2c/devices/%d-0041/module_rx_los_%d"
#define MODULE_TXDIS_FORMAT     "/sys/bus/i2c/devices/%d-0041/module_tx_dis_%d"
#define MODULE_PRESENT_ALL_PATH "/sys/bus/i2c/devices/%d-0041/module_present_all"
#define MODULE_RXLOS_ALL_PATH   "/sys/bus/i2c/devices/%d-0041/module_rx_los_all"
#define MODULE_TXDIS_ALL_PATH   "/sys/bus/i2c/devices/%d-0041/module_tx_dis_all"

static int front_port_to_eeprom_bus(int port)
{
    return (gPlat_id == PID_TN4810M_PVT) ?
           (port + 6) : (gPlat_id == PID_TN4810M_NONPVT) ? (port + 2) : (port - 46);
}

static int front_port_to_sysfs_index(int port)
{
    return (gPlat_id == PID_TN4810M_PVT ||  gPlat_id == PID_TN4810M_NONPVT) ?
            port : (port - 48);
}

int
onlp_sfpi_init(void)
{
    /* Called at initialization time */
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
    int present;
    int sysfs_idx = front_port_to_sysfs_index(port);
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    if (port < plat_info->sfp_start_idx || port > plat_info->sfp_end_idx) {
        AIM_LOG_ERROR("The port %d is invalid\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    sprintf(fullpath, MODULE_PRESENT_FORMAT, plat_info->cpld_bus, sysfs_idx);
    if (plat_os_file_read_int(&present, fullpath, NULL) < 0) {
        AIM_LOG_ERROR("Unable to read present status from port %d\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return present == PLAT_SFP_STATE_PRESENT ? 1 : 0;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint64_t presence_all;
    FILE* fp;
    int i;
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    AIM_BITMAP_CLR_ALL(dst);

    /* Read present status of each port */
    sprintf(fullpath, MODULE_PRESENT_ALL_PATH, plat_info->cpld_bus);
    fp = fopen(fullpath, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the module_present_all"
                      " device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    int count = fscanf(fp, "0x%016lx", &presence_all);
    fclose(fp);
    if(count != 1) {
        /* Likely a CPLD read timeout. */
        AIM_LOG_ERROR("Unable to read all fields the module_present_all"
                      " device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Populate bitmap */
    for(i = plat_info->sfp_start_idx; i <= plat_info->sfp_end_idx; i++) {
        AIM_BITMAP_MOD(dst, i, !(presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint64_t rx_los_all;
    FILE* fp;
    int i;
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    AIM_BITMAP_CLR_ALL(dst);

    /* Read rx_los status of each port */
    sprintf(fullpath, MODULE_RXLOS_ALL_PATH, plat_info->cpld_bus);
    fp = fopen(fullpath, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the module_rxlos_all"
                      " device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    int count = fscanf(fp, "0x%016lx", &rx_los_all);
    fclose(fp);
    if(count != 1) {
        /* Likely a CPLD read timeout. */
        AIM_LOG_ERROR("Unable to read all fields the module_rxlos_all"
                      " device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Populate bitmap */
    for(i = plat_info->sfp_start_idx; rx_los_all; i++) {
        AIM_BITMAP_MOD(dst, i, (rx_los_all & 1));
        rx_los_all >>= 1;
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
    int size = 0;
    int eeprom_bus = front_port_to_eeprom_bus(port);
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    if (port < plat_info->sfp_start_idx || port > plat_info->sfp_end_idx) {
        AIM_LOG_ERROR("The port %d is invalid\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }


    /*
     * Read the SFP eeprom into data[]
     *
     * Return OK if eeprom is read
     */
    memset(data, 0, 256);
    sprintf(fullpath, PORT_EEPROM_FORMAT, eeprom_bus);
    if (plat_os_file_read(data, 256, &size, fullpath) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d),"
                      " size is different!\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    return onlp_sfpi_eeprom_read(port, data);
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    /*
     * value is 1 if the tx disable
     * value is 0 if the tx enable
     */
    int rc;
    int present;
    int sysfs_idx = front_port_to_sysfs_index(port);
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    present = onlp_sfpi_is_present(port);
    if (!present) {
        AIM_LOG_INFO("The port %d is not present and can not set tx_disable\n",
                     port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch (control)
    {
    case ONLP_SFP_CONTROL_TX_DISABLE:
        sprintf(fullpath, MODULE_TXDIS_FORMAT, plat_info->cpld_bus, sysfs_idx);
        rc = plat_os_file_write_int(value, fullpath, NULL);
        if (rc < 0) {
            AIM_LOG_ERROR("Unable to set tx_disable status to port(%d)\n",
                          port);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    default:
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc;
    int present;
    int sysfs_idx = front_port_to_sysfs_index(port);
    char fullpath[PATH_MAX] = {0};
    plat_info_t *plat_info = &gPlat_info[gPlat_id];

    present = onlp_sfpi_is_present(port);
    if (!present) {
        AIM_LOG_INFO("The Port %d is not present\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch (control)
    {
    case ONLP_SFP_CONTROL_RX_LOS:
        sprintf(fullpath, MODULE_RXLOS_FORMAT, plat_info->cpld_bus, sysfs_idx);
        rc = plat_os_file_read_int(value, fullpath, NULL);
        if (rc < 0) {
            AIM_LOG_ERROR("Unable to read rx_los status from port(%d)\n",
                          port);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    case ONLP_SFP_CONTROL_TX_DISABLE:
        sprintf(fullpath, MODULE_TXDIS_FORMAT, plat_info->cpld_bus, sysfs_idx);
        rc = plat_os_file_read_int(value, fullpath, NULL);
        if (rc < 0) {
            AIM_LOG_ERROR("Unable to read tx_disabled status from port(%d)\n",
                          port);
            return ONLP_STATUS_E_INTERNAL;
        }
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
