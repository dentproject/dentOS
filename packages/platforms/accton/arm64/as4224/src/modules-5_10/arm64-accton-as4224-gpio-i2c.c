// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/*
 * Copyright (c) 2019-2020 Marvell International Ltd. All rights reserved.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define GPIO_I2C_NGPIOS		144

#define GPIO_I2C_RETRY_COUNT		10
#define GPIO_I2C_RETRY_INTERVAL	60      /* ms */

extern int as4224_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as4224_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

struct gpio_i2c_reg {
	struct list_head head;
	u32 gpio_num;
	u32 reg_addr;
	u32 reg_mask;
};

struct gpio_i2c_chip {
	struct gpio_chip gpio_chip;
	struct device *dev;

	struct list_head gpio_list;
};

static int gpio_i2c_read(struct gpio_i2c_chip *chip, u8 reg)
{
	int status = 0, retry = GPIO_I2C_RETRY_COUNT;

	while (retry) {
		status = as4224_cpld_read(0x40, reg);
		if (unlikely(status < 0)) {
			msleep(GPIO_I2C_RETRY_INTERVAL);
			retry--;
			continue;
		}
		break;
	}

	return status;
}

static int gpio_i2c_write(struct gpio_i2c_chip *chip, u8 reg, u8 value)
{
	int status = 0, retry = GPIO_I2C_RETRY_COUNT;

	while (retry) {
		status = as4224_cpld_write(0x40, reg, value);
		if (unlikely(status < 0)) {
			msleep(GPIO_I2C_RETRY_INTERVAL);
			retry--;
			continue;
		}
		break;
	}

	return status;
}

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
	int val;

	gpio_reg = gpio_i2c_reg_by_num(chip, offs);
	if (!gpio_reg) {
		dev_err(chip->dev, "invalid gpio offset (0x%x)\n", offs);
		return -EINVAL;
	}

	val = gpio_i2c_read(chip, gpio_reg->reg_addr);
	val &= gpio_reg->reg_mask;

	return val;
}

static void gpio_i2c_set_value(struct gpio_chip *gc, unsigned int offs, int set)
{
	struct gpio_i2c_chip *chip = gpiochip_get_data(gc);
	struct gpio_i2c_reg *gpio_reg;
	unsigned int val;

	gpio_reg = gpio_i2c_reg_by_num(chip, offs);
	if (!gpio_reg) {
		dev_err(chip->dev, "invalid gpio offset (0x%x)\n", offs);
		return;
	}

	val = gpio_i2c_read(chip, gpio_reg->reg_addr);

	val &= ~gpio_reg->reg_mask;
	if (set)
		val |= gpio_reg->reg_mask;

	gpio_i2c_write(chip, gpio_reg->reg_addr, val);
}

static int gpio_i2c_reg_parse(struct gpio_i2c_chip *chip,
			struct device_node *np)
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

static int gpio_i2c_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *gpio_map_np;
	struct gpio_i2c_chip *chip;
	int err;

	if (!np)
		return -EINVAL;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	INIT_LIST_HEAD(&chip->gpio_list);
	chip->dev = dev;

	gpio_map_np = of_find_node_by_name(np, "gpio-map");
	if (gpio_map_np) {
		err = gpio_i2c_map_parse(chip, gpio_map_np);
		if (err)
			return err;
	}

	chip->gpio_chip.parent = dev;

	dev_set_drvdata(dev, chip);

	chip->gpio_chip.label = dev_name(dev);
	chip->gpio_chip.get = gpio_i2c_get_value;
	chip->gpio_chip.set = gpio_i2c_set_value;
	chip->gpio_chip.base = -1;

	chip->gpio_chip.ngpio = GPIO_I2C_NGPIOS;

	chip->gpio_chip.owner = THIS_MODULE;
	chip->gpio_chip.can_sleep = true;

	return devm_gpiochip_add_data(dev, &chip->gpio_chip, chip);
}

static int gpio_i2c_remove(struct platform_device *pdev)
{
	return 0;
}


static const struct of_device_id gpio_i2c_id[] = {
	{ .compatible = "gpio-i2c" },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_i2c_id);

static struct platform_driver gpio_i2c_driver = {
	.driver = {
		.name = "gpio-i2c",
		.owner = THIS_MODULE,
		.of_match_table = gpio_i2c_id,
	},
	.probe = gpio_i2c_probe,
	.remove = gpio_i2c_remove,
};

module_platform_driver(gpio_i2c_driver);


MODULE_AUTHOR("Vadym Kochan <vadym.kochan@plvision.eu>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("GPIO to I2C driver");
