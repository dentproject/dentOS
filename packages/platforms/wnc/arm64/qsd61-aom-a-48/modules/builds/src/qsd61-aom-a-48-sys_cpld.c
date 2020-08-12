#include <linux/module.h>
#include <linux/i2c.h>

#include "qsd61-aom-a-48-sys_cpld_reg.h"
#include "qsd61-aom-a-48-sys_cpld_sysfs.h"

static int debug_flag = 0;

struct system_cpld_data {
    struct mutex lock;
    struct i2c_client *client;
    struct device_attribute bin;
};
struct system_cpld_data *system_cpld;

static const struct i2c_device_id system_cpld_ids[] = {
    { "qsd61_48_sys_cpld1", 0 },
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

    SYSFS_ATTR_PTR(mb_pcb_ver                 ),
    SYSFS_ATTR_PTR(tpm_rst_n                  ),
    SYSFS_ATTR_PTR(oob_rst_n                  ),
    SYSFS_ATTR_PTR(a2_sysrst_n                ),
    SYSFS_ATTR_PTR(a385_sysrst_in_n           ),
    SYSFS_ATTR_PTR(a7040_sysrst_in_n          ),
    SYSFS_ATTR_PTR(wd_rst_n                   ),
    SYSFS_ATTR_PTR(shift_reg_rst_n            ),
    SYSFS_ATTR_PTR(mux1_rst_n                 ),
    SYSFS_ATTR_PTR(mux0_rst_n                 ),
    SYSFS_ATTR_PTR(cpld2_rst_n                ),
    SYSFS_ATTR_PTR(a7040_cpld1_gpio0          ),
    SYSFS_ATTR_PTR(core_vdd_pmb_alrt_n        ),
    SYSFS_ATTR_PTR(tpm_int_n                  ),
    SYSFS_ATTR_PTR(oob_int_n                  ),
    SYSFS_ATTR_PTR(temp3_alert_n              ),
    SYSFS_ATTR_PTR(temp2_alert_n              ),
    SYSFS_ATTR_PTR(temp1_alert_n              ),
    SYSFS_ATTR_PTR(a385_bootmode5             ),
    SYSFS_ATTR_PTR(a385_bootmode4             ),
    SYSFS_ATTR_PTR(a385_bootmode3             ),
    SYSFS_ATTR_PTR(a385_bootmode2             ),
    SYSFS_ATTR_PTR(a385_bootmode1             ),
    SYSFS_ATTR_PTR(a385_bootmode0             ),
    SYSFS_ATTR_PTR(pwm_ctrl                   ),
    SYSFS_ATTR_PTR(fan_tach_sel               ),
    SYSFS_ATTR_PTR(fan_tach_a                 ),
    SYSFS_ATTR_PTR(fan_tach_b                 ),
    SYSFS_ATTR_PTR(a7040_cpld1_i2c_bz         ),
    SYSFS_ATTR_PTR(cpld1_irq                  ),
    SYSFS_ATTR_PTR(cpld1_irq_msk              ),
    SYSFS_ATTR_PTR(sys_led                    ),
    SYSFS_ATTR_PTR(cpld1_int_n_msk            ),
    SYSFS_ATTR_PTR(core_vdd_pmb_alrt_n_msk    ),
    SYSFS_ATTR_PTR(tpm_int_n_msk              ),
    SYSFS_ATTR_PTR(oob_int_n_msk              ),
    SYSFS_ATTR_PTR(temp3_alert_n_msk          ),
    SYSFS_ATTR_PTR(temp2_alert_n_msk          ),
    SYSFS_ATTR_PTR(temp1_alert_n_msk          ),
    SYSFS_ATTR_PTR(core_vdd_pmb_alrt_n_proc   ),
    SYSFS_ATTR_PTR(tpm_int_n_proc             ),
    SYSFS_ATTR_PTR(oob_int_n_proc             ),
    SYSFS_ATTR_PTR(temp3_alert_n_proc         ),
    SYSFS_ATTR_PTR(temp2_alert_n_proc         ),
    SYSFS_ATTR_PTR(temp1_alert_n_proc         ),
    SYSFS_ATTR_PTR(cpld1_rev_cap              ),
    SYSFS_ATTR_PTR(cpld1_rev_sub              ),
    NULL
};

static const struct attribute_group system_cpld_group_misc = {
    .attrs = misc_attributes,
};

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

    printk(KERN_INFO "%s: 10G System CPLD1 created.\n", __FUNCTION__);

    return 0;
}

static int system_cpld_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &system_cpld_group_misc);

    printk(KERN_INFO "%s: 10G System CPLD1 removed.\n", __FUNCTION__);
    return 0;
}

static struct i2c_driver system_cpld_driver = {
    .driver = {
        .name = "qsd61_48_sys_cpld1",
        .owner = THIS_MODULE,
    },
    .probe = system_cpld_probe,
    .remove = system_cpld_remove,
    .id_table = system_cpld_ids,
};

static int __init system_cpld_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
    return i2c_add_driver(&system_cpld_driver);
}
module_init(system_cpld_init);

static void __exit system_cpld_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    i2c_del_driver(&system_cpld_driver);
}
module_exit(system_cpld_exit);

MODULE_DESCRIPTION("Driver for 10g system CPLD1");
MODULE_AUTHOR("Sunway Liu <Sunway.Liu@wnc.com.tw>");
MODULE_LICENSE("GPL");
