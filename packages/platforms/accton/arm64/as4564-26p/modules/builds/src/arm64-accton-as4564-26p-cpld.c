/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *	 Accton as4564_26p CPLD1/CPLD2/CPLD3
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
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#define DRVNAME "as4564_26p_cpld"

#define I2C_RW_RETRY_COUNT 10
#define I2C_RW_RETRY_INTERVAL 60 /* ms */
#define BOARD_INFO_REG_OFFSET 0x00
#define I2C_WRITE_REQUEST_REG 0xE0
#define I2C_LOCK_BY_7040_VAL 0x10
#define I2C_WRITE_REQUEST_7040_VAL 0x1
#define I2C_WRITE_REQUEST_RETRY_TIMES 3
#define WTD_RESET_GPIO_PIN_MPP3 35

static ssize_t show_module(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_wtd(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_wtd(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t reset_wtd(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static int as4564_26p_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as4564_26p_cpld_write_internal(struct i2c_client *client, u8 reg,
			u8 value);
static ssize_t show_i2c_request(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_i2c_request(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);
static struct mutex list_lock;

struct cpld_client_node {
	struct i2c_client *client;
	struct list_head list;
};

enum cpld_type {
	as4564_26p_cpld1
};

struct as4564_26p_cpld_data {
	enum cpld_type type;
	struct device *hwmon_dev;
	struct mutex update_lock;
};

static const struct i2c_device_id as4564_26p_cpld_id[] = {
	{ "as4564_26p_cpld1", as4564_26p_cpld1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, as4564_26p_cpld_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index) MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index) MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index) MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index) MODULE_TXFAULT_##index

enum as4564_26p_cpld_sysfs_attributes {
	VERSION,
	SUB_VERSION,
	ACCESS,
	I2C_ACCESS_REQUEST_7040,
	WTD_RESET_7040,   /* Trigger GPIO to reset wtd timer */
	WTD_STATE_7040,   /* Register 0x90 bit 1 */
	WTD_ENABLE_7040,  /* Register 0x91 bit 1 */
	WTD_CLOCK_7040,   /* Register 0x92 bit 7:6 */
	WTD_COUNTER_7040, /* Register 0x92 bit 5:0 */
	MODULE_PRESENT_ALL,
	MODULE_RXLOS_ALL,
	/* transceiver attributes */
	TRANSCEIVER_PRESENT_ATTR_ID(25),
	TRANSCEIVER_PRESENT_ATTR_ID(26),
	TRANSCEIVER_TXDISABLE_ATTR_ID(25),
	TRANSCEIVER_TXDISABLE_ATTR_ID(26),
	TRANSCEIVER_RXLOS_ATTR_ID(25),
	TRANSCEIVER_RXLOS_ATTR_ID(26),
	TRANSCEIVER_TXFAULT_ATTR_ID(25),
	TRANSCEIVER_TXFAULT_ATTR_ID(26),

};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, VERSION);
static SENSOR_DEVICE_ATTR(sub_version, S_IRUGO, show_version, NULL,SUB_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
static SENSOR_DEVICE_ATTR(wtd_reset_7040, S_IWUSR, NULL, reset_wtd,
	WTD_RESET_7040);
static SENSOR_DEVICE_ATTR(wtd_state_7040, S_IRUGO, show_wtd, NULL,
	WTD_STATE_7040);
static SENSOR_DEVICE_ATTR(wtd_enable_7040, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
	WTD_ENABLE_7040);
static SENSOR_DEVICE_ATTR(wtd_clock_7040, S_IRUGO | S_IWUSR, show_wtd, set_wtd,
	WTD_CLOCK_7040);
static SENSOR_DEVICE_ATTR(wtd_counter_7040, S_IRUGO | S_IWUSR, show_wtd,
	set_wtd, WTD_COUNTER_7040);
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL,
	MODULE_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(module_rx_los_all, S_IRUGO, show_rxlos_all, NULL,
	MODULE_RXLOS_ALL);
static SENSOR_DEVICE_ATTR(i2c_access_request_7040, S_IRUGO | S_IWUSR,
	show_i2c_request, set_i2c_request, I2C_ACCESS_REQUEST_7040);

static struct attribute *cpld_attributes_common[] = {
	&sensor_dev_attr_version.dev_attr.attr,
	&sensor_dev_attr_sub_version.dev_attr.attr,
	&sensor_dev_attr_access.dev_attr.attr,
	&sensor_dev_attr_i2c_access_request_7040.dev_attr.attr,
	&sensor_dev_attr_wtd_reset_7040.dev_attr.attr,
	&sensor_dev_attr_wtd_state_7040.dev_attr.attr,
	&sensor_dev_attr_wtd_enable_7040.dev_attr.attr,
	&sensor_dev_attr_wtd_clock_7040.dev_attr.attr,
	&sensor_dev_attr_wtd_counter_7040.dev_attr.attr,
	&sensor_dev_attr_module_present_all.dev_attr.attr,
	&sensor_dev_attr_module_rx_los_all.dev_attr.attr,
	NULL
};

#define CPLD_ATTRS_COMMON() { .attrs = cpld_attributes_common }

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

MODULE_ATTRS(25);
MODULE_ATTRS(26);

#define MODULE_ATTR_GROUP(index) { .attrs = module_attributes##index }

static struct attribute_group cpld_group[] = {
	CPLD_ATTRS_COMMON(),
	MODULE_ATTR_GROUP(25),
	MODULE_ATTR_GROUP(26),
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	u8 value;
	struct i2c_client *client = to_i2c_client(dev);
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	status = as4564_26p_cpld_read_internal(client, 0x41);
	if (status < 0)
		goto exit;

	value = ~(u8)(status >> 2);

	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x\n", value & 0x3);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	u8 value;
	struct i2c_client *client = to_i2c_client(dev);
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	/* Enable the interrupt to CPU */
	status = as4564_26p_cpld_write_internal(client, 0x36, 0);
	if (unlikely(status < 0))
		goto exit;

	status = as4564_26p_cpld_read_internal(client, 0x40);
	if (status < 0)
		goto exit;

	status >>= 2;
	value = (u8)status;

	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x\n", value & 0x3);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_module(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 reg = 0, mask = 0, invert = 0;

	switch (attr->index) {
	case MODULE_PRESENT_25 ... MODULE_PRESENT_26:
		invert = 1;
		reg  = 0x41;
		mask = 0x1 << (attr->index - MODULE_PRESENT_25 + 2);
		break;
	case MODULE_RXLOS_25 ... MODULE_RXLOS_26:
		reg  = 0x40;
		mask = 0x1 << (attr->index - MODULE_RXLOS_25 + 2);
		break;
	case MODULE_TXFAULT_25 ... MODULE_TXFAULT_26:
		reg  = 0x40;
		mask = 0x10 << (attr->index - MODULE_TXFAULT_25 + 6);
		break;
	case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_26:
		reg  = 0x42;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_25 + 2);
		break;
	default:
		return 0;
	}

	mutex_lock(&data->update_lock);
	if ((attr->index >= MODULE_TXFAULT_25 &&
		attr->index <= MODULE_TXFAULT_26) ||
		(attr->index >= MODULE_TXDISABLE_25 &&
		attr->index <= MODULE_TXDISABLE_26)) {
		status = as4564_26p_cpld_read_internal(client, 0x36);
		if (unlikely(status < 0))
			goto exit;

		/* Enable the interrupt to CPU */
		status = as4564_26p_cpld_write_internal(client, 0x36,
						status & (~mask));
		if (unlikely(status < 0))
			goto exit;
	}

	status = as4564_26p_cpld_read_internal(client, reg);
	if (unlikely(status < 0))
		goto exit;
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", invert ? !(status & mask) :
					!!(status & mask));

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
		status = i2c_smbus_read_byte_data(client,
						I2C_WRITE_REQUEST_REG);
		if (unlikely(status < 0))
			continue;

		if (status & I2C_LOCK_BY_7040_VAL)
			return 0; /* I2C already lock by 7040, just return */

		status |= I2C_WRITE_REQUEST_7040_VAL;
		status = i2c_smbus_write_byte_data(client,
						I2C_WRITE_REQUEST_REG, status);
		if (unlikely(status < 0))
			continue;

		/* Read out to make sure if 7040 get the access right */
		msleep(50);
		status = i2c_smbus_read_byte_data(client,
						I2C_WRITE_REQUEST_REG);
		if (unlikely(status < 0))
			continue;

		if (status & I2C_LOCK_BY_7040_VAL)
			return 0;

		status = -EBUSY;

		if (retry != I2C_WRITE_REQUEST_RETRY_TIMES)
			msleep(1000);
	}

	return status;
}

static int i2c_write_request_end(struct i2c_client *client)
{
	int status = 0;

	status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
	if (unlikely(status < 0))
		return status;

	status &= ~I2C_WRITE_REQUEST_7040_VAL;
	status = i2c_smbus_write_byte_data(client, I2C_WRITE_REQUEST_REG,
					status);
	if (unlikely(status < 0))
		return status;

	return 0;
}

static ssize_t show_i2c_request(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int status = 0;

	status = i2c_smbus_read_byte_data(client, I2C_WRITE_REQUEST_REG);
	if (unlikely(status < 0))
		return status;

	return sprintf(buf, "%d\n", !!(status & I2C_LOCK_BY_7040_VAL));
}

static ssize_t set_i2c_request(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	long value;
	int status = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	if (value)
		status = i2c_write_request_begin(client);
	else
		status = i2c_write_request_end(client);

	return status ? status : count;
}

static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	switch (attr->index) {
	case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_26:
		reg  = 0x42;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_25 + 2);
		break;
	default:
		return 0;
	}

	/* Read current status */
	mutex_lock(&data->update_lock);
	status = as4564_26p_cpld_read_internal(client, reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update tx_disable status */
	if (value)
		value = (status | mask);
	else
		value = (status & ~mask);

	/* Set value to CPLD */
	status = as4564_26p_cpld_write_internal(client, reg, value);
	if (unlikely(status < 0))
		goto exit;

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
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
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
	status = as4564_26p_cpld_read_internal(client, reg);
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
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

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

	status = as4564_26p_cpld_read_internal(client, reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update wtd status */
	status = (value & mask) | (status & ~mask);
	status = as4564_26p_cpld_write_internal(client, reg, status);
	if (unlikely(status < 0))
		goto exit;

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
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	if (!value)
		return count;

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
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);

	if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2)
		return -EINVAL;

	if (addr > 0xFF || val > 0xFF)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	status = as4564_26p_cpld_write_internal(client, addr, val);
	if (unlikely(status < 0))
		goto exit;
	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static void as4564_26p_cpld_add_client(struct i2c_client *client)
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

static void as4564_26p_cpld_remove_client(struct i2c_client *client)
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
	int val = 0, reg = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	if (attr->index == VERSION)
		reg = 0x1;
	else /* SUB_VERSION */
		reg = 0xFF;

	val = i2c_smbus_read_byte_data(client, reg);
	if (val < 0) {
		dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n",
					client->addr, val);
	}

	return sprintf(buf, "%d\n", val);
}

/*
 * I2C init/probing/exit functions
 */
static int as4564_26p_cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct as4564_26p_cpld_data *data;
	int ret = -ENODEV;
	int i = 0;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		goto exit;

	data = kzalloc(sizeof(struct as4564_26p_cpld_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->type = id->driver_data;

	/* Register sysfs hooks */
	for (i = 0; i < ARRAY_SIZE(cpld_group); i++) {
		ret = sysfs_create_group(&client->dev.kobj, &cpld_group[i]);
		if (ret)
			goto exit_free;
	}

	ret = gpio_request(WTD_RESET_GPIO_PIN_MPP3, "wtd_reset_7040");
	if (ret) {
		dev_err(&client->dev, "Failed to request MPP3 gpio\n");
		goto exit_free;
	}

	as4564_26p_cpld_add_client(client);
	return 0;

exit_free:
	kfree(data);
exit:
	for (--i; i >= 0; i--) {
		sysfs_remove_group(&client->dev.kobj, &cpld_group[i]);
	}
	return ret;
}

static int as4564_26p_cpld_remove(struct i2c_client *client)
{
	int i = 0;
	struct as4564_26p_cpld_data *data = i2c_get_clientdata(client);
	as4564_26p_cpld_remove_client(client);
	gpio_free(WTD_RESET_GPIO_PIN_MPP3);

	for (i = 0; i < ARRAY_SIZE(cpld_group); i++) {
		sysfs_remove_group(&client->dev.kobj, &cpld_group[i]);
	}

	kfree(data);
	return 0;
}

static int as4564_26p_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as4564_26p_cpld_write_internal(struct i2c_client *client, u8 reg,
					u8 value)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	status = i2c_write_request_begin(client);
	if (unlikely(status < 0))
		return status;

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
	if (unlikely(status < 0))
		return status;

	return status;
}

int as4564_26p_cpld_read(unsigned short cpld_addr, u8 reg)
{
	struct list_head   *list_node = NULL;
	struct cpld_client_node *node = NULL;
	int ret = -EPERM;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		node = list_entry(list_node, struct cpld_client_node, list);

		if (node->client->addr == cpld_addr) {
			ret = as4564_26p_cpld_read_internal(node->client, reg);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as4564_26p_cpld_read);

int as4564_26p_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
	struct list_head   *list_node = NULL;
	struct cpld_client_node *node = NULL;
	int ret = -EIO;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		node = list_entry(list_node, struct cpld_client_node, list);

		if (node->client->addr == cpld_addr) {
			ret = as4564_26p_cpld_write_internal(node->client, reg,
							value);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as4564_26p_cpld_write);

static struct i2c_driver as4564_26p_cpld_driver = {
	.driver = {
		.name = "as4564_26p_cpld",
		.owner = THIS_MODULE,
	},
	.probe = as4564_26p_cpld_probe,
	.remove = as4564_26p_cpld_remove,
	.id_table = as4564_26p_cpld_id,
};

static int __init as4564_26p_cpld_init(void)
{
	mutex_init(&list_lock);
	return i2c_add_driver(&as4564_26p_cpld_driver);
}

static void __exit as4564_26p_cpld_exit(void)
{
	i2c_del_driver(&as4564_26p_cpld_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4564_26p_cpld driver");
MODULE_LICENSE("GPL");

module_init(as4564_26p_cpld_init);
module_exit(as4564_26p_cpld_exit);
