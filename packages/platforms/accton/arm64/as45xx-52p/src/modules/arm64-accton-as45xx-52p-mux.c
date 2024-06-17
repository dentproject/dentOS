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
#include <linux/delay.h>
#include <dt-bindings/mux/mux.h>

#define DRVNAME "as45xx_mux"

#define I2C_RW_RETRY_COUNT 10
#define I2C_RW_RETRY_INTERVAL 60 /* ms */

#define CPLD_MAX_NCHANS 6
#define CPLD_CHANNEL_SELECT_REG 0x0
#define CPLD_CHANNEL_DESELECT_VAL 0x0

#define PCA9641_DESELECT_CHANNEL_REG 0x1
#define PCA9641_DESELECT_CHANNEL_VAL 0x0

enum mux_type {
	pca9641_mux,
	as45xx_cpldm_mux
};

struct chip_desc {
	u8 nchans;
	u8 select_reg;
	u8 deselect_val;
};

struct as45xx_mux_data {
	enum mux_type type;
	struct mutex update_lock;
	struct i2c_client *client;
};

/* Provide specs for the as45xx CPLD types we know about */
static const struct chip_desc chips[] = {
	[pca9641_mux] = {
		.nchans = 1,
		.select_reg = PCA9641_DESELECT_CHANNEL_REG,
		.deselect_val = PCA9641_DESELECT_CHANNEL_VAL
	},
	[as45xx_cpldm_mux] = {
		.nchans = 6,
		.select_reg = CPLD_CHANNEL_SELECT_REG,
		.deselect_val = CPLD_CHANNEL_DESELECT_VAL
	}
};

static const struct i2c_device_id as45xx_mux_id[] = {
	{ "pca9641_mux", pca9641_mux },
	{ "as45xx_cpldm_mux", as45xx_cpldm_mux },
	{ }
};
MODULE_DEVICE_TABLE(i2c, as45xx_mux_id);

static const struct of_device_id as45xx_mux_of_match[] = {
	{ .compatible = "edgecore,pca9641_mux", .data = &chips[pca9641_mux] },
	{ .compatible = "edgecore,as45xx_cpldm_mux", .data = &chips[as45xx_cpldm_mux] },
	{}
};
MODULE_DEVICE_TABLE(of, as45xx_mux_of_match);

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int as45xx_mux_write(struct i2c_adapter *adap,
				 struct i2c_client *client, u8 reg, u8 val)
{
	union i2c_smbus_data data;

	data.byte = val;
	return __i2c_smbus_xfer(adap, client->addr, client->flags,
				I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, &data);
}

static int as45xx_mux_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
	struct as45xx_mux_data *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	int ret = 0;

	mutex_lock(&data->update_lock);
	switch (data->type) {
	case as45xx_cpldm_mux:
		ret = as45xx_mux_write(muxc->parent, client,
				chips[data->type].select_reg, BIT(chan));
		break;
	case pca9641_mux:
	{
		int i = 0;
		u8 regs[] = { 0x5, 0x3, 0x1, 0x1 };
		u8 vals[] = { 0x7b, 0x0, 0x1, 0x5 };

		for (i = 0; i < ARRAY_SIZE(regs); i++) {
			ret = as45xx_mux_write(muxc->parent, client, regs[i], vals[i]);
			if (unlikely(ret < 0)) {
				ret = -EIO;

				/* Try to close the mux channel */
				as45xx_mux_write(muxc->parent, client,
							chips[data->type].select_reg,
							chips[data->type].deselect_val);
				break;
			}
		}
		break;
	}
	default:
		break;
	}

	mutex_unlock(&data->update_lock);
	return ret;
}

static int as45xx_mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
	struct as45xx_mux_data *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	int ret = 0;

	mutex_lock(&data->update_lock);
	ret = as45xx_mux_write(muxc->parent, client,
			chips[data->type].select_reg, chips[data->type].deselect_val);
	mutex_unlock(&data->update_lock);
	return ret;
}

static void as45xx_mux_cleanup(struct i2c_mux_core *muxc)
{
	i2c_mux_del_adapters(muxc);
}

/*
 * I2C init/probing/exit functions
 */
static int as45xx_mux_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct as45xx_mux_data *data;
	struct i2c_mux_core *muxc;
	int ret = -ENODEV;
	int i = 0;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		return -ENODEV;

	muxc = i2c_mux_alloc(adap, dev, CPLD_MAX_NCHANS, sizeof(*data), 0,
				as45xx_mux_select_chan, as45xx_mux_deselect_mux);
	if (!muxc)
		return -ENOMEM;

	data = i2c_mux_priv(muxc);
	mutex_init(&data->update_lock);
	data->type = id->driver_data;
	data->client = client;
	i2c_set_clientdata(client, muxc);

	/* Now create an adapter for each channel */
	for (i = 0; i < chips[data->type].nchans; i++) {
		ret = i2c_mux_add_adapter(muxc, 0, i, 0);
		if (ret)
			break;
	}

	if (ret)
		as45xx_mux_cleanup(muxc);

	return ret;
}

static int as45xx_mux_remove(struct i2c_client *client)
{
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
	as45xx_mux_cleanup(muxc);
	return 0;
}

static struct i2c_driver as45xx_mux_driver = {
	.driver = {
		.name = "as45xx_mux",
		.owner = THIS_MODULE,
	},
	.probe = as45xx_mux_probe,
	.remove = as45xx_mux_remove,
	.id_table = as45xx_mux_id,
};

module_i2c_driver(as45xx_mux_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@edge-core.com>");
MODULE_DESCRIPTION("as45xx_mux driver");
MODULE_LICENSE("GPL");
