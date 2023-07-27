/*
 * arm64-delta-tn48m-cpld.c - Read/Write tn48m CPLD registers
 *
 * Copyright (C) 2020 Delta network Technology Corporation.
 * Chenglin Tsai <chenglin.tsai@deltaww.com>
 *
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

#include <linux/kernel.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

/* CPLD Register Define */
#define TN48M_CPLD_SLAVE_ADDR           0x41

#define TN48M_CPLD_REG_HW_VER           0x00    /* RO */
#define TN48M_CPLD_REG_PLATFORM_ID      0x01    /* RO */
#define TN48M_CPLD_REG_CPLD_VER         0x02    /* RO */
#define TN48M_CPLD_REG_PSU_STATUS       0x0A    /* RO */
#define TN48M_CPLD_REG_SFP_TX_DIS_1     0x31    /* RW */
#define TN48M_CPLD_REG_SFP_PSNT_1       0x3A    /* RO */
#define TN48M_CPLD_REG_SFP_RX_LOS_1     0x40    /* RO */

#define TN48M_CPLD_BIT_PSU1_PRESENT     0
#define TN48M_CPLD_BIT_PSU2_PRESENT     1
#define TN48M_CPLD_BIT_PSU1_PG          2
#define TN48M_CPLD_BIT_PSU2_PG          3
#define TN48M_CPLD_BIT_DCIN_PG          6

#define I2C_RW_RETRY_COUNT              10
#define I2C_RW_RETRY_INTERVAL           60      /* ms */

#define TN48M_GPIO_I2C_NGPIOS           12
#define TN4810M_GPIO_I2C_NGPIOS         144

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_type {
    tn48m_cpld,
    tn4810m_cpld
};

enum tn48m_platform_id_e {
    PID_TN48M = 0x0A,
    PID_TN48M_POE,
    PID_TN4810M,
    PID_TN48M2,
    PID_UNKNOWN
};

struct tn48m_cpld_data {
    char valid;
    struct device *hwmon_dev;
    struct mutex  update_lock;
    unsigned long last_updated;     /* in jiffies */

    enum cpld_type type;
    int sfp_group_num;
    u8 reg_offset;
    u8 reg_data;

    /* register values */
    u8 hw_ver;              /* 0x00 RO*/
    u8 platform_id;         /* 0x01 RO*/
    u8 cpld_ver;            /* 0x02 RO*/
    u8 psu_status;          /* 0x0A RO*/
    u8 sfp_tx_dis[6];       /* 0x31 ~ 0x36 RW */
    u8 sfp_present[6];      /* 0x3A ~ 0x3F RO */
    u8 sfp_rx_los[6];       /* 0x40 ~ 0x45 RO */
};

struct gpio_i2c_reg {
    struct list_head head;
    u32 gpio_num;
    u32 reg_addr;
    u32 reg_mask;
};

struct gpio_i2c_chip {
    struct gpio_chip gpio_chip;
    struct i2c_client *i2c;
    struct device *dev;

    struct list_head gpio_list;
};

static struct tn48m_cpld_data *tn48m_cpld_update_device(struct device *dev);

enum tn48m_cpld_sysfs_attributes {
    PSU1_PRESENT,
    PSU2_PRESENT,
    PSU1_PG,
    PSU2_PG,
    DCIN_PG,
    MODULE_PRESENT_ALL,
    MODULE_RX_LOS_ALL,
    MODULE_TX_DIS_ALL,
    MODULE_TX_DIS,
    MODULE_PRESENT,
    MODULE_RX_LOS,
};

/*
 * CPLD read/write functions
 */
static int tn48m_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int tn48m_cpld_write_internal(struct i2c_client *client,
                                     u8 reg, u8 value)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_write_byte_data(client, reg, value);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }
        break;
    }

    return status;
}

int tn48m_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    struct tn48m_cpld_data *data;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            data = i2c_get_clientdata(cpld_node->client);
            mutex_lock(&data->update_lock);
            ret = tn48m_cpld_read_internal(cpld_node->client, reg);
            mutex_unlock(&data->update_lock);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(tn48m_cpld_read);

int tn48m_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    struct tn48m_cpld_data *data;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            data = i2c_get_clientdata(cpld_node->client);
            mutex_lock(&data->update_lock);
            ret = tn48m_cpld_write_internal(cpld_node->client, reg, value);
            mutex_unlock(&data->update_lock);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(tn48m_cpld_write);

static struct gpio_i2c_reg *gpio_i2c_reg_by_num(struct gpio_i2c_chip *chip,
                                                unsigned int offs)
{
    struct gpio_i2c_reg *gpio_reg;

    list_for_each_entry(gpio_reg, &chip->gpio_list, head) {
        if (gpio_reg->gpio_num == offs)
            return gpio_reg;
    }

    return NULL;
}

static int gpio_i2c_get_value(struct gpio_chip *gc, unsigned int offs)
{
    struct gpio_i2c_chip *chip = gpiochip_get_data(gc);
    struct gpio_i2c_reg *gpio_reg;
    struct tn48m_cpld_data *data = i2c_get_clientdata(chip->i2c);
    int val;

    gpio_reg = gpio_i2c_reg_by_num(chip, offs);
    if (!gpio_reg) {
        dev_err(chip->dev, "invalid gpio offset (0x%x)\n", offs);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    val = tn48m_cpld_read_internal(chip->i2c, gpio_reg->reg_addr);
    mutex_unlock(&data->update_lock);
    val &= gpio_reg->reg_mask;

    return val;
}

static void gpio_i2c_set_value(struct gpio_chip *gc, unsigned int offs, int set)
{
    struct gpio_i2c_chip *chip = gpiochip_get_data(gc);
    struct gpio_i2c_reg *gpio_reg;
    struct tn48m_cpld_data *data = i2c_get_clientdata(chip->i2c);
    unsigned int val;

    gpio_reg = gpio_i2c_reg_by_num(chip, offs);
    if (!gpio_reg) {
        dev_err(chip->dev, "invalid gpio offset (0x%x)\n", offs);
        return;
    }

    mutex_lock(&data->update_lock);
    val = tn48m_cpld_read_internal(chip->i2c, gpio_reg->reg_addr);
    val &= ~gpio_reg->reg_mask;
    if (set)
        val |= gpio_reg->reg_mask;

    tn48m_cpld_write_internal(chip->i2c, gpio_reg->reg_addr, val);
    mutex_unlock(&data->update_lock);
}

/* Read and Write CPLD register data codes */
static ssize_t show_reg_offset(struct device *dev,
                               struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);
    u8 reg_offset;

    mutex_lock(&data->update_lock);
    reg_offset = data->reg_offset;
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "0x%x\n", reg_offset);
}

static ssize_t set_reg_offset(struct device *dev,
                              struct device_attribute *devattr,
                              const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);

    u8 reg_offset = simple_strtoul(buf, NULL, 16);
    mutex_lock(&data->update_lock);
    data->reg_offset = reg_offset;
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_reg_data(struct device *dev,
                             struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);
    u8 reg_data;

    mutex_lock(&data->update_lock);
    reg_data = tn48m_cpld_read_internal(client, data->reg_offset);
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "0x%x\n", reg_data);
}

static ssize_t set_reg_data(struct device *dev,
                            struct device_attribute *devattr,
                            const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);
    u8 reg_data;
    int err;

    reg_data = simple_strtoul(buf, NULL, 16);

    mutex_lock(&data->update_lock);
    err = tn48m_cpld_write_internal(client, data->reg_offset, reg_data);
    mutex_unlock(&data->update_lock);

    return err < 0 ? err : count;
}

static SENSOR_DEVICE_ATTR(reg_offset,
                          S_IRUGO | S_IWUSR,
                          show_reg_offset, set_reg_offset, 0);
static SENSOR_DEVICE_ATTR(reg_data,
                          S_IRUGO | S_IWUSR, show_reg_data, set_reg_data, 0);

static ssize_t show_hw_version(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    u8 hw_version;

    hw_version = data->hw_ver & 0x0f;

    return sprintf(buf, "0x%x\n", hw_version);
}

static ssize_t show_platform_id(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    u8 platform_id;

    platform_id = data->platform_id;

    return sprintf(buf, "0x%x\n", platform_id);
}

static ssize_t show_cpld_version(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    u8 cpld_ver;

    cpld_ver = data->cpld_ver;

    return sprintf(buf, "%x\n", cpld_ver);
}

/* HW Version, Register 0x00 */
static SENSOR_DEVICE_ATTR(hw_version, S_IRUGO, show_hw_version, NULL, 0);
/* Product ID, Register 0x01 */
static SENSOR_DEVICE_ATTR(platform_id, S_IRUGO, show_platform_id, NULL, 0);
/* CPLD Version, Register 0x02 */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);

static ssize_t show_psu_status_bit(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    int bit;

    switch (attr->index) {
    case PSU1_PRESENT:
        bit = !!(data->psu_status & BIT(TN48M_CPLD_BIT_PSU1_PRESENT));
        break;
    case PSU2_PRESENT:
        bit = !!(data->psu_status & BIT(TN48M_CPLD_BIT_PSU2_PRESENT));
        break;
    case PSU1_PG:
        bit = !!(data->psu_status & BIT(TN48M_CPLD_BIT_PSU1_PG));
        break;
    case PSU2_PG:
        bit = !!(data->psu_status & BIT(TN48M_CPLD_BIT_PSU2_PG));
        break;
    case DCIN_PG:
        bit = !!(data->psu_status & BIT(TN48M_CPLD_BIT_DCIN_PG));
        break;
    default:
        bit = 0;
        dev_err(dev, "Unknown case %d in show_psu_status_bit.\n", attr->index);
    }

    return sprintf(buf, "%d\n", bit);
}

/* PSU Stauts, Register 0x0A */
static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO, show_psu_status_bit, NULL, PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO, show_psu_status_bit, NULL, PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_powergood, S_IRUGO, show_psu_status_bit, NULL, PSU1_PG);
static SENSOR_DEVICE_ATTR(psu2_powergood, S_IRUGO, show_psu_status_bit, NULL, PSU2_PG);
static SENSOR_DEVICE_ATTR(dcin_powergood, S_IRUGO, show_psu_status_bit, NULL, DCIN_PG);

static ssize_t show_module_status_all(struct device *dev,
                                      struct device_attribute *devattr,
                                      char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    u64 reg_val = 0;
    int i;

    switch (attr->index) {
    case MODULE_TX_DIS_ALL:
        for (i = 0; i < data->sfp_group_num; i++)
            reg_val |= ((u64)data->sfp_tx_dis[i] << (i * 8));
        break;
    case MODULE_PRESENT_ALL:
        for (i = 0; i < data->sfp_group_num; i++)
            reg_val |= ((u64)data->sfp_present[i] << (i * 8));
        break;
    case MODULE_RX_LOS_ALL:
        for (i = 0; i < data->sfp_group_num; i++)
            reg_val |= ((u64)data->sfp_rx_los[i] << (i * 8));
        break;
    default:
        dev_err(dev, "Unknown case %d in show_module_status_all\n",
                attr->index);
    }

    return sprintf(buf, "0x%016llx\n", reg_val);
}

static ssize_t set_module_txdis_all(struct device *dev,
                                    struct device_attribute *devattr,
                                    const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    int err, i;
    u64 txdis_bitmap;
    u8 reg_val;

    err = kstrtoull(buf, 16, &txdis_bitmap);
    if (err < 0)
        return err;

    for(i = 0; i < data->sfp_group_num ; i ++) {
        reg_val = (txdis_bitmap >> (i * 8)) & 0xff;

        mutex_lock(&data->update_lock);
        err = tn48m_cpld_write_internal(client,
                                        TN48M_CPLD_REG_SFP_TX_DIS_1 + i,
                                        reg_val);
        mutex_unlock(&data->update_lock);

        if(err < 0)
            return err;
    }

    return count;
}


/* transceiver attributes all */
static SENSOR_DEVICE_ATTR(module_tx_dis_all,
                          S_IWUSR | S_IRUGO,
                          show_module_status_all, set_module_txdis_all,
                          MODULE_TX_DIS_ALL);
static SENSOR_DEVICE_ATTR(module_present_all,
                          S_IRUGO,
                          show_module_status_all, NULL,
                          MODULE_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(module_rx_los_all,
                          S_IRUGO,
                          show_module_status_all, NULL,
                          MODULE_RX_LOS_ALL);

static ssize_t show_sfp_status_bit(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    struct sensor_device_attribute_2 *attr_2 = to_sensor_dev_attr_2(devattr);
    int port_num = attr_2->index;
    int group_idx = (port_num - 1) / 8;
    int shift_idx  = (port_num - 1) % 8;
    int nr = attr_2->nr;
    int bit;

    switch (nr) {
    case MODULE_TX_DIS:
        bit = !!(data->sfp_tx_dis[group_idx] & BIT(shift_idx));
        break;
    case MODULE_PRESENT:
        bit = !!(data->sfp_present[group_idx] & BIT(shift_idx));
        break;
    case MODULE_RX_LOS:
        bit = !!(data->sfp_rx_los[group_idx] & BIT(shift_idx));
        break;
    default:
        bit = 0;
        dev_err(dev, "Unknown case %d in get_sfp_status_bit.\n", nr);
    }

    return sprintf(buf, "%d\n", bit);
}

static ssize_t set_sfp_txdis_bit(struct device *dev,
                                 struct device_attribute *devattr,
                                 const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = tn48m_cpld_update_device(dev);
    struct sensor_device_attribute_2 *attr_2 = to_sensor_dev_attr_2(devattr);
    int port_num  = attr_2->index;
    int group_idx = (port_num - 1) / 8;
    int shift_idx = (port_num - 1) % 8;
    int err;
    long val;
    u8 reg_val;

    err = kstrtol(buf, 16, &val);
    if (err < 0)
        return err;

    val = clamp_val(val, 0, 1);

    reg_val = data->sfp_tx_dis[group_idx];
    reg_val &= ~(BIT(shift_idx));
    reg_val |= (val << shift_idx);

    mutex_lock(&data->update_lock);
    err = tn48m_cpld_write_internal(client,
                                    TN48M_CPLD_REG_SFP_TX_DIS_1 + group_idx,
                                    reg_val);
    mutex_unlock(&data->update_lock);

    return err < 0 ? err : count;
}

/* SFP transceiver attributes */
#define SENSOR_DEV_ATTR_SFP_MODULE(index)               \
static SENSOR_DEVICE_ATTR_2(module_tx_dis_##index,      \
                            S_IRUGO | S_IWUSR,          \
                            show_sfp_status_bit,        \
                            set_sfp_txdis_bit,          \
                            MODULE_TX_DIS,              \
                            index);                     \
static SENSOR_DEVICE_ATTR_2(module_present_##index,     \
                            S_IRUGO,                    \
                            show_sfp_status_bit,        \
                            NULL,                       \
                            MODULE_PRESENT,             \
                            index);                     \
static SENSOR_DEVICE_ATTR_2(module_rx_los_##index,      \
                            S_IRUGO,                    \
                            show_sfp_status_bit,        \
                            NULL,                       \
                            MODULE_RX_LOS,              \
                            index);

#define SENSOR_ATTR_SFP_MODULE(index)                       \
    &sensor_dev_attr_module_tx_dis_##index.dev_attr.attr,   \
    &sensor_dev_attr_module_present_##index.dev_attr.attr,  \
    &sensor_dev_attr_module_rx_los_##index.dev_attr.attr

SENSOR_DEV_ATTR_SFP_MODULE(1);
SENSOR_DEV_ATTR_SFP_MODULE(2);
SENSOR_DEV_ATTR_SFP_MODULE(3);
SENSOR_DEV_ATTR_SFP_MODULE(4);
SENSOR_DEV_ATTR_SFP_MODULE(5);
SENSOR_DEV_ATTR_SFP_MODULE(6);
SENSOR_DEV_ATTR_SFP_MODULE(7);
SENSOR_DEV_ATTR_SFP_MODULE(8);
SENSOR_DEV_ATTR_SFP_MODULE(9);
SENSOR_DEV_ATTR_SFP_MODULE(10);
SENSOR_DEV_ATTR_SFP_MODULE(11);
SENSOR_DEV_ATTR_SFP_MODULE(12);
SENSOR_DEV_ATTR_SFP_MODULE(13);
SENSOR_DEV_ATTR_SFP_MODULE(14);
SENSOR_DEV_ATTR_SFP_MODULE(15);
SENSOR_DEV_ATTR_SFP_MODULE(16);
SENSOR_DEV_ATTR_SFP_MODULE(17);
SENSOR_DEV_ATTR_SFP_MODULE(18);
SENSOR_DEV_ATTR_SFP_MODULE(19);
SENSOR_DEV_ATTR_SFP_MODULE(20);
SENSOR_DEV_ATTR_SFP_MODULE(21);
SENSOR_DEV_ATTR_SFP_MODULE(22);
SENSOR_DEV_ATTR_SFP_MODULE(23);
SENSOR_DEV_ATTR_SFP_MODULE(24);
SENSOR_DEV_ATTR_SFP_MODULE(25);
SENSOR_DEV_ATTR_SFP_MODULE(26);
SENSOR_DEV_ATTR_SFP_MODULE(27);
SENSOR_DEV_ATTR_SFP_MODULE(28);
SENSOR_DEV_ATTR_SFP_MODULE(29);
SENSOR_DEV_ATTR_SFP_MODULE(30);
SENSOR_DEV_ATTR_SFP_MODULE(31);
SENSOR_DEV_ATTR_SFP_MODULE(32);
SENSOR_DEV_ATTR_SFP_MODULE(33);
SENSOR_DEV_ATTR_SFP_MODULE(34);
SENSOR_DEV_ATTR_SFP_MODULE(35);
SENSOR_DEV_ATTR_SFP_MODULE(36);
SENSOR_DEV_ATTR_SFP_MODULE(37);
SENSOR_DEV_ATTR_SFP_MODULE(38);
SENSOR_DEV_ATTR_SFP_MODULE(39);
SENSOR_DEV_ATTR_SFP_MODULE(40);
SENSOR_DEV_ATTR_SFP_MODULE(41);
SENSOR_DEV_ATTR_SFP_MODULE(42);
SENSOR_DEV_ATTR_SFP_MODULE(43);
SENSOR_DEV_ATTR_SFP_MODULE(44);
SENSOR_DEV_ATTR_SFP_MODULE(45);
SENSOR_DEV_ATTR_SFP_MODULE(46);
SENSOR_DEV_ATTR_SFP_MODULE(47);
SENSOR_DEV_ATTR_SFP_MODULE(48);

static struct attribute *tn48m_cpld_attributes[] = {
    &sensor_dev_attr_reg_offset.dev_attr.attr,
    &sensor_dev_attr_reg_data.dev_attr.attr,
    &sensor_dev_attr_hw_version.dev_attr.attr,
    &sensor_dev_attr_platform_id.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_powergood.dev_attr.attr,
    &sensor_dev_attr_psu2_powergood.dev_attr.attr,
    &sensor_dev_attr_dcin_powergood.dev_attr.attr,
    /* tn48m transceiver attributes */
    &sensor_dev_attr_module_tx_dis_all.dev_attr.attr,
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    &sensor_dev_attr_module_rx_los_all.dev_attr.attr,
    SENSOR_ATTR_SFP_MODULE(1),
    SENSOR_ATTR_SFP_MODULE(2),
    SENSOR_ATTR_SFP_MODULE(3),
    SENSOR_ATTR_SFP_MODULE(4),
    NULL
};

static struct attribute *tn4810m_cpld_attributes[] = {
    &sensor_dev_attr_reg_offset.dev_attr.attr,
    &sensor_dev_attr_reg_data.dev_attr.attr,
    &sensor_dev_attr_hw_version.dev_attr.attr,
    &sensor_dev_attr_platform_id.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_powergood.dev_attr.attr,
    &sensor_dev_attr_psu2_powergood.dev_attr.attr,
    /* tn4810m transceiver attributes */
    &sensor_dev_attr_module_tx_dis_all.dev_attr.attr,
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    &sensor_dev_attr_module_rx_los_all.dev_attr.attr,
    SENSOR_ATTR_SFP_MODULE(1),
    SENSOR_ATTR_SFP_MODULE(2),
    SENSOR_ATTR_SFP_MODULE(3),
    SENSOR_ATTR_SFP_MODULE(4),
    SENSOR_ATTR_SFP_MODULE(5),
    SENSOR_ATTR_SFP_MODULE(6),
    SENSOR_ATTR_SFP_MODULE(7),
    SENSOR_ATTR_SFP_MODULE(8),
    SENSOR_ATTR_SFP_MODULE(9),
    SENSOR_ATTR_SFP_MODULE(10),
    SENSOR_ATTR_SFP_MODULE(11),
    SENSOR_ATTR_SFP_MODULE(12),
    SENSOR_ATTR_SFP_MODULE(13),
    SENSOR_ATTR_SFP_MODULE(14),
    SENSOR_ATTR_SFP_MODULE(15),
    SENSOR_ATTR_SFP_MODULE(16),
    SENSOR_ATTR_SFP_MODULE(17),
    SENSOR_ATTR_SFP_MODULE(18),
    SENSOR_ATTR_SFP_MODULE(19),
    SENSOR_ATTR_SFP_MODULE(20),
    SENSOR_ATTR_SFP_MODULE(21),
    SENSOR_ATTR_SFP_MODULE(22),
    SENSOR_ATTR_SFP_MODULE(23),
    SENSOR_ATTR_SFP_MODULE(24),
    SENSOR_ATTR_SFP_MODULE(25),
    SENSOR_ATTR_SFP_MODULE(26),
    SENSOR_ATTR_SFP_MODULE(27),
    SENSOR_ATTR_SFP_MODULE(28),
    SENSOR_ATTR_SFP_MODULE(29),
    SENSOR_ATTR_SFP_MODULE(30),
    SENSOR_ATTR_SFP_MODULE(31),
    SENSOR_ATTR_SFP_MODULE(32),
    SENSOR_ATTR_SFP_MODULE(33),
    SENSOR_ATTR_SFP_MODULE(34),
    SENSOR_ATTR_SFP_MODULE(35),
    SENSOR_ATTR_SFP_MODULE(36),
    SENSOR_ATTR_SFP_MODULE(37),
    SENSOR_ATTR_SFP_MODULE(38),
    SENSOR_ATTR_SFP_MODULE(39),
    SENSOR_ATTR_SFP_MODULE(40),
    SENSOR_ATTR_SFP_MODULE(41),
    SENSOR_ATTR_SFP_MODULE(42),
    SENSOR_ATTR_SFP_MODULE(43),
    SENSOR_ATTR_SFP_MODULE(44),
    SENSOR_ATTR_SFP_MODULE(45),
    SENSOR_ATTR_SFP_MODULE(46),
    SENSOR_ATTR_SFP_MODULE(47),
    SENSOR_ATTR_SFP_MODULE(48),
    NULL
};

static const struct attribute_group tn48m_cpld_group = {
    .attrs = tn48m_cpld_attributes,
};

static const struct attribute_group tn4810m_cpld_group = {
    .attrs = tn4810m_cpld_attributes,
};

static int gpio_i2c_reg_parse(struct gpio_i2c_chip *chip, struct device_node *np)
{
    struct gpio_i2c_reg *gpio_reg;
    struct device *dev = chip->dev;
    u32 gpio_reg_map[2];
    u32 gpio_num;
    int err;

    err = of_property_read_u32_array(np, "reg-map", gpio_reg_map,
                     ARRAY_SIZE(gpio_reg_map));

    if (err) {
        dev_err(dev, "error while parsing 'reg-map' property\n");
        return err;
    }

    err = of_property_read_u32(np, "gpio-num", &gpio_num);
    if (err) {
        dev_err(dev, "error while parsing 'gpio-num' property\n");
        return err;
    }

    gpio_reg = devm_kmalloc(dev, sizeof(*gpio_reg), GFP_KERNEL);
    if (!gpio_reg)
        return err;

    gpio_reg->reg_addr = gpio_reg_map[0];
    gpio_reg->reg_mask = gpio_reg_map[1];
    gpio_reg->gpio_num = gpio_num;

    list_add(&gpio_reg->head, &chip->gpio_list);

    return 0;
}

static int gpio_i2c_map_parse(struct gpio_i2c_chip *chip,
                  struct device_node *gpio_map_np)
{
    struct device_node *node;

    for_each_child_of_node(gpio_map_np, node) {
        int err;

        err = gpio_i2c_reg_parse(chip, node);
        if (err)
            return err;
    }

    return 0;
}

static void tn48m_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node =
                    kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev,
                "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void tn48m_cpld_remove_client(struct i2c_client *client)
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

/* I2C Probe/Remove functions */
static int tn48m_cpld_probe(struct i2c_client *client,
                            const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct device *dev = &client->dev;
    struct device_node *np = dev->of_node;
    struct device_node *gpio_map_np;
    struct gpio_i2c_chip *chip;
    struct tn48m_cpld_data *data;
    int ret = -ENODEV;
    int product_id;
    int err;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        goto exit;

    if (!np) {
        ret = -EINVAL;
        goto exit;
    }

    chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
    if (!chip) {
        ret = -ENOMEM;
        goto exit;
    }

    INIT_LIST_HEAD(&chip->gpio_list);
    chip->i2c = client;
    chip->dev = dev;

    gpio_map_np = of_find_node_by_name(np, "gpio-map");
    if (gpio_map_np) {
        ret = gpio_i2c_map_parse(chip, gpio_map_np);
        if (ret)
            goto exit;
    }

    chip->gpio_chip.parent = dev;

    i2c_set_clientdata(client, chip);

    chip->gpio_chip.label = dev_name(dev);
    chip->gpio_chip.get = gpio_i2c_get_value;
    chip->gpio_chip.set = gpio_i2c_set_value;
    chip->gpio_chip.base = -1;
    chip->gpio_chip.owner = THIS_MODULE;
    chip->gpio_chip.can_sleep = true;

    data = kzalloc(sizeof(struct tn48m_cpld_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->reg_offset = 0x00;
    data->reg_data   = 0x00;
    data->type       = id->driver_data;

    if (data->type == tn4810m_cpld) {
        /* Read and check the product ID from CPLD */
        product_id = tn48m_cpld_read_internal(client,
                                              TN48M_CPLD_REG_PLATFORM_ID);
        if (product_id != PID_TN4810M) {
            dev_dbg(&client->dev,
            "The driver dev_id (0x%x) doesn't match CPLD product_id (0x%x)\n",
                    data->type, product_id);
            goto exit_free;
        }

        chip->gpio_chip.ngpio = TN4810M_GPIO_I2C_NGPIOS;
        data->sfp_group_num = 6;

        ret = sysfs_create_group(&client->dev.kobj, &tn4810m_cpld_group);
        if (ret) goto exit_free;
    } else {
        chip->gpio_chip.ngpio = TN48M_GPIO_I2C_NGPIOS;
        data->sfp_group_num = 1;

        ret = sysfs_create_group(&client->dev.kobj, &tn48m_cpld_group);
        if (ret) goto exit_free;
    }

    tn48m_cpld_add_client(client);

    return devm_gpiochip_add_data(dev, &chip->gpio_chip, chip);

exit_free:
    kfree(data);
exit:
    return ret;
}

static int tn48m_cpld_remove(struct i2c_client *client)
{
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);

    tn48m_cpld_remove_client(client);

    /* Remove sysfs hooks */
    if (data->type == tn4810m_cpld)
        sysfs_remove_group(&client->dev.kobj, &tn4810m_cpld_group);
    else
        sysfs_remove_group(&client->dev.kobj, &tn48m_cpld_group);

    kfree(data);

    return 0;
}

int tn48m_platform_id(void)
{
    int pid = tn48m_cpld_read(TN48M_CPLD_SLAVE_ADDR, TN48M_CPLD_REG_PLATFORM_ID);
    pid &= 0xF;

    if (pid < PID_TN48M || pid > PID_TN48M2) {
        return PID_UNKNOWN;
    }

    return pid;
}
EXPORT_SYMBOL(tn48m_platform_id);

static struct tn48m_cpld_data *tn48m_cpld_update_device(struct device *dev)
{
    int i;
    struct i2c_client *client = to_i2c_client(dev);
    struct tn48m_cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if(time_after(jiffies, data->last_updated + HZ) || !data->valid) {
        data->hw_ver =
            tn48m_cpld_read_internal(client, TN48M_CPLD_REG_HW_VER);
        data->platform_id =
            tn48m_cpld_read_internal(client, TN48M_CPLD_REG_PLATFORM_ID);
        data->cpld_ver =
            tn48m_cpld_read_internal(client, TN48M_CPLD_REG_CPLD_VER);

        data->psu_status =
            tn48m_cpld_read_internal(client, TN48M_CPLD_REG_PSU_STATUS);

        for (i = 0; i < data->sfp_group_num; i++) {
            data->sfp_tx_dis[i] =
                tn48m_cpld_read_internal(client,
                                         TN48M_CPLD_REG_SFP_TX_DIS_1 + i);
            data->sfp_present[i] =
                tn48m_cpld_read_internal(client,
                                         TN48M_CPLD_REG_SFP_PSNT_1 + i);
            data->sfp_rx_los[i] =
                tn48m_cpld_read_internal(client,
                                         TN48M_CPLD_REG_SFP_RX_LOS_1 + i);
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

/*
 * Driver Data
 */
static const struct i2c_device_id tn48m_cpld_id[] = {
    { "tn48m_cpld", tn48m_cpld },
    { "tn4810m_cpld", tn4810m_cpld },
    { }
};
MODULE_DEVICE_TABLE(i2c, tn48m_cpld_id);

#ifdef CONFIG_OF
static const struct of_device_id tn48m_cpld_of_match[] = {
    {
        .compatible = "dni,tn48m_cpld",
        .data = (void *)tn48m_cpld,
    },
    {
        .compatible = "dni,tn4810m_cpld",
        .data = (void *)tn4810m_cpld,
    },
    {},
};
MODULE_DEVICE_TABLE(i2c, tn48m_cpld_id);
#endif

static struct i2c_driver tn48m_cpld_driver = {
    .driver = {
        .name  = "arm64_delta_tn48m_cpld",
        .of_match_table = of_match_ptr(tn48m_cpld_of_match),
        .owner = THIS_MODULE,
    },
    .probe     = tn48m_cpld_probe,
    .remove    = tn48m_cpld_remove,
    .id_table  = tn48m_cpld_id,
};

/* I2C init/exit functions */
static int __init tn48m_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&tn48m_cpld_driver);
}

static void __exit tn48m_cpld_exit(void)
{
    i2c_del_driver(&tn48m_cpld_driver);
}

module_init(tn48m_cpld_init);
module_exit(tn48m_cpld_exit);

MODULE_AUTHOR("Chenglin Tsai <chenglin.tsai@deltaww.com>");
MODULE_DESCRIPTION("TN48M/TN4810M I2C CPLD driver");
MODULE_LICENSE("GPL");
