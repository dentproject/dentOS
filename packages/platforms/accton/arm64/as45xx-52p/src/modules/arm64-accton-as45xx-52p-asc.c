// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/*
 * A hwmon driver for the Accton as45xx asc
 *
 * Copyright (C) 2024 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com>
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
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define DRVNAME "as45xx_asc"

#define I2C_RW_RETRY_COUNT 10
#define I2C_RW_RETRY_INTERVAL 60

#define ASC_WRITE_MEAS_CTRL_REG 0x51
#define ASC_READ_MEAS_CTRL_REG  0x52

static struct as45xx_asc_data *as45xx_asc_update_device(struct device *dev);
static ssize_t asc_show_value(struct device *dev, struct device_attribute *da,
		char *buf);

/* asc related data, the index should match sysfs_asc_attributes
 */
static const u8 asc_reg[] = {
	0x80,	/* tmon_meas_1_high */
	0x81,	/* tmon_meas_1_low */
	0x82,	/* tmon_meas_2_high */
	0x83,	/* tmon_meas_2_low */
	0x84,	/* tmon_meas_int_high */
	0x85 	/* tmon_meas_int_low1 */
};

enum sysfs_asc_attributes {
	TMON_MEAS_1,   /* tmon_meas_1_high / tmon_meas_1_low */
	TMON_MEAS_2,   /* tmon_meas_2_high / tmon_meas_2_low */
	TMON_MEAS_INT  /* tmon_meas_int_high / tmon_meas_int_low1 */
};

/* asc data */
struct as45xx_asc_data {
	struct i2c_client *client;
	struct device *hwmon_dev;
	struct mutex update_lock;
	char valid; /* != 0 if registers are valid */
	unsigned long last_updated; /* In jiffies */
	u8 reg_val[ARRAY_SIZE(asc_reg)]; /* Register value */
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, asc_show_value, NULL,
			TMON_MEAS_1);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, asc_show_value, NULL,
			TMON_MEAS_2);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, asc_show_value, NULL,
			TMON_MEAS_INT);

static struct attribute *asc_attributes[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	NULL
};

static const struct attribute_group asc_attrgroup = {
	.attrs = asc_attributes,
};

static const struct attribute_group *asc_groups[] = {
	&asc_attrgroup,
	NULL
};

static int as45xx_asc_read_value(struct i2c_client *client, u8 reg)
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

static int as45xx_asc_write_value(struct i2c_client *client, u8 reg, u8 value)
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

/* asc utility functions
 */
static int twos_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data & BIT(valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static int tmon_meas_to_int(u8 tmon_meas_high, u8 tmon_meas_low)
{
	int tmon_meas;
	u16 data = (((u16)tmon_meas_high << 8) | (u16)tmon_meas_low);
	tmon_meas = twos_complement_to_int(data >> 5, 11, 0x7FF);

	return (tmon_meas * 1000) / 4; /* convert to mili-celsius */
}

static ssize_t asc_show_value(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct as45xx_asc_data *data = as45xx_asc_update_device(dev);
	ssize_t ret = -ENODEV;

	mutex_lock(&data->update_lock);

	if (data->valid) {
		switch (attr->index) {
		case TMON_MEAS_1:
		case TMON_MEAS_2:
		case TMON_MEAS_INT:
			ret = sprintf(buf, "%d\n",
				tmon_meas_to_int(data->reg_val[attr->index*2],
								 data->reg_val[attr->index*2 + 1]));
		break;
		default:
			break;
		}
	}

	mutex_unlock(&data->update_lock);
	return ret;
}

static struct as45xx_asc_data *as45xx_asc_update_device(struct device *dev)
{
	struct as45xx_asc_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
		!data->valid) {
		int i;

		dev_dbg(dev, "Starting as45xx_asc update\n");
		data->valid = 0;

		/* Update asc data
		 */
		for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
			int status;

			status = as45xx_asc_write_value(data->client,
								ASC_WRITE_MEAS_CTRL_REG,
								asc_reg[i]);
			if (status < 0) {
				dev_dbg(dev, "reg %d, err %d\n", asc_reg[i], status);
				goto exit;
			}

			status = as45xx_asc_read_value(data->client, ASC_READ_MEAS_CTRL_REG);
			if (status < 0) {
				dev_dbg(dev, "reg %d, err %d\n", asc_reg[i], status);
				goto exit;
			}
			else {
				data->reg_val[i] = status;
			}
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

exit:
	mutex_unlock(&data->update_lock);
	return data;
}

static int as45xx_asc_probe(struct i2c_client *client,
				   const struct i2c_device_id *dev_id)
{
	struct as45xx_asc_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		status = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct as45xx_asc_data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	data->valid = 0;
	data->client = client;
	mutex_init(&data->update_lock);
	dev_info(&client->dev, "chip found\n");

	data->hwmon_dev =
		hwmon_device_register_with_groups(&client->dev, client->name, data,
						asc_groups);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_free;
	}

	dev_info(&client->dev, "%s: asc '%s'\n",
		 dev_name(data->hwmon_dev), client->name);

	return 0;

 exit_free:
	kfree(data);
 exit:
	return status;
}

static int as45xx_asc_remove(struct i2c_client *client)
{
	struct as45xx_asc_data *data = i2c_get_clientdata(client);
	hwmon_device_unregister(data->hwmon_dev);
	kfree(data);
	return 0;
}

/* Addresses to scan */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static const struct i2c_device_id as45xx_asc_id[] = {
	{ DRVNAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, as45xx_asc_id);

static struct i2c_driver as45xx_asc_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		   .name = DRVNAME,
		   },
	.probe = as45xx_asc_probe,
	.remove = as45xx_asc_remove,
	.id_table = as45xx_asc_id,
	.address_list = normal_i2c,
};

module_i2c_driver(as45xx_asc_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as45xx_asc driver");
MODULE_LICENSE("GPL");
