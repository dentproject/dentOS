/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
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
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#define DRVNAME "as4564_thermal"
#define A7K_TEMP_ADDRESS 0xF06F808C

static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
	char *buf);
static int as4564_26p_thermal_probe(struct platform_device *pdev);
static int as4564_26p_thermal_remove(struct platform_device *pdev);

enum temp_data_index {
	TEMP_INPUT
};

struct as4564_26p_thermal_data {
	struct platform_device *pdev;
	struct mutex update_lock;
	unsigned long last_updated;	/* In jiffies */
	u32 __iomem *hw_addr;
	unsigned int reg_value;
};

struct as4564_26p_thermal_data *data = NULL;

static struct platform_driver as4564_26p_thermal_driver = {
	.probe = as4564_26p_thermal_probe,
	.remove = as4564_26p_thermal_remove,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
	},
};

enum as4564_26p_thermal_sysfs_attrs {
	TEMP1_INPUT
};

#define DECLARE_THERMAL_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(temp##index##_input, S_IRUGO, show_temp, \
					NULL, TEMP##index##_INPUT)

#define DECLARE_THERMAL_ATTR(index) \
	&sensor_dev_attr_temp##index##_input.dev_attr.attr

DECLARE_THERMAL_SENSOR_DEVICE_ATTR(1);

static struct attribute *as4564_26p_thermal_attributes[] = {
	DECLARE_THERMAL_ATTR(1),
	NULL
};

static const struct attribute_group as4564_26p_thermal_group = {
	.attrs = as4564_26p_thermal_attributes,
};

static int twos_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static int regval_to_temp(u32 reg_val)
{
	int value = twos_complement_to_int(reg_val, 10, 0x3FF);
	return (value * 423 + 150000);
}

static ssize_t show_temp(struct device *dev, struct device_attribute *da,
							char *buf)
{
	int status = 0;

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ * 5)) {
		data->reg_value = ioread32(data->hw_addr);
		data->last_updated = jiffies;
	}

	/* Get temperature in degree celsius */
	status = regval_to_temp(data->reg_value);

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", status);
}

static int as4564_26p_thermal_probe(struct platform_device *pdev)
{
	int status = -1;

	data->hw_addr = ioremap(A7K_TEMP_ADDRESS, 32);
	if (data->hw_addr == NULL) {
		status = -ENOMEM;
		goto exit;
	}

	/* Register sysfs hooks */
	status = sysfs_create_group(&pdev->dev.kobj, &as4564_26p_thermal_group);
	if (status)
		goto exit_io;

	dev_info(&pdev->dev, "device created\n");

	return 0;

exit_io:
	iounmap(data->hw_addr);
exit:
	return status;
}

static int as4564_26p_thermal_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &as4564_26p_thermal_group);
	iounmap(data->hw_addr);
	return 0;
}

static int __init as4564_26p_thermal_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as4564_26p_thermal_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);

	ret = platform_driver_register(&as4564_26p_thermal_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	return 0;

dev_reg_err:
	platform_driver_unregister(&as4564_26p_thermal_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as4564_26p_thermal_exit(void)
{
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as4564_26p_thermal_driver);
	kfree(data);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4564_thermal driver");
MODULE_LICENSE("GPL");

module_init(as4564_26p_thermal_init);
module_exit(as4564_26p_thermal_exit);
