/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *     Accton as4224 CPLD1/CPLD2/CPLD3
 *
 * Based on:
 *    pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *    pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *    i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *    pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#define DRVNAME "as4224_cpld"

#define I2C_RW_RETRY_COUNT               10
#define I2C_RW_RETRY_INTERVAL            60 /* ms */
#define BOARD_INFO_REG_OFFSET            0x00
#define I2C_WRITE_REQUEST_REG            0xE0
#define I2C_LOCK_BY_7040_VAL             0x10
#define I2C_WRITE_REQUEST_7040_VAL       0x1
#define I2C_WRITE_REQUEST_RETRY_TIMES    3
#define WTD_RESET_GPIO_PIN_MPP3          35

static ssize_t show_module(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_module_48x(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_module_52x(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_control_48x(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t set_control_52x(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_platform_id(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t reset_wtd(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static int as4224_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as4224_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);
static ssize_t show_i2c_request(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_i2c_request(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_type {
    as4224_cpld1
};

enum as4224_platform_id {
    AS4224_48X,
    AS4224_52P,
    AS4224_52T,
    AS4224_52T_DAC,
    PID_UNKNOWN
};

struct as4224_cpld_data {
    enum cpld_type   type;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    enum as4224_platform_id platform_id;
};

static const struct i2c_device_id as4224_cpld_id[] = {
    { "as4224_cpld1", as4224_cpld1 },
    { }
};
MODULE_DEVICE_TABLE(i2c, as4224_cpld_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)       MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index)     MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index)         MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index)       MODULE_TXFAULT_##index

enum as4224_cpld_sysfs_attributes {
    PLATFORM_ID,
    CPLD_VERSION,
    ACCESS,
    I2C_ACCESS_REQUEST_7040,
    WTD_RESET_7040,   /* Trigger GPIO to reset wtd timer */
    WTD_STATE_7040,   /* Register 0x90 bit 1 */
    WTD_ENABLE_7040,  /* Register 0x91 bit 1 */
    WTD_CLOCK_7040,   /* Register 0x92 bit 7:6 */
    WTD_COUNTER_7040, /* Register 0x92 bit 5:0 */
    MODULE_PRESENT_ALL,
    MODULE_RXLOS_ALL,
    MODULE_COUNT,
    MODULE_INDEX_BEGIN,
    /* transceiver attributes */
    TRANSCEIVER_PRESENT_ATTR_ID(1),
    TRANSCEIVER_PRESENT_ATTR_ID(2),
    TRANSCEIVER_PRESENT_ATTR_ID(3),
    TRANSCEIVER_PRESENT_ATTR_ID(4),
    TRANSCEIVER_PRESENT_ATTR_ID(5),
    TRANSCEIVER_PRESENT_ATTR_ID(6),
    TRANSCEIVER_PRESENT_ATTR_ID(7),
    TRANSCEIVER_PRESENT_ATTR_ID(8),
    TRANSCEIVER_PRESENT_ATTR_ID(9),
    TRANSCEIVER_PRESENT_ATTR_ID(10),
    TRANSCEIVER_PRESENT_ATTR_ID(11),
    TRANSCEIVER_PRESENT_ATTR_ID(12),
    TRANSCEIVER_PRESENT_ATTR_ID(13),
    TRANSCEIVER_PRESENT_ATTR_ID(14),
    TRANSCEIVER_PRESENT_ATTR_ID(15),
    TRANSCEIVER_PRESENT_ATTR_ID(16),
    TRANSCEIVER_PRESENT_ATTR_ID(17),
    TRANSCEIVER_PRESENT_ATTR_ID(18),
    TRANSCEIVER_PRESENT_ATTR_ID(19),
    TRANSCEIVER_PRESENT_ATTR_ID(20),
    TRANSCEIVER_PRESENT_ATTR_ID(21),
    TRANSCEIVER_PRESENT_ATTR_ID(22),
    TRANSCEIVER_PRESENT_ATTR_ID(23),
    TRANSCEIVER_PRESENT_ATTR_ID(24),
    TRANSCEIVER_PRESENT_ATTR_ID(25),
    TRANSCEIVER_PRESENT_ATTR_ID(26),
    TRANSCEIVER_PRESENT_ATTR_ID(27),
    TRANSCEIVER_PRESENT_ATTR_ID(28),
    TRANSCEIVER_PRESENT_ATTR_ID(29),
    TRANSCEIVER_PRESENT_ATTR_ID(30),
    TRANSCEIVER_PRESENT_ATTR_ID(31),
    TRANSCEIVER_PRESENT_ATTR_ID(32),
    TRANSCEIVER_PRESENT_ATTR_ID(33),
    TRANSCEIVER_PRESENT_ATTR_ID(34),
    TRANSCEIVER_PRESENT_ATTR_ID(35),
    TRANSCEIVER_PRESENT_ATTR_ID(36),
    TRANSCEIVER_PRESENT_ATTR_ID(37),
    TRANSCEIVER_PRESENT_ATTR_ID(38),
    TRANSCEIVER_PRESENT_ATTR_ID(39),
    TRANSCEIVER_PRESENT_ATTR_ID(40),
    TRANSCEIVER_PRESENT_ATTR_ID(41),
    TRANSCEIVER_PRESENT_ATTR_ID(42),
    TRANSCEIVER_PRESENT_ATTR_ID(43),
    TRANSCEIVER_PRESENT_ATTR_ID(44),
    TRANSCEIVER_PRESENT_ATTR_ID(45),
    TRANSCEIVER_PRESENT_ATTR_ID(46),
    TRANSCEIVER_PRESENT_ATTR_ID(47),
    TRANSCEIVER_PRESENT_ATTR_ID(48),
    TRANSCEIVER_PRESENT_ATTR_ID(49),
    TRANSCEIVER_PRESENT_ATTR_ID(50),
    TRANSCEIVER_PRESENT_ATTR_ID(51),
    TRANSCEIVER_PRESENT_ATTR_ID(52),
    TRANSCEIVER_TXDISABLE_ATTR_ID(1),
    TRANSCEIVER_TXDISABLE_ATTR_ID(2),
    TRANSCEIVER_TXDISABLE_ATTR_ID(3),
    TRANSCEIVER_TXDISABLE_ATTR_ID(4),
    TRANSCEIVER_TXDISABLE_ATTR_ID(5),
    TRANSCEIVER_TXDISABLE_ATTR_ID(6),
    TRANSCEIVER_TXDISABLE_ATTR_ID(7),
    TRANSCEIVER_TXDISABLE_ATTR_ID(8),
    TRANSCEIVER_TXDISABLE_ATTR_ID(9),
    TRANSCEIVER_TXDISABLE_ATTR_ID(10),
    TRANSCEIVER_TXDISABLE_ATTR_ID(11),
    TRANSCEIVER_TXDISABLE_ATTR_ID(12),
    TRANSCEIVER_TXDISABLE_ATTR_ID(13),
    TRANSCEIVER_TXDISABLE_ATTR_ID(14),
    TRANSCEIVER_TXDISABLE_ATTR_ID(15),
    TRANSCEIVER_TXDISABLE_ATTR_ID(16),
    TRANSCEIVER_TXDISABLE_ATTR_ID(17),
    TRANSCEIVER_TXDISABLE_ATTR_ID(18),
    TRANSCEIVER_TXDISABLE_ATTR_ID(19),
    TRANSCEIVER_TXDISABLE_ATTR_ID(20),
    TRANSCEIVER_TXDISABLE_ATTR_ID(21),
    TRANSCEIVER_TXDISABLE_ATTR_ID(22),
    TRANSCEIVER_TXDISABLE_ATTR_ID(23),
    TRANSCEIVER_TXDISABLE_ATTR_ID(24),
    TRANSCEIVER_TXDISABLE_ATTR_ID(25),
    TRANSCEIVER_TXDISABLE_ATTR_ID(26),
    TRANSCEIVER_TXDISABLE_ATTR_ID(27),
    TRANSCEIVER_TXDISABLE_ATTR_ID(28),
    TRANSCEIVER_TXDISABLE_ATTR_ID(29),
    TRANSCEIVER_TXDISABLE_ATTR_ID(30),
    TRANSCEIVER_TXDISABLE_ATTR_ID(31),
    TRANSCEIVER_TXDISABLE_ATTR_ID(32),
    TRANSCEIVER_TXDISABLE_ATTR_ID(33),
    TRANSCEIVER_TXDISABLE_ATTR_ID(34),
    TRANSCEIVER_TXDISABLE_ATTR_ID(35),
    TRANSCEIVER_TXDISABLE_ATTR_ID(36),
    TRANSCEIVER_TXDISABLE_ATTR_ID(37),
    TRANSCEIVER_TXDISABLE_ATTR_ID(38),
    TRANSCEIVER_TXDISABLE_ATTR_ID(39),
    TRANSCEIVER_TXDISABLE_ATTR_ID(40),
    TRANSCEIVER_TXDISABLE_ATTR_ID(41),
    TRANSCEIVER_TXDISABLE_ATTR_ID(42),
    TRANSCEIVER_TXDISABLE_ATTR_ID(43),
    TRANSCEIVER_TXDISABLE_ATTR_ID(44),
    TRANSCEIVER_TXDISABLE_ATTR_ID(45),
    TRANSCEIVER_TXDISABLE_ATTR_ID(46),
    TRANSCEIVER_TXDISABLE_ATTR_ID(47),
    TRANSCEIVER_TXDISABLE_ATTR_ID(48),
    TRANSCEIVER_TXDISABLE_ATTR_ID(49),
    TRANSCEIVER_TXDISABLE_ATTR_ID(50),
    TRANSCEIVER_TXDISABLE_ATTR_ID(51),
    TRANSCEIVER_TXDISABLE_ATTR_ID(52),
    TRANSCEIVER_RXLOS_ATTR_ID(1),
    TRANSCEIVER_RXLOS_ATTR_ID(2),
    TRANSCEIVER_RXLOS_ATTR_ID(3),
    TRANSCEIVER_RXLOS_ATTR_ID(4),
    TRANSCEIVER_RXLOS_ATTR_ID(5),
    TRANSCEIVER_RXLOS_ATTR_ID(6),
    TRANSCEIVER_RXLOS_ATTR_ID(7),
    TRANSCEIVER_RXLOS_ATTR_ID(8),
    TRANSCEIVER_RXLOS_ATTR_ID(9),
    TRANSCEIVER_RXLOS_ATTR_ID(10),
    TRANSCEIVER_RXLOS_ATTR_ID(11),
    TRANSCEIVER_RXLOS_ATTR_ID(12),
    TRANSCEIVER_RXLOS_ATTR_ID(13),
    TRANSCEIVER_RXLOS_ATTR_ID(14),
    TRANSCEIVER_RXLOS_ATTR_ID(15),
    TRANSCEIVER_RXLOS_ATTR_ID(16),
    TRANSCEIVER_RXLOS_ATTR_ID(17),
    TRANSCEIVER_RXLOS_ATTR_ID(18),
    TRANSCEIVER_RXLOS_ATTR_ID(19),
    TRANSCEIVER_RXLOS_ATTR_ID(20),
    TRANSCEIVER_RXLOS_ATTR_ID(21),
    TRANSCEIVER_RXLOS_ATTR_ID(22),
    TRANSCEIVER_RXLOS_ATTR_ID(23),
    TRANSCEIVER_RXLOS_ATTR_ID(24),
    TRANSCEIVER_RXLOS_ATTR_ID(25),
    TRANSCEIVER_RXLOS_ATTR_ID(26),
    TRANSCEIVER_RXLOS_ATTR_ID(27),
    TRANSCEIVER_RXLOS_ATTR_ID(28),
    TRANSCEIVER_RXLOS_ATTR_ID(29),
    TRANSCEIVER_RXLOS_ATTR_ID(30),
    TRANSCEIVER_RXLOS_ATTR_ID(31),
    TRANSCEIVER_RXLOS_ATTR_ID(32),
    TRANSCEIVER_RXLOS_ATTR_ID(33),
    TRANSCEIVER_RXLOS_ATTR_ID(34),
    TRANSCEIVER_RXLOS_ATTR_ID(35),
    TRANSCEIVER_RXLOS_ATTR_ID(36),
    TRANSCEIVER_RXLOS_ATTR_ID(37),
    TRANSCEIVER_RXLOS_ATTR_ID(38),
    TRANSCEIVER_RXLOS_ATTR_ID(39),
    TRANSCEIVER_RXLOS_ATTR_ID(40),
    TRANSCEIVER_RXLOS_ATTR_ID(41),
    TRANSCEIVER_RXLOS_ATTR_ID(42),
    TRANSCEIVER_RXLOS_ATTR_ID(43),
    TRANSCEIVER_RXLOS_ATTR_ID(44),
    TRANSCEIVER_RXLOS_ATTR_ID(45),
    TRANSCEIVER_RXLOS_ATTR_ID(46),
    TRANSCEIVER_RXLOS_ATTR_ID(47),
    TRANSCEIVER_RXLOS_ATTR_ID(48),
    TRANSCEIVER_RXLOS_ATTR_ID(49),
    TRANSCEIVER_RXLOS_ATTR_ID(50),
    TRANSCEIVER_RXLOS_ATTR_ID(51),
    TRANSCEIVER_RXLOS_ATTR_ID(52),
    TRANSCEIVER_TXFAULT_ATTR_ID(1),
    TRANSCEIVER_TXFAULT_ATTR_ID(2),
    TRANSCEIVER_TXFAULT_ATTR_ID(3),
    TRANSCEIVER_TXFAULT_ATTR_ID(4),
    TRANSCEIVER_TXFAULT_ATTR_ID(5),
    TRANSCEIVER_TXFAULT_ATTR_ID(6),
    TRANSCEIVER_TXFAULT_ATTR_ID(7),
    TRANSCEIVER_TXFAULT_ATTR_ID(8),
    TRANSCEIVER_TXFAULT_ATTR_ID(9),
    TRANSCEIVER_TXFAULT_ATTR_ID(10),
    TRANSCEIVER_TXFAULT_ATTR_ID(11),
    TRANSCEIVER_TXFAULT_ATTR_ID(12),
    TRANSCEIVER_TXFAULT_ATTR_ID(13),
    TRANSCEIVER_TXFAULT_ATTR_ID(14),
    TRANSCEIVER_TXFAULT_ATTR_ID(15),
    TRANSCEIVER_TXFAULT_ATTR_ID(16),
    TRANSCEIVER_TXFAULT_ATTR_ID(17),
    TRANSCEIVER_TXFAULT_ATTR_ID(18),
    TRANSCEIVER_TXFAULT_ATTR_ID(19),
    TRANSCEIVER_TXFAULT_ATTR_ID(20),
    TRANSCEIVER_TXFAULT_ATTR_ID(21),
    TRANSCEIVER_TXFAULT_ATTR_ID(22),
    TRANSCEIVER_TXFAULT_ATTR_ID(23),
    TRANSCEIVER_TXFAULT_ATTR_ID(24),
    TRANSCEIVER_TXFAULT_ATTR_ID(25),
    TRANSCEIVER_TXFAULT_ATTR_ID(26),
    TRANSCEIVER_TXFAULT_ATTR_ID(27),
    TRANSCEIVER_TXFAULT_ATTR_ID(28),
    TRANSCEIVER_TXFAULT_ATTR_ID(29),
    TRANSCEIVER_TXFAULT_ATTR_ID(30),
    TRANSCEIVER_TXFAULT_ATTR_ID(31),
    TRANSCEIVER_TXFAULT_ATTR_ID(32),
    TRANSCEIVER_TXFAULT_ATTR_ID(33),
    TRANSCEIVER_TXFAULT_ATTR_ID(34),
    TRANSCEIVER_TXFAULT_ATTR_ID(35),
    TRANSCEIVER_TXFAULT_ATTR_ID(36),
    TRANSCEIVER_TXFAULT_ATTR_ID(37),
    TRANSCEIVER_TXFAULT_ATTR_ID(38),
    TRANSCEIVER_TXFAULT_ATTR_ID(39),
    TRANSCEIVER_TXFAULT_ATTR_ID(40),
    TRANSCEIVER_TXFAULT_ATTR_ID(41),
    TRANSCEIVER_TXFAULT_ATTR_ID(42),
    TRANSCEIVER_TXFAULT_ATTR_ID(43),
    TRANSCEIVER_TXFAULT_ATTR_ID(44),
    TRANSCEIVER_TXFAULT_ATTR_ID(45),
    TRANSCEIVER_TXFAULT_ATTR_ID(46),
    TRANSCEIVER_TXFAULT_ATTR_ID(47),
    TRANSCEIVER_TXFAULT_ATTR_ID(48),
    TRANSCEIVER_TXFAULT_ATTR_ID(49),
    TRANSCEIVER_TXFAULT_ATTR_ID(50),
    TRANSCEIVER_TXFAULT_ATTR_ID(51),
    TRANSCEIVER_TXFAULT_ATTR_ID(52),
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(platform_id, S_IRUGO, show_platform_id, NULL, PLATFORM_ID);
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
static SENSOR_DEVICE_ATTR(wtd_reset_7040, S_IWUSR, NULL, reset_wtd, WTD_RESET_7040);
static SENSOR_DEVICE_ATTR(wtd_state_7040, S_IRUGO, show_wtd, NULL, WTD_STATE_7040);
static SENSOR_DEVICE_ATTR(wtd_enable_7040, S_IRUGO | S_IWUSR, show_wtd, set_wtd, WTD_ENABLE_7040);
static SENSOR_DEVICE_ATTR(wtd_clock_7040, S_IRUGO | S_IWUSR, show_wtd, set_wtd, WTD_CLOCK_7040);
static SENSOR_DEVICE_ATTR(wtd_counter_7040, S_IRUGO | S_IWUSR, show_wtd, set_wtd, WTD_COUNTER_7040);
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(module_rx_los_all, S_IRUGO, show_rxlos_all, NULL, MODULE_RXLOS_ALL);
static SENSOR_DEVICE_ATTR(module_count, S_IRUGO, show_module, NULL, MODULE_COUNT);
static SENSOR_DEVICE_ATTR(module_index_begin, S_IRUGO, show_module, NULL, MODULE_INDEX_BEGIN);
static SENSOR_DEVICE_ATTR(i2c_access_request_7040, S_IRUGO | S_IWUSR, show_i2c_request, set_i2c_request, I2C_ACCESS_REQUEST_7040);

static struct attribute *cpld_attributes_common[] = {
    &sensor_dev_attr_platform_id.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    &sensor_dev_attr_i2c_access_request_7040.dev_attr.attr,
    &sensor_dev_attr_wtd_reset_7040.dev_attr.attr,
    &sensor_dev_attr_wtd_state_7040.dev_attr.attr,
    &sensor_dev_attr_wtd_enable_7040.dev_attr.attr,
    &sensor_dev_attr_wtd_clock_7040.dev_attr.attr,
    &sensor_dev_attr_wtd_counter_7040.dev_attr.attr,
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    &sensor_dev_attr_module_rx_los_all.dev_attr.attr,
    &sensor_dev_attr_module_count.dev_attr.attr,
    &sensor_dev_attr_module_index_begin.dev_attr.attr,
    NULL
};

#define CPLD_ATTRS_COMMON() { .attrs = cpld_attributes_common }

#define MODULE_ATTRS(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_module_52x, NULL, MODULE_PRESENT_##index); \
    static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR, show_module_52x, set_control_52x, MODULE_TXDISABLE_##index); \
    static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, show_module_52x, NULL, MODULE_RXLOS_##index); \
    static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, show_module_52x, NULL, MODULE_TXFAULT_##index); \
    static struct attribute *module_attributes##index[] = { \
        &sensor_dev_attr_module_present_##index.dev_attr.attr, \
        &sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
        &sensor_dev_attr_module_rx_los_##index.dev_attr.attr, \
        &sensor_dev_attr_module_tx_fault_##index.dev_attr.attr, \
        NULL \
    }

MODULE_ATTRS(1);
MODULE_ATTRS(2);
MODULE_ATTRS(3);
MODULE_ATTRS(4);
MODULE_ATTRS(5);
MODULE_ATTRS(6);
MODULE_ATTRS(7);
MODULE_ATTRS(8);
MODULE_ATTRS(9);
MODULE_ATTRS(10);
MODULE_ATTRS(11);
MODULE_ATTRS(12);
MODULE_ATTRS(13);
MODULE_ATTRS(14);
MODULE_ATTRS(15);
MODULE_ATTRS(16);
MODULE_ATTRS(17);
MODULE_ATTRS(18);
MODULE_ATTRS(19);
MODULE_ATTRS(20);
MODULE_ATTRS(21);
MODULE_ATTRS(22);
MODULE_ATTRS(23);
MODULE_ATTRS(24);
MODULE_ATTRS(25);
MODULE_ATTRS(26);
MODULE_ATTRS(27);
MODULE_ATTRS(28);
MODULE_ATTRS(29);
MODULE_ATTRS(30);
MODULE_ATTRS(31);
MODULE_ATTRS(32);
MODULE_ATTRS(33);
MODULE_ATTRS(34);
MODULE_ATTRS(35);
MODULE_ATTRS(36);
MODULE_ATTRS(37);
MODULE_ATTRS(38);
MODULE_ATTRS(39);
MODULE_ATTRS(40);
MODULE_ATTRS(41);
MODULE_ATTRS(42);
MODULE_ATTRS(43);
MODULE_ATTRS(44);
MODULE_ATTRS(45);
MODULE_ATTRS(46);
MODULE_ATTRS(47);
MODULE_ATTRS(48);
MODULE_ATTRS(49);
MODULE_ATTRS(50);
MODULE_ATTRS(51);
MODULE_ATTRS(52);

#define MODULE_ATTR_GROUP(index)  { .attrs = module_attributes##index }

static struct attribute_group cpld_group[] = {
    CPLD_ATTRS_COMMON(),
    MODULE_ATTR_GROUP(1),
    MODULE_ATTR_GROUP(2),
    MODULE_ATTR_GROUP(3),
    MODULE_ATTR_GROUP(4),
    MODULE_ATTR_GROUP(5),
    MODULE_ATTR_GROUP(6),
    MODULE_ATTR_GROUP(7),
    MODULE_ATTR_GROUP(8),
    MODULE_ATTR_GROUP(9),
    MODULE_ATTR_GROUP(10),
    MODULE_ATTR_GROUP(11),
    MODULE_ATTR_GROUP(12),
    MODULE_ATTR_GROUP(13),
    MODULE_ATTR_GROUP(14),
    MODULE_ATTR_GROUP(15),
    MODULE_ATTR_GROUP(16),
    MODULE_ATTR_GROUP(17),
    MODULE_ATTR_GROUP(18),
    MODULE_ATTR_GROUP(19),
    MODULE_ATTR_GROUP(20),
    MODULE_ATTR_GROUP(21),
    MODULE_ATTR_GROUP(22),
    MODULE_ATTR_GROUP(23),
    MODULE_ATTR_GROUP(24),
    MODULE_ATTR_GROUP(25),
    MODULE_ATTR_GROUP(26),
    MODULE_ATTR_GROUP(27),
    MODULE_ATTR_GROUP(28),
    MODULE_ATTR_GROUP(29),
    MODULE_ATTR_GROUP(30),
    MODULE_ATTR_GROUP(31),
    MODULE_ATTR_GROUP(32),
    MODULE_ATTR_GROUP(33),
    MODULE_ATTR_GROUP(34),
    MODULE_ATTR_GROUP(35),
    MODULE_ATTR_GROUP(36),
    MODULE_ATTR_GROUP(37),
    MODULE_ATTR_GROUP(38),
    MODULE_ATTR_GROUP(39),
    MODULE_ATTR_GROUP(40),
    MODULE_ATTR_GROUP(41),
    MODULE_ATTR_GROUP(42),
    MODULE_ATTR_GROUP(43),
    MODULE_ATTR_GROUP(44),
    MODULE_ATTR_GROUP(45),
    MODULE_ATTR_GROUP(46),
    MODULE_ATTR_GROUP(47),
    MODULE_ATTR_GROUP(48),
    MODULE_ATTR_GROUP(49),
    MODULE_ATTR_GROUP(50),
    MODULE_ATTR_GROUP(51),
    MODULE_ATTR_GROUP(52),
};

static int get_platform_id(struct i2c_client *client)
{
    int status;

    status = as4224_cpld_read_internal(client, BOARD_INFO_REG_OFFSET);
    if (status < 0) {
        dev_dbg(&client->dev, "cpld reg (0x%x) err %d\n", client->addr, status);
        return PID_UNKNOWN;
    }

    if (status & 0x10) {
        return AS4224_52T;
    }
    else if (status & 0x20) {
        return AS4224_52T_DAC;
    }
    else if (status & 0x80) {
        return AS4224_48X;
    }
    else {
        return AS4224_52P;
    }
}

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int i, status;
    u8 values[6]  = {0};
    u8 regs_52x[] = {0x41};
    u8 regs_48x[] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5};
    u8 *regs[] = {regs_48x, regs_52x, regs_52x, regs_52x};
    u8  size[] = {ARRAY_SIZE(regs_48x), ARRAY_SIZE(regs_52x), ARRAY_SIZE(regs_52x), ARRAY_SIZE(regs_52x)};
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    for (i = 0; i < size[data->platform_id]; i++) {
        status = as4224_cpld_read_internal(client, regs[data->platform_id][i]);
        if (status < 0) {
            goto exit;
        }

        values[i] = ~(u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values in order */
    if (data->platform_id == AS4224_48X) {
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x\n", values[0], values[1],
                                                               values[2], values[3],
                                                               values[4], values[5]);
    }
    else { /* AS4224_52X */
        return sprintf(buf, "%.2x\n", values[0] & 0xF);
    }

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int i, status;
    u8 values[6]  = {0};
    u8 regs_52x[] = {0x40};
    u8 regs_48x[] = {0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB};
    u8 *regs[] = {regs_48x, regs_52x, regs_52x, regs_52x};
    u8  size[] = {ARRAY_SIZE(regs_48x), ARRAY_SIZE(regs_52x), ARRAY_SIZE(regs_52x), ARRAY_SIZE(regs_52x)};
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    for (i = 0; i < size[data->platform_id]; i++) {
        status = as4224_cpld_read_internal(client, regs[data->platform_id][i]);
        if (status < 0) {
            goto exit;
        }

        values[i] = (u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values in order */
    if (data->platform_id == AS4224_48X) {
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x\n", values[0], values[1],
                                                               values[2], values[3],
                                                               values[4], values[5]);
    }
    else { /* AS4224_52X */
        return sprintf(buf, "%.2x\n", values[0] & 0xF);
    }

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_module_48x(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0, invert = 0;

    switch (attr->index) {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
        invert = 1;
        reg  = 0xC0;
        mask = 0x1 << (attr->index - MODULE_PRESENT_1);
        break;
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
        invert = 1;
        reg  = 0xC1;
        mask = 0x1 << (attr->index - MODULE_PRESENT_9);
        break;
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
        invert = 1;
        reg  = 0xC2;
        mask = 0x1 << (attr->index - MODULE_PRESENT_17);
        break;
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        invert = 1;
        reg  = 0xC3;
        mask = 0x1 << (attr->index - MODULE_PRESENT_25);
        break;
    case MODULE_PRESENT_33 ... MODULE_PRESENT_40:
        invert = 1;
        reg  = 0xC4;
        mask = 0x1 << (attr->index - MODULE_PRESENT_33);
        break;
    case MODULE_PRESENT_41 ... MODULE_PRESENT_48:
        invert = 1;
        reg  = 0xC5;
        mask = 0x1 << (attr->index - MODULE_PRESENT_41);
        break;
    case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
        reg  = 0xC6;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
        break;
    case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
        reg  = 0xC7;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
        break;
    case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
        reg  = 0xC8;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
        break;
    case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_32:
        reg  = 0xC9;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
        break;
    case MODULE_TXDISABLE_33 ... MODULE_TXDISABLE_40:
        reg  = 0xCA;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_33);
        break;
    case MODULE_TXDISABLE_41 ... MODULE_TXDISABLE_48:
        reg  = 0xCB;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_41);
        break;
    case MODULE_RXLOS_1 ... MODULE_RXLOS_8:
        reg  = 0xA6;
        mask = 0x1 << (attr->index - MODULE_RXLOS_1);
        break;
    case MODULE_RXLOS_9 ... MODULE_RXLOS_16:
        reg  = 0xA7;
        mask = 0x1 << (attr->index - MODULE_RXLOS_9);
        break;
    case MODULE_RXLOS_17 ... MODULE_RXLOS_24:
        reg  = 0xA8;
        mask = 0x1 << (attr->index - MODULE_RXLOS_17);
        break;
    case MODULE_RXLOS_25 ... MODULE_RXLOS_32:
        reg  = 0xA9;
        mask = 0x1 << (attr->index - MODULE_RXLOS_25);
        break;
    case MODULE_RXLOS_33 ... MODULE_RXLOS_40:
        reg  = 0xAA;
        mask = 0x1 << (attr->index - MODULE_RXLOS_33);
        break;
    case MODULE_RXLOS_41 ... MODULE_RXLOS_48:
        reg  = 0xAB;
        mask = 0x1 << (attr->index - MODULE_RXLOS_41);
        break;
    case MODULE_TXFAULT_1 ... MODULE_TXFAULT_8:
        reg  = 0xA0;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_1);
        break;
    case MODULE_TXFAULT_9 ... MODULE_TXFAULT_16:
        reg  = 0xA1;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_9);
        break;
    case MODULE_TXFAULT_17 ... MODULE_TXFAULT_24:
        reg  = 0xA2;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_17);
        break;
    case MODULE_TXFAULT_25 ... MODULE_TXFAULT_32:
        reg  = 0xA3;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_25);
        break;
    case MODULE_TXFAULT_33 ... MODULE_TXFAULT_40:
        reg  = 0xA4;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_33);
        break;
    case MODULE_TXFAULT_41 ... MODULE_TXFAULT_48:
        reg  = 0xA5;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_41);
        break;
    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", invert ? !(status & mask) : !!(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_module_52x(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0, invert = 0;

    if (data->platform_id == AS4224_48X) {
        return show_module_48x(dev, da, buf);
    }

    switch (attr->index) {
    case MODULE_PRESENT_49 ... MODULE_PRESENT_52:
        invert = 1;
        reg  = 0x41;
        mask = 0x1 << (attr->index - MODULE_PRESENT_49);
        break;
    case MODULE_RXLOS_49 ... MODULE_RXLOS_52:
        reg  = 0x40;
        mask = 0x1 << (attr->index - MODULE_RXLOS_49);
        break;
    case MODULE_TXFAULT_49 ... MODULE_TXFAULT_52:
        reg  = 0x40;
        mask = 0x10 << (attr->index - MODULE_PRESENT_49);
        break;
    case MODULE_TXDISABLE_49 ... MODULE_TXDISABLE_52:
        reg  = 0x42;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_49);
        break;
    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", invert ? !(status & mask) : !!(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_module(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    int ret = 0;

    switch (attr->index) {
    case MODULE_INDEX_BEGIN:
        ret = (data->platform_id == AS4224_48X) ? 1: 49;
        break;
    case MODULE_COUNT:
        ret = (data->platform_id == AS4224_48X) ? 48: 4;
        break;
    default:
        return 0;
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_control_48x(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    long value;
    int status;
    u8 reg = 0, mask = 0;

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    switch (attr->index) {
    case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
        reg  = 0xC6;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
        break;
    case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
        reg  = 0xC7;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
        break;
    case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
        reg  = 0xC8;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
        break;
    case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_32:
        reg  = 0xC9;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
        break;
    case MODULE_TXDISABLE_33 ... MODULE_TXDISABLE_40:
        reg  = 0xCA;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_33);
        break;
    case MODULE_TXDISABLE_41 ... MODULE_TXDISABLE_48:
        reg  = 0xCB;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_41);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update tx_disable status */
    if (value) {
        value = (status | mask);
    }
    else {
        value = (status & ~mask);
    }

    /* Set value to CPLD */
    status = as4224_cpld_write_internal(client, reg, value);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int i2c_write_request_begin(struct i2c_client *client)
{
    int status = 0;
    int retry  = 0;

    for (retry = 0; retry <= I2C_WRITE_REQUEST_RETRY_TIMES; retry++) {
        /* Read current status */
        status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
        if (unlikely(status < 0)) {
            continue;
        }

        if (status & I2C_LOCK_BY_7040_VAL) {
            return 0; /* I2C already lock by 7040, just return */
        }

        status |= I2C_WRITE_REQUEST_7040_VAL;
        status = i2c_smbus_write_byte_data(client, I2C_WRITE_REQUEST_REG, status);
        if (unlikely(status < 0)) {
            continue;
        }

        /* Read out to make sure if 7040 get the access right */
        msleep(50);
        status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
        if (unlikely(status < 0)) {
            continue;
        }

        if (status & I2C_LOCK_BY_7040_VAL) {
            return 0;
        }

        status = -EBUSY;

        if (retry != I2C_WRITE_REQUEST_RETRY_TIMES) {
            msleep(1000);
        }
    }

    return status;
}

static int i2c_write_request_end(struct i2c_client *client)
{
    int status = 0;

    status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
    if (unlikely(status < 0)) {
        return status;
    }

    status &= ~I2C_WRITE_REQUEST_7040_VAL;
    status = i2c_smbus_write_byte_data(client, I2C_WRITE_REQUEST_REG, status);
    if (unlikely(status < 0)) {
        return status;
    }

    return 0;
}

static ssize_t show_i2c_request(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int status = 0;

    status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
    if (unlikely(status < 0)) {
        return status;
    }

    return sprintf(buf, "%d\n", !!(status & I2C_LOCK_BY_7040_VAL));
}

static ssize_t set_i2c_request(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    long value;
    int status = 0;

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    if (value) {
        status = i2c_write_request_begin(client);
    }
    else {
        status = i2c_write_request_end(client);
    }

    return status ? status : count;
}

static ssize_t set_control_52x(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    long value;
    int status;
    u8 reg = 0, mask = 0;

    if (data->platform_id == AS4224_48X) {
        return set_control_48x(dev, da, buf, count);
    }

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    switch (attr->index) {
    case MODULE_TXDISABLE_49 ... MODULE_TXDISABLE_52:
        reg  = 0x42;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_49);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update tx_disable status */
    if (value) {
        value = (status | mask);
    }
    else {
        value = (status & ~mask);
    }

    /* Set value to CPLD */
    status = as4224_cpld_write_internal(client, reg, value);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0;

    switch (attr->index) {
    case WTD_STATE_7040:
        reg  = 0x90;
        mask = 0x2;
        break;
    case WTD_ENABLE_7040:
        reg  = 0x91;
        mask = 0x10;
        break;
    case WTD_CLOCK_7040:
        reg  = 0x92;
        mask = 0xC0;
        break;
    case WTD_COUNTER_7040:
        reg  = 0x92;
        mask = 0x3F;
        break;
    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    while (!(mask & 0x1)) {
        status >>= 1;
        mask >>= 1;
    }

    return sprintf(buf, "%d\n", (status & mask));
exit:
    mutex_unlock(&data->update_lock);
    return status;
}

#define VALIDATE_WTD_VAL_RETURN(value, mask) {if (value & ~mask) return -EINVAL;}

static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    long value;
    int status;
    u8 reg = 0, mask = 0;

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    switch (attr->index) {
    case WTD_ENABLE_7040:
        reg   = 0x91;
        mask  = 0xD0;
        value = ((!!value) | 0x8) << 4;
        VALIDATE_WTD_VAL_RETURN(value, mask);
        break;
    case WTD_CLOCK_7040:
        reg  = 0x92;
        mask = 0xC0;
        value <<= 6;
        VALIDATE_WTD_VAL_RETURN(value, mask);
        break;
    case WTD_COUNTER_7040:
        reg  = 0x92;
        mask = 0x3F;
        value &= mask;
        VALIDATE_WTD_VAL_RETURN(value, mask);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);

    status = as4224_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update wtd status */
    status = (value & mask) | (status & ~mask);
    status = as4224_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t reset_wtd(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    long value;
    int status;

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    if (!value) {
        return count;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);

	/* Set gpio as output and set value as 0->1 to reset wtd */
    gpio_direction_output(WTD_RESET_GPIO_PIN_MPP3, 0);
    gpio_direction_output(WTD_RESET_GPIO_PIN_MPP3, 1);

    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int status;
    u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as4224_cpld_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static void as4224_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void as4224_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }

    mutex_unlock(&list_lock);
}

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

    val = i2c_smbus_read_byte_data(client, 0x1);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d\n", val);
}

static ssize_t show_platform_id(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    return sprintf(buf, "%d\n", data->platform_id);
}

/*
 * I2C init/probing/exit functions
 */
static int as4224_cpld_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct as4224_cpld_data *data;
    int ret = -ENODEV;
    int i   = 0;
    u64 port_map[4] = { [AS4224_48X]     = 0xFFFFFFFFFFFF,
                        [AS4224_52P]     = 0xF000000000000,
                        [AS4224_52T]     = 0xF000000000000,
                        [AS4224_52T_DAC] = 0xF000000000000};

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        goto exit;

    data = kzalloc(sizeof(struct as4224_cpld_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->type = id->driver_data;
    data->platform_id = get_platform_id(client);

    if (data->platform_id == PID_UNKNOWN) {
        ret = -ENODEV;
        goto exit_free;
    }

    /* Register sysfs hooks */
    for (i = 0; i < ARRAY_SIZE(cpld_group); i++) {
        /* index 0 is for common attriute, register anyway */
        if ((i != 0) && !(port_map[data->platform_id] & BIT(i-1))) {
            continue;
        }

        ret = sysfs_create_group(&client->dev.kobj, &cpld_group[i]);
        if (ret) {
            goto exit_free;
        }
    }

    ret = gpio_request(WTD_RESET_GPIO_PIN_MPP3, "wtd_reset_7040");
    if (ret) {
        dev_err(&client->dev, "Failed to request MPP3 gpio\n");
        goto exit_free;
    }

    as4224_cpld_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
    for (--i; i >= 0; i--) {
        sysfs_remove_group(&client->dev.kobj, &cpld_group[i]);
    }
    return ret;
}

static int as4224_cpld_remove(struct i2c_client *client)
{
    int i = 0;
    struct as4224_cpld_data *data = i2c_get_clientdata(client);
    as4224_cpld_remove_client(client);
	gpio_free(WTD_RESET_GPIO_PIN_MPP3);

    for (i = 0; i < ARRAY_SIZE(cpld_group); i++) {
        sysfs_remove_group(&client->dev.kobj, &cpld_group[i]);
    }

    kfree(data);
    return 0;
}

static int as4224_cpld_read_internal(struct i2c_client *client, u8 reg)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_read_byte_data(client, reg);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    return status;
}

static int as4224_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    status = i2c_write_request_begin(client);
    if (unlikely(status < 0)) {
        return status;
    }

    while (retry) {
        status = i2c_smbus_write_byte_data(client, reg, value);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    status = i2c_write_request_end(client);
    if (unlikely(status < 0)) {
        return status;
    }

    return status;
}

int as4224_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as4224_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as4224_cpld_read);

int as4224_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as4224_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as4224_cpld_write);

static struct i2c_driver as4224_cpld_driver = {
    .driver        = {
        .name    = "as4224_cpld",
        .owner    = THIS_MODULE,
    },
    .probe        = as4224_cpld_probe,
    .remove        = as4224_cpld_remove,
    .id_table    = as4224_cpld_id,
};

static int __init as4224_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as4224_cpld_driver);
}

static void __exit as4224_cpld_exit(void)
{
    i2c_del_driver(&as4224_cpld_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4224_cpld driver");
MODULE_LICENSE("GPL");

module_init(as4224_cpld_init);
module_exit(as4224_cpld_exit);