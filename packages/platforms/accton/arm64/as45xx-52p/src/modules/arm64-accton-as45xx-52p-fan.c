/*
 * A hwmon driver for the Accton as45xx_fan
 *
 * Copyright (C) 2016 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
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
#include <linux/dmi.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define DRVNAME "as45xx_fan"

#define FAN_STATUS_I2C_ADDR 0x62
#define MAX_FAN_SPEED_RPM 17600
#define FAN_DUTY_CYCLE_REG_MASK 0xFF
#define FAN_MAX_DUTY_CYCLE 100
#define FAN_REG_VAL_TO_SPEED_RPM_STEP 100

static struct as45xx_fan_data *as45xx_fan_update_device(struct device *dev);
static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
		char *buf);
static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
		const char *buf, size_t count);
static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
		char *buf);
static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
		const char *buf, size_t count);
static int _reset_wtd(void);
static ssize_t reset_wtd(struct device *dev, struct device_attribute *da,
		const char *buf, size_t count);
extern int as45xx_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as45xx_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

/* fan related data, the index should match sysfs_fan_attributes
 */
static const u8 fan_reg[] = {
	0x70,	   /* fan1 PWM */
	0x71,	   /* fan2 PWM */
	0x72,	   /* fan3 PWM */
	0x73,	   /* fan4 PWM */
	0x80,	   /* fan 1 tach speed */
	0x81,	   /* fan 2 tach speed */
	0x82,	   /* fan 3 tach speed */
	0x83,	   /* fan 4 tach speed */
	0x62,	  /* fan tech speed setting */
};

/* fan data */
struct as45xx_fan_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	char valid; /* != 0 if registers are valid */
	unsigned long last_updated; /* In jiffies */
	u8 reg_val[ARRAY_SIZE(fan_reg)]; /* Register value */
};

enum fan_id {
	FAN1_ID,
	FAN2_ID,
	FAN3_ID,
	FAN4_ID
};

enum sysfs_fan_attributes {
	FAN1_PWM,
	FAN2_PWM,
	FAN3_PWM,
	FAN4_PWM,
	FAN1_SPEED_RPM,
	FAN2_SPEED_RPM,
	FAN3_SPEED_RPM,
	FAN4_SPEED_RPM,
	FAN_TECH_SETTING,
	FAN1_FAULT,
	FAN2_FAULT,
	FAN3_FAULT,
	FAN4_FAULT,
	FAN_MAX_RPM,
	FAN1_SPEED_MAX,
	FAN2_SPEED_MAX,
	FAN3_SPEED_MAX,
	FAN4_SPEED_MAX,
	FAN1_SPEED_MIN,
	FAN2_SPEED_MIN,
	FAN3_SPEED_MIN,
	FAN4_SPEED_MIN,
	WTD_CLOCK,
	WTD_COUNTER,
	WTD_ENABLE,
	WTD_RESET
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(fan_max_rpm, S_IRUGO, fan_show_value, NULL,
			FAN_MAX_RPM);

/* Fan watchdog */
static SENSOR_DEVICE_ATTR(wtd_clock, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
			WTD_CLOCK);
static SENSOR_DEVICE_ATTR(wtd_counter, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
			WTD_COUNTER);
static SENSOR_DEVICE_ATTR(wtd_enable, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
			WTD_ENABLE);
static SENSOR_DEVICE_ATTR(wtd_reset, S_IWUSR, NULL, reset_wtd, WTD_RESET);

static struct attribute *fan_attributes_common[] = {
	&sensor_dev_attr_fan_max_rpm.dev_attr.attr,
	&sensor_dev_attr_wtd_clock.dev_attr.attr,
	&sensor_dev_attr_wtd_counter.dev_attr.attr,
	&sensor_dev_attr_wtd_enable.dev_attr.attr,
	&sensor_dev_attr_wtd_reset.dev_attr.attr,
	NULL
};

#define FAN_ATTRS_COMMON() { .attrs = fan_attributes_common }

#define FAN_ATTRS(fid) \
	static SENSOR_DEVICE_ATTR(fan##fid##_pwm, \
		S_IWUSR | S_IRUGO, fan_show_value, set_duty_cycle, \
		FAN##fid##_PWM); \
	static SENSOR_DEVICE_ATTR(fan##fid##_input, S_IRUGO, fan_show_value, \
		NULL, FAN##fid##_SPEED_RPM); \
	static SENSOR_DEVICE_ATTR(fan##fid##_fault, S_IRUGO, fan_show_value, \
		NULL, FAN##fid##_FAULT); \
	static SENSOR_DEVICE_ATTR(fan##fid##_max, S_IRUGO, fan_show_value, \
		NULL, FAN##fid##_SPEED_MAX); \
	static SENSOR_DEVICE_ATTR(fan##fid##_min, S_IRUGO, fan_show_value, \
		NULL, FAN##fid##_SPEED_MIN); \
	static struct attribute *fan_attributes##fid[] = { \
		&sensor_dev_attr_fan##fid##_pwm.dev_attr.attr, \
		&sensor_dev_attr_fan##fid##_input.dev_attr.attr, \
		&sensor_dev_attr_fan##fid##_fault.dev_attr.attr, \
		&sensor_dev_attr_fan##fid##_max.dev_attr.attr, \
		&sensor_dev_attr_fan##fid##_min.dev_attr.attr, \
	NULL \
}

FAN_ATTRS(1);
FAN_ATTRS(2);
FAN_ATTRS(3);
FAN_ATTRS(4);

#define FAN_ATTR_GROUP(fid)  { .attrs = fan_attributes##fid }

static struct attribute_group fan_group[] = {
	FAN_ATTRS_COMMON(),
	FAN_ATTR_GROUP(1),
	FAN_ATTR_GROUP(2),
	FAN_ATTR_GROUP(3),
	FAN_ATTR_GROUP(4)
};

static int as45xx_fan_read_value(u8 reg)
{
	return as45xx_cpld_read(FAN_STATUS_I2C_ADDR, reg);
}

static int as45xx_fan_write_value(u8 reg, u8 value)
{
	return as45xx_cpld_write(FAN_STATUS_I2C_ADDR, reg, value);
}

/* fan utility functions
 */
static u32 reg_val_to_duty_cycle(u8 reg_val)
{
	reg_val &= FAN_DUTY_CYCLE_REG_MASK;
	return ((((u32)reg_val * 10000 / 255) + 50) / 100);
}

static u8 duty_cycle_to_reg_val(u8 duty_cycle)
{
	return (duty_cycle * 255 / 100);
}

static u32 reg_val_to_speed_rpm(u8 reg_val, u8 tech_reg_val)
{
	u32 timer[] = { 1048, 2097, 4194, 8389 };
	u8 counter = (tech_reg_val & 0x3F);
	u8 clock   = (tech_reg_val >> 6) & 0x3;

	return (reg_val * 3000000) / (timer[clock] * counter);
}

static u8 is_fan_fault(struct as45xx_fan_data *data, enum fan_id id)
{
	return !reg_val_to_speed_rpm(data->reg_val[FAN1_SPEED_RPM + id],
					data->reg_val[FAN_TECH_SETTING]);
}

static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int error, value;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct as45xx_fan_data *data = dev_get_drvdata(dev);

	error = kstrtoint(buf, 10, &value);
	if (error)
		return error;

	if (value < 10)
		value = 10;
	else if (value > 100)
		value = 100;

	/* Disable the watchdog timer
	 */
	error = _reset_wtd();
	if (unlikely(error < 0)) {
		dev_dbg(dev, "Unable to reset the watchdog timer\n");
		return error;
	}

	as45xx_fan_write_value(fan_reg[attr->index - FAN1_PWM],
				duty_cycle_to_reg_val(value));
	data->valid = 0;

	return count;
}

static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct as45xx_fan_data *data = as45xx_fan_update_device(dev);
	ssize_t ret = 0;

	if (data->valid) {
		switch (attr->index) {
		case FAN1_PWM ... FAN4_PWM:
			ret = sprintf(buf, "%u\n",
			reg_val_to_duty_cycle(data->reg_val[attr->index]));
			break;
		case FAN1_SPEED_RPM ... FAN4_SPEED_RPM:
			ret = sprintf(buf, "%u\n",
				reg_val_to_speed_rpm(data->reg_val[attr->index],
					data->reg_val[FAN_TECH_SETTING]));
			break;
		case FAN1_FAULT ... FAN4_FAULT:
			ret = sprintf(buf, "%d\n",
				is_fan_fault(data, attr->index - FAN1_FAULT));
			break;
		case FAN_MAX_RPM:
		case FAN1_SPEED_MAX ... FAN4_SPEED_MAX:
			ret = sprintf(buf, "%d\n", MAX_FAN_SPEED_RPM);
			break;
		case FAN1_SPEED_MIN ... FAN4_SPEED_MIN:
			ret = sprintf(buf, "0\n");
			break;
		default:
			break;
		}
	}

	return ret;
}

static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct as45xx_fan_data *data = dev_get_drvdata(dev);
	int status = 0;
	u8 reg = 0, mask = 0;

	switch (attr->index) {
	case WTD_ENABLE:
		reg  = 0x60;
		mask = 0x01;
		break;
	case WTD_CLOCK:
		reg  = 0x61;
		mask = 0xC0;
		break;
	case WTD_COUNTER:
		reg  = 0x61;
		mask = 0x3F;
		break;
	default:
		return 0;
	}

	mutex_lock(&data->update_lock);
	status = as45xx_fan_read_value(reg);
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
	struct as45xx_fan_data *data = dev_get_drvdata(dev);
	long value;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	switch (attr->index) {
	case WTD_ENABLE:
		reg  = 0x60;
		mask = 0x01;
		value &= mask;
		VALIDATE_WTD_VAL_RETURN(value, mask);
		break;
	case WTD_CLOCK:
		reg  = 0x61;
		mask = 0xC0;
		value <<= 6;
		VALIDATE_WTD_VAL_RETURN(value, mask);
		break;
	case WTD_COUNTER:
		reg  = 0x61;
		mask = 0x3F;
		value &= mask;
		VALIDATE_WTD_VAL_RETURN(value, mask);
		break;
	default:
		return 0;
	}

	/* Read current status */
	mutex_lock(&data->update_lock);

	status = as45xx_fan_read_value(reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update wtd status */
	status = (value & mask) | (status & ~mask);
	status = as45xx_fan_write_value(reg, status);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static int _reset_wtd(void)
{
	int status;

	/* Set value as 0->1 to reset wtd */
	status = as45xx_fan_write_value(0x60, 0);
	if (unlikely(status < 0))
		return status;

	msleep(50);
	status = as45xx_fan_write_value(0x60, 1);
	if (unlikely(status < 0))
		return status;

	return status;
}

static ssize_t reset_wtd(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	long value;
	int status;
	struct as45xx_fan_data *data = dev_get_drvdata(dev);

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	if (!value)
		return count;

	/* Read current status */
	mutex_lock(&data->update_lock);

	status = _reset_wtd();
	if (unlikely(status < 0)) {
		dev_dbg(dev, "Unable to reset the watchdog timer\n");
		return status;
	}

	mutex_unlock(&data->update_lock);
	return count;
}

static struct as45xx_fan_data *as45xx_fan_update_device(struct device *dev)
{
	struct as45xx_fan_data *data = dev_get_drvdata(dev);
	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
		!data->valid) {
		int i;

		dev_dbg(dev, "Starting as45xx_fan update\n");
		data->valid = 0;

		/* Update fan data
		 */
		for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
			int status = as45xx_fan_read_value(fan_reg[i]);

			if (status < 0) {
				data->valid = 0;
				mutex_unlock(&data->update_lock);
				dev_dbg(dev, "reg %d, err %d\n",
						fan_reg[i], status);
				return data;
			}
			else {
				data->reg_val[i] = status;
			}
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}

static int as45xx_fan_probe(struct platform_device *pdev)
{
	int status = -1;
	int i = 0;
	struct as45xx_fan_data *data;

	data = kzalloc(sizeof(struct as45xx_fan_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	mutex_init(&data->update_lock);
	platform_set_drvdata(pdev, data);

	data->hwmon_dev = hwmon_device_register_with_groups(&pdev->dev,
											DRVNAME, data, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_free;
	}

	/* Register sysfs hooks */
	for (i = 0; i < ARRAY_SIZE(fan_group); i++) {
		/* Register sysfs hooks */
		status = sysfs_create_group(&data->hwmon_dev->kobj, &fan_group[i]);
		if (status) {
			goto exit_sysfs_group;
		}
	}

	dev_info(&pdev->dev, "device created\n");
	return 0;

exit_sysfs_group:
	for (--i; i >= 0; i--) {
		sysfs_remove_group(&data->hwmon_dev->kobj, &fan_group[i]);
	}

	hwmon_device_unregister(data->hwmon_dev);
exit_free:
	kfree(data);
	return status;
}

static int as45xx_fan_remove(struct platform_device *pdev)
{
	int i = 0;
	struct as45xx_fan_data *data = platform_get_drvdata(pdev);

	for (i = 0; i < ARRAY_SIZE(fan_group); i++) {
		sysfs_remove_group(&data->hwmon_dev->kobj, &fan_group[i]);
	}

	hwmon_device_unregister(data->hwmon_dev);
	kfree(data);
	return 0;
}

static const struct platform_device_id as45xx_fan_id[] = {
	{ DRVNAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(platform, as45xx_fan_id);

static struct platform_driver as45xx_fan_driver = {
	.probe	  = as45xx_fan_probe,
	.remove	 = as45xx_fan_remove,
	.id_table	= as45xx_fan_id,
	.driver	 = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

module_platform_driver(as45xx_fan_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as45xx_fan driver");
MODULE_LICENSE("GPL");
