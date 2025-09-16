/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *	 Accton as45xx CPLD1/CPLD2/CPLD3
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
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <dt-bindings/mux/mux.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#define DRVNAME "as45xx_cpld"

#define I2C_RW_RETRY_COUNT 10
#define I2C_RW_RETRY_INTERVAL 60 /* ms */
#define BOARD_INFO_REG_OFFSET 0x00

static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static int __as45xx_cpld_read(struct i2c_client *client, u8 reg);
static int __as45xx_cpld_write(struct i2c_client *client, u8 reg,
			u8 value);

static LIST_HEAD(cpld_client_list);
static struct mutex list_lock;

struct cpld_client_node {
	struct i2c_client *client;
	struct list_head list;
};

enum cpld_type {
	as4500_cpld_m,
	as4581_cpld_m,
	as45xx_cpld_s
};

struct as45xx_cpld_data {
	enum cpld_type type;
	struct mutex update_lock;
	struct i2c_client *client;
    struct platform_device *fan_pdev;
    struct platform_device *sfp_pdev;
};

static const struct i2c_device_id as45xx_cpld_id[] = {
	{ "as4500_cpld_m", as4500_cpld_m },
	{ "as4581_cpld_m", as4581_cpld_m },
	{ "as45xx_cpld_s", as45xx_cpld_s },
	{ }
};
MODULE_DEVICE_TABLE(i2c, as45xx_cpld_id);


enum as45xx_cpld_sysfs_attributes {
	CPLD_VERSION_MAJOR,
	CPLD_VERSION_MINOR,
	HW_PCB_VERSION,
	ACCESS,
	WTD_ENABLE,  /* Register 0x91 bit 1 */
	WTD_CLOCK,   /* Register 0x92 bit 7:6 */
	WTD_COUNTER, /* Register 0x92 bit 5:0 */
	PLATFORM_ID  /* 0:as4500, 1:as4581 */
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(version_major, S_IRUGO, show_version, NULL,
	CPLD_VERSION_MAJOR);
static SENSOR_DEVICE_ATTR(version_minor, S_IRUGO, show_version, NULL,
	CPLD_VERSION_MINOR);
static SENSOR_DEVICE_ATTR(hw_pcb_version, S_IRUGO, show_version, NULL,
	HW_PCB_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);

static SENSOR_DEVICE_ATTR(platform_id, S_IRUGO, show_version, NULL,
	PLATFORM_ID);
static SENSOR_DEVICE_ATTR(wtd_enable, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
	WTD_ENABLE);
static SENSOR_DEVICE_ATTR(wtd_clock, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
	WTD_CLOCK);
static SENSOR_DEVICE_ATTR(wtd_counter, S_IRUGO | S_IWUSR, show_wtd,
	set_wtd, WTD_COUNTER);

static struct attribute *cpld_attributes_common[] = {
	&sensor_dev_attr_version_major.dev_attr.attr,
	&sensor_dev_attr_version_minor.dev_attr.attr,
	&sensor_dev_attr_hw_pcb_version.dev_attr.attr,
	&sensor_dev_attr_access.dev_attr.attr,
	NULL
};

static struct attribute *as4500_cpld_m_attributes[] = {
	&sensor_dev_attr_platform_id.dev_attr.attr,
	&sensor_dev_attr_wtd_enable.dev_attr.attr,
	&sensor_dev_attr_wtd_clock.dev_attr.attr,
	&sensor_dev_attr_wtd_counter.dev_attr.attr,
	NULL
};

static struct attribute *as4581_cpld_m_attributes[] = {
	&sensor_dev_attr_platform_id.dev_attr.attr,
	&sensor_dev_attr_wtd_enable.dev_attr.attr,
	&sensor_dev_attr_wtd_clock.dev_attr.attr,
	&sensor_dev_attr_wtd_counter.dev_attr.attr,
	NULL
};

#define CPLD_ATTRS_COMMON() { .attrs = cpld_attributes_common }
#define AS4500_CPLD_M_ATTRS() { .attrs = as4500_cpld_m_attributes }
#define AS4581_CPLD_M_ATTRS() { .attrs = as4581_cpld_m_attributes }

static struct attribute_group as45xx_cpld_s_group[] = {
	CPLD_ATTRS_COMMON()
};

static struct attribute_group as4500_cpld_m_group[] = {
	CPLD_ATTRS_COMMON(),
	AS4500_CPLD_M_ATTRS()
};

static struct attribute_group as4581_cpld_m_group[] = {
	CPLD_ATTRS_COMMON(),
	AS4581_CPLD_M_ATTRS()
};

static const struct attribute_group* cpld_groups[] = {
	as4500_cpld_m_group,
	as4581_cpld_m_group,
	as45xx_cpld_s_group
};

static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as45xx_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 reg = 0, mask = 0;

	switch (attr->index) {
	case WTD_ENABLE:
		reg  = 0x91;
		mask  = 0x7;
		break;
	case WTD_CLOCK:
		reg  = 0x92;
		mask = 0xC0;
		break;
	case WTD_COUNTER:
		reg  = 0x92;
		mask = 0x3F;
		break;
	default:
		return 0;
	}

	mutex_lock(&data->update_lock);
	status = __as45xx_cpld_read(client, reg);
	if (unlikely(status < 0))
		goto exit;
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

#define VALIDATE_WTD_VAL_RETURN(value, mask) \
	do { \
		if (value & ~mask) \
			return -EINVAL; \
	} while (0)

static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as45xx_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	switch (attr->index) {
	case WTD_ENABLE:
		reg   = 0x91;
		mask  = 0x7;
		value = (!!value) | 0x4;
		VALIDATE_WTD_VAL_RETURN(value, mask);
		break;
	case WTD_CLOCK:
		reg  = 0x92;
		mask = 0xC0;
		value <<= 6;
		value &= mask;
		VALIDATE_WTD_VAL_RETURN(value, mask);
		break;
	case WTD_COUNTER:
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

	status = __as45xx_cpld_read(client, reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update wtd status */
	status = (value & mask) | (status & ~mask);
	status = __as45xx_cpld_write(client, reg, status);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int status;
	u32 addr, val;
	struct i2c_client *client = to_i2c_client(dev);
	struct as45xx_cpld_data *data = i2c_get_clientdata(client);

	if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2)
		return -EINVAL;

	if (addr > 0xFF || val > 0xFF)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	status = __as45xx_cpld_write(client, addr, val);
	if (unlikely(status < 0))
		goto exit;
	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static void as45xx_cpld_add_client(struct i2c_client *client)
{
	struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node),
						GFP_KERNEL);

	if (!node) {
		dev_dbg(&client->dev, "Can't allocate cpld_client_node(0x%x)\n",
					client->addr);
		return;
	}

	node->client = client;

	mutex_lock(&list_lock);
	list_add(&node->list, &cpld_client_list);
	mutex_unlock(&list_lock);
}

static void as45xx_cpld_remove_client(struct i2c_client *client)
{
	struct list_head	*list_node = NULL;
	struct cpld_client_node *node = NULL;
	int found = 0;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list) {
		node = list_entry(list_node, struct cpld_client_node, list);

		if (node->client == client) {
			found = 1;
			break;
		}
	}

	if (found) {
		list_del(list_node);
		kfree(node);
	}

	mutex_unlock(&list_lock);
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
				char *buf)
{
	int status = 0, reg = 0, mask = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct as45xx_cpld_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	if (attr->index == PLATFORM_ID) {
		mutex_lock(&data->update_lock);
		status = data->type;
		mutex_unlock(&data->update_lock);
		return sprintf(buf, "%d\n", status);
	}

	switch (attr->index) {
	case CPLD_VERSION_MAJOR:
		reg  = 0x01;
		mask = 0x7F;
		break;
	case CPLD_VERSION_MINOR:
		reg  = (data->type == as45xx_cpld_s) ? 0xFF : 0x2;
		mask = 0xFF;
		break;
	case HW_PCB_VERSION:
		reg  = 0x0;
		mask = 0xFF;
		break;
	default:
		return -ENXIO;
	}

	status = i2c_smbus_read_byte_data(client, reg);
	if (status < 0) {
		dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n",
					client->addr, status);
	}

	while (!(mask & 0x1)) {
		status >>= 1;
		mask >>= 1;
	}

	return sprintf(buf, "%d\n", (status & mask));
}

/*
 * I2C init/probing/exit functions
 */
static int as45xx_cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct as45xx_cpld_data *data;
	int ret = -ENODEV;
	int i = 0;
	int arr_size = 0;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		return -ENODEV;

	data = kzalloc(sizeof(struct as45xx_cpld_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->type = id->driver_data;

	arr_size = (data->type == as45xx_cpld_s) ? ARRAY_SIZE(as45xx_cpld_s_group) :
										ARRAY_SIZE(as4500_cpld_m_group);
	/* Register sysfs hooks */
	for (i = 0; i < arr_size; i++) {
		ret = sysfs_create_group(&client->dev.kobj, &cpld_groups[data->type][i]);
		if (ret)
			goto exit_sysfs_group;
	}

	as45xx_cpld_add_client(client);

	if (data->type == as45xx_cpld_s) {
		data->fan_pdev = platform_device_register_simple("as45xx_fan", -1, NULL, 0);
		if (IS_ERR(data->fan_pdev)) {
			ret = PTR_ERR(data->fan_pdev);
			goto exit_remove_client;
		}

		data->sfp_pdev = platform_device_register_simple("as45xx_sfp", -1, NULL, 0);
		if (IS_ERR(data->sfp_pdev)) {
			ret = PTR_ERR(data->sfp_pdev);
			goto exit_unregister_fan;
		}
	}

 	return 0;

exit_unregister_fan:
	if (data->fan_pdev)
		platform_device_unregister(data->fan_pdev);
exit_remove_client:
	as45xx_cpld_remove_client(client);
exit_sysfs_group:
	for (--i; i >= 0; i--) {
		sysfs_remove_group(&client->dev.kobj, &cpld_groups[data->type][i]);
	}

	kfree(data);
exit:
	return ret;
}

static int as45xx_cpld_remove(struct i2c_client *client)
{
	int i = 0;
	int arr_size = 0;
	struct as45xx_cpld_data *data = i2c_get_clientdata(client);

	if (data->fan_pdev)
		platform_device_unregister(data->fan_pdev);

	if (data->sfp_pdev)
		platform_device_unregister(data->sfp_pdev);

	as45xx_cpld_remove_client(client);

	arr_size = (data->type == as45xx_cpld_s) ? ARRAY_SIZE(as45xx_cpld_s_group) :
										ARRAY_SIZE(as4500_cpld_m_group);
	for (i = 0; i < arr_size; i++) {
		sysfs_remove_group(&client->dev.kobj, &cpld_groups[data->type][i]);
	}

	kfree(data);
	return 0;
}

static int __as45xx_cpld_read(struct i2c_client *client, u8 reg)
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

static int __as45xx_cpld_write(struct i2c_client *client, u8 reg,
					u8 value)
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

int as45xx_cpld_read(unsigned short cpld_addr, u8 reg)
{
	struct list_head   *list_node = NULL;
	struct cpld_client_node *node = NULL;
	int ret = -EPERM;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		node = list_entry(list_node, struct cpld_client_node, list);

		if (node->client->addr == cpld_addr) {
			ret = __as45xx_cpld_read(node->client, reg);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as45xx_cpld_read);

int as45xx_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
	struct list_head   *list_node = NULL;
	struct cpld_client_node *node = NULL;
	int ret = -EIO;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		node = list_entry(list_node, struct cpld_client_node, list);

		if (node->client->addr == cpld_addr) {
			ret = __as45xx_cpld_write(node->client, reg,
							value);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as45xx_cpld_write);

static struct i2c_driver as45xx_cpld_driver = {
	.driver = {
		.name = "as45xx_cpld",
		.owner = THIS_MODULE,
	},
	.probe = as45xx_cpld_probe,
	.remove = as45xx_cpld_remove,
	.id_table = as45xx_cpld_id,
};

static int __init as45xx_cpld_init(void)
{
	mutex_init(&list_lock);
	return i2c_add_driver(&as45xx_cpld_driver);
}

static void __exit as45xx_cpld_exit(void)
{
	i2c_del_driver(&as45xx_cpld_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as45xx_cpld driver");
MODULE_LICENSE("GPL");

module_init(as45xx_cpld_init);
module_exit(as45xx_cpld_exit);
