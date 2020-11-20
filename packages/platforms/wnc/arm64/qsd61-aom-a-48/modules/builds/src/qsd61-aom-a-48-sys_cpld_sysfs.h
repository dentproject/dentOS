#ifndef __wnc10gSysCpldFs_h__
#define __wnc10gSysCpldFs_h__


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
SYSFS_RAW_RW_ATTR_DEF(tpm_rst_n                  )
SYSFS_RAW_RW_ATTR_DEF(oob_rst_n                  )
SYSFS_RAW_RW_ATTR_DEF(a2_sysrst_n                )
SYSFS_RAW_RW_ATTR_DEF(a385_sysrst_in_n           )
SYSFS_RAW_RW_ATTR_DEF(a7040_sysrst_in_n          )
SYSFS_RAW_RW_ATTR_DEF(wd_rst_n                   )
SYSFS_RAW_RW_ATTR_DEF(shift_reg_rst_n            )
SYSFS_RAW_RW_ATTR_DEF(mux1_rst_n                 )
SYSFS_RAW_RW_ATTR_DEF(mux0_rst_n                 )
SYSFS_RAW_RW_ATTR_DEF(cpld2_rst_n                )
SYSFS_RAW_RW_ATTR_DEF(a7040_cpld1_gpio0          )
SYSFS_RAW_RO_ATTR_DEF(core_vdd_pmb_alrt_n        )
SYSFS_RAW_RO_ATTR_DEF(tpm_int_n                  )
SYSFS_RAW_RO_ATTR_DEF(oob_int_n                  )
SYSFS_RAW_RO_ATTR_DEF(temp3_alert_n              )
SYSFS_RAW_RO_ATTR_DEF(temp2_alert_n              )
SYSFS_RAW_RO_ATTR_DEF(temp1_alert_n              )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode5             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode4             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode3             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode2             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode1             )
SYSFS_RAW_RW_ATTR_DEF(a385_bootmode0             )
SYSFS_RAW_RW_ATTR_DEF(pwm_ctrl                   )
SYSFS_RAW_RW_ATTR_DEF(fan_tach_sel               )
SYSFS_RAW_RO_ATTR_DEF(fan_tach_a                 )
SYSFS_RAW_RO_ATTR_DEF(fan_tach_b                 )
SYSFS_RAW_RW_ATTR_DEF(a7040_cpld1_i2c_bz         )
SYSFS_RAW_RW_ATTR_DEF(cpld1_irq                  )
SYSFS_RAW_RW_ATTR_DEF(cpld1_irq_msk              )
SYSFS_RAW_RW_ATTR_DEF(sys_led                    )
SYSFS_RAW_RW_ATTR_DEF(cpld1_int_n_msk            )
SYSFS_RAW_RW_ATTR_DEF(core_vdd_pmb_alrt_n_msk    )
SYSFS_RAW_RW_ATTR_DEF(tpm_int_n_msk              )
SYSFS_RAW_RW_ATTR_DEF(oob_int_n_msk              )
SYSFS_RAW_RW_ATTR_DEF(temp3_alert_n_msk          )
SYSFS_RAW_RW_ATTR_DEF(temp2_alert_n_msk          )
SYSFS_RAW_RW_ATTR_DEF(temp1_alert_n_msk          )
SYSFS_RAW_RW_ATTR_DEF(core_vdd_pmb_alrt_n_proc   )
SYSFS_RAW_RW_ATTR_DEF(tpm_int_n_proc             )
SYSFS_RAW_RW_ATTR_DEF(oob_int_n_proc             )
SYSFS_RAW_RW_ATTR_DEF(temp3_alert_n_proc         )
SYSFS_RAW_RW_ATTR_DEF(temp2_alert_n_proc         )
SYSFS_RAW_RW_ATTR_DEF(temp1_alert_n_proc         )
SYSFS_RAW_RO_ATTR_DEF(cpld1_rev_cap              )
SYSFS_RAW_RO_ATTR_DEF(cpld1_rev_sub              )

#endif /* __wnc10gSysCpldFs_h__ */
