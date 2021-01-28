// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * arm64-delta-tx4810-dn-cpld.c - Read/Write tx4810-dn CPLD registers
 *
 * Copyright (C) 2021 Delta network Technology Corporation.
 * Chenglin Tsai <chenglin.tsai@deltaww.com>
 *
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *	pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

/* CPLD Register Define */
#define TX4810_DN_CPLD_SLAVE_ADDR       0x41

#define TX4810_DN_CPLD_REG_HW_VER       0x00 /* RO */
#define TX4810_DN_CPLD_REG_PLATFORM_ID  0x01 /* RO */
#define TX4810_DN_CPLD_REG_CPLD_VER     0x02 /* RO */
#define TX4810_DN_CPLD_REG_PSU_STATUS   0x0A /* RO */

#define TX4810_DN_CPLD_BIT_PSU1_PRESENT 0
#define TX4810_DN_CPLD_BIT_PSU2_PRESENT 1
#define TX4810_DN_CPLD_BIT_PSU1_PG      2
#define TX4810_DN_CPLD_BIT_PSU2_PG      3

#define I2C_RW_RETRY_COUNT              10
#define I2C_RW_RETRY_INTERVAL           60 /* ms */

static LIST_HEAD(cpld_client_list);
static struct mutex list_lock;

struct cpld_client_node {
	struct i2c_client *client;
	struct list_head list;
};

enum cpld_type {
	tx4810_dn_cpld
};

enum tx4810_dn_platform_id_e {
	PID_TX4810_DN = 0x0E,
	PID_UNKNOWN
};

struct tx4810_dn_cpld_data {
	char valid;
	struct device *hwmon_dev;
	struct mutex  update_lock;
	unsigned long last_updated; /* in jiffies */

	enum cpld_type type;
	int sfp_group_num;
	u8 reg_offset;
	u8 reg_data;

	/* register values */
	u8 hw_ver;	/* 0x00 RO*/
	u8 platform_id;	/* 0x01 RO*/
	u8 cpld_ver;	/* 0x02 RO*/
	u8 psu_status;	/* 0x0A RO*/
};

static struct tx4810_dn_cpld_data *tx4810_dn_cpld_update_device(
						struct device *dev);

/*
 * Driver Data
 */
static const struct i2c_device_id tx4810_dn_cpld_id[] = {
	{ "tx4810_dn_cpld", tx4810_dn_cpld },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tx4810_dn_cpld_id);

enum tx4810_dn_cpld_sysfs_attributes {
	PSU1_PRESENT,
	PSU2_PRESENT,
	PSU1_PG,
	PSU2_PG,
};

/*
 * CPLD read/write functions
 */
static int tx4810_dn_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int tx4810_dn_cpld_write_internal(struct i2c_client *client,
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

int tx4810_dn_cpld_read(unsigned short cpld_addr, u8 reg)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	int ret = -EPERM;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		cpld_node = list_entry(list_node,
				       struct cpld_client_node, list);

		if (cpld_node->client->addr == cpld_addr) {
			ret = tx4810_dn_cpld_read_internal(cpld_node->client,
							   reg);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(tx4810_dn_cpld_read);

int tx4810_dn_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	int ret = -EIO;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		cpld_node = list_entry(list_node,
				       struct cpld_client_node, list);

		if (cpld_node->client->addr == cpld_addr) {
			ret = tx4810_dn_cpld_write_internal(cpld_node->client,
							    reg, value);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(tx4810_dn_cpld_write);


/* Read and Write CPLD register data codes */
static ssize_t show_reg_offset(struct device *dev,
			       struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);
	u8 reg_offset;

	reg_offset = data->reg_offset;

	return sprintf(buf, "0x%x\n", reg_offset);
}

static ssize_t set_reg_offset(struct device *dev,
			      struct device_attribute *devattr,
			      const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);

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
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);
	u8 reg_data;

	mutex_lock(&data->update_lock);
	reg_data = tx4810_dn_cpld_read_internal(client, data->reg_offset);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "0x%x\n", reg_data);
}

static ssize_t set_reg_data(struct device *dev,
			    struct device_attribute *devattr,
			    const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);
	u8 reg_data;
	int err;

	reg_data = simple_strtoul(buf, NULL, 16);

	mutex_lock(&data->update_lock);
	err = tx4810_dn_cpld_write_internal(client,
					    data->reg_offset,
					    reg_data);
	mutex_unlock(&data->update_lock);

	return err < 0 ? err : count;
}

static SENSOR_DEVICE_ATTR(reg_offset, S_IRUGO | S_IWUSR,
			  show_reg_offset, set_reg_offset, 0);
static SENSOR_DEVICE_ATTR(reg_data, S_IRUGO | S_IWUSR,
			  show_reg_data, set_reg_data, 0);

static ssize_t show_hw_version(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct tx4810_dn_cpld_data *data = tx4810_dn_cpld_update_device(dev);
	u8 hw_version;

	hw_version = data->hw_ver & 0x0f;

	return sprintf(buf, "0x%x\n", hw_version);
}

static ssize_t show_platform_id(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tx4810_dn_cpld_data *data = tx4810_dn_cpld_update_device(dev);
	u8 platform_id;

	platform_id = data->platform_id;

	return sprintf(buf, "0x%x\n", platform_id);
}

static ssize_t show_cpld_version(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct tx4810_dn_cpld_data *data = tx4810_dn_cpld_update_device(dev);
	u8 cpld_major_ver, cpld_minor_ver;

	cpld_major_ver = (data->cpld_ver & 0xf0) >> 4;
	cpld_minor_ver = data->cpld_ver & 0x0f;

	return sprintf(buf, "%x.%x\n", cpld_major_ver, cpld_minor_ver);
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
	struct tx4810_dn_cpld_data *data = tx4810_dn_cpld_update_device(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	int bit;

	switch (attr->index) {
	case PSU1_PRESENT:
		bit = !!(data->psu_status &
			 BIT(TX4810_DN_CPLD_BIT_PSU1_PRESENT));
		break;
	case PSU2_PRESENT:
		bit = !!(data->psu_status &
			 BIT(TX4810_DN_CPLD_BIT_PSU2_PRESENT));
		break;
	case PSU1_PG:
		bit = !!(data->psu_status &
			 BIT(TX4810_DN_CPLD_BIT_PSU1_PG));
		break;
	case PSU2_PG:
		bit = !!(data->psu_status &
			 BIT(TX4810_DN_CPLD_BIT_PSU2_PG));
		break;
	default:
		bit = 0;
		dev_err(dev,
			"Unknown case %d in show_psu_status_bit.\n",
			attr->index);
	}

	return sprintf(buf, "%d\n", bit);
}

/* PSU Stauts, Register 0x0A */
static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO,
			  show_psu_status_bit, NULL, PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO,
			  show_psu_status_bit, NULL, PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_powergood, S_IRUGO,
			  show_psu_status_bit, NULL, PSU1_PG);
static SENSOR_DEVICE_ATTR(psu2_powergood, S_IRUGO,
			  show_psu_status_bit, NULL, PSU2_PG);

static struct attribute *tx4810_dn_cpld_attributes[] = {
	&sensor_dev_attr_reg_offset.dev_attr.attr,
	&sensor_dev_attr_reg_data.dev_attr.attr,
	&sensor_dev_attr_hw_version.dev_attr.attr,
	&sensor_dev_attr_platform_id.dev_attr.attr,
	&sensor_dev_attr_cpld_version.dev_attr.attr,
	&sensor_dev_attr_psu1_present.dev_attr.attr,
	&sensor_dev_attr_psu2_present.dev_attr.attr,
	&sensor_dev_attr_psu1_powergood.dev_attr.attr,
	&sensor_dev_attr_psu2_powergood.dev_attr.attr,
	NULL
};

static const struct attribute_group tx4810_dn_cpld_group = {
	.attrs = tx4810_dn_cpld_attributes,
};

static void tx4810_dn_cpld_add_client(struct i2c_client *client)
{
	struct cpld_client_node *node =
			kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

	if (!node) {
		dev_dbg(&client->dev,
			"Can't allocate cpld_client_node (0x%x)\n",
			client->addr);
		return;
	}

	node->client = client;

	mutex_lock(&list_lock);
	list_add(&node->list, &cpld_client_list);
	mutex_unlock(&list_lock);
}

static void tx4810_dn_cpld_remove_client(struct i2c_client *client)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	int found = 0;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		cpld_node = list_entry(list_node,
				       struct cpld_client_node, list);

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
static int tx4810_dn_cpld_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct tx4810_dn_cpld_data *data;
	int ret = -ENODEV;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		goto exit;

	data = kzalloc(sizeof(struct tx4810_dn_cpld_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->reg_offset = 0x00;
	data->reg_data   = 0x00;
	data->type	 = id->driver_data;

	data->sfp_group_num = 6;
	ret = sysfs_create_group(&client->dev.kobj, &tx4810_dn_cpld_group);
	if (ret)
		goto exit_free;

	tx4810_dn_cpld_add_client(client);
	return 0;

exit_free:
	kfree(data);
exit:
	return ret;
}

static int tx4810_dn_cpld_remove(struct i2c_client *client)
{
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);

	tx4810_dn_cpld_remove_client(client);

	/* Remove sysfs hooks */
	sysfs_remove_group(&client->dev.kobj, &tx4810_dn_cpld_group);

	kfree(data);

	return 0;
}

static struct i2c_driver tx4810_dn_cpld_driver = {
	.driver = {
		.name  = "arm64_delta_tx4810_dn_cpld",
		.owner = THIS_MODULE,
	},
	.probe = tx4810_dn_cpld_probe,
	.remove	= tx4810_dn_cpld_remove,
	.id_table = tx4810_dn_cpld_id,
};

int tx4810_dn_platform_id(void)
{
	int pid = tx4810_dn_cpld_read(TX4810_DN_CPLD_SLAVE_ADDR,
				      TX4810_DN_CPLD_REG_PLATFORM_ID);
	pid &= 0xF;

	if (pid != PID_TX4810_DN) {
		return PID_UNKNOWN;
	}

	return pid;
}
EXPORT_SYMBOL(tx4810_dn_platform_id);

static struct tx4810_dn_cpld_data *tx4810_dn_cpld_update_device(
						struct device *dev)
{
	int i;
	struct i2c_client *client = to_i2c_client(dev);
	struct tx4810_dn_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if(time_after(jiffies, data->last_updated + HZ) || !data->valid) {
		data->hw_ver = tx4810_dn_cpld_read_internal(client,
					TX4810_DN_CPLD_REG_HW_VER);
		data->platform_id = tx4810_dn_cpld_read_internal(client,
					TX4810_DN_CPLD_REG_PLATFORM_ID);
		data->cpld_ver = tx4810_dn_cpld_read_internal(client,
					TX4810_DN_CPLD_REG_CPLD_VER);
		data->psu_status = tx4810_dn_cpld_read_internal(client,
					TX4810_DN_CPLD_REG_PSU_STATUS);

		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}

/* I2C init/exit functions */
static int __init tx4810_dn_cpld_init(void)
{
	mutex_init(&list_lock);
	return i2c_add_driver(&tx4810_dn_cpld_driver);
}

static void __exit tx4810_dn_cpld_exit(void)
{
	i2c_del_driver(&tx4810_dn_cpld_driver);
}

MODULE_AUTHOR("Chenglin Tsai <chenglin.tsai@deltaww.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("TX4810_DN I2C CPLD driver");

module_init(tx4810_dn_cpld_init);
module_exit(tx4810_dn_cpld_exit);
