/*
 * An hwmon driver for accton as4224 Power Module
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

#define DRVNAME "as4224_psu"

#define BOARD_INFO_REG_OFFSET	   0x00
#define PSU_STATUS_I2C_ADDR		 0x40
#define PSU_STATUS_I2C_REG_OFFSET   0x03

#define IS_POWER_GOOD(id, value)	(!!(value & BIT(id + 1)))

static ssize_t show_count(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_status(struct device *dev, struct device_attribute *da,
			char *buf);
extern int as4224_cpld_read(unsigned short cpld_addr, u8 reg);

/* Each client has this additional data
 */
struct as4224_psu_data {
	struct platform_device *pdev;
	struct device *hwmon_dev;
	struct mutex update_lock;
	char valid; /* !=0 if registers are valid */
	unsigned long last_updated; /* In jiffies */
	u8 status; /* power_good status read from CPLD */
	u8 psu_count;
};

struct as4224_psu_data *data = NULL;

enum as4224_psu_sysfs_attributes {
	PSU1_POWER_GOOD,
	PSU2_POWER_GOOD,
	PSU_COUNT
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_count, S_IRUGO, show_count, NULL, PSU_COUNT);

static struct attribute *psu_attributes_common[] = {
	&sensor_dev_attr_psu_count.dev_attr.attr,
	NULL
};

#define PSU_ATTRS_COMMON() { .attrs = psu_attributes_common }

#define PSU_ATTRS(pid) \
	static struct sensor_device_attribute sensor_attr_psu##pid = \
		SENSOR_ATTR(psu##pid##_power_good, S_IRUGO, show_status, \
				NULL, PSU##pid##_POWER_GOOD); \
	static struct attribute *psu_attributes##pid[] = { \
		&sensor_attr_psu##pid.dev_attr.attr, \
		NULL \
	}

PSU_ATTRS(1);
PSU_ATTRS(2);

#define PSU_ATTR_GROUP(pid)  { .attrs = psu_attributes##pid }

static struct attribute_group psu_group[] = {
	PSU_ATTRS_COMMON(),
	PSU_ATTR_GROUP(1),
	PSU_ATTR_GROUP(2),
};

static int as4224_psu_read_value(u8 reg)
{
	return as4224_cpld_read(PSU_STATUS_I2C_ADDR, reg);
}

static struct as4224_psu_data *as4224_psu_update_device(struct device *dev)
{
	if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
			!data->valid) {
		int status;

		dev_dbg(dev, "Starting as4224 update\n");
		data->valid = 0;

		/* Read psu status */
		status = as4224_psu_read_value(PSU_STATUS_I2C_REG_OFFSET);

		if (status < 0) {
			dev_dbg(dev, "cpld reg (0x%x) err %d\n",
					PSU_STATUS_I2C_REG_OFFSET, status);
			goto exit;
		}
		else {
			data->status = status;
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

exit:
	return data;
}

static ssize_t show_count(struct device *dev, struct device_attribute *da,
			char *buf)
{
	return sprintf(buf, "%d\n", data->psu_count);
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 status = 0;

	mutex_lock(&data->update_lock);

	data = as4224_psu_update_device(dev);
	if (!data->valid) {
		mutex_unlock(&data->update_lock);
		return sprintf(buf, "0\n");
	}

	status = IS_POWER_GOOD(attr->index, data->status);

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", status);
}

static int as4224_psu_get_psu_count(void)
{
	int status;

	status = as4224_psu_read_value(BOARD_INFO_REG_OFFSET);
	if (status < 0)
		return 0;

	return (status & 0x10) ? 1 : 2;
}

static int as4224_psu_probe(struct platform_device *pdev)
{
	int status = -1;
	int i = 0;

	/* Register sysfs hooks */
	for (i = 0; i < (data->psu_count + 1); i++) {
		/* Register sysfs hooks */
		status = sysfs_create_group(&pdev->dev.kobj, &psu_group[i]);
		if (status)
			goto exit;
	}

	dev_info(&pdev->dev, "device created\n");

	return 0;

exit:
	for (--i; i >= 0; i--) {
		sysfs_remove_group(&pdev->dev.kobj, &psu_group[i]);
	}
	return status;
}


static int as4224_psu_remove(struct platform_device *pdev)
{
	int i = 0;

	for (i = 0; i < (data->psu_count + 1); i++) {
		sysfs_remove_group(&pdev->dev.kobj, &psu_group[i]);
	}

	return 0;
}

static struct platform_driver as4224_psu_driver = {
	.probe	  = as4224_psu_probe,
	.remove	 = as4224_psu_remove,
	.driver	 = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

static int __init as4224_psu_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as4224_psu_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);

	data->psu_count = as4224_psu_get_psu_count();
	if (!data->psu_count)
		return -EIO;

	ret = platform_driver_register(&as4224_psu_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	return 0;

dev_reg_err:
	platform_driver_unregister(&as4224_psu_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as4224_psu_exit(void)
{
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as4224_psu_driver);
	kfree(data);
}

module_init(as4224_psu_init);
module_exit(as4224_psu_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4224_psu driver");
MODULE_LICENSE("GPL");
