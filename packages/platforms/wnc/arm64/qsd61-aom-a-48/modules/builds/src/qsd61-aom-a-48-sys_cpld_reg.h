#ifndef __wnc10gSysCpldReg_h__
#define __wnc10gSysCpldReg_h__


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
/*           register name                         offset  width */
/*           ---------------------------------     ------- ----- */
REG_DEF(     mb_pcb_ver_reg         ,                 0x00,   8)
REG_DEF(     reset_out_A_reg        ,                 0x02,   8)
REG_DEF(     reset_out_B_reg        ,                 0x03,   8)
REG_DEF(     interrupt_reg          ,                 0x04,   8)
REG_DEF(     a385_bootmode_reg      ,                 0x05,   8)
REG_DEF(     fan_ctrl_reg           ,                 0x06,   8)
REG_DEF(     fan_tach_sel_reg       ,                 0x07,   8)
REG_DEF(     fan_tach_a_reg         ,                 0x08,   8)
REG_DEF(     fan_tach_b_reg         ,                 0x09,   8)
REG_DEF(     i2c_ctrl_reg           ,                 0x0A,   8)
REG_DEF(     cpld_irq_reg           ,                 0x0B,   8)
REG_DEF(     cpld_irq_msk_reg       ,                 0x0C,   8)
REG_DEF(     sys_led_reg            ,                 0x10,   8)
REG_DEF(     interrupt_msk_reg      ,                 0x20,   8)
REG_DEF(     interrupt_proc_reg     ,                 0x21,   8)
REG_DEF(     cpld_ver_reg           ,                 0xFD,   8)

/* Declare system CPLD register's fields */
/*                      register name               field name                  shift  width */
/*                      ----------------------      ----------------            ------ ----- */
FLD_RAW_RO_DEF(         mb_pcb_ver_reg,             mb_pcb_ver,                 0,      2)
FLD_RAW_RW_DEF(         reset_out_A_reg,            tpm_rst_n,                  4,      1)
FLD_RAW_RW_DEF(         reset_out_A_reg,            oob_rst_n,                  3,      1)
FLD_RAW_RW_DEF(         reset_out_A_reg,            a2_sysrst_n,                2,      1)
FLD_RAW_RW_DEF(         reset_out_A_reg,            a385_sysrst_in_n,           1,      1)
FLD_RAW_RW_DEF(         reset_out_A_reg,            a7040_sysrst_in_n,          0,      1)
FLD_RAW_RW_DEF(         reset_out_B_reg,            wd_rst_n,                   4,      1)
FLD_RAW_RW_DEF(         reset_out_B_reg,            shift_reg_rst_n,            3,      1)
FLD_RAW_RW_DEF(         reset_out_B_reg,            mux1_rst_n,                 2,      1)
FLD_RAW_RW_DEF(         reset_out_B_reg,            mux0_rst_n,                 1,      1)
FLD_RAW_RW_DEF(         reset_out_B_reg,            cpld2_rst_n,                0,      1)
FLD_RAW_RW_DEF(         interrupt_reg,              a7040_cpld1_gpio0,          7,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              core_vdd_pmb_alrt_n,        6,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              tpm_int_n,                  4,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              oob_int_n,                  3,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              temp3_alert_n,              2,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              temp2_alert_n,              1,      1)
FLD_RAW_RO_DEF(         interrupt_reg,              temp1_alert_n,              0,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode5,             5,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode4,             4,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode3,             3,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode2,             2,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode1,             1,      1)
FLD_RAW_RW_DEF(         a385_bootmode_reg,          a385_bootmode0,             0,      1)
FLD_RAW_RW_DEF(         fan_ctrl_reg,               pwm_ctrl,                   0,      8)
FLD_RAW_RW_DEF(         fan_tach_sel_reg,           fan_tach_sel,               0,      3)
FLD_RAW_RO_DEF(         fan_tach_a_reg,             fan_tach_a,                 0,      8)
FLD_RAW_RO_DEF(         fan_tach_b_reg,             fan_tach_b,                 0,      8)
FLD_RAW_RW_DEF(         i2c_ctrl_reg,               a7040_cpld1_i2c_bz,         0,      1)
FLD_RAW_RW_DEF(         cpld_irq_reg,               cpld1_irq,                  0,      1)
FLD_RAW_RW_DEF(         cpld_irq_msk_reg,           cpld1_irq_msk,              0,      1)
FLD_RAW_RW_DEF(         sys_led_reg,                sys_led,                    0,      3)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          cpld1_int_n_msk,            7,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          core_vdd_pmb_alrt_n_msk,    6,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          tpm_int_n_msk,              4,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          oob_int_n_msk,              3,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          temp3_alert_n_msk,          2,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          temp2_alert_n_msk,          1,      1)
FLD_RAW_RW_DEF(         interrupt_msk_reg,          temp1_alert_n_msk,          0,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         core_vdd_pmb_alrt_n_proc,   6,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         tpm_int_n_proc,             4,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         oob_int_n_proc,             3,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         temp3_alert_n_proc,         2,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         temp2_alert_n_proc,         1,      1)
FLD_RAW_RW_DEF(         interrupt_proc_reg,         temp1_alert_n_proc,         0,      1)
FLD_RAW_RO_DEF(         cpld_ver_reg,               cpld1_rev_cap,              6,      2)
FLD_RAW_RO_DEF(         cpld_ver_reg,               cpld1_rev_sub,              0,      6)

#endif /* __wnc10gSysCpldReg_h__ */