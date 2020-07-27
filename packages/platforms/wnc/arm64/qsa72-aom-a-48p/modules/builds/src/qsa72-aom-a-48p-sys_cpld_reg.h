#ifndef __wnc48pSysCpldReg_h__
#define __wnc48pSysCpldReg_h__


static int system_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name);
static int system_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name);

/* Generic CPLD read function */
#define FLD_RAW_RD_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_read(struct device *dev, struct device_attribute *attr, char *buf) { \
    return system_cpld_raw_read(dev, attr, buf, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* Generic CPLD write function */
#define FLD_RAW_WR_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { \
    return system_cpld_raw_write(dev, attr, buf, count, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* CPLD register definition macros */
#define REG_DEF(_reg, _off, _wdh) \
static unsigned int _reg##_offset = (unsigned int)(_off); \
static unsigned int _reg##_width = (unsigned int)(_wdh);

/* CPLD register field definition macros, with generic read/write function */
#define FLD_RAW_RO_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh)

#define FLD_RAW_RW_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh) FLD_RAW_WR_FUNC(_reg, _fld, _wdh)

/* Declare system CPLD registers */
/*           register name                                      offset  width */
/*           ---------------------------------------            ------- ----- */
REG_DEF(     mb_pcb_ver_reg                     ,                 0x00,   8)
REG_DEF(     reset_output_A_reg                 ,                 0x02,   8)
REG_DEF(     reset_output_B_reg                 ,                 0x03,   8)
REG_DEF(     interrupt_reg                      ,                 0x04,   8)
REG_DEF(     a385_bootmode_reg                  ,                 0x05,   8)
REG_DEF(     fan_ctrl_reg                       ,                 0x06,   8)
REG_DEF(     fan_tach_sel_reg                   ,                 0x07,   8)
REG_DEF(     fan1_tach_reg                      ,                 0x08,   8)
REG_DEF(     fan2_tach_reg                      ,                 0x09,   8)
REG_DEF(     i2c_ctrl_reg                       ,                 0x0A,   8)
REG_DEF(     cpld_irq_n_reg                     ,                 0x0B,   8)
REG_DEF(     cpld_irq_msk_reg                   ,                 0x0C,   8)
REG_DEF(     sys_led_reg                        ,                 0x10,   8)
REG_DEF(     cpld_ver_reg                       ,                 0xFD,   8)
REG_DEF(     sfp_led_power_reg                  ,                 0x12,   8)
REG_DEF(     psu_status_reg                     ,                 0x13,   8)
REG_DEF(     pd69200_status_n_ctrl_reg          ,                 0x14,   8)
REG_DEF(     interrupt_msk_reg                  ,                 0x20,   8)
REG_DEF(     interrupt_proc_reg                 ,                 0x21,   8)

/* Declare system CPLD register's fields */
/*                      register name                       field name                        shift  width */
/*                      ----------------------              ----------------                  ------ ----- */
FLD_RAW_RO_DEF(         mb_pcb_ver_reg,                     mb_pcb_ver,                         0,      2)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 phy_s_rst_n,                        7,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 phy_m_rst_n,                        6,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 mac_s_rst_n,                        5,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 tpm_rst_n,                          4,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 oob_rst_n,                          3,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 mac_m_rst_n,                        2,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 a385_sysrst_in_n,                   1,      1)
FLD_RAW_RW_DEF(         reset_output_A_reg,                 a7040_sysrst_in_n,                  0,      1)
FLD_RAW_RW_DEF(         reset_output_B_reg,                 pd69200_rst_n,                      2,      1)
FLD_RAW_RW_DEF(         reset_output_B_reg,                 pca9548_rst_n,                      1,      1)
FLD_RAW_RW_DEF(         reset_output_B_reg,                 tca9539_rst_n,                      0,      1)
FLD_RAW_RO_DEF(         interrupt_reg,                      tpm_int_n,                          4,      1)
FLD_RAW_RO_DEF(         interrupt_reg,                      oob_int_n,                          3,      1)
FLD_RAW_RO_DEF(         interrupt_reg,                      xint_out,                           2,      1)
FLD_RAW_RO_DEF(         interrupt_reg,                      lm75_int2_l,                        1,      1)
FLD_RAW_RO_DEF(         interrupt_reg,                      lm75_int1_3_l,                      0,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode5,                     5,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode4,                     4,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode3,                     3,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode2,                     2,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode1,                     1,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,                  a385_bootmode0,                     0,      1)
FLD_RAW_RW_DEF(         fan_ctrl_reg,                       pwm_ctrl,                           0,      8)
FLD_RAW_RW_DEF(         fan_tach_sel_reg,                   fan_sel,                            0,      3)
FLD_RAW_RO_DEF(         fan1_tach_reg,                      fan1_tach,                          0,      8)
FLD_RAW_RO_DEF(         fan2_tach_reg,                      fan2_tach,                          0,      8)
FLD_RAW_RO_DEF(         i2c_ctrl_reg,                       a385_cpld1_bz,                      1,      1)
FLD_RAW_RO_DEF(         i2c_ctrl_reg,                       a7040_cpld1_bz,                     0,      1)
FLD_RAW_RO_DEF(         cpld_irq_n_reg,                     mb_cpld1_irq_n,                     0,      1)
FLD_RAW_RW_DEF(         cpld_irq_msk_reg,                   cpld1_irq_msk,                      0,      1)
FLD_RAW_RW_DEF(         sys_led_reg,                        sys_led,                            0,      3)
FLD_RAW_RO_DEF(         cpld_ver_reg,                       cpld1_rev_cap,                      6,      2)
FLD_RAW_RO_DEF(         cpld_ver_reg,                       cpld1_rev_sub,                      0,      6)
FLD_RAW_RW_DEF(         sfp_led_power_reg,                  gpio_led_ctrl_sfp,                  0,      1)
FLD_RAW_RO_DEF(         psu_status_reg,                     pg_55v_1,                           3,      1)
FLD_RAW_RO_DEF(         psu_status_reg,                     pg_55v_0,                           2,      1)
FLD_RAW_RO_DEF(         psu_status_reg,                     pg_12v_1,                           1,      1)
FLD_RAW_RO_DEF(         psu_status_reg,                     pg_12v_0,                           0,      1)
FLD_RAW_RW_DEF(         pd69200_status_n_ctrl_reg,          x_disable_ports,                    1,      1)
FLD_RAW_RO_DEF(         pd69200_status_n_ctrl_reg,          system_ok,                          0,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  a7040_gpio0_int_msk,                7,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  tpm_int_msk,                        4,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  oob_int_msk,                        3,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  xint_int_msk,                       2,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  lm75_int2_msk,                      1,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,                  lm75_int1_3_msk,                    0,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 a7040_gpio0_int_proc,               7,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 tpm_int_proc,                       4,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 oob_int_proc,                       3,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 xint_int_proc,                      2,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 lm75_int2_proc,                     1,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,                 lm75_int1_3_proc,                   0,      1)


#endif /* __wnc48pSysCpldReg_h__ */