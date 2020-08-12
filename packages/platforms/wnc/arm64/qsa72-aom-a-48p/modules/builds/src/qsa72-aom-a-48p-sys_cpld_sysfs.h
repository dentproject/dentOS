#ifndef __wnc48pSysCpldFs_h__
#define __wnc48pSysCpldFs_h__


/* Generic CPLD sysfs file definition macros */
#define SYSFS_RAW_RO_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, system_cpld_##field##_raw_read, NULL);

#define SYSFS_RAW_RW_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUSR, system_cpld_##field##_raw_read, system_cpld_##field##_raw_write);

#define SYSFS_MISC_RO_ATTR_DEF(field, _read)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, _read, NULL);

#define SYSFS_MISC_RW_ATTR_DEF(field, _read, _write)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUSR, _read, _write);

#define SYSFS_ATTR_PTR(field) &field.attr

SYSFS_RAW_RO_ATTR_DEF(mb_pcb_ver                 )
SYSFS_RAW_RW_ATTR_DEF(phy_s_rst_n                )
SYSFS_RAW_RW_ATTR_DEF(phy_m_rst_n                )
SYSFS_RAW_RW_ATTR_DEF(mac_s_rst_n                )
SYSFS_RAW_RW_ATTR_DEF(tpm_rst_n                  )
SYSFS_RAW_RW_ATTR_DEF(oob_rst_n                  )
SYSFS_RAW_RW_ATTR_DEF(mac_m_rst_n                )
SYSFS_RAW_RW_ATTR_DEF(a385_sysrst_in_n           )
SYSFS_RAW_RW_ATTR_DEF(a7040_sysrst_in_n          )
SYSFS_RAW_RW_ATTR_DEF(pd69200_rst_n              )
SYSFS_RAW_RW_ATTR_DEF(pca9548_rst_n              )
SYSFS_RAW_RW_ATTR_DEF(tca9539_rst_n              )
SYSFS_RAW_RO_ATTR_DEF(tpm_int_n                  )
SYSFS_RAW_RO_ATTR_DEF(oob_int_n                  )
SYSFS_RAW_RO_ATTR_DEF(xint_out                   )
SYSFS_RAW_RO_ATTR_DEF(lm75_int2_l                )
SYSFS_RAW_RO_ATTR_DEF(lm75_int1_3_l              )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode5             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode4             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode3             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode2             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode1             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode0             )
SYSFS_RAW_RW_ATTR_DEF(pwm_ctrl                   )
SYSFS_RAW_RW_ATTR_DEF(fan_sel                    )
SYSFS_RAW_RO_ATTR_DEF(fan1_tach                  )
SYSFS_RAW_RO_ATTR_DEF(fan2_tach                  )
SYSFS_RAW_RO_ATTR_DEF(a385_cpld1_bz              )
SYSFS_RAW_RO_ATTR_DEF(a7040_cpld1_bz             )
SYSFS_RAW_RO_ATTR_DEF(mb_cpld1_irq_n             )
SYSFS_RAW_RW_ATTR_DEF(cpld1_irq_msk              )
SYSFS_RAW_RW_ATTR_DEF(sys_led                    )
SYSFS_RAW_RO_ATTR_DEF(cpld1_rev_cap              )
SYSFS_RAW_RO_ATTR_DEF(cpld1_rev_sub              )
SYSFS_RAW_RW_ATTR_DEF(gpio_led_ctrl_sfp          )
SYSFS_RAW_RO_ATTR_DEF(pg_55v_1                   )
SYSFS_RAW_RO_ATTR_DEF(pg_55v_0                   )
SYSFS_RAW_RO_ATTR_DEF(pg_12v_1                   )
SYSFS_RAW_RO_ATTR_DEF(pg_12v_0                   )
SYSFS_RAW_RW_ATTR_DEF(x_disable_ports            )
SYSFS_RAW_RO_ATTR_DEF(system_ok                  )
SYSFS_RAW_RW_ATTR_DEF(a7040_gpio0_int_msk        )
SYSFS_RAW_RW_ATTR_DEF(tpm_int_msk                )
SYSFS_RAW_RW_ATTR_DEF(oob_int_msk                )
SYSFS_RAW_RW_ATTR_DEF(xint_int_msk               )
SYSFS_RAW_RW_ATTR_DEF(lm75_int2_msk              )
SYSFS_RAW_RW_ATTR_DEF(lm75_int1_3_msk            )
SYSFS_RAW_RW_ATTR_DEF(a7040_gpio0_int_proc       )
SYSFS_RAW_RW_ATTR_DEF(tpm_int_proc               )
SYSFS_RAW_RW_ATTR_DEF(oob_int_proc               )
SYSFS_RAW_RW_ATTR_DEF(xint_int_proc              )
SYSFS_RAW_RW_ATTR_DEF(lm75_int2_proc             )
SYSFS_RAW_RW_ATTR_DEF(lm75_int1_3_proc           )

#endif /* __wnc48pSysCpldFs_h__ */
