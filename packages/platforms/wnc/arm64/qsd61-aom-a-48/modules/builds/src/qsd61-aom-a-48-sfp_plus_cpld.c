#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "qsd61-aom-a-48-sfp_plus_cpld_reg.h"
#include "qsd61-aom-a-48-sfp_plus_cpld_sysfs.h"

static int debug_flag = 0;

static LIST_HEAD(cpld_client_list);
static struct mutex list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct system_cpld_data {
    struct mutex lock;
    struct i2c_client *client;
    struct device_attribute bin;
};
struct system_cpld_data *system_cpld;

static const struct i2c_device_id system_cpld_ids[] = {
    { "qsd61_48_sfp_cpld2", 0 },
    { /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, system_cpld_ids);

static int system_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name)
{
    unsigned int reg_val = 0, fld_val;
    static int debug_flag;
    struct system_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int err;

    if(reg_width != 8)
    {
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }

    mutex_lock(&data->lock);

    if((err = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0)
    {
        /* CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n", reg_name, err);
        return err;
    }
    reg_val = err;

    if(debug_flag)
    {
        printk("%s: reg_offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }

    mutex_unlock(&data->lock);

    if(fld_width == reg_width)
    {
        fld_val = reg_val & fld_mask;
    }
    else
    {
        fld_val = (reg_val >> fld_shift) & fld_mask;
    }

    return sprintf(buf, "0x%x\n", fld_val);
}

static int system_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name)
{
    int ret_code;
    unsigned int reg_val, fld_val;
    unsigned long val;
    static int debug_flag;
    struct system_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;

    if(reg_width != 8)
    {
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }

    /* Parse buf and store to fld_val */
    if((ret_code = kstrtoul(buf, 16, &val)))
    {
        printk("%s: Conversion value = %s failed, errno = %d.\n", reg_name, buf, ret_code);
        return ret_code;
    }
    fld_val = (unsigned int)val;

    mutex_lock(&data->lock);

    if((ret_code = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0)
    {
        /* Handle CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    reg_val = ret_code;

    if(debug_flag)
    {
        printk("%s: offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }

    if(fld_width == reg_width)
    {
        reg_val = fld_val & fld_mask;
    }
    else
    {
        reg_val = (reg_val & ~(fld_mask << fld_shift)) |
                    ((fld_val & (fld_mask)) << fld_shift);
    }

    if((ret_code = i2c_smbus_write_byte_data(client, (u8)reg_offset, (u8)reg_val)) != 0)
    {
        /* Handle CPLD write error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c write failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    else if(debug_flag)
    {
        printk("%s: offset = %d, width = %d, new value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }

    mutex_unlock(&data->lock);

    return count;
}

/*-------------------- Special file for debug ---------------------- */
static ssize_t system_cpld_debug_read(struct device *dev, struct device_attribute *attr,
             char *buf)
{
    return sprintf(buf, "%d\n", debug_flag);
}

static ssize_t system_cpld_debug_write(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    int temp;
    int error;

    error = kstrtoint(buf, 10, &temp);
    if(error)
    {
        printk(KERN_INFO "%s: Conversion value = %s failed.\n", __FUNCTION__, buf);
        return count;
    }
    debug_flag = temp;

    if(debug_flag)
        printk("%s, debug_flag = %d\n", __FUNCTION__, debug_flag);

    return count;
}
SYSFS_MISC_RW_ATTR_DEF(debug, system_cpld_debug_read, system_cpld_debug_write)

/* ---------------- Define misc group ---------------------------- */
static struct attribute *misc_attributes[] = {

	SYSFS_ATTR_PTR(cpld2_irq                  ),
	SYSFS_ATTR_PTR(cpld2_int_n                ),
	SYSFS_ATTR_PTR(p08_mod_abs                ),
	SYSFS_ATTR_PTR(p07_mod_abs                ),
	SYSFS_ATTR_PTR(p06_mod_abs                ),
	SYSFS_ATTR_PTR(p05_mod_abs                ),
	SYSFS_ATTR_PTR(p04_mod_abs                ),
	SYSFS_ATTR_PTR(p03_mod_abs                ),
	SYSFS_ATTR_PTR(p02_mod_abs                ),
	SYSFS_ATTR_PTR(p01_mod_abs                ),
	SYSFS_ATTR_PTR(p16_mod_abs                ),
	SYSFS_ATTR_PTR(p15_mod_abs                ),
	SYSFS_ATTR_PTR(p14_mod_abs                ),
	SYSFS_ATTR_PTR(p13_mod_abs                ),
	SYSFS_ATTR_PTR(p12_mod_abs                ),
	SYSFS_ATTR_PTR(p11_mod_abs                ),
	SYSFS_ATTR_PTR(p10_mod_abs                ),
	SYSFS_ATTR_PTR(p09_mod_abs                ),
	SYSFS_ATTR_PTR(p24_mod_abs                ),
	SYSFS_ATTR_PTR(p23_mod_abs                ),
	SYSFS_ATTR_PTR(p22_mod_abs                ),
	SYSFS_ATTR_PTR(p21_mod_abs                ),
	SYSFS_ATTR_PTR(p20_mod_abs                ),
	SYSFS_ATTR_PTR(p19_mod_abs                ),
	SYSFS_ATTR_PTR(p18_mod_abs                ),
	SYSFS_ATTR_PTR(p17_mod_abs                ),
	SYSFS_ATTR_PTR(p32_mod_abs                ),
	SYSFS_ATTR_PTR(p31_mod_abs                ),
	SYSFS_ATTR_PTR(p30_mod_abs                ),
	SYSFS_ATTR_PTR(p29_mod_abs                ),
	SYSFS_ATTR_PTR(p28_mod_abs                ),
	SYSFS_ATTR_PTR(p27_mod_abs                ),
	SYSFS_ATTR_PTR(p26_mod_abs                ),
	SYSFS_ATTR_PTR(p25_mod_abs                ),
	SYSFS_ATTR_PTR(p40_mod_abs                ),
	SYSFS_ATTR_PTR(p39_mod_abs                ),
	SYSFS_ATTR_PTR(p38_mod_abs                ),
	SYSFS_ATTR_PTR(p37_mod_abs                ),
	SYSFS_ATTR_PTR(p36_mod_abs                ),
	SYSFS_ATTR_PTR(p35_mod_abs                ),
	SYSFS_ATTR_PTR(p34_mod_abs                ),
	SYSFS_ATTR_PTR(p33_mod_abs                ),
	SYSFS_ATTR_PTR(p48_mod_abs                ),
	SYSFS_ATTR_PTR(p47_mod_abs                ),
	SYSFS_ATTR_PTR(p46_mod_abs                ),
	SYSFS_ATTR_PTR(p45_mod_abs                ),
	SYSFS_ATTR_PTR(p44_mod_abs                ),
	SYSFS_ATTR_PTR(p43_mod_abs                ),
	SYSFS_ATTR_PTR(p42_mod_abs                ),
	SYSFS_ATTR_PTR(p41_mod_abs                ),
	SYSFS_ATTR_PTR(p08_rx_los                 ),
	SYSFS_ATTR_PTR(p07_rx_los                 ),
	SYSFS_ATTR_PTR(p06_rx_los                 ),
	SYSFS_ATTR_PTR(p05_rx_los                 ),
	SYSFS_ATTR_PTR(p04_rx_los                 ),
	SYSFS_ATTR_PTR(p03_rx_los                 ),
	SYSFS_ATTR_PTR(p02_rx_los                 ),
	SYSFS_ATTR_PTR(p01_rx_los                 ),
	SYSFS_ATTR_PTR(p16_rx_los                 ),
	SYSFS_ATTR_PTR(p15_rx_los                 ),
	SYSFS_ATTR_PTR(p14_rx_los                 ),
	SYSFS_ATTR_PTR(p13_rx_los                 ),
	SYSFS_ATTR_PTR(p12_rx_los                 ),
	SYSFS_ATTR_PTR(p11_rx_los                 ),
	SYSFS_ATTR_PTR(p10_rx_los                 ),
	SYSFS_ATTR_PTR(p09_rx_los                 ),
	SYSFS_ATTR_PTR(p24_rx_los                 ),
	SYSFS_ATTR_PTR(p23_rx_los                 ),
	SYSFS_ATTR_PTR(p22_rx_los                 ),
	SYSFS_ATTR_PTR(p21_rx_los                 ),
	SYSFS_ATTR_PTR(p20_rx_los                 ),
	SYSFS_ATTR_PTR(p19_rx_los                 ),
	SYSFS_ATTR_PTR(p18_rx_los                 ),
	SYSFS_ATTR_PTR(p17_rx_los                 ),
	SYSFS_ATTR_PTR(p32_rx_los                 ),
	SYSFS_ATTR_PTR(p31_rx_los                 ),
	SYSFS_ATTR_PTR(p30_rx_los                 ),
	SYSFS_ATTR_PTR(p29_rx_los                 ),
	SYSFS_ATTR_PTR(p28_rx_los                 ),
	SYSFS_ATTR_PTR(p27_rx_los                 ),
	SYSFS_ATTR_PTR(p26_rx_los                 ),
	SYSFS_ATTR_PTR(p25_rx_los                 ),
	SYSFS_ATTR_PTR(p40_rx_los                 ),
	SYSFS_ATTR_PTR(p39_rx_los                 ),
	SYSFS_ATTR_PTR(p38_rx_los                 ),
	SYSFS_ATTR_PTR(p37_rx_los                 ),
	SYSFS_ATTR_PTR(p36_rx_los                 ),
	SYSFS_ATTR_PTR(p35_rx_los                 ),
	SYSFS_ATTR_PTR(p34_rx_los                 ),
	SYSFS_ATTR_PTR(p33_rx_los                 ),
	SYSFS_ATTR_PTR(p48_rx_los                 ),
	SYSFS_ATTR_PTR(p47_rx_los                 ),
	SYSFS_ATTR_PTR(p46_rx_los                 ),
	SYSFS_ATTR_PTR(p45_rx_los                 ),
	SYSFS_ATTR_PTR(p44_rx_los                 ),
	SYSFS_ATTR_PTR(p43_rx_los                 ),
	SYSFS_ATTR_PTR(p42_rx_los                 ),
	SYSFS_ATTR_PTR(p41_rx_los                 ),
	SYSFS_ATTR_PTR(p08_tx_fault               ),
	SYSFS_ATTR_PTR(p07_tx_fault               ),
	SYSFS_ATTR_PTR(p06_tx_fault               ),
	SYSFS_ATTR_PTR(p05_tx_fault               ),
	SYSFS_ATTR_PTR(p04_tx_fault               ),
	SYSFS_ATTR_PTR(p03_tx_fault               ),
	SYSFS_ATTR_PTR(p02_tx_fault               ),
	SYSFS_ATTR_PTR(p01_tx_fault               ),
	SYSFS_ATTR_PTR(p16_tx_fault               ),
	SYSFS_ATTR_PTR(p15_tx_fault               ),
	SYSFS_ATTR_PTR(p14_tx_fault               ),
	SYSFS_ATTR_PTR(p13_tx_fault               ),
	SYSFS_ATTR_PTR(p12_tx_fault               ),
	SYSFS_ATTR_PTR(p11_tx_fault               ),
	SYSFS_ATTR_PTR(p10_tx_fault               ),
	SYSFS_ATTR_PTR(p09_tx_fault               ),
	SYSFS_ATTR_PTR(p24_tx_fault               ),
	SYSFS_ATTR_PTR(p23_tx_fault               ),
	SYSFS_ATTR_PTR(p22_tx_fault               ),
	SYSFS_ATTR_PTR(p21_tx_fault               ),
	SYSFS_ATTR_PTR(p20_tx_fault               ),
	SYSFS_ATTR_PTR(p19_tx_fault               ),
	SYSFS_ATTR_PTR(p18_tx_fault               ),
	SYSFS_ATTR_PTR(p17_tx_fault               ),
	SYSFS_ATTR_PTR(p32_tx_fault               ),
	SYSFS_ATTR_PTR(p31_tx_fault               ),
	SYSFS_ATTR_PTR(p30_tx_fault               ),
	SYSFS_ATTR_PTR(p29_tx_fault               ),
	SYSFS_ATTR_PTR(p28_tx_fault               ),
	SYSFS_ATTR_PTR(p27_tx_fault               ),
	SYSFS_ATTR_PTR(p26_tx_fault               ),
	SYSFS_ATTR_PTR(p25_tx_fault               ),
	SYSFS_ATTR_PTR(p40_tx_fault               ),
	SYSFS_ATTR_PTR(p39_tx_fault               ),
	SYSFS_ATTR_PTR(p38_tx_fault               ),
	SYSFS_ATTR_PTR(p37_tx_fault               ),
	SYSFS_ATTR_PTR(p36_tx_fault               ),
	SYSFS_ATTR_PTR(p35_tx_fault               ),
	SYSFS_ATTR_PTR(p34_tx_fault               ),
	SYSFS_ATTR_PTR(p33_tx_fault               ),
	SYSFS_ATTR_PTR(p48_tx_fault               ),
	SYSFS_ATTR_PTR(p47_tx_fault               ),
	SYSFS_ATTR_PTR(p46_tx_fault               ),
	SYSFS_ATTR_PTR(p45_tx_fault               ),
	SYSFS_ATTR_PTR(p44_tx_fault               ),
	SYSFS_ATTR_PTR(p43_tx_fault               ),
	SYSFS_ATTR_PTR(p42_tx_fault               ),
	SYSFS_ATTR_PTR(p41_tx_fault               ),
	SYSFS_ATTR_PTR(p08_tx_disable             ),
	SYSFS_ATTR_PTR(p07_tx_disable             ),
	SYSFS_ATTR_PTR(p06_tx_disable             ),
	SYSFS_ATTR_PTR(p05_tx_disable             ),
	SYSFS_ATTR_PTR(p04_tx_disable             ),
	SYSFS_ATTR_PTR(p03_tx_disable             ),
	SYSFS_ATTR_PTR(p02_tx_disable             ),
	SYSFS_ATTR_PTR(p01_tx_disable             ),
	SYSFS_ATTR_PTR(p16_tx_disable             ),
	SYSFS_ATTR_PTR(p15_tx_disable             ),
	SYSFS_ATTR_PTR(p14_tx_disable             ),
	SYSFS_ATTR_PTR(p13_tx_disable             ),
	SYSFS_ATTR_PTR(p12_tx_disable             ),
	SYSFS_ATTR_PTR(p11_tx_disable             ),
	SYSFS_ATTR_PTR(p10_tx_disable             ),
	SYSFS_ATTR_PTR(p09_tx_disable             ),
	SYSFS_ATTR_PTR(p24_tx_disable             ),
	SYSFS_ATTR_PTR(p23_tx_disable             ),
	SYSFS_ATTR_PTR(p22_tx_disable             ),
	SYSFS_ATTR_PTR(p21_tx_disable             ),
	SYSFS_ATTR_PTR(p20_tx_disable             ),
	SYSFS_ATTR_PTR(p19_tx_disable             ),
	SYSFS_ATTR_PTR(p18_tx_disable             ),
	SYSFS_ATTR_PTR(p17_tx_disable             ),
	SYSFS_ATTR_PTR(p32_tx_disable             ),
	SYSFS_ATTR_PTR(p31_tx_disable             ),
	SYSFS_ATTR_PTR(p30_tx_disable             ),
	SYSFS_ATTR_PTR(p29_tx_disable             ),
	SYSFS_ATTR_PTR(p28_tx_disable             ),
	SYSFS_ATTR_PTR(p27_tx_disable             ),
	SYSFS_ATTR_PTR(p26_tx_disable             ),
	SYSFS_ATTR_PTR(p25_tx_disable             ),
	SYSFS_ATTR_PTR(p40_tx_disable             ),
	SYSFS_ATTR_PTR(p39_tx_disable             ),
	SYSFS_ATTR_PTR(p38_tx_disable             ),
	SYSFS_ATTR_PTR(p37_tx_disable             ),
	SYSFS_ATTR_PTR(p36_tx_disable             ),
	SYSFS_ATTR_PTR(p35_tx_disable             ),
	SYSFS_ATTR_PTR(p34_tx_disable             ),
	SYSFS_ATTR_PTR(p33_tx_disable             ),
	SYSFS_ATTR_PTR(p48_tx_disable             ),
	SYSFS_ATTR_PTR(p47_tx_disable             ),
	SYSFS_ATTR_PTR(p46_tx_disable             ),
	SYSFS_ATTR_PTR(p45_tx_disable             ),
	SYSFS_ATTR_PTR(p44_tx_disable             ),
	SYSFS_ATTR_PTR(p43_tx_disable             ),
	SYSFS_ATTR_PTR(p42_tx_disable             ),
	SYSFS_ATTR_PTR(p41_tx_disable             ),
	SYSFS_ATTR_PTR(cpld2_irq_msk              ),
	SYSFS_ATTR_PTR(cpld2_int_n_msk            ),
	SYSFS_ATTR_PTR(p08_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p07_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p06_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p05_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p04_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p03_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p02_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p01_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p16_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p15_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p14_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p13_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p12_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p11_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p10_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p09_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p24_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p23_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p22_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p21_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p20_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p19_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p18_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p17_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p32_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p31_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p30_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p29_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p28_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p27_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p26_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p25_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p40_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p39_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p38_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p37_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p36_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p35_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p34_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p33_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p48_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p47_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p46_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p45_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p44_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p43_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p42_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p41_mod_abs_msk            ),
	SYSFS_ATTR_PTR(p08_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p07_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p06_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p05_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p04_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p03_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p02_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p01_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p16_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p15_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p14_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p13_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p12_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p11_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p10_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p09_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p24_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p23_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p22_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p21_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p20_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p19_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p18_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p17_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p32_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p31_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p30_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p29_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p28_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p27_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p26_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p25_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p40_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p39_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p38_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p37_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p36_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p35_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p34_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p33_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p48_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p47_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p46_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p45_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p44_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p43_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p42_mod_abs_proc           ),
	SYSFS_ATTR_PTR(p41_mod_abs_proc           ),
	SYSFS_ATTR_PTR(cpld2_rev_cap              ),
	SYSFS_ATTR_PTR(cpld2_rev_sub              ),

    NULL
};

static const struct attribute_group system_cpld_group_misc = {
    .attrs = misc_attributes,
};

#define I2C_RW_RETRY_COUNT               10
#define I2C_RW_RETRY_INTERVAL            60 /* ms */

static void qsd61_sfp_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void qsd61_sfp_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

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

static int qsd61_sfp_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int qsd61_sfp_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

int qsd61_sfp_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = qsd61_sfp_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(qsd61_sfp_cpld_read);

int qsd61_sfp_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = qsd61_sfp_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(qsd61_sfp_cpld_write);

static int system_cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err;

    /* allocate memory to system_cpld */
    system_cpld = devm_kzalloc(&client->dev, sizeof(struct system_cpld_data), GFP_KERNEL);

    if(!system_cpld)
        return -ENOMEM;

    mutex_init(&system_cpld->lock);

    err = sysfs_create_group(&client->dev.kobj, &system_cpld_group_misc);
    if(err)
    {
        printk("%s: Error create misc group.\n", __FUNCTION__);
    }

    system_cpld->client = client;
    i2c_set_clientdata(client, system_cpld);

    printk(KERN_INFO "%s: 10g SFP plus CPLD2 created.\n", __FUNCTION__);

	qsd61_sfp_cpld_add_client(client);

    return 0;
}

static int system_cpld_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &system_cpld_group_misc);

	qsd61_sfp_cpld_remove_client(client);

    printk(KERN_INFO "%s: 10g SFP plus CPLD2 removed.\n", __FUNCTION__);
    return 0;
}

static struct i2c_driver system_cpld_driver = {
    .driver = {
        .name = "qsd61_48_sfp_cpld2",
        .owner = THIS_MODULE,
    },
    .probe = system_cpld_probe,
    .remove = system_cpld_remove,
    .id_table = system_cpld_ids,
};

static int __init system_cpld_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
	mutex_init(&list_lock);
    return i2c_add_driver(&system_cpld_driver);
}
module_init(system_cpld_init);

static void __exit system_cpld_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    i2c_del_driver(&system_cpld_driver);
}
module_exit(system_cpld_exit);

MODULE_DESCRIPTION("Driver for 10g SFP plus CPLD2");
MODULE_AUTHOR("Sunway Liu <Sunway.Liu@wnc.com.tw>");
MODULE_LICENSE("GPL");
