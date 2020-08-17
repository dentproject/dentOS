/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"
#include "arm64_accton_as4224_int.h"
#include "arm64_accton_as4224_log.h"

#define PORT_EEPROM_FORMAT              "/sys/bus/i2c/devices/%d-0050/eeprom"
#define MODULE_PRESENT_FORMAT           "module_present_%d"
#define MODULE_RXLOS_FORMAT             "module_rx_los_%d"
#define MODULE_TXFAULT_FORMAT           "module_tx_fault_%d"
#define MODULE_TXDISABLE_FORMAT         "module_tx_disable_%d"
#define MODULE_PRESENT_ALL_ATTR         "/sys/bus/i2c/devices/0-0040/module_present_all"
#define MODULE_RXLOS_ALL_ATTR           "/sys/bus/i2c/devices/0-0040/module_rx_los_all"

static const int port_bus_index[] = {
 3,  4,  5,  6,  7,  8,  9, 10,
11, 12, 13, 14, 15, 16, 17, 18,
19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34,
35, 36, 37, 38, 39, 40, 41, 42,
43, 44, 45, 46, 47, 48, 49, 50,
 3,  4,  5,  6
};

#define PORT_BUS_INDEX(port) (port_bus_index[port-1])

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    /* Called at initialization time */
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_get_port_attr_int(int port, char *fmt, int *value)
{
    char attr_name[32] = {0};
    sprintf(attr_name, fmt, port);
    return get_cpld_attr_int(attr_name, value);
}

int
onlp_sfpi_set_port_attr_int(int port, char *fmt, int value)
{
    char attr_name[32] = {0};
    sprintf(attr_name, fmt, port);
    return set_cpld_attr_int(attr_name, value);
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    int ret = 0;
    int module_count = 0;
    int module_index_begin = 0;

    ret = get_cpld_attr_int("module_count", &module_count);
    if (ret < 0) {
        return ret;
    }

    ret = get_cpld_attr_int("module_index_begin", &module_index_begin);
    if (ret < 0) {
        return ret;
    }

    for (p = module_index_begin; p < (module_index_begin + module_count); p++) {
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
    int ret = 0;
    int present = 0;

    ret = onlp_sfpi_get_port_attr_int(port, MODULE_PRESENT_FORMAT, &present);

    return (ret < 0) ? ret : present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i = 0;
    int ret = 0;
    int module_count = 0;
    uint32_t bytes[7] = {0};
    FILE* fp = NULL;
    int count  = 0;

    ret = get_cpld_attr_int("module_count", &module_count);
    if (ret < 0) {
        return ret;
    }

    /* Read present status of each port */
    fp = fopen(MODULE_PRESENT_ALL_ATTR, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the module_present_all device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    if (module_count == 4) {
        count = fscanf(fp, "%x", bytes+6);
        fclose(fp);

        if(count != 1) {
            /* Likely a CPLD read timeout. */
            AIM_LOG_ERROR("Unable to read all fields the module_present_all device file of CPLD.");
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Mask out non-existant QSFP ports */
        bytes[6] &= 0x0F;
    }
    else if (module_count == 48) {
        count = fscanf(fp, "%x %x %x %x %x %x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);
        fclose(fp);

        if(count != 6) {
            /* Likely a CPLD read timeout. */
            AIM_LOG_ERROR("Unable to read all fields the module_present_all device file of CPLD.");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    /* Convert to 64 bit integer in port order */
    AIM_BITMAP_CLR_ALL(dst);
    uint32_t data_len = (module_count == 48) ? (AIM_ARRAYSIZE(bytes)-1): AIM_ARRAYSIZE(bytes);
    uint64_t presence_all = 0 ;
    for(i = (data_len-1); i >= 0; i--) {
        presence_all <<= 8;
        presence_all |= bytes[i];
    }

    /* Populate bitmap */
    for(i = 0; presence_all; i++) {
        AIM_BITMAP_MOD(dst, i+1, (presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
   int i = 0;
    int ret = 0;
    int module_count = 0;
    uint32_t bytes[7] = {0};
    FILE* fp = NULL;
    int count  = 0;

    ret = get_cpld_attr_int("module_count", &module_count);
    if (ret < 0) {
        return ret;
    }

    /* Read rxlos status of each port */
    fp = fopen(MODULE_RXLOS_ALL_ATTR, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the module_rx_los_all device file of CPLD.");
        return ONLP_STATUS_E_INTERNAL;
    }

    if (module_count == 4) {
        count = fscanf(fp, "%x", bytes+6);
        fclose(fp);

        if(count != 1) {
            /* Likely a CPLD read timeout. */
            AIM_LOG_ERROR("Unable to read all fields the module_rx_los_all device file of CPLD.");
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Mask out non-existant QSFP ports */
        bytes[6] &= 0x0F;
    }
    else if (module_count == 48) {
        count = fscanf(fp, "%x %x %x %x %x %x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);
        fclose(fp);

        if(count != 6) {
            /* Likely a CPLD read timeout. */
            AIM_LOG_ERROR("Unable to read all fields the module_rx_los_all device file of CPLD.");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    /* Convert to 64 bit integer in port order */
    AIM_BITMAP_CLR_ALL(dst);
    uint32_t data_len = (module_count == 48) ? (AIM_ARRAYSIZE(bytes)-1): AIM_ARRAYSIZE(bytes);
    uint64_t rx_los_all = 0 ;
    for(i = (data_len-1); i >= 0; i--) {
        rx_los_all <<= 8;
        rx_los_all |= bytes[i];
    }

    /* Populate bitmap */
    for(i = 0; rx_los_all; i++) {
        AIM_BITMAP_MOD(dst, i+1, (rx_los_all & 1));
        rx_los_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    int size = 0;
    memset(data, 0, 256);

    if(onlp_file_read(data, 256, &size, PORT_EEPROM_FORMAT, PORT_BUS_INDEX(port)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    FILE* fp;
    char file[64] = {0};

    sprintf(file, PORT_EEPROM_FORMAT, PORT_BUS_INDEX(port));
    fp = fopen(file, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fseek(fp, 256, SEEK_CUR) != 0) {
        fclose(fp);
        AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    int ret = fread(data, 1, 256, fp);
    fclose(fp);
    if (ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int bus = PORT_BUS_INDEX(port);
    return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int bus = PORT_BUS_INDEX(port);
    return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    int bus = PORT_BUS_INDEX(port);
    return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int bus = PORT_BUS_INDEX(port);
    return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rv;

    switch(control)
        {
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (port < 1 || port > 52) {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }

                if (onlp_sfpi_set_port_attr_int(port, MODULE_TXDISABLE_FORMAT, value) < 0) {
                    AIM_LOG_ERROR("Unable to set tx_disable status to port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
                break;
            }
        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
        }

    return rv;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rv;

    if (port < 1 || port > 52) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch(control)
        {
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if (onlp_sfpi_get_port_attr_int(port, MODULE_RXLOS_FORMAT, value) < 0) {
                    AIM_LOG_ERROR("Unable to read rx_loss status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
                break;
            }

        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (onlp_sfpi_get_port_attr_int(port, MODULE_TXFAULT_FORMAT, value) < 0) {
                    AIM_LOG_ERROR("Unable to read tx_fault status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
                break;
            }

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (onlp_sfpi_get_port_attr_int(port, MODULE_TXDISABLE_FORMAT, value) < 0) {
                    AIM_LOG_ERROR("Unable to read tx_disabled status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
                break;
            }

        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rv;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}