/*
 * An hwmon driver for accton as45xx Power Module
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>

#define DRVNAME "as45xx_sfp"

#define SFP_STATUS_I2C_ADDR 0x62

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_module(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);

extern int as45xx_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as45xx_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

/* Platform specific data
 */
struct as45xx_sfp_data {
	struct mutex update_lock;
};

#define TRANSCEIVER_PRESENT_ATTR_ID(index) MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index) MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index) MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index) MODULE_TXFAULT_##index

enum as45xx_sfp_sysfs_attributes {
	MODULE_PRESENT_ALL,
	MODULE_RXLOS_ALL,
	/* transceiver attributes */
	TRANSCEIVER_PRESENT_ATTR_ID(49),
	TRANSCEIVER_PRESENT_ATTR_ID(50),
	TRANSCEIVER_PRESENT_ATTR_ID(51),
	TRANSCEIVER_PRESENT_ATTR_ID(52),
	TRANSCEIVER_TXDISABLE_ATTR_ID(49),
	TRANSCEIVER_TXDISABLE_ATTR_ID(50),
	TRANSCEIVER_TXDISABLE_ATTR_ID(51),
	TRANSCEIVER_TXDISABLE_ATTR_ID(52),
	TRANSCEIVER_RXLOS_ATTR_ID(49),
	TRANSCEIVER_RXLOS_ATTR_ID(50),
	TRANSCEIVER_RXLOS_ATTR_ID(51),
	TRANSCEIVER_RXLOS_ATTR_ID(52),
	TRANSCEIVER_TXFAULT_ATTR_ID(49),
	TRANSCEIVER_TXFAULT_ATTR_ID(50),
	TRANSCEIVER_TXFAULT_ATTR_ID(51),
	TRANSCEIVER_TXFAULT_ATTR_ID(52)
};

/* sysfs attributes for hwmon
 */
#define MODULE_ATTRS_COMMON() \
	static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, \
		show_present_all, NULL, MODULE_PRESENT_ALL); \
	static SENSOR_DEVICE_ATTR(module_rx_los_all, S_IRUGO, \
		show_rxlos_all, NULL, MODULE_RXLOS_ALL); \
	static struct attribute *module_attributes_common[] = { \
		&sensor_dev_attr_module_present_all.dev_attr.attr, \
		&sensor_dev_attr_module_rx_los_all.dev_attr.attr, \
		NULL \
	}

#define MODULE_ATTRS(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, \
		show_module, NULL, MODULE_PRESENT_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR,\
		show_module, set_control, MODULE_TXDISABLE_##index); \
	static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, \
		show_module, NULL, MODULE_RXLOS_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, \
		show_module, NULL, MODULE_TXFAULT_##index); \
	static struct attribute *module_attributes##index[] = { \
		&sensor_dev_attr_module_present_##index.dev_attr.attr, \
		&sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
		&sensor_dev_attr_module_rx_los_##index.dev_attr.attr, \
		&sensor_dev_attr_module_tx_fault_##index.dev_attr.attr, \
		NULL \
	}

MODULE_ATTRS_COMMON();
MODULE_ATTRS(49);
MODULE_ATTRS(50);
MODULE_ATTRS(51);
MODULE_ATTRS(52);

#define MODULE_ATTR_GROUP_COMMON() { .attrs = module_attributes_common }
#define MODULE_ATTR_GROUP(index) { .attrs = module_attributes##index }

static struct attribute_group sfp_group[] = {
	MODULE_ATTR_GROUP_COMMON(),
	MODULE_ATTR_GROUP(49),
	MODULE_ATTR_GROUP(50),
	MODULE_ATTR_GROUP(51),
	MODULE_ATTR_GROUP(52)
};

static int as45xx_sfp_read_value(u8 reg)
{
	return as45xx_cpld_read(SFP_STATUS_I2C_ADDR, reg);
}

static int as45xx_sfp_write_value(u8 reg, u8 value)
{
	return as45xx_cpld_write(SFP_STATUS_I2C_ADDR, reg, value);
}

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	u8 value;
	struct as45xx_sfp_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->update_lock);
	status = as45xx_sfp_read_value(0x41);
	if (status < 0)
		goto exit;

	value = ~(u8)status;
	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x\n", value & 0xF);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	u8 value;
	struct as45xx_sfp_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->update_lock);
	status = as45xx_sfp_read_value(0x40);
	if (status < 0)
		goto exit;

	value = (u8)status;
	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x\n", value & 0xF);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_module(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct as45xx_sfp_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int status = 0;
	u8 reg = 0, mask = 0, invert = 0;

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
		mask = 0x10 << (attr->index - MODULE_TXFAULT_49);
		break;
	case MODULE_TXDISABLE_49 ... MODULE_TXDISABLE_52:
		reg  = 0x42;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_49);
		break;
	default:
		return 0;
	}

	mutex_lock(&data->update_lock);
	status = as45xx_sfp_read_value(reg);
	if (unlikely(status < 0))
		goto exit;
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", invert ? !(status & mask) : !!(status & mask));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct as45xx_sfp_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	long value;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

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
	status = as45xx_sfp_read_value(reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update tx_disable status */
	if (value)
		value = (status | mask);
	else
		value = (status & ~mask);

	/* Set value to CPLD */
	status = as45xx_sfp_write_value(reg, value);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static int as45xx_sfp_probe(struct platform_device *pdev)
{
	int status = -1;
	int i = 0;
	struct as45xx_sfp_data *data;

	data = kzalloc(sizeof(struct as45xx_sfp_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	mutex_init(&data->update_lock);
	platform_set_drvdata(pdev, data);

	/* Register sysfs hooks */
	for (i = 0; i < ARRAY_SIZE(sfp_group); i++) {
		/* Register sysfs hooks */
		status = sysfs_create_group(&pdev->dev.kobj, &sfp_group[i]);
		if (status)
			goto exit_sysfs_group;
	}

	dev_info(&pdev->dev, "device created\n");
	return 0;

exit_sysfs_group:
	for (--i; i >= 0; i--) {
		sysfs_remove_group(&pdev->dev.kobj, &sfp_group[i]);
	}

	kfree(data);
	return status;
}


static int as45xx_sfp_remove(struct platform_device *pdev)
{
	struct as45xx_sfp_data *data = platform_get_drvdata(pdev);
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(sfp_group); i++) {
		sysfs_remove_group(&pdev->dev.kobj, &sfp_group[i]);
	}

	kfree(data);
	return 0;
}

static const struct platform_device_id as45xx_sfp_id[] = {
	{ DRVNAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(platform, as45xx_sfp_id);

static struct platform_driver as45xx_sfp_driver = {
	.probe	  = as45xx_sfp_probe,
	.remove	 = as45xx_sfp_remove,
	.id_table	= as45xx_sfp_id,
	.driver	 = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

module_platform_driver(as45xx_sfp_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as45xx_sfp driver");
MODULE_LICENSE("GPL");
